// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "location.h"

using namespace Qt::Literals::StringLiterals;

Location::Location(double latitude, double longitude)
    : latitude(latitude)
    , longitude(longitude)
{
}

double Location::getLatitude() const
{
    return latitude;
}

double Location::getLongitude() const
{
    return longitude;
}

QUrl Location::getUrl() const
{
    static const QString base = u"https://www.openstreetmap.org/?mlat=%1&mlon=%2"_s;
    return QUrl(base.arg(latitude, 0, 'g', 6).arg(longitude, 0, 'g', 6));
}
