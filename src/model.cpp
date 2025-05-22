// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "model.h"

#include "loader.h"
#include "location.h"

#include <QEventLoop>
#include <QMutex>
#include <QQueue>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>

using namespace Qt::Literals::StringLiterals;

Model::Model(const Settings &settings, CompleterModel* completerModel, QObject *parent)
    : QAbstractTableModel(parent)
    , settings(settings)
    , thumbnailCache(settings.thumbnailCacheSize)
    , completerModel(completerModel)
    , mapIcon(QImage(u":/resources/map.png"_s)
                  .scaled(settings.thumbnailSize / 4,
                          settings.thumbnailSize / 4,
                          Qt::KeepAspectRatio,
                          Qt::SmoothTransformation))
{
}

Model::~Model()
{
    qDeleteAll(images);
    images.clear();
}

void Model::load(const QList<const FileGroup *> &fileGroupList, const QDir &directory, const QString &dbPath)
{
    images.reserve(fileGroupList.size());

    // prepare database
    if (QFile(dbPath).exists()) {
        qInfo() << "Thumbnails database already exists at" << dbPath;
    } else {
        qInfo() << "Thumbnails database does not exist at" << dbPath;
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(u"QSQLITE"_s, u"load"_s);
        db.setDatabaseName(dbPath);
        if (db.open()) {
            qInfo() << "Thumbnails database opened";
        } else {
            qWarning() << "Thumbnails database opening failure:" << db.lastError();
        }
        QSqlQuery query(db);
        if (query.exec(u"DROP TABLE IF EXISTS thumbnails"_s)) {
            qInfo() << "Thumbnails database table (v1) dropped";
        } else {
            qWarning() << "Thumbnails database table (v1) drop failure:" << query.lastError();
        }
        if (query.exec(u"CREATE TABLE IF NOT EXISTS thumbnails2 "
                        "(path TEXT PRIMARY KEY, "
                        " timestamp INTEGER NOT NULL, "
                        " latitude REAL, "
                        " longitude REAL, "
                        " thumbnail BLOB)"_s)) {
            qInfo() << "Thumbnails database table (v2) created";
        } else {
            qWarning() << "Thumbnails database table (v2) creation failure:" << query.lastError();
        }

        QQueue<const FileGroup *> newFiles;
        newFiles.reserve(fileGroupList.size());

        // retrieve thumbnails & locations
        for (const FileGroup * const fileGroup : fileGroupList) {
            query.prepare(u"SELECT timestamp, latitude, longitude, thumbnail "
                           "FROM thumbnails2 "
                           "WHERE path = :path AND timestamp >= :timestamp"_s);
            query.bindValue(u":path"_s, fileGroup->filePath);
            query.bindValue(u":timestamp"_s, fileGroup->lastModified);
            if (query.exec()) {
                qInfo() << "Thumbnail retrieval query succeeded:" << fileGroup->filePath;
            } else {
                qWarning() << "Thumbnail retrieval query failed:" << fileGroup->filePath << ":" << query.lastError();
            }
            if (query.next()) {
                qInfo() << "Thumbnail retrieval query has a result:" << fileGroup->filePath;
                std::optional<Location> possibleLocation;
                if (! query.value(u"latitude"_s).isNull()
                    && ! query.value(u"longitude"_s).isNull())
                {
                    possibleLocation.emplace(Location(query.value(u"latitude"_s).toDouble(),
                                                      query.value(u"longitude"_s).toDouble()));
                }
                std::optional<QPixmap> possibleThumbnail;
                if (! query.value(u"thumbnail"_s).isNull())
                {
                    const QByteArray bytes = query.value(u"thumbnail"_s).toByteArray();
                    QPixmap pixmap = QPixmap();
                    pixmap.loadFromData(bytes);
                    possibleThumbnail.emplace(pixmap);
                }

                qInfo() << "Thumbnail retrieval result is not null:" << fileGroup->filePath;
                ModelItem *item = new ModelItem(settings, directory, thumbnailCache, fileGroup, possibleLocation, possibleThumbnail);
                connect(item, &ModelItem::fileRenameFailed,
                        this, &Model::fileRenameFailed);
                connect(item, &ModelItem::invalidCharacters,
                        this, &Model::invalidCharacters);
                connect(item, &ModelItem::invalidCharacterSequence,
                        this, &Model::invalidCharacterSequence);
                images.append(item);
                Q_EMIT loadingProgressed(11);
            } else {
                qInfo() << "Thumbnail retrieval query has no result:" << fileGroup->filePath;
                newFiles.append(fileGroup);
            }
        }

        newFiles.squeeze();

        // compute new thumbnails
        const qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        QList<NewItem> newThumbnails;
        {
            newThumbnails.reserve(newFiles.size());

            QMutex newFilesMutex;
            QMutex newThumbnailsMutex;
            QVector<QThread*> threads;
            QVector<Loader*> loaders;
            QEventLoop eventLoop;
            QAtomicInt finishedCount = 0;
            const int threadCount = QThread::idealThreadCount();

            threads.reserve(threadCount);
            loaders.reserve(threadCount);

            for (int i = 0 ; i < threadCount ; i++) {
                QThread* thread = new QThread;
                Loader* loader = new Loader(newFiles, newFilesMutex,
                                            newThumbnails, newThumbnailsMutex,
                                            settings, directory, thumbnailCache);
                loader->moveToThread(thread);

                connect(thread, &QThread::started, loader, &Loader::loadAll);
                connect(thread, &QThread::finished, thread, &QThread::deleteLater);

                connect(loader, &Loader::loadingProgressed, this, &Model::loadingProgressed);
                connect(loader, &Loader::finished, thread, &QThread::quit);
                connect(loader, &Loader::finished, loader, &Loader::deleteLater);
                connect(loader, &Loader::finished,
                        this,
                        [&eventLoop, &finishedCount, threadCount]() {
                            if (++finishedCount == threadCount) {
                                eventLoop.quit();
                            }
                        });

                threads.append(thread);
                loaders.append(loader);

                thread->start();
            }

            eventLoop.exec();
        }

        // store new thumbnails
        if (query.exec(u"BEGIN TRANSACTION"_s)) {
            qInfo() << "Thumbnail storing transaction begin query succeeded";
        } else {
            qWarning() << "Thumbnail storing transaction begin query failed:" << query.lastError();
        }
        for (const NewItem &item : std::as_const(newThumbnails)) {
            connect(item.modelItem, &ModelItem::fileRenameFailed,
                    this, &Model::fileRenameFailed);
            connect(item.modelItem, &ModelItem::invalidCharacters,
                    this, &Model::invalidCharacters);
            connect(item.modelItem, &ModelItem::invalidCharacterSequence,
                    this, &Model::invalidCharacterSequence);
            images.append(item.modelItem);

            QVariant latitude, longitude, thumbnail;
            if (const std::optional<Location> &location = item.modelItem->getLocation()) {
                latitude = location->getLatitude();
                longitude = location->getLongitude();
            }
            if (item.thumbnail != nullptr)
                thumbnail = *item.thumbnail;
            query.prepare(u"INSERT OR REPLACE INTO thumbnails2 (path, timestamp, latitude, longitude, thumbnail) "
                           "VALUES (:path, :timestamp, :latitude, :longitude, :thumbnail)"_s);
            query.bindValue(u":path"_s, item.filePath);
            query.bindValue(u":timestamp"_s, timestamp);
            query.bindValue(u":latitude"_s, latitude);
            query.bindValue(u":longitude"_s, longitude);
            query.bindValue(u":thumbnail"_s, thumbnail);
            if (query.exec()) {
                qInfo() << "Thumbnail storing query succeeded:" << item.filePath;
            } else {
                qWarning() << "Thumbnail storing query failed:" << item.filePath << ":" << query.lastError();
            }

            Q_EMIT loadingProgressed();
            delete item.thumbnail;
        }
        if (query.exec(u"END TRANSACTION"_s)) {
            qInfo() << "Thumbnail storing transaction end query succeeded";
        } else {
            qWarning() << "Thumbnail storing transaction end query failed:" << query.lastError();
        }

        // limit database size
        if (query.exec(u"DELETE FROM thumbnails2 "
                        "WHERE path NOT IN (SELECT path FROM thumbnails2 ORDER BY timestamp DESC LIMIT %1)"_s.arg(settings.thumbnailDatabaseLimit))) {
            qInfo() << "Thumbnails database size limiting query succeeded";
        } else {
            qWarning() << "Thumbnails database size limiting query failed:" << query.lastError();
        }

        // repacking database
        if (query.exec(u"VACUUM"_s)) {
            qInfo() << "Thumbnails database repacking query succeeded";
        } else {
            qWarning() << "Thumbnails database repacking query failed:" << query.lastError();
        }

        db.close();
    }

    QSqlDatabase::removeDatabase(u"load"_s);

    images.squeeze();

    const qint64 dbFileSize = QFile(dbPath).size();
    qInfo() << "Thumbnails database file size:" << dbFileSize << "bytes";

    completerModel->insert(images);

    Q_EMIT loadingFinished(dbFileSize);
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    if (index.column() == IdentifierCol) {
        return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    } else if (index.column() == MapCol) {
        if (images.at(index.row())->getLocation().has_value())
            return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
        return Qt::ItemNeverHasChildren;
    } else if (index.column() >= FirstNameCol) {
        if (images.at(index.row())->getNamesSize() > (index.column() - 1 - FirstNameCol))
            return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        else
            return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable;
    }
    else
        return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

int Model::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return images.size();
}

int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return settings.maxNames + FirstNameCol;
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case ThumbnailCol: return tr("Thumbnail");
        case IdentifierCol: return tr("Identifier");
        case MapCol: return tr("Map");
        default: return tr("Name %1").arg(section + 1 - FirstNameCol);
        }
    }

    return QVariant();
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= images.size())
        return QVariant();

    if (index.column() >= settings.maxNames + FirstNameCol)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        int col = index.column();
        if (col == ThumbnailCol) {
            if (images.at(index.row())->getThumbnail().has_value())
                return QVariant();
            else
                return tr("%1 file").arg(images.at(index.row())->getExtension().toUpper());
        } else if (col == IdentifierCol) {
            return images.at(index.row())->getId();
        } else if (col >= FirstNameCol) {
            col -= FirstNameCol;
            if (col >= images.at(index.row())->getNamesSize()) {
                return QVariant();
            } else {
                return images.at(index.row())->getName(col);
            }
        } else {
            return QVariant();
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == ThumbnailCol) {
            return images.at(index.row())->getThumbnail().value_or(QPixmap());
        } else if (index.column() == MapCol) {
            if (images.at(index.row())->getLocation().has_value())
                return mapIcon;
            return QVariant();
        } else {
            return QVariant();
        }
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == ThumbnailCol) {
            return images.at(index.row())->getTooltip();
        } else {
            return QVariant();
        }
    } else if (role == ImagePathRole) {
        if (index.column() == ThumbnailCol) {
            return images.at(index.row())->getPath();
        } else {
            return QVariant();
        }
    } else if (role == MapUrlRole) {
        if (index.column() == MapCol) {
            if (const std::optional<Location> &location = images.at(index.row())->getLocation())
                return location->getUrl();
            return QVariant();
        } else {
            return QVariant();
        }
    } else
        return QVariant();
}

bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (! index.isValid())
        return false;

    if (index.column() == IdentifierCol) {
        if (role == ChangeIdRole) {
            const QString valueStr = value.toString();
            if (images.at(index.row())->getId() == valueStr)
                return false;
            if (std::any_of(images.cbegin(), images.cend(),
                            [&valueStr](const ModelItem *item) {
                                return valueStr == item->getId();
                            }))
            {
                Q_EMIT identifiersMustBeUnique(valueStr);
                return false;
            }
            if (images.at(index.row())->setId(valueStr)) {
                Q_EMIT dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
                Q_EMIT dataChanged(index.siblingAtColumn(ThumbnailCol),
                                   index.siblingAtColumn(ThumbnailCol),
                                   {ImagePathRole});
                return true;
            }
        }
    }
    else if (index.column() >= FirstNameCol) {
        const int nameIdx = index.column() - FirstNameCol;

        if (role == Qt::EditRole) {
            QString beforeStr;
            if (nameIdx < images.at(index.row())->getNamesSize())
                beforeStr = images.at(index.row())->getName(nameIdx);
            const QString valueStr = value.toString();
            if (images.at(index.row())->setName(nameIdx, valueStr)) {
                const QString afterStr = images.at(index.row())->getName(nameIdx);
                completerModel->insert(afterStr);
                if (! beforeStr.isEmpty())
                    completerModel->remove(beforeStr);
                Q_EMIT dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
                Q_EMIT dataChanged(index.siblingAtColumn(ThumbnailCol),
                                   index.siblingAtColumn(ThumbnailCol),
                                   {ImagePathRole});
                return true;
            }
        }
        else if (role == InsertRole) {
            const QString valueStr = value.toString();
            if (images.at(index.row())->insertName(nameIdx, valueStr)) {
                const int namesSize = images.at(index.row())->getNamesSize();
                const QString afterStr = images.at(index.row())->getName(nameIdx);
                completerModel->insert(afterStr);
                Q_EMIT dataChanged(index,
                                   index.siblingAtColumn(FirstNameCol + namesSize - 1),
                                   {Qt::DisplayRole, Qt::EditRole});
                Q_EMIT dataChanged(index.siblingAtColumn(ThumbnailCol),
                                   index.siblingAtColumn(ThumbnailCol),
                                   {ImagePathRole});
                return true;
            }
        }
        else if (role == DeleteRole) {
            const int namesSize = images.at(index.row())->getNamesSize();
            QString beforeStr;
            if (nameIdx < namesSize)
                beforeStr = images.at(index.row())->getName(nameIdx);
            if (images.at(index.row())->deleteName(nameIdx)) {
                if (! beforeStr.isEmpty())
                    completerModel->remove(beforeStr);
                Q_EMIT dataChanged(index,
                                   index.siblingAtColumn(FirstNameCol + namesSize - 1),
                                   {Qt::DisplayRole, Qt::EditRole});
                Q_EMIT dataChanged(index.siblingAtColumn(ThumbnailCol),
                                   index.siblingAtColumn(ThumbnailCol),
                                   {ImagePathRole});
                return true;
            }
        }
    }

    return false;
}
