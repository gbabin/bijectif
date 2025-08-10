// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VIDEO_H
#define VIDEO_H

#include "location.h"

#include <QPixmap>

std::pair<std::optional<Location>, std::optional<QPixmap>> computeVideoThumbnailLocation(int thumbnailSize, const QString &filePath);

#endif // VIDEO_H
