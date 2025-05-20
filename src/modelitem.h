// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELITEM_H
#define MODELITEM_H

#include "filegroup.h"
#include "location.h"
#include "settings.h"

#include <QDir>
#include <QPixmap>

class ModelItem : public QObject
{
    Q_OBJECT

public:
    explicit ModelItem(const Settings &settings,
                       const QDir &directory,
                       QCache<QString, QString> &thumbnailCache,
                       const FileGroup *fileGroup,
                       const std::optional<Location> &possibleLocation,
                       const std::optional<QPixmap> &possibleThumbnail,
                       QObject *parent = nullptr);
    void setLocation(const Location &location);
    void setThumbnail(const QPixmap &thumbnail);
    [[nodiscard]] QString getPath() const;
    [[nodiscard]] QString getTooltip() const;
    [[nodiscard]] const QString& getId() const;
    bool setId(const QString &id);
    [[nodiscard]] int getNamesSize() const;
    [[nodiscard]] const QString& getName(int index) const;
    [[nodiscard]] const QString& getExtension() const;
    [[nodiscard]] const std::optional<Location>& getLocation() const;
    [[nodiscard]] const std::optional<QPixmap>& getThumbnail() const;
    bool setName(int index, const QString &name);
    bool deleteName(int index);
    bool insertName(int index, const QString &name);

Q_SIGNALS:
    void fileRenameFailed(const QString &before, const QString &after);
    void invalidCharacters();
    void invalidCharacterSequence();

private:
    [[nodiscard]] static QString computeBasePath(const QDir &folder, const QString &id, const QStringList &names);
    [[nodiscard]] static QList<QPair<QString, QString>> computeNewPaths(const QDir &folder,
                                                                        const QString &oldId, const QStringList &oldNames,
                                                                        const QString &newId, const QStringList &newNames,
                                                                        const QStringList &extensions);
    [[nodiscard]] bool isValidPathChars(const QString &str);
    bool syncFilename(const QList<QPair<QString, QString>> &pathsList);

    const Settings &settings;
    QCache<QString, QString> &thumbnailCache;
    const QDir &folder;
    QString id;
    QStringList names;
    const QStringList extensions;
    const std::optional<Location> possibleLocation;
    const std::optional<QPixmap> possibleThumbnail;
};

#endif // MODELITEM_H
