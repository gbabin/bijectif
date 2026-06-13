// Copyright (C) 2026 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef PYRAMID_H
#define PYRAMID_H

#include <QMetaType>
#include <QPixmap>
#include <QSharedPointer>
#include <QString>
#include <QVector>

class Pyramid
{
    Q_DISABLE_COPY_MOVE(Pyramid)

public:
    explicit Pyramid() = default;
    explicit Pyramid(const QString& path);
    ~Pyramid() = default;

    [[nodiscard]] bool isEmpty() const;
    [[nodiscard]] qsizetype getMemorySize() const;
    [[nodiscard]] QSize getOriginalSize() const;
    [[nodiscard]] QPair<QPixmap, qreal> getPixmap(QSize size) const;
    [[nodiscard]] QPair<QPixmap, qreal> getPixmap(qreal scale) const;

private:
    QVector<QPixmap> pixmaps;
    qsizetype memorySize;
};

Q_DECLARE_METATYPE(QSharedPointer<const Pyramid>)

#endif // PYRAMID_H
