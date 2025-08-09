// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FILEKIND_H
#define FILEKIND_H

#include <QVariant>

enum FileKind {
    IMAGE,
    OTHER,
    VIDEO
};

Q_DECLARE_METATYPE(FileKind)

#endif // FILEKIND_H
