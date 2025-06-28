// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "modelitem.h"

#include <libexif/exif-data.h>

#include <QGeoView/Raster/QGVIcon.h>

#include <QBuffer>
#include <QCache>
#include <QImageReader>
#include <QSqlQuery>
#include <QStack>

using namespace Qt::Literals::StringLiterals;


QImage ModelItem::selectedPlaceholderIcon;
QImage ModelItem::unselectedPlaceholderIcon;

void ModelItem::loadIcons()
{
    selectedPlaceholderIcon = QImage(u":/resources/placeholder.png"_s)
                                  .scaled(32,
                                          32,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation);

    unselectedPlaceholderIcon = QImage(u":/resources/placeholder_grayscale.png"_s)
                                    .scaled(32,
                                            32,
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
}

ModelItem::ModelItem(const Settings &settings,
                     const QDir &directory,
                     MemoryCache &memoryCache,
                     const FileGroup *fileGroup,
                     const std::optional<Location> &possibleLocation,
                     const std::optional<QPixmap> &possibleThumbnail,
                     QObject *parent)
    : QObject(parent)
    , settings(settings)
    , memoryCache(memoryCache)
    , folder(directory)
    , id(fileGroup->id)
    , names(fileGroup->names)
    , extensions(fileGroup->getExtensions())
    , possibleLocation(possibleLocation)
    , possibleThumbnail(possibleThumbnail)
{
    if (possibleLocation) {
        const Location& location = possibleLocation.value();
        possibleMapIcon = new QGVIcon();
        possibleMapIcon->setFlag(QGV::ItemFlag::SelectCustom);
        possibleMapIcon->setGeometry(QGV::GeoPos(location.getLatitude(), location.getLongitude()));
        possibleMapIcon->setSelectable(true);
        possibleMapIcon->loadImage(unselectedPlaceholderIcon);
        possibleMapIcon->setZValue(0);
    } else {
        possibleMapIcon = nullptr;
    }
}

ModelItem::~ModelItem()
{
    delete possibleMapIcon;
}

void ModelItem::setSelected(bool selected)
{
    if (possibleMapIcon != nullptr) {
        if (selected) {
            if (! possibleMapIcon->isSelected())
                possibleMapIcon->select();
            if (possibleMapIcon->getZValue() != 1) {
                possibleMapIcon->loadImage(selectedPlaceholderIcon);
                possibleMapIcon->setZValue(1);
            }
        } else {
            if (possibleMapIcon->isSelected())
                possibleMapIcon->unselect();
            if (possibleMapIcon->getZValue() != 0) {
                possibleMapIcon->loadImage(unselectedPlaceholderIcon);
                possibleMapIcon->setZValue(0);
            }
        }
    }
}

QString ModelItem::getPath() const
{
    return computeBasePath(folder, id, names) + u"."_s + getExtension();
}

const QString& ModelItem::getId() const
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

QGVIcon* ModelItem::getMapIcon() const
{
    return possibleMapIcon;
}

const std::optional<QPixmap>& ModelItem::getThumbnail() const
{
    return possibleThumbnail;
}

bool ModelItem::setName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    if (name.isEmpty()) return false;
    if (index < names.size() && names.at(index) == name) return false;
    if (index > names.size()) return false;

    QStringList newNames(names);

    if (index == newNames.size()) {
        newNames.append(name);
    } else {
        newNames.replace(index, name);
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

    if (name.isEmpty()) return false;
    if (index > names.size()) return false;
    if (names.size() == settings.maxNames) return false;

    QStringList newNames(names);

    newNames.insert(index, name);

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
        Q_EMIT invalidCharacters();

        return false;
    }

    if (str.contains(u" - "_s))
    {
        Q_EMIT invalidCharacterSequence();

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

            Q_EMIT fileRenameFailed(paths.first, paths.second);

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
