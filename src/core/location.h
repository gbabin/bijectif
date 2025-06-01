// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOCATION_H
#define LOCATION_H

#include <QMetaType>
#include <QUrl>

class Location
{
public:
    explicit Location() = default; // because of qvariant_cast
    explicit Location(double latitude, double longitude);
    [[nodiscard]] double getLatitude() const;
    [[nodiscard]] double getLongitude() const;
    [[nodiscard]] QUrl getUrl() const;

private:
    double latitude;
    double longitude;
};

Q_DECLARE_METATYPE(Location)

#endif // LOCATION_H
