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

    /// Maximal number of name parts
    const int maxNames;

    /// Maximal file size considered for computing thumbnails (in bytes)
    const int maxFileSize;

    /// Font size (in pixels)
    const int fontSize;

    /// Max number of thumbnails in memory cache
    const int thumbnailCacheSize;

    /// Max number of thumbnails in database cache
    const int thumbnailDatabaseLimit;

    /// Log verbosity (0 to 4, 4 is most verbose)
    const int logVerbosity;

private:
    Settings(const QSettings &settings);
};

#endif // SETTINGS_H
