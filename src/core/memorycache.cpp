// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "memorycache.h"

#include <QVariant>

using namespace Qt::Literals::StringLiterals;

MemoryCache::MemoryCache(qsizetype maxCost)
    : cache(maxCost)
{
}

void MemoryCache::insertPyramid(const QString& path, QSharedPointer<const Pyramid> pyramid)
{
    const QWriteLocker locker(&lock);
    if (cache.contains(path)) {
        qDebug() << path << "already in memory cache";
    } else {
        const qsizetype cost = pyramid->getMemorySize();
        qDebug() << path << "insert in memory cache with cost =" << cost;
        cache.insert(path, new QVariant(QVariant::fromValue(pyramid)), cost);
    }
}

QSharedPointer<const Pyramid> MemoryCache::getPyramid(const QString& path)
{
    const QReadLocker locker(&lock);
    const QVariant* variant = cache.object(path);
    if (variant == nullptr) {
        qDebug() << path << "not in memory cache";
        return QSharedPointer<const Pyramid>();
    } else {
        qDebug() << path << "in memory cache";
        return variant->value<QSharedPointer<const Pyramid>>();
    }
}
