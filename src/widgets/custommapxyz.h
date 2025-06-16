// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef CUSTOMMAPXYZ_H
#define CUSTOMMAPXYZ_H

#include <QGeoView/QGVLayerTilesOnline.h>

class CustomMapXYZ : public QGVLayerTilesOnline
{
    Q_OBJECT

public:
    explicit CustomMapXYZ(QString url, int minZoomLevel, int maxZoomLevel);

private:
    [[nodiscard]] int minZoomlevel() const override;
    [[nodiscard]] int maxZoomlevel() const override;
    [[nodiscard]] QString tilePosToUrl(const QGV::GeoTilePos& tilePos) const override;

    const QString url;
    const int minZoomLevel;
    const int maxZoomLevel;
};

#endif // CUSTOMMAPXYZ_H
