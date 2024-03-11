// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "../src/settings.h"

Settings::Settings()
    : fileName(QStringLiteral("/fake.ini"))
    , thumbnailSize(128)
    , tooltipSize(512)
    , minWidth(800)
    , minHeight(600)
    , maxNames(8)
    , maxFileSize(20*1024*1024)
    , fontSize(16)
    , thumbnailCacheSize(100)
    , thumbnailDatabaseLimit(10000)
    , logVerbosity(2)
{
}

Settings::Settings(const QSettings &settings)
    : Settings()
{
    throw;
}
