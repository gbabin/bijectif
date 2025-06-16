// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "custommapxyz.h"

using namespace Qt::Literals::StringLiterals;

CustomMapXYZ::CustomMapXYZ(QString url, int minZoomLevel, int maxZoomLevel)
    : url(std::move(url))
    , minZoomLevel(minZoomLevel)
    , maxZoomLevel(maxZoomLevel)
{
}

int CustomMapXYZ::minZoomlevel() const
{
    return minZoomLevel;
}

int CustomMapXYZ::maxZoomlevel() const
{
    return maxZoomLevel;
}

QString CustomMapXYZ::tilePosToUrl(const QGV::GeoTilePos& tilePos) const
{
    return QString(url)
        .replace(u"{z}"_s, QString::number(tilePos.zoom()))
        .replace(u"{x}"_s, QString::number(tilePos.pos().x()))
        .replace(u"{y}"_s, QString::number(tilePos.pos().y()));
}
