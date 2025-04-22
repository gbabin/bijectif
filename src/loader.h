// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOADER_H
#define LOADER_H

#include "location.h"
#include "modelitem.h"
#include "settings.h"

#include <QFileInfo>
#include <QMutex>
#include <QPixmap>

struct NewItem
{
    QString filePath;
    ModelItem* modelItem;
    QByteArray* thumbnail;
};

class Loader : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Loader)

public:
    static const QStringList imageExtensions;
    static const QStringList videoExtensions;

    explicit Loader(QQueue<QFileInfo> &entries, QMutex &entriesMutex,
                    QList<NewItem> &newThumbnails, QMutex &newThumbnailsMutex,
                    const Settings &settings, QCache<QString, QString> &thumbnailCache,
                    QObject *parent = nullptr);
    ~Loader() override = default;

public Q_SLOTS:
    void loadAll();

Q_SIGNALS:
    void loadingProgressed(int step = 1);
    void tooManyParts(const QString &path);
    void finished();

private:
    static std::pair<std::optional<Location>, std::optional<QPixmap>> load(const Settings &settings, const QFileInfo &path);
    static std::optional<QPixmap> computeImageThumbnail(const Settings &settings, const QString &filePath, const QByteArray &bytes);
    static std::optional<QPixmap> computeVideoThumbnail(const Settings &settings, const QString &filePath);
    static std::optional<Location> readLocation(const QString &filePath, const QByteArray &bytes);

    QQueue<QFileInfo> &entries;
    QMutex &entriesMutex;
    QList<NewItem> &newThumbnails;
    QMutex &newThumbnailsMutex;
    const Settings &settings;
    QCache<QString, QString> &thumbnailCache;
};

#endif // LOADER_H
