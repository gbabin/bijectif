// Copyright (C) 2026 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "pyramid.h"

#include <QImageReader>

#define MIN_SIZE 128

Pyramid::Pyramid(const QString& path)
    : memorySize(0)
{
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Failed to load image in pyramid:" << path << ": Error" << reader.error() << reader.errorString();
    } else {
        const QPixmap pixmap = QPixmap::fromImage(image);
        pixmaps.append(pixmap);
        memorySize = ((qsizetype)pixmap.height() * pixmap.width() * pixmap.depth()) / 8;

        QPixmap last = pixmap;

        while (last.width() / 2 >= MIN_SIZE
               || last.height() / 2 >= MIN_SIZE)
        {
            last = last.scaled(last.width() / 2,
                               last.height() / 2,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
            pixmaps.append(last);
            memorySize += ((qsizetype)last.height() * last.width() * last.depth()) / 8;
        }
    }
}

bool Pyramid::isEmpty() const
{
    return pixmaps.isEmpty();
}

qsizetype Pyramid::getMemorySize() const
{
    Q_ASSERT(! isEmpty());

    return memorySize;
}

QSize Pyramid::getOriginalSize() const
{
    Q_ASSERT(! isEmpty());

    return pixmaps.constFirst().size();
}

QPair<QPixmap, qreal> Pyramid::getPixmap(QSize size) const
{
    Q_ASSERT(! isEmpty());
    Q_ASSERT(! size.isEmpty());

    const QSize originalSize = getOriginalSize();
    const qreal xScale = (qreal)size.width() / originalSize.width();
    const qreal yScale = (qreal)size.height() / originalSize.height();
    const qreal scale = qMax(xScale, yScale);

    return getPixmap(scale);
}

QPair<QPixmap, qreal> Pyramid::getPixmap(qreal scale) const
{
    Q_ASSERT(! isEmpty());
    Q_ASSERT(scale > 0.0);

    if (scale > 0.5)
        return qMakePair(pixmaps.constFirst(), 1.0);

    const unsigned int zoom = std::floor(1.0 / scale);
    const int wantedIndex = std::bit_width(zoom) - 1;
    const int index = qMin(wantedIndex, pixmaps.size() - 1);
    const qreal realScale = 1.0 / std::pow(2, index);

    return qMakePair(pixmaps[index], realScale);
}
