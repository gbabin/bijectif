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
#include <QStack>

using namespace Qt::Literals::StringLiterals;

ModelItem::ModelItem(const Settings &settings,
                     const QDir &directory,
                     QCache<QString, QString> &thumbnailCache,
                     const FileGroup *fileGroup,
                     const std::optional<Location> &possibleLocation,
                     const std::optional<QPixmap> &possibleThumbnail,
                     QObject *parent)
    : QObject(parent)
    , settings(settings)
    , thumbnailCache(thumbnailCache)
    , folder(directory)
    , id(fileGroup->id)
    , names(fileGroup->names)
    , extensions(fileGroup->getExtensions())
    , possibleLocation(possibleLocation)
    , possibleThumbnail(possibleThumbnail)
{
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
    return computeBasePath(folder, id, names) + u"."_s + getExtension();
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

    const QList<QPair<QString, QString>> newPaths = computeNewPaths(folder,
                                                                    this->id, this->names,
                                                                    id, this->names,
                                                                    extensions);

    if (syncFilename(newPaths)) {
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
    return extensions.constFirst();
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

    const QList<QPair<QString, QString>> newPaths = computeNewPaths(folder,
                                                                    this->id, this->names,
                                                                    this->id, newNames,
                                                                    extensions);

    if (syncFilename(newPaths)) {
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

    const QList<QPair<QString, QString>> newPaths = computeNewPaths(folder,
                                                                    this->id, this->names,
                                                                    this->id, newNames,
                                                                    extensions);

    if (syncFilename(newPaths)) {
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

    const QList<QPair<QString, QString>> newPaths = computeNewPaths(folder,
                                                                    this->id, this->names,
                                                                    this->id, newNames,
                                                                    extensions);
    if (syncFilename(newPaths)) {
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

QString ModelItem::computeBasePath(const QDir &folder, const QString &id, const QStringList &names)
{
    if (names.isEmpty())
        return folder.filePath(id);
    else
        return folder.filePath(id + u" - "_s
                               + names.join(u", "_s));
}

QList<QPair<QString, QString>> ModelItem::computeNewPaths(const QDir &folder,
                                                          const QString &oldId, const QStringList &oldNames,
                                                          const QString &newId, const QStringList &newNames,
                                                          const QStringList &extensions)
{
    QList<QPair<QString, QString>> list;
    list.reserve(extensions.size());

    const QString oldBasepath = computeBasePath(folder, oldId, oldNames);
    const QString newBasepath = computeBasePath(folder, newId, newNames);

    for (const QString& extension : extensions) {
        list.append(qMakePair(oldBasepath + u"."_s + extension,
                              newBasepath + u"."_s + extension));
    }

    return list;
}

bool ModelItem::syncFilename(const QList<QPair<QString, QString>> &pathsList)
{
    Q_ASSERT(! pathsList.isEmpty());

    QStack<QPair<QString, QString>> renamed;

    for (const QPair<QString, QString>& paths : pathsList) {
        if (QFile::rename(paths.first, paths.second)) {
            qDebug() << "Renamed" << paths.first << "to" << paths.second;
            renamed.push(paths);
        } else {
            qCritical() << "Failed to rename" << paths.first << "to" << paths.second;

            while (! renamed.isEmpty()) {
                const QPair<QString, QString> renamedFile = renamed.pop();

                if (QFile::rename(renamedFile.second, renamedFile.first))
                    qDebug() << "Reverted" << renamedFile.second << "to" << renamedFile.first;
                else
                    qDebug() << "Failed to revert" << renamedFile.second << "to" << renamedFile.first;
            }

            QMessageBox::critical(nullptr,
                                  tr("Unable to rename file"),
                                  tr("Before : %1\nAfter : %2").arg(paths.first, paths.second));
            return false;
        }
    }

    const QSqlDatabase db = QSqlDatabase::database(u"update"_s, false);
    QSqlQuery query(db);
    query.prepare(u"UPDATE thumbnails2 "
                   "SET path = :newPath "
                   "WHERE path = :oldPath"_s);
    query.bindValue(u":oldPath"_s, pathsList.constFirst().first);
    query.bindValue(u":newPath"_s, pathsList.constFirst().second);
    if (! query.exec()) {
        qWarning() << "Failed to update database:" << pathsList.constFirst().second;
    }

    return true;
}
