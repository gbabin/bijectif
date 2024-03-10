// Copyright (C) 2023, 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "modelitem.h"

#include <QBuffer>
#include <QCache>
#include <QImageReader>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QSqlQuery>
#include <QThreadPool>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

const QStringList ModelItem::imageExtensions = {QStringLiteral("bmp"),
                                                QStringLiteral("gif"),
                                                QStringLiteral("jpg"),
                                                QStringLiteral("jpeg"),
                                                QStringLiteral("png"),
                                                QStringLiteral("tif"),
                                                QStringLiteral("tiff")};

const QStringList ModelItem::videoExtensions = {QStringLiteral("avi"),
                                                QStringLiteral("mkv"),
                                                QStringLiteral("mov"),
                                                QStringLiteral("mp4"),
                                                QStringLiteral("mpg"),
                                                QStringLiteral("mts"),
                                                QStringLiteral("vob"),
                                                QStringLiteral("wmv")};

TooManyPartsError::TooManyPartsError(QString path)
    : path(std::move(path))
{
}

void TooManyPartsError::raise() const
{
    throw *this;
}

TooManyPartsError* TooManyPartsError::clone() const
{
    return new TooManyPartsError(*this);
}

QVariant ModelItem::createThumbnail(const Settings &settings, const QFileInfo &path, const QPixmap &pixmap)
{
    if (pixmap.isNull()) {
        qInfo() << "Pixmap argument is null:" << path.filePath();
        if (imageExtensions.contains(path.suffix().toLower())) {
            if (path.size() <= settings.maxFileSize) {
                QImageReader reader(path.filePath());
                reader.setAutoTransform(true);
                QImage image = reader.read();
                if (image.isNull()) {
                    qWarning() << "Image loading failed:" << path.filePath();
                    return tr("Failure");
                } else {
                    qInfo() << "Image loading succeeded:" << path.filePath();
                    return QPixmap::fromImage(
                        image.scaled(settings.thumbnailSize,
                                     settings.thumbnailSize,
                                     Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation));
                }
            } else {
                qInfo() << "Pixmap creation ignored because of size:" << path.filePath();
                return tr("File too large");
            }
        } else if (videoExtensions.contains(path.suffix().toLower())) {
            QImage image;
            QVideoSink videoSink;
            QMediaPlayer mediaPlayer;
            mediaPlayer.setVideoSink(&videoSink);

            {
                QEventLoop eventLoop;
                QTimer::singleShot(3000, &eventLoop,
                                   [&eventLoop](){ eventLoop.exit(1); });
                connect(&mediaPlayer, &QMediaPlayer::mediaStatusChanged,
                        &eventLoop,
                        [&eventLoop](QMediaPlayer::MediaStatus status){
                            switch (status) {
                            case QMediaPlayer::LoadingMedia:
                                break;
                            case QMediaPlayer::LoadedMedia:
                                eventLoop.exit();
                                break;
                            default:
                                eventLoop.exit(1);
                                break;
                            }
                        });
                mediaPlayer.setSource(QUrl::fromLocalFile(path.filePath()));
                QThreadPool::globalInstance()->releaseThread();
                int returnCode = eventLoop.exec();
                QThreadPool::globalInstance()->reserveThread();
                mediaPlayer.disconnect();
                if (returnCode != 0) {
                    qWarning() << "Video loading failed (setSource):" << path.filePath() << mediaPlayer.mediaStatus() << mediaPlayer.error() << mediaPlayer.errorString();
                    return tr("Failure");
                }
            }

            {
                QEventLoop eventLoop;
                QTimer::singleShot(3000, &eventLoop,
                                   [&eventLoop](){ eventLoop.exit(1); });
                connect(&videoSink, &QVideoSink::videoFrameChanged,
                        &eventLoop,
                        [&eventLoop, &image](const QVideoFrame &frame){
                            if (image.isNull()) image = frame.toImage();
                            eventLoop.exit();
                        });
                mediaPlayer.play();
                QThreadPool::globalInstance()->releaseThread();
                int returnCode = eventLoop.exec();
                QThreadPool::globalInstance()->reserveThread();
                videoSink.disconnect();
                mediaPlayer.stop();
                if (returnCode != 0) {
                    qWarning() << "Video loading failed (play):" << path.filePath() << mediaPlayer.mediaStatus() << mediaPlayer.error() << mediaPlayer.errorString();
                    return tr("Failure");
                }
            }

            if (image.isNull()) {
                qWarning() << "Video loading failed (null):" << path.filePath();
                return tr("Failure");
            } else {
                return QPixmap::fromImage(
                    image.scaled(settings.thumbnailSize,
                                 settings.thumbnailSize,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation));
            }
        } else {
            qInfo() << "Pixmap creation ignored because of extension:" << path.filePath();
            return path.suffix().toUpper();
        }
    } else {
        qInfo() << "Pixmap argument is not null:" << path.filePath();
        return pixmap;
    }
}

ModelItem::ModelItem(const Settings &settings, QCache<QString, QString> &thumbnailCache, const QFileInfo &path, const QPixmap &pixmap, QObject *parent)
    : QObject(parent)
    , settings(settings)
    , thumbnailCache(thumbnailCache)
    , folder(path.dir())
    , extension(path.suffix())
    , thumbnail(createThumbnail(settings, path, pixmap))
{
    QString basename = path.completeBaseName();
    qsizetype index_sep = basename.indexOf(QStringLiteral(" - "));

    if (index_sep != -1) {
        id = basename.sliced(0, index_sep);
        names = basename.sliced(index_sep+3).split(QStringLiteral(", "), Qt::SkipEmptyParts);
        if (names.size() > settings.maxNames) {
            throw TooManyPartsError(path.filePath());
        }
    } else {
        id = basename;
    }
}

QString ModelItem::getPath() const
{
    if (names.isEmpty())
        return folder.filePath(id + QStringLiteral(".") + extension);
    else
        return folder.filePath(id + QStringLiteral(" - ")
                               + names.join(QStringLiteral(", "))
                               + QStringLiteral(".") + extension);
}

QString ModelItem::getTooltip() const
{
    if (thumbnail.isNull() || thumbnail.userType() != QMetaType::QPixmap) {
        return tr("Thumbnail unavailable");
    } else if (videoExtensions.contains(QFileInfo(getPath()).suffix().toLower())) {
        return tr("Video");
    } else {
        QString path = getPath();
        QString* imgBase64 = thumbnailCache.object(path);
        if (imgBase64 == nullptr) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const QImage image = reader.read();

            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            buffer.open(QIODevice::WriteOnly);
            image.scaled(settings.tooltipSize,
                         settings.tooltipSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation)
                .save(&buffer, "PNG");
            buffer.close();

            const QString tmp = QString::fromLatin1(byteArray.toBase64().data());
            imgBase64 = new QString(tmp);
            thumbnailCache.insert(path, imgBase64, 1);
        }

        return QStringLiteral("<img src='data:image/png;base64, %1'>").arg(*imgBase64);
    }
}

QString ModelItem::getId() const
{
    return id;
}

bool ModelItem::setId(const QString &id)
{
    if (! isValidPathChars(id)) return false;

    if (id.isEmpty()) return false;

    QString oldPath = getPath();
    QString oldId = this->id;

    this->id = id;

    return syncFilename(oldPath, oldId);
}

int ModelItem::getNamesSize()
{
    return names.size();
}

QString ModelItem::getName(int index) const
{
    return names.at(index);
}

QVariant ModelItem::getThumbnail() const
{
    return thumbnail;
}

bool ModelItem::setName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    QString nameT = name.trimmed();

    if (nameT.isEmpty()) return false;
    if (index < names.size() && names.at(index) == nameT) return false;
    if (index > names.size()) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    if (index == names.size()) {
        names.append(nameT);
    } else {
        names.replace(index, nameT);
    }

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::deleteName(int index)
{
    if (index >= names.size()) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    names.remove(index);

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::insertName(int index, const QString &name)
{
    if (! isValidPathChars(name)) return false;

    QString nameT = name.trimmed();

    if (nameT.isEmpty()) return false;
    if (index > names.size()) return false;
    if (names.size() == settings.maxNames) return false;

    QString oldPath = getPath();
    QStringList oldNames(names);

    names.insert(index, nameT);

    return syncFilename(oldPath, oldNames);
}

bool ModelItem::isValidPathChars(const QString &str)
{
    if (str.contains(QStringLiteral("<"))
        || str.contains(QStringLiteral(">"))
        || str.contains(QStringLiteral(":"))
        || str.contains(QStringLiteral("\""))
        || str.contains(QStringLiteral("/"))
        || str.contains(QStringLiteral("\\"))
        || str.contains(QStringLiteral("|"))
        || str.contains(QStringLiteral("?"))
        || str.contains(QStringLiteral("*"))
        || str.contains(QStringLiteral(",")))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden characters"),
                                 tr("The following characters are not allowed:\n< > : \" / \\ | ? * ,"));
        return false;
    }

    if (str.contains(QStringLiteral(" - ")))
    {
        QMessageBox::information(nullptr,
                                 tr("Forbidden character sequence"),
                                 tr("The following character sequence is not allowed: \" - \" (hyphen with surrounding spaces)"));
        return false;
    }

    return true;
}

bool ModelItem::syncFilename(const QString &oldPath, const QStringList &oldNames)
{
    QString newPath = getPath();

    if (QFile::rename(oldPath, newPath)) {
        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE thumbnails "
                                     "SET path = :newPath "
                                     "WHERE path = :oldPath"));
        query.bindValue(QStringLiteral(":newPath"), newPath);
        query.bindValue(QStringLiteral(":oldPath"), oldPath);
        query.exec();
        return true;
    } else {
        names.clear();
        names.append(oldNames);
        QMessageBox::critical(nullptr,
                              tr("Unable to rename file"),
                              tr("Before : %1\nAfter : %2").arg(oldPath, newPath));
        return false;
    }
}

bool ModelItem::syncFilename(const QString &oldPath, const QString &oldId)
{
    QString newPath = getPath();

    if (QFile::rename(oldPath, newPath)) {
        QSqlQuery query;
        query.prepare(QStringLiteral("UPDATE thumbnails "
                                     "SET path = :newPath "
                                     "WHERE path = :oldPath"));
        query.bindValue(QStringLiteral(":newPath"), newPath);
        query.bindValue(QStringLiteral(":oldPath"), oldPath);
        query.exec();
        return true;
    } else {
        id = oldId;
        QMessageBox::critical(nullptr,
                              tr("Unable to rename file"),
                              tr("Before : %1\nAfter : %2").arg(oldPath, newPath));
        return false;
    }
}
