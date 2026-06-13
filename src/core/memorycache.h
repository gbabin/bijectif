// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef MEMORYCACHE_H
#define MEMORYCACHE_H

#include "pyramid.h"

#include <QCache>
#include <QReadWriteLock>

class MemoryCache
{
    Q_DISABLE_COPY_MOVE(MemoryCache)

public:
    explicit MemoryCache(qsizetype maxCost);
    ~MemoryCache() = default;

    void insertPyramid(const QString& path, QSharedPointer<const Pyramid> pyramid);
    [[nodiscard]] QSharedPointer<const Pyramid> getPyramid(const QString& path);

private:
    QCache<QString, QVariant> cache;
    QReadWriteLock lock;
};

#endif // MEMORYCACHE_H
