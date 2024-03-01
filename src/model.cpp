// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "model.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent>

Model::Model(QObject *parent)
    : QAbstractTableModel(parent)
{
}

Model::~Model()
{
    qDeleteAll(images);
    images.clear();
}

void Model::load(const QFileInfoList &files, const QString &dbPath)
{
    images.reserve(files.count());

    // prepare database
    if (QFile(dbPath).exists()) {
        qInfo() << "Thumbnails database already exists at" << dbPath;
    } else {
        qInfo() << "Thumbnails database does not exist at" << dbPath;
    }

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
        db.setDatabaseName(dbPath);
        if (db.open()) {
            qInfo() << "Thumbnails database opened";
        } else {
            qWarning() << "Thumbnails database opening failure:" << db.lastError();
        }
        QSqlQuery query(db);
        if (query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS thumbnails "
                                      "(path TEXT PRIMARY KEY, "
                                      " timestamp INTEGER, "
                                      " thumbnail BLOB)"))) {
            qInfo() << "Thumbnails database table created";
        } else {
            qWarning() << "Thumbnails database table creation failure:" << query.lastError();
        }

        QFileInfoList newFiles;

        // retrieve thumbnails
        for (const QFileInfo &file : files) {
            qint64 timestamp = file.lastModified().toSecsSinceEpoch();
            query.prepare(QStringLiteral("SELECT timestamp, thumbnail "
                                         "FROM thumbnails "
                                         "WHERE path = :path AND timestamp >= :timestamp"));
            query.bindValue(QStringLiteral(":path"), file.filePath());
            query.bindValue(QStringLiteral(":timestamp"), timestamp);
            if (query.exec()) {
                qInfo() << "Thumbnail retrieval query succeeded:" << file.filePath();
            } else {
                qWarning() << "Thumbnail retrieval query failed:" << file.filePath() << ":" << query.lastError();
            }
            if (query.next()) {
                qInfo() << "Thumbnail retrieval query has a result:" << file.filePath();
                QByteArray bytes = query.value(QStringLiteral("thumbnail")).toByteArray();
                QPixmap pixmap = QPixmap();
                pixmap.loadFromData(bytes);
                if (pixmap.isNull()) {
                    qInfo() << "Thumbnail retrieval result is null:" << file.filePath();
                    newFiles.append(file);
                } else {
                    qInfo() << "Thumbnail retrieval result is not null:" << file.filePath();
                    ModelItem *item = new ModelItem(file, pixmap);
                    images.append(item);
                    emit loadingProgressed(11);
                }
            } else {
                qInfo() << "Thumbnail retrieval query has no result:" << file.filePath();
                newFiles.append(file);
            }
        }

        struct NewItem {
            QString filePath;
            ModelItem* modelItem;
            QByteArray* thumbnail;
        } ;

        // compute new thumbnails
        const qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
        QList<NewItem> newThumbnails;
        QMutex newThumbnailsMutex;
        try {
            QtConcurrent::blockingMap(std::as_const(newFiles),
                                      [this, &newThumbnailsMutex, &newThumbnails](const QFileInfo & entry){
                                          ModelItem *item = new ModelItem(entry);
                                          QByteArray *bytes = new QByteArray();
                                          if (item->getThumbnail().userType() == QMetaType::QPixmap) {
                                              const QPixmap thumbnail = item->getThumbnail().value<QPixmap>();
                                              QBuffer buffer(bytes);
                                              buffer.open(QIODevice::WriteOnly);
                                              thumbnail.save(&buffer, "PNG");
                                              buffer.close();
                                          }
                                          newThumbnailsMutex.lock();
                                          newThumbnails.append(NewItem{entry.filePath(), item, bytes});
                                          newThumbnailsMutex.unlock();
                                          emit loadingProgressed(10);
                                      });
        } catch (TooManyPartsError &e) {
            qCritical() << "Too many name parts:" << e.path;
            emit loadingFinished();
            throw;
        }

        // store new thumbnails
        if (query.exec(QStringLiteral("BEGIN TRANSACTION"))) {
            qInfo() << "Thumbnail storing transaction begin query succeeded";
        } else {
            qWarning() << "Thumbnail storing transaction begin query failed:" << query.lastError();
        }
        for (const NewItem &item : std::as_const(newThumbnails)) {
            images.append(item.modelItem);
            if (item.thumbnail->isNull()) {
                qInfo() << "Thumbnail storing ignored (null):" << item.filePath;
            } else {
                query.prepare(QStringLiteral("INSERT OR REPLACE INTO thumbnails (path, timestamp, thumbnail) "
                                             "VALUES (:path, :timestamp, :thumbnail)"));
                query.bindValue(QStringLiteral(":path"), item.filePath);
                query.bindValue(QStringLiteral(":timestamp"), timestamp);
                query.bindValue(QStringLiteral(":thumbnail"), *item.thumbnail);
                if (query.exec()) {
                    qInfo() << "Thumbnail storing query succeeded:" << item.filePath;
                } else {
                    qWarning() << "Thumbnail storing query failed:" << item.filePath << ":" << query.lastError();
                }
            }
            emit loadingProgressed();
            delete item.thumbnail;
        }
        if (query.exec(QStringLiteral("END TRANSACTION"))) {
            qInfo() << "Thumbnail storing transaction end query succeeded";
        } else {
            qWarning() << "Thumbnail storing transaction end query failed:" << query.lastError();
        }

        // limit database size
        if (query.exec(QStringLiteral("DELETE FROM thumbnails "
                                      "WHERE path NOT IN (SELECT path FROM thumbnails ORDER BY timestamp DESC LIMIT 10000)"))) {
            qInfo() << "Thumbnails database size limiting query succeeded";
        } else {
            qWarning() << "Thumbnails database size limiting query failed:" << query.lastError();
        }

        // repacking database
        if (query.exec(QStringLiteral("VACUUM"))) {
            qInfo() << "Thumbnails database repacking query succeeded";
        } else {
            qWarning() << "Thumbnails database repacking query failed:" << query.lastError();
        }

        db.close();
    }

    for (const QString &name : QSqlDatabase::connectionNames())
        QSqlDatabase::removeDatabase(name);

    images.squeeze();

    qint64 dbFileSize = QFile(dbPath).size();
    qInfo() << "Thumbnails database file size:" << dbFileSize << "bytes";

    emit loadingFinished(dbFileSize);
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    if (index.column() == 1) {
        return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
    else if (index.column() > 1) {
        if (images.at(index.row())->getNamesSize() > (index.column() - 3))
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

    return images.count();
}

int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return maxNames+2;
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("Thumbnail");
        case 1: return tr("Identifier");
        default: return tr("Name %1").arg(section-1);
        }
    }

    return QVariant();
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= images.count())
        return QVariant();

    if (index.column() > maxNames+1)
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        int col = index.column();
        if (col == 0) {
            return images.at(index.row())->getThumbnail();
        } else if (col == 1) {
            return images.at(index.row())->getId();
        } else{
            col-=2;
            if (col >= images.at(index.row())->getNamesSize()) {
                return QVariant();
            } else {
                return images.at(index.row())->getName(col);
            }
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == 0) {
            return images.at(index.row())->getThumbnail();
        } else {
            return QVariant();
        }
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == 0) {
            return images.at(index.row())->getTooltip();
        } else {
            return QVariant();
        }
    } else if (role == ImagePathRole) {
        if (index.column() == 0) {
            return images.at(index.row())->getPath();
        } else {
            return QVariant();
        }
    } else
        return QVariant();
}

bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.column() == 1) {
        if (role == ChangeIdRole) {
            QString valueStr = value.toString();
            valueStr = valueStr.trimmed();
            if (std::any_of(images.cbegin(), images.cend(),
                            [&valueStr](const ModelItem *item){
                                return QString::compare(valueStr, item->getId()) == 0; }))
                return false;
            if (images.at(index.row())->setId(valueStr)) {
                emit dataChanged(index, index);
                return true;
            }
        }
    }
    else if (index.isValid() && index.column() >= 2) {
        int nameIdx = index.column() - 2;

        if (role == Qt::EditRole) {
            QString valueStr = value.toString();
            if (images.at(index.row())->setName(nameIdx, valueStr)) {
                emit dataChanged(index, index);
                return true;
            }
        }
        else if (role == InsertRole) {
            QString valueStr = value.toString();
            if (images.at(index.row())->insertName(nameIdx, valueStr)) {
                emit dataChanged(index, index.siblingAtColumn(maxNames+1));
                return true;
            }
        }
        else if (role == DeleteRole) {
            if (images.at(index.row())->deleteName(nameIdx)) {
                emit dataChanged(index, index.siblingAtColumn(maxNames+1));
                return true;
            }
        }
    }

    return false;
}
