// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"

using namespace Qt::Literals::StringLiterals;

Settings::Settings()
    : fileName(QStringLiteral("/fake.ini"))
    , thumbnailSize(128)
    , tooltipSize(512)
    , minWidth(800)
    , minHeight(600)
    , listedExtensions(QString(u"jpg,dnf,webp"_s).split(u","_s))
    , imageExtensions(QString(u"jpg,webp"_s).split(u","_s))
    , videoExtensions(QString(u""_s).split(u","_s))
    , maxNames(8)
    , maxFileSize(20*1024*1024)
    , fontSize(16)
    , thumbnailCacheSize(100)
    , thumbnailDatabaseLimit(10000)
    , logVerbosity(2)
    , resizableColumns(false)
    , showCommandHistory(true)
{
}

Settings::Settings(const QSettings &settings)
    : Settings()
{
    Q_UNUSED(settings)

    std::abort();
}
