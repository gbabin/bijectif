// Copyright (C) 2023 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MODELITEM_H
#define MODELITEM_H

#include <QDir>
#include <QException>
#include <QPixmap>

class TooManyPartsError : public QException
{
public:
    TooManyPartsError(const QString &path) : path(path) {};
    void raise() const override { throw *this; }
    TooManyPartsError *clone() const override { return new TooManyPartsError(*this); }

    const QString path;
};

class ModelItem : public QObject
{
    Q_OBJECT

public:
    static const int maxFileSize = 20*1024*1024;
    static QVariant createThumbnail(const QFileInfo &path, const QPixmap &pixmap = QPixmap());

    explicit ModelItem(const QFileInfo &path, const QPixmap &pixmap = QPixmap());
    QString getPath() const;
    QString getTooltip() const;
    QString getId() const;
    bool setId(const QString &id);
    int getNamesSize();
    QString getName(int index) const;
    QVariant getThumbnail() const;
    bool setName(int index, const QString &name);
    bool deleteName(int index);
    bool insertName(int index, const QString &name);
    bool syncFilename(const QString &oldPath, const QStringList &oldNames);
    bool syncFilename(const QString &oldPath, const QString &oldId);

private:
    static const QStringList imageExtensions;
    static QCache<QString, QString> cache;

    const QDir folder;
    QString id;
    QStringList names;
    const QString extension;
    const QVariant thumbnail;

    static bool isValidPathChars(const QString &str);
};

#endif // MODELITEM_H
