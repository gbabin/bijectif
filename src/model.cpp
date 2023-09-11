// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "model.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtConcurrent>

void Model::load(const QFileInfoList &files, const QString &dbDir)
{
    images.reserve(files.count());

    // prepare database
    QDir dir(dbDir.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) : dbDir);
    QDir().mkpath(dir.path());
    QString dbPath = dir.filePath("thumbnails.sqlite");
    if (QFile(dbPath).exists()) {
        qInfo() << "Thumbnails database already exists at" << dbPath;
    } else {
        qInfo() << "Thumbnails database does not exist at" << dbPath;
    }
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (db.open()) {
        qInfo() << "Thumbnails database opened";
    } else {
        qWarning() << "Thumbnails database opening failure:" << db.lastError();
    }
    QSqlQuery query(db);
    if (query.exec("CREATE TABLE IF NOT EXISTS thumbnails "
                   "(path TEXT PRIMARY KEY, "
                   " timestamp INTEGER, "
                   " thumbnail BLOB)")) {
        qInfo() << "Thumbnails database table created";
    } else {
        qWarning() << "Thumbnails database table creation failure:" << query.lastError();
    }

    QFileInfoList newFiles;

    // retrieve thumbnails
    for (const QFileInfo &file : files) {
        qint64 timestamp = file.lastModified().toSecsSinceEpoch();
        query.prepare("SELECT timestamp, thumbnail "
                      "FROM thumbnails "
                      "WHERE path = :path AND timestamp >= :timestamp");
        query.bindValue(":path", file.filePath());
        query.bindValue(":timestamp", timestamp);
        if (query.exec()) {
            qInfo() << "Thumbnail retrieval query succeeded:" << file.filePath();
        } else {
            qWarning() << "Thumbnail retrieval query failed:" << file.filePath() << ":" << query.lastError();
        }
        if (query.next()) {
            qInfo() << "Thumbnail retrieval query has a result:" << file.filePath();
            QByteArray bytes = query.value("thumbnail").toByteArray();
            QPixmap pixmap = QPixmap();
            pixmap.loadFromData(bytes);
            if (pixmap.isNull()) {
                qInfo() << "Thumbnail retrieval result is null:" << file.filePath();
                newFiles.append(file);
            } else {
                qInfo() << "Thumbnail retrieval result is not null:" << file.filePath();
                ModelItem *item = new ModelItem(file, pixmap);
                images.append(item);
                emit loadingProgressed(2);
            }
        } else {
            qInfo() << "Thumbnail retrieval query has no result:" << file.filePath();
            newFiles.append(file);
        }
    }

    // compute new thumbnails
    qint64 timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    QList<QPair<QString, ModelItem*>> newThumbnails;
    QMutex newThumbnailsMutex;
    try {
        QtConcurrent::blockingMap(qAsConst(newFiles),
                                  [this, &newThumbnailsMutex, &newThumbnails](const QFileInfo & entry){
                                      ModelItem *item = new ModelItem(entry);
                                      newThumbnailsMutex.lock();
                                      newThumbnails.append(QPair<QString, ModelItem*>(entry.filePath(), item));
                                      newThumbnailsMutex.unlock();
                                      emit loadingProgressed();
                                  });
    } catch (TooManyPartsError &e) {
        qCritical() << "Too many name parts:" << e.path;
        emit loadingFinished();
        throw;
    }

    // store new thumbnails
    for (const QPair<QString, ModelItem*> &item : qAsConst(newThumbnails)) {
        images.append(item.second);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        item.second->getThumbnail().save(&buffer, "PNG");
        query.prepare("INSERT OR REPLACE INTO thumbnails (path, timestamp, thumbnail) "
                      "VALUES (:path, :timestamp, :thumbnail)");
        query.bindValue(":path", item.first);
        query.bindValue(":timestamp", timestamp);
        query.bindValue(":thumbnail", bytes);
        if (query.exec()) {
            qInfo() << "Thumbnail storing query succeeded:" << item.first;
        } else {
            qWarning() << "Thumbnail storing query failed:" << item.first << ":" << query.lastError();
        }
        emit loadingProgressed();
    }

    // limit database size
    if (query.exec("DELETE FROM thumbnails "
                   "WHERE path NOT IN (SELECT path FROM thumbnails ORDER BY timestamp DESC LIMIT 10000);")) {
        qInfo() << "Thumbnails database size limiting query succeeded";
    } else {
        qWarning() << "Thumbnails database size limiting query failed:" << query.lastError();
    }

    db.close();

    images.squeeze();

    emit loadingFinished();
}

Model::~Model()
{
    qDeleteAll(images);
    images.clear();
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
            return Qt::ItemNeverHasChildren;
    }
    else
        return Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
}

int Model::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return images.count();
}

int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
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
            return QVariant();
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
                            [&valueStr](const ModelItem *item){ return QString::compare(valueStr, item->getId()) == 0; }))
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
            if (valueStr.isEmpty())
                valueStr = tr("TEMPORARY");
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
