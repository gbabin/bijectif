// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "filegroup.h"

using namespace Qt::Literals::StringLiterals;

const QStringList FileGroup::fileExtensions = {// images
                                               u"avif"_s,
                                               u"bmp"_s,
                                               u"exr"_s,
                                               u"gif"_s,
                                               u"heic"_s,
                                               u"heif"_s,
                                               u"jp2"_s,
                                               u"jpg"_s,
                                               u"jpeg"_s,
                                               u"jxl"_s,
                                               u"png"_s,
                                               u"tif"_s,
                                               u"tiff"_s,
                                               u"webp"_s,
                                               // raw
                                               u"3fr"_s,
                                               u"ari"_s,
                                               u"arw"_s,
                                               u"bay"_s,
                                               u"braw"_s,
                                               u"cap"_s,
                                               u"cr2"_s,
                                               u"cr3"_s,
                                               u"cri"_s,
                                               u"crw"_s,
                                               u"dcr"_s,
                                               u"dcs"_s,
                                               u"dng"_s,
                                               u"drf"_s,
                                               u"eip"_s,
                                               u"erf"_s,
                                               u"fff"_s,
                                               u"gpr"_s,
                                               u"iiq"_s,
                                               u"k25"_s,
                                               u"kdc"_s,
                                               u"mdc"_s,
                                               u"mef"_s,
                                               u"mos"_s,
                                               u"mrw"_s,
                                               u"nef"_s,
                                               u"nrw"_s,
                                               u"orf"_s,
                                               u"pef"_s,
                                               u"ptx"_s,
                                               u"pxn"_s,
                                               u"r3d"_s,
                                               u"raf"_s,
                                               u"raw"_s,
                                               u"rw2"_s,
                                               u"rwl"_s,
                                               u"rwz"_s,
                                               u"sr2"_s,
                                               u"srf"_s,
                                               u"srw"_s,
                                               u"tco"_s,
                                               u"x3f"_s,
                                               // others
                                               u"psd"_s,
                                               u"svg"_s,
                                               u"xcf"_s,
                                               // videos
                                               u"avi"_s,
                                               u"mkv"_s,
                                               u"mov"_s,
                                               u"mp4"_s,
                                               u"mpg"_s,
                                               u"mts"_s,
                                               u"vob"_s,
                                               u"wmv"_s};

TooManyPartsError::TooManyPartsError(QString path)
    : path(std::move(path))
{
}

const char* TooManyPartsError::what() const noexcept
{
    return "The file has too many parts in its name.";
}

FileGroup::FileGroup(QString filePath, QString id, QStringList names, const QString &extension, qint64 lastModified, qint64 size)
    : filePath(std::move(filePath))
    , lastModified(lastModified)
    , size(size)
    , id(std::move(id))
    , names(std::move(names))
{
    extensions.append(extension);
}

const QString& FileGroup::getMainExtension() const
{
    return extensions.constFirst();
}

const QStringList& FileGroup::getExtensions() const
{
    return extensions;
}

QList<const FileGroup *> FileGroup::fileGroupList(const Settings &settings, const QDir &directory)
{
    QList<const FileGroup *> fileGroupList;
    QMap<QString, QPair<QString, FileGroup *>> prefixes; // maps filename to suffix and group

    QFileInfoList fileInfoList = directory.entryInfoList({u"*.*"_s}, QDir::Files);

    // shorter names first to detect suffixes
    std::sort(fileInfoList.begin(), fileInfoList.end(),
              [](const QFileInfo& a, const QFileInfo& b) {
                  return a.fileName().size() < b.fileName().size();
              });

    for (const QString& extension : fileExtensions) {
        QFileInfoList::iterator itFile = fileInfoList.begin();
        while (itFile != fileInfoList.end()) {
            const QString basename = (*itFile).completeBaseName();
            const QString suffix = (*itFile).suffix();
            if (auto prefix = prefixes.find(basename); prefix != prefixes.end()) {
                const QString completeSuffix = prefix->first.isEmpty() ?
                                                   suffix :
                                                   prefix->first + u"."_s + suffix;
                prefix->second->extensions.append(completeSuffix);
                prefixes.insert((*itFile).fileName(), qMakePair(completeSuffix, prefix->second));
                itFile = fileInfoList.erase(itFile);
            } else {
                if (suffix.toLower() == extension) {
                    QString id;
                    QStringList names;
                    const qsizetype index_sep = basename.indexOf(u" - "_s);

                    if (index_sep != -1) {
                        id = basename.sliced(0, index_sep);
                        names = basename.sliced(index_sep + 3).split(u", "_s, Qt::SkipEmptyParts);
                        if (names.size() > settings.maxNames) {
                            qDeleteAll(fileGroupList);
                            throw TooManyPartsError((*itFile).filePath());
                        }
                    } else {
                        id = basename;
                    }

                    const qint64 lastModified = (*itFile).lastModified().toSecsSinceEpoch();
                    const qint64 size = (*itFile).size();

                    FileGroup* fileGroup = new FileGroup((*itFile).filePath(), id, names, suffix, lastModified, size);
                    fileGroupList.append(fileGroup);
                    prefixes.insert(basename, qMakePair(QString(), fileGroup));
                    prefixes.insert((*itFile).fileName(), qMakePair(suffix, fileGroup));
                    itFile = fileInfoList.erase(itFile);
                } else {
                    ++itFile;
                }
            }
        }
    }

    for (const QFileInfo& fileInfo : std::as_const(fileInfoList)) {
        qInfo() << "Ignored file:" << fileInfo.filePath();
    }

    return fileGroupList;
}
