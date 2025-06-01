// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"

using namespace Qt::Literals::StringLiterals;

Settings::Settings()
    : Settings(QSettings(QSettings::IniFormat,
                         QSettings::UserScope,
                         u"bijectif"_s))
{
}

Settings::Settings(const QSettings &settings)
    : fileName(settings.fileName())
    , thumbnailSize(settings.value("thumbnail size", 128).toInt())
    , tooltipSize(settings.value("tooltip size", 512).toInt())
    , minWidth(settings.value("min width", 800).toInt())
    , minHeight(settings.value("min height", 600).toInt())
    , listedExtensions(settings.value("listed extensions", u"jpg,jpeg,tif,tiff,png,bmp,gif,dng,mp4,mov,avi"_s.split(u","_s)).toStringList())
    , imageExtensions(settings.value("image extensions", u"jpg,jpeg,tif,tiff,png,bmp,gif"_s.split(u","_s)).toStringList())
    , videoExtensions(settings.value("video extensions", u"mp4,mov,avi"_s.split(u","_s)).toStringList())
    , maxNames(settings.value("max names", 8).toInt())
    , maxFileSize(settings.value("max file size", 20).toInt() * 1024 * 1024)
    , fontSize(settings.value("font size", 16).toInt())
    , thumbnailCacheSize(settings.value("thumbnail cache size", 100).toInt())
    , thumbnailDatabaseLimit(settings.value("thumbnail database limit", 10000).toInt())
    , logVerbosity(settings.value("log verbosity", 2).toInt())
    , resizableColumns(settings.value("resizable columns", false).toBool())
    , showCommandHistory(settings.value("show command history", true).toBool())
    , initialMapWidth(settings.value("initial map width", 400).toInt())
    , mapCacheSize(settings.value("map cache size", 200).toInt() * 1024 * 1024)
{
}

void Settings::log() const
{
    qDebug() << "Settings: thumbnailSize =" << thumbnailSize;
    qDebug() << "Settings: tooltipSize =" << tooltipSize;
    qDebug() << "Settings: minWidth =" << minWidth;
    qDebug() << "Settings: minHeight =" << minHeight;
    qDebug() << "Settings: listedExtensions =" << listedExtensions;
    qDebug() << "Settings: imageExtensions =" << imageExtensions;
    qDebug() << "Settings: videoExtensions =" << videoExtensions;
    qDebug() << "Settings: maxNames =" << maxNames;
    qDebug() << "Settings: maxFileSize =" << maxFileSize;
    qDebug() << "Settings: fontSize =" << fontSize;
    qDebug() << "Settings: thumbnailCacheSize =" << thumbnailCacheSize;
    qDebug() << "Settings: thumbnailDatabaseLimit =" << thumbnailDatabaseLimit;
    qDebug() << "Settings: logVerbosity =" << logVerbosity;
    qDebug() << "Settings: resizableColumns =" << resizableColumns;
    qDebug() << "Settings: showCommandHistory =" << showCommandHistory;
    qDebug() << "Settings: initialMapWidth =" << initialMapWidth;
    qDebug() << "Settings: mapCacheSize =" << mapCacheSize;
}
