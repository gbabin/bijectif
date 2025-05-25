// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOCATION_H
#define LOCATION_H

#include <QUrl>

class Location
{
public:
    explicit Location(double latitude, double longitude);
    [[nodiscard]] double getLatitude() const;
    [[nodiscard]] double getLongitude() const;
    [[nodiscard]] QUrl getUrl() const;

private:
    double latitude;
    double longitude;
};

#endif // LOCATION_H
