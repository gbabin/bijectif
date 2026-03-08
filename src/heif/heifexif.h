// Copyright (C) 2026 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef HEIFEXIF_H
#define HEIFEXIF_H

#include <QByteArray>
#include <QString>

QByteArray getExifBytesHeif(const QString &filePath, const QByteArray &bytes);

#endif // HEIFEXIF_H
