// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "modelitem.h"
#include "model.h"
#include "window.h"

#include <QBuffer>
#include <QCache>
#include <QMessageBox>
#include <QSqlQuery>

const QStringList ModelItem::imageExtensions = {"bmp",
                                                "gif",
                                                "jpg", "jpeg",
                                                "png",
                                                "tif", "tiff"};

QCache<QString, QString> ModelItem::cache = QCache<QString, QString>(100);

QPixmap ModelItem::createThumbnail(const QFileInfo &path, const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        qInfo() << "Pixmap argument is null:" << path.filePath();
        if (imageExtensions.contains(path.suffix().toLower())) {
            if (path.size() <= maxFileSize) {
                QImage image = QImage(path.filePath());
                if (image.isNull()) {
                    qWarning() << "Image loading failed:" << path.filePath();
                    return QPixmap();
                } else {
                    qInfo() << "Image loading succeeded:" << path.filePath();
                    return QPixmap::fromImage(
                        image.scaled(Window::thumbnailSize,
                                     Window::thumbnailSize,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
                }
            } else {
                qInfo() << "Pixmap creation ignored because of size:" << path.filePath();
                return QPixmap();
            }
        } else {
            qInfo() << "Pixmap creation ignored because of extension:" << path.filePath();
            return QPixmap();
        }
    } else {
        qInfo() << "Pixmap argument is not null:" << path.filePath();
        return pixmap;
    }
}

ModelItem::ModelItem(const QFileInfo &path, const QPixmap &pixmap)
    : folder(path.dir())
    , extension(path.suffix())
    , thumbnail(createThumbnail(path, pixmap))
{
    QString basename = path.completeBaseName();
    qsizetype index_sep = basename.indexOf(" - ");

    if (index_sep != -1) {
        id = basename.sliced(0, index_sep);
        names = basename.sliced(index_sep+3).split(", ", Qt::SkipEmptyParts);
        if (names.size() > Model::maxNames) {
            throw TooManyPartsError(path.filePath());
        }
    } else {
        id = basename;
    }
}

QString ModelItem::getPath() const
{
    if (names.isEmpty())
        return folder.filePath(id + "." + extension);
    else
        return folder.filePath(id + " - " + names.join(", ") + "." + extension);
}

QString ModelItem::getTooltip() const
{
    if (thumbnail.isNull()) {
        return QString(tr("Image unavailable or too large"));
    } else {
        QString path = getPath();
        QString* imgBase64 = cache.object(path);
        if (imgBase64 == nullptr) {
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            QImage(path).scaled(Window::tooltipSize,
                                Window::tooltipSize,
                                Qt::KeepAspectRatio,
                                Qt::SmoothTransformation)
                .save(&buffer, "PNG");
            QString tmp = QString::fromLatin1(byteArray.toBase64().data());
            buffer.close();
            imgBase64 = new QString(tmp);
            cache.insert(path, imgBase64, 1);
        }

        return QString("<img src='data:image/png;base64, %1'>").arg(*imgBase64);
    }
}

QString ModelItem::getId() const
{
    return id;
}

bool ModelItem::setId(const QString &id)
{
    if (! isValidPathChars(id)) return false;

    if (id.isEmpty()) return false;

    QString oldPath = getPath();
    QString oldId = this->id;

    this->id = id;

    return syncFilename(oldPath, oldId);
}

int ModelItem::getNamesSize()
{
    return names.size();
}

QString ModelItem::getName(int index) const
{
    return names.at(index);
}

QPixmap ModelItem::getThumbnail() const
{
    return thumbnail;
}

bool ModelItem::setName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    QString nameT = name.trimmed();

    if (nameT.isEmpty()) return false;
    if (index < names.size() && names.at(index) == nameT) return false;
    if (index > names.size()) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    if (index == names.size()) {
        names.append(nameT);
    } else {
        names.replace(index, nameT);
    }

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::deleteName(int index)
{
    if (index >= names.size()) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    names.remove(index);

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::insertName(int index, const QString &name)
{
    if (index > names.size()) return false;
    if (names.size() == Model::maxNames) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    names.insert(index, name);

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::isValidPathChars(const QString &str)
{
    if (str.contains("<")
        || str.contains(">")
        || str.contains(":")
        || str.contains("\"")
        || str.contains("/")
        || str.contains("\\")
        || str.contains("|")
        || str.contains("?")
        || str.contains("*"))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden characters"),
                                 tr("The following caracters are not allowed:\n< > : \" / \\ | ? *"));
        return false;
    }

    if (str.contains(",")
        || str.contains(" - "))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden characters"),
                                 tr("The following caracters are not allowed: \",\" and \" - \""));
        return false;
    }

    return true;
}

bool ModelItem::syncFilename(const QString &oldPath, const QStringList &oldNames)
{
    QString newPath = getPath();

    if (QFile::rename(oldPath, newPath)) {
        QSqlQuery query;
        query.prepare("UPDATE thumbnails "
                      "SET path = :newPath "
                      "WHERE path = :oldPath");
        query.bindValue(":newPath", newPath);
        query.bindValue(":oldPath", oldPath);
        query.exec();
        return true;
    } else {
        names.clear();
        names.append(oldNames);
        QMessageBox::critical(nullptr,
                              tr("Unable to rename file"),
                              tr("Before : %1\nAfter : %2").arg(oldPath, newPath));
        return false;
    }
}

bool ModelItem::syncFilename(const QString &oldPath, const QString &oldId)
{
    QString newPath = getPath();

    if (QFile::rename(oldPath, newPath)) {
        QSqlQuery query;
        query.prepare("UPDATE thumbnails "
                      "SET path = :newPath "
                      "WHERE path = :oldPath");
        query.bindValue(":newPath", newPath);
        query.bindValue(":oldPath", oldPath);
        query.exec();
        return true;
    } else {
        id = oldId;
        QMessageBox::critical(nullptr,
                              tr("Unable to rename file"),
                              tr("Before : %1\nAfter : %2").arg(oldPath, newPath));
        return false;
    }
}
