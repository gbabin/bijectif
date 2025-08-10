// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOADER_H
#define LOADER_H

#include "filegroup.h"
#include "location.h"
#include "modelitem.h"
#include "settings.h"

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
    explicit Loader(QQueue<const FileGroup *> &fileGroups, QMutex &fileGroupsMutex,
                    QList<NewItem> &newThumbnails, QMutex &newThumbnailsMutex,
                    const Settings &settings, const QDir &directory, MemoryCache &memoryCache,
                    QObject *parent = nullptr);
    ~Loader() override = default;

public Q_SLOTS:
    void loadAll();

Q_SIGNALS:
    void loadingProgressed(int step = 1);
    void finished();

private:
    static std::pair<std::optional<Location>, std::optional<QPixmap>> load(const Settings &settings, const FileGroup *fileGroup);
    static std::optional<QPixmap> computeImageThumbnail(const Settings &settings, const QString &filePath, const QByteArray &bytes);
    static std::pair<std::optional<Location>, std::optional<QPixmap>> computeVideoThumbnailLocation(const Settings &settings, const QString &filePath);
    static std::optional<Location> readLocation(const QString &filePath, const QByteArray &bytes);

    QQueue<const FileGroup *> &fileGroups;
    QMutex &fileGroupsMutex;
    QList<NewItem> &newThumbnails;
    QMutex &newThumbnailsMutex;
    const Settings &settings;
    const QDir &directory;
    MemoryCache &memoryCache;
};

#endif // LOADER_H
