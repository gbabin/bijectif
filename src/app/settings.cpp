// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "settings.h"

using namespace Qt::Literals::StringLiterals;

CustomMap::CustomMap(QString name,
                     QString url,
                     int minZoomLevel,
                     int maxZoomLevel,
                     QString copyrightNotice)
    : name(std::move(name))
    , url(std::move(url))
    , minZoomLevel(minZoomLevel)
    , maxZoomLevel(maxZoomLevel)
    , copyrightNotice(std::move(copyrightNotice))
{
}

Settings::Settings()
    : Settings(QSettings(QSettings::IniFormat,
                         QSettings::UserScope,
                         u"bijectif"_s))
{
}

Settings::Settings(QSettings settings)
    : fileName(settings.fileName())
    , thumbnailSize(settings.value("thumbnail size", 128).toInt())
    , minWidth(settings.value("min width", 800).toInt())
    , minHeight(settings.value("min height", 600).toInt())
    , listedExtensions(settings.value("listed extensions", u"jpg,jpeg,tif,tiff,png,bmp,gif,dng,mp4,mov,avi"_s.split(u","_s)).toStringList())
    , imageExtensions(settings.value("image extensions", u"jpg,jpeg,tif,tiff,png,bmp,gif"_s.split(u","_s)).toStringList())
    , videoExtensions(settings.value("video extensions", u"mp4,mov,avi"_s.split(u","_s)).toStringList())
    , maxNames(settings.value("max names", 8).toInt())
    , maxFileSize(settings.value("max file size", 20).toInt() * 1024 * 1024)
    , fontSize(settings.value("font size", 16).toInt())
    , memoryCacheSize(settings.value("memory cache size", 500).toInt() * 1024 * 1024)
    , thumbnailDatabaseLimit(settings.value("thumbnail database limit", 10000).toInt())
    , logVerbosity(settings.value("log verbosity", 2).toInt())
    , resizableColumns(settings.value("resizable columns", false).toBool())
    , showCommandHistory(settings.value("show command history", true).toBool())
    , initialMapWidth(settings.value("initial map width", 400).toInt())
    , mapCacheSize(settings.value("map cache size", 200).toInt() * 1024 * 1024)
    , customMaps(readCustomMaps(settings))
{
}

void Settings::log() const
{
    qDebug() << "Settings: thumbnailSize =" << thumbnailSize;
    qDebug() << "Settings: minWidth =" << minWidth;
    qDebug() << "Settings: minHeight =" << minHeight;
    qDebug() << "Settings: listedExtensions =" << listedExtensions;
    qDebug() << "Settings: imageExtensions =" << imageExtensions;
    qDebug() << "Settings: videoExtensions =" << videoExtensions;
    qDebug() << "Settings: maxNames =" << maxNames;
    qDebug() << "Settings: maxFileSize =" << maxFileSize;
    qDebug() << "Settings: fontSize =" << fontSize;
    qDebug() << "Settings: memoryCacheSize =" << memoryCacheSize;
    qDebug() << "Settings: thumbnailDatabaseLimit =" << thumbnailDatabaseLimit;
    qDebug() << "Settings: logVerbosity =" << logVerbosity;
    qDebug() << "Settings: resizableColumns =" << resizableColumns;
    qDebug() << "Settings: showCommandHistory =" << showCommandHistory;
    qDebug() << "Settings: initialMapWidth =" << initialMapWidth;
    qDebug() << "Settings: mapCacheSize =" << mapCacheSize;
    for (const CustomMap& customMap : customMaps) {
        qDebug() << "Settings: customMaps name =" << customMap.name;
        qDebug() << "Settings: customMaps url =" << customMap.url;
        qDebug() << "Settings: customMaps minZoomLevel =" << customMap.minZoomLevel;
        qDebug() << "Settings: customMaps maxZoomLevel =" << customMap.maxZoomLevel;
        qDebug() << "Settings: customMaps copyrightNotice =" << customMap.copyrightNotice;
    }
}

QList<CustomMap> Settings::readCustomMaps(QSettings &settings)
{
    QList<CustomMap> customMaps;

    const CustomMap defaultMap(u"OpenStreetMap"_s,
                               u"https://tile.openstreetmap.org/{z}/{x}/{y}.png"_s,
                               0,
                               20,
                               u"© OpenStreetMap"_s);
    customMaps.append(defaultMap);

    for (const QString& group : settings.childGroups()) {
        settings.beginGroup(group);
        if (settings.contains("tiles url")) {
            const CustomMap map(group,
                                settings.value("tiles url").toString(),
                                settings.value("min zoom level").toInt(),
                                settings.value("max zoom level").toInt(),
                                settings.value("copyright notice").toString());
            customMaps.append(map);
        }
        settings.endGroup();
    }

    return customMaps;
}
