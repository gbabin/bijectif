// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QString>

class Settings
{
public:
    Settings();
    void log() const;

    /// Settings file path
    const QString fileName;

    /// Thumbnail size in main window (in pixels)
    const int thumbnailSize;

    /// Thumbnail size in tooltips (in pixels)
    const int tooltipSize;

    /// Minimal window width (in pixels)
    const int minWidth;

    /// Minimal window height (in pixels)
    const int minHeight;

    /// Extensions of listed files (comma-separated list, lower case)
    /// The order determines the main files from the sidecar files.
    const QStringList listedExtensions;

    /// Extensions of image files for thumbnails (comma-separated list, lower case)
    /// https://doc.qt.io/qt-6/qimagereader.html#supportedImageFormats
    /// https://doc.qt.io/qt-6/qtimageformats-index.html#the-image-i-o-plugins
    const QStringList imageExtensions;

    /// Extensions of video files for thumbnails (comma-separated list, lower case)
    /// https://doc.qt.io/qt-6/qmediaformat.html#FileFormat-enum
    const QStringList videoExtensions;

    /// Maximal number of name parts
    const int maxNames;

    /// Maximal file size considered for computing thumbnails (in bytes)
    /// [in megabytes in settings file]
    const int maxFileSize;

    /// Font size (in pixels)
    const int fontSize;

    /// Max number of thumbnails in memory cache
    const int thumbnailCacheSize;

    /// Max number of thumbnails in database cache
    const int thumbnailDatabaseLimit;

    /// Log verbosity (0 to 4, 4 is most verbose)
    const int logVerbosity;

    /// Resizable columns (true (manually resizable) or false (content size))
    const bool resizableColumns;

    /// Show command history (true or false)
    const bool showCommandHistory;

    /// Initial map widget width (in pixels)
    const int initialMapWidth;

    /// Maximal cache size for map tiles (in bytes)
    /// [in megabytes in settings file]
    const int mapCacheSize;

private:
    explicit Settings(const QSettings &settings);
};

#endif // SETTINGS_H
