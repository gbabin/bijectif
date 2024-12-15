// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELITEM_H
#define MODELITEM_H

#include "settings.h"
#include "location.h"

#include <QDir>
#include <QException>
#include <QPixmap>

class TooManyPartsError : public QException
{
public:
    TooManyPartsError(QString path);
    void raise() const override;
    [[nodiscard]] TooManyPartsError* clone() const override;

    const QString path;
};

class ModelItem : public QObject
{
    Q_OBJECT

public:
    explicit ModelItem(const Settings &settings,
                       QCache<QString, QString> &thumbnailCache,
                       const QFileInfo &path,
                       const std::optional<Location> &possibleLocation,
                       const std::optional<QPixmap> &possibleThumbnail,
                       QObject *parent = nullptr);
    void setLocation(const Location &location);
    void setThumbnail(const QPixmap &thumbnail);
    [[nodiscard]] QString getPath() const;
    [[nodiscard]] QString getTooltip() const;
    [[nodiscard]] QString getId() const;
    bool setId(const QString &id);
    [[nodiscard]] int getNamesSize() const;
    [[nodiscard]] const QString& getName(int index) const;
    [[nodiscard]] const QString& getExtension() const;
    [[nodiscard]] const std::optional<Location>& getLocation() const;
    [[nodiscard]] const std::optional<QPixmap>& getThumbnail() const;
    bool setName(int index, const QString &name);
    bool deleteName(int index);
    bool insertName(int index, const QString &name);

private:
    [[nodiscard]] static bool isValidPathChars(const QString &str);
    [[nodiscard]] static QString computePath(const ModelItem &item, const QString &id, const QStringList &names);
    static bool syncFilename(const QString &oldPath, const QString &newPath);

    const Settings &settings;
    QCache<QString, QString> &thumbnailCache;
    const QDir folder;
    QString id;
    QStringList names;
    const QString extension;
    std::optional<Location> possibleLocation;
    std::optional<QPixmap> possibleThumbnail;
};

#endif // MODELITEM_H
