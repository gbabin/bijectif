// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"

Settings::Settings()
    : Settings(QSettings(QSettings::IniFormat,
                         QSettings::UserScope,
                         QStringLiteral("bijectif")))
{
}

Settings::Settings(const QSettings &settings)
    : fileName(settings.fileName())
    , thumbnailSize(settings.value("thumbnail size", 128).toInt())
    , tooltipSize(settings.value("tooltip size", 512).toInt())
    , minWidth(settings.value("min width", 800).toInt())
    , minHeight(settings.value("min height", 600).toInt())
    , maxNames(settings.value("max names", 8).toInt())
    , maxFileSize(settings.value("max file size", 20*1024*1024).toInt())
    , fontSize(settings.value("font size", 16).toInt())
    , thumbnailCacheSize(settings.value("thumbnail cache size", 100).toInt())
    , thumbnailDatabaseLimit(settings.value("thumbnail database limit", 10000).toInt())
{
}
