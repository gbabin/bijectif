// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "memorycache.h"

#include <QVariant>

using namespace Qt::Literals::StringLiterals;

MemoryCache::MemoryCache(qsizetype maxCost)
    : cache(maxCost)
{
}

void MemoryCache::insertFullImage(const QString& path, const QPixmap& pixmap)
{
    const QString cacheKey = u"fullimage %1"_s.arg(path);

    const QWriteLocker locker(&lock);
    if (cache.contains(cacheKey)) {
        qDebug() << cacheKey << "already in memory cache";
    } else {
        const qsizetype cost = (pixmap.height() * pixmap.width() * pixmap.depth()) / 8;
        qDebug() << cacheKey << "insert in memory cache with cost =" << cost;
        cache.insert(cacheKey, new QVariant(pixmap), cost);
    }
}

QPixmap MemoryCache::getFullImage(const QString& path)
{
    const QString cacheKey = u"fullimage %1"_s.arg(path);

    const QReadLocker locker(&lock);
    const QVariant* variant = cache.object(cacheKey);
    if (variant == nullptr) {
        qDebug() << cacheKey << "not in memory cache";
        return QPixmap();
    } else {
        qDebug() << cacheKey << "in memory cache";
        return variant->value<QPixmap>();
    }
}

void MemoryCache::insertTooltip(const QString& path, const QString& tooltip)
{
    const QString cacheKey = u"tooltip %1"_s.arg(path);

    const QWriteLocker locker(&lock);
    if (cache.contains(cacheKey)) {
        qDebug() << cacheKey << "already in memory cache";
    } else {
        const qsizetype cost = tooltip.capacity();
        qDebug() << cacheKey << "insert in memory cache with cost =" << cost;
        cache.insert(cacheKey, new QVariant(tooltip), cost);
    }
}

QString MemoryCache::getTooltip(const QString& path)
{
    const QString cacheKey = u"tooltip %1"_s.arg(path);

    const QReadLocker locker(&lock);
    const QVariant* variant = cache.object(cacheKey);
    if (variant == nullptr) {
        qDebug() << cacheKey << "not in memory cache";
        return QString();
    } else {
        qDebug() << cacheKey << "in memory cache";
        return variant->toString();
    }
}
