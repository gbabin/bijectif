// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELITEM_H
#define MODELITEM_H

#include "settings.h"

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
    static QVariant createThumbnail(const Settings &settings, const QFileInfo &path, const QPixmap &pixmap = QPixmap());

    explicit ModelItem(const Settings &settings, QCache<QString, QString> &thumbnailCache, const QFileInfo &path, const QPixmap &pixmap = QPixmap(), QObject *parent = nullptr);
    [[nodiscard]] QString getPath() const;
    [[nodiscard]] QString getTooltip() const;
    [[nodiscard]] QString getId() const;
    bool setId(const QString &id);
    int getNamesSize();
    [[nodiscard]] QString getName(int index) const;
    [[nodiscard]] QVariant getThumbnail() const;
    bool setName(int index, const QString &name);
    bool deleteName(int index);
    bool insertName(int index, const QString &name);
    bool syncFilename(const QString &oldPath, const QStringList &oldNames);
    bool syncFilename(const QString &oldPath, const QString &oldId);

private:
    static bool isValidPathChars(const QString &str);
    static const QStringList imageExtensions;
    static const QStringList videoExtensions;

    const Settings &settings;
    QCache<QString, QString> &thumbnailCache;
    const QDir folder;
    QString id;
    QStringList names;
    const QString extension;
    const QVariant thumbnail;
};

#endif // MODELITEM_H
