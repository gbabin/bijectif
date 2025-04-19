// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "modelitem.h"
#include "loader.h"

#include <libexif/exif-data.h>

#include <QBuffer>
#include <QCache>
#include <QImageReader>
#include <QMessageBox>
#include <QSqlQuery>

using namespace Qt::Literals::StringLiterals;

TooManyPartsError::TooManyPartsError(QString path)
    : path(std::move(path))
{
}

void TooManyPartsError::raise() const
{
    throw *this;
}

TooManyPartsError* TooManyPartsError::clone() const
{
    return new TooManyPartsError(*this);
}

ModelItem::ModelItem(const Settings &settings,
                     QCache<QString, QString> &thumbnailCache,
                     const QFileInfo &path,
                     const std::optional<Location> &possibleLocation,
                     const std::optional<QPixmap> &possibleThumbnail,
                     QObject *parent)
    : QObject(parent)
    , settings(settings)
    , thumbnailCache(thumbnailCache)
    , folder(path.dir())
    , extension(path.suffix())
    , possibleLocation(possibleLocation)
    , possibleThumbnail(possibleThumbnail)
{
    const QString basename = path.completeBaseName();
    const qsizetype index_sep = basename.indexOf(u" - "_s);

    if (index_sep != -1) {
        id = basename.sliced(0, index_sep);
        names = basename.sliced(index_sep+3).split(u", "_s, Qt::SkipEmptyParts);
        if (names.size() > settings.maxNames) {
            throw TooManyPartsError(path.filePath());
        }
    } else {
        id = basename;
    }
}

void ModelItem::setLocation(const Location &location)
{
    possibleLocation.emplace(location);
}

void ModelItem::setThumbnail(const QPixmap &thumbnail)
{
    possibleThumbnail.emplace(thumbnail);
}

QString ModelItem::getPath() const
{
    return computePath(*this, this->id, this->names);
}

QString ModelItem::getTooltip() const
{
    if (! possibleThumbnail.has_value()) {
        return tr("Thumbnail unavailable");
    } else if (Loader::videoExtensions.contains(QFileInfo(getPath()).suffix().toLower())) {
        return tr("Video");
    } else {
        const QString path = getPath();
        QString* imgBase64 = thumbnailCache.object(path);
        if (imgBase64 == nullptr) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QImage image = reader.read();

            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            image.scaled(settings.tooltipSize,
                         settings.tooltipSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation)
                .save(&buffer, "PNG");
            buffer.close();

            const QString tmp = QString::fromLatin1(byteArray.toBase64().data());
            imgBase64 = new QString(tmp);
            thumbnailCache.insert(path, imgBase64, 1);
        }

        return u"<img src='data:image/png;base64, %1'>"_s.arg(*imgBase64);
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

    const QString newPath = computePath(*this, id, this->names);

    if (syncFilename(getPath(), newPath)) {
        this->id = id;
        return true;
    } else {
        return false;
    }
}

int ModelItem::getNamesSize() const
{
    return names.size();
}

const QString& ModelItem::getName(int index) const
{
    return names.at(index);
}

const QString& ModelItem::getExtension() const
{
    return extension;
}

const std::optional<Location>& ModelItem::getLocation() const
{
    return possibleLocation;
}

const std::optional<QPixmap>& ModelItem::getThumbnail() const
{
    return possibleThumbnail;
}

bool ModelItem::setName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    const QString nameT = name.trimmed();

    if (nameT.isEmpty()) return false;
    if (index < names.size() && names.at(index) == nameT) return false;
    if (index > names.size()) return false;

    QStringList newNames(names);

    if (index == newNames.size()) {
        newNames.append(nameT);
    } else {
        newNames.replace(index, nameT);
    }

    const QString newPath = computePath(*this, this->id, newNames);

    if (syncFilename(getPath(), newPath)) {
        names = newNames;
        return true;
    } else {
        return false;
    }
}

bool ModelItem::deleteName(int index)
{
    if (index >= names.size()) return false;

    QStringList newNames(names);

    newNames.remove(index);

    const QString newPath = computePath(*this, this->id, newNames);

    if (syncFilename(getPath(), newPath)) {
        names = newNames;
        return true;
    } else {
        return false;
    }
}

bool ModelItem::insertName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    const QString nameT = name.trimmed();

    if (nameT.isEmpty()) return false;
    if (index > names.size()) return false;
    if (names.size() == settings.maxNames) return false;

    QStringList newNames(names);

    newNames.insert(index, nameT);

    const QString newPath = computePath(*this, this->id, newNames);

    if (syncFilename(getPath(), newPath)) {
        names = newNames;
        return true;
    } else {
        return false;
    }
}

bool ModelItem::isValidPathChars(const QString &str)
{
    if (str.contains(u"<"_s)
        || str.contains(u">"_s)
        || str.contains(u":"_s)
        || str.contains(u"\""_s)
        || str.contains(u"/"_s)
        || str.contains(u"\\"_s)
        || str.contains(u"|"_s)
        || str.contains(u"?"_s)
        || str.contains(u"*"_s)
        || str.contains(u","_s))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden characters"),
                                 tr("The following characters are not allowed:\n< > : \" / \\ | ? * ,"));
        return false;
    }

    if (str.contains(u" - "_s))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden character sequence"),
                                 tr("The following character sequence is not allowed: \" - \" (hyphen with surrounding spaces)"));
        return false;
    }

    return true;
}

QString ModelItem::computePath(const ModelItem &item, const QString &id, const QStringList &names)
{
    if (names.isEmpty())
        return item.folder.filePath(id + u"."_s + item.extension);
    else
        return item.folder.filePath(id + u" - "_s
                                    + names.join(u", "_s)
                                    + u"."_s + item.extension);
}

bool ModelItem::syncFilename(const QString &oldPath, const QString &newPath)
{
    if (QFile::rename(oldPath, newPath)) {
        QFile::rename(oldPath + u".dop"_s, newPath + u".dop"_s); // Rename DxO PhotoLab sidecar file if possible
        QSqlQuery query;
        query.prepare(u"UPDATE thumbnails2 "
                       "SET path = :newPath "
                       "WHERE path = :oldPath"_s);
        query.bindValue(u":newPath"_s, newPath);
        query.bindValue(u":oldPath"_s, oldPath);
        query.exec();
        return true;
    } else {
        QMessageBox::critical(nullptr,
                              tr("Unable to rename file"),
                              tr("Before : %1\nAfter : %2").arg(oldPath, newPath));
        return false;
    }
}
