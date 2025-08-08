// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VIDEO_H
#define VIDEO_H

#include <QPixmap>

std::optional<QPixmap> computeVideoThumbnail(int thumbnailSize, const QString &filePath);

#endif // VIDEO_H
