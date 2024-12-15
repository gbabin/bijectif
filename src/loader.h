// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef LOADER_H
#define LOADER_H

#include "settings.h"
#include "location.h"

#include <QFileInfo>
#include <QPixmap>

class Loader
{
public:
    static std::pair<std::optional<Location>, std::optional<QPixmap>> load(const Settings &settings, const QFileInfo &path);

    static const QStringList imageExtensions;
    static const QStringList videoExtensions;

private:
    static std::optional<QPixmap> computeImageThumbnail(const Settings &settings, const QString &filePath, const QByteArray &bytes);
    static std::optional<QPixmap> computeVideoThumbnail(const Settings &settings, const QString &filePath);
    static std::optional<Location> readLocation(const QString &filePath, const QByteArray &bytes);
};

#endif // LOADER_H
