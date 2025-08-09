// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef FILEGROUP_H
#define FILEGROUP_H

#include "filekind.h"
#include "settings.h"

#include <QDir>

class TooManyPartsError : public std::exception
{
    Q_DISABLE_COPY_MOVE(TooManyPartsError)

public:
    explicit TooManyPartsError(QString path);
    ~TooManyPartsError() override = default;
    [[nodiscard]] const char* what() const noexcept override;

    const QString path;
};

class FileGroup
{
    Q_DISABLE_COPY_MOVE(FileGroup)

public:
    static QList<const FileGroup *> fileGroupList(const Settings &settings, const QDir &directory);

    FileGroup(const Settings &settings, QString filePath, QString id, QStringList names, const QString &extension, qint64 lastModified, qint64 size);
    ~FileGroup() = default;

    [[nodiscard]] const QString& getMainExtension() const;
    [[nodiscard]] const QStringList& getExtensions() const;

    const QString filePath;
    const qint64 lastModified; // seconds since Unix epoch
    const qint64 size; // bytes
    const FileKind kind;
    const QString id;
    const QStringList names;

private:
    [[nodiscard]] static FileKind findKind(const Settings &settings, const QString &extension);

    QStringList extensions;
};

#endif // FILEGROUP_H
