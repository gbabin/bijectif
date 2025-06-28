// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include <QCache>
#include <QReadWriteLock>
#include <QPixmap>

class MemoryCache
{
    Q_DISABLE_COPY_MOVE(MemoryCache)

public:
    explicit MemoryCache(qsizetype maxCost);
    ~MemoryCache() = default;

    void insertFullImage(const QString& path, const QPixmap& pixmap);
    [[nodiscard]] QPixmap getFullImage(const QString& path);

private:
    QCache<QString, QVariant> cache;
    QReadWriteLock lock;
};

#endif // MEMORYCACHE_H
