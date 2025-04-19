// Copyright (C) 2024 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "loader.h"

#include <libexif/exif-data.h>

#include <QBuffer>
#include <QImageReader>
#include <QMediaPlayer>
#include <QThreadPool>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

using namespace Qt::Literals::StringLiterals;

const QStringList Loader::imageExtensions = {u"bmp"_s,
                                             u"gif"_s,
                                             u"jpg"_s,
                                             u"jpeg"_s,
                                             u"png"_s,
                                             u"tif"_s,
                                             u"tiff"_s};

const QStringList Loader::videoExtensions = {u"avi"_s,
                                             u"mkv"_s,
                                             u"mov"_s,
                                             u"mp4"_s,
                                             u"mpg"_s,
                                             u"mts"_s,
                                             u"vob"_s,
                                             u"wmv"_s};

std::pair<std::optional<Location>, std::optional<QPixmap>> Loader::load(const Settings &settings, const QFileInfo &path)
{
    std::optional<Location> possibleLocation;
    std::optional<QPixmap> possibleThumbnail;

    if (imageExtensions.contains(path.suffix().toLower())) {
        if (path.size() <= settings.maxFileSize) {
            QFile file(path.filePath());
            file.open(QIODevice::ReadOnly);
            const QByteArray bytes = file.readAll();
            file.close();
            possibleThumbnail = computeImageThumbnail(settings, path.filePath(), bytes);
            possibleLocation = readLocation(path.filePath(), bytes);
        } else {
            qInfo() << "Image ignored because of size:" << path.filePath();
        }
    } else if (videoExtensions.contains(path.suffix().toLower())) {
        possibleThumbnail = computeVideoThumbnail(settings, path.filePath());
    } else {
        qInfo() << "File ignored because of extension:" << path.filePath();
    }

    return std::make_pair(possibleLocation, possibleThumbnail);
}


std::optional<QPixmap> Loader::computeImageThumbnail(const Settings &settings, const QString &filePath, const QByteArray &bytes)
{
    std::optional<QPixmap> possibleThumbnail;

    QBuffer buffer;
    buffer.setData(bytes);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        qWarning() << "Image loading failed:" << filePath;
    } else {
        qInfo() << "Image loading succeeded:" << filePath;
        possibleThumbnail.emplace(QPixmap::fromImage(
            image.scaled(settings.thumbnailSize,
                         settings.thumbnailSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation)));
    }
    buffer.close();

    return possibleThumbnail;
}

std::optional<QPixmap> Loader::computeVideoThumbnail(const Settings &settings, const QString &filePath)
{
    std::optional<QPixmap> possibleThumbnail;

    QImage image;
    QVideoSink videoSink;
    QMediaPlayer mediaPlayer;
    mediaPlayer.setVideoSink(&videoSink);

    {
        QEventLoop eventLoop;
        QTimer::singleShot(3000, &eventLoop,
                           [&eventLoop](){ eventLoop.exit(1); });
        QObject::connect(&mediaPlayer, &QMediaPlayer::mediaStatusChanged,
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
        mediaPlayer.setSource(QUrl::fromLocalFile(filePath));
        QThreadPool::globalInstance()->releaseThread();
        const int returnCode = eventLoop.exec();
        QThreadPool::globalInstance()->reserveThread();
        mediaPlayer.disconnect();
        if (returnCode != 0) {
            qWarning() << "Video loading failed (setSource):" << filePath << mediaPlayer.mediaStatus() << mediaPlayer.error() << mediaPlayer.errorString();
        }
    }

    {
        QEventLoop eventLoop;
        QTimer::singleShot(3000, &eventLoop,
                           [&eventLoop](){ eventLoop.exit(1); });
        QObject::connect(&videoSink, &QVideoSink::videoFrameChanged,
                         &eventLoop,
                         [&eventLoop, &image](const QVideoFrame &frame){
                             if (image.isNull()) image = frame.toImage();
                             eventLoop.exit();
                         });
        mediaPlayer.play();
        QThreadPool::globalInstance()->releaseThread();
        const int returnCode = eventLoop.exec();
        QThreadPool::globalInstance()->reserveThread();
        videoSink.disconnect();
        mediaPlayer.stop();
        if (returnCode != 0) {
            qWarning() << "Video loading failed (play):" << filePath << mediaPlayer.mediaStatus() << mediaPlayer.error() << mediaPlayer.errorString();
        }
    }

    if (image.isNull()) {
        qWarning() << "Video loading failed (null):" << filePath;
    } else {
        possibleThumbnail.emplace(QPixmap::fromImage(
            image.scaled(settings.thumbnailSize,
                         settings.thumbnailSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation)));
    }

    return possibleThumbnail;
}

std::optional<Location> Loader::readLocation(const QString &filePath, const QByteArray &bytes)
{
    std::optional<Location> possibleLocation;

    double latitude = 0.0, longitude = 0.0;
    unsigned char latitude_ref = 0, longitude_ref = 0;

    ExifData *ed = exif_data_new_from_data(reinterpret_cast<const unsigned char *>(bytes.constData()), bytes.size());
    if (ed == nullptr) {
        qInfo() << "Image EXIF reading data failed:" << filePath;
    } else {
        const ExifByteOrder bo = exif_data_get_byte_order(ed);
        ExifEntry *entry = nullptr;

        entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE));
        if (entry == nullptr) {
            qInfo() << "Image EXIF reading GPS latitude failed:" << filePath;
        } else {
            if (entry->format != EXIF_FORMAT_RATIONAL) {
                qWarning() << "Image EXIF bad GPS latitude format:" << filePath;
            } else {
                if (entry->components != 3) {
                    qWarning() << "Image EXIF wrong GPS latitude components count:" << filePath;
                } else {
                    const unsigned char rational_size = exif_format_get_size(EXIF_FORMAT_RATIONAL);
                    ExifRational r;
                    double degrees = 0.0, minutes = 0.0, seconds = 0.0;
                    r = exif_get_rational(entry->data, bo);
                    if (r.denominator != 0)
                        degrees = (double)r.numerator / (double)r.denominator;
                    r = exif_get_rational(entry->data + rational_size, bo);
                    if (r.denominator != 0)
                        minutes = (double)r.numerator / (double)r.denominator;
                    r = exif_get_rational(entry->data + 2*rational_size, bo);
                    if (r.denominator != 0)
                        seconds = (double)r.numerator / (double)r.denominator;
                    latitude = degrees + minutes / 60.0 + seconds / 3600.0;
                    qDebug() << "Image EXIF latitude:" << filePath << latitude;
                }
            }
        }

        entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(EXIF_TAG_GPS_LATITUDE_REF));
        if (entry == nullptr) {
            qInfo() << "Image EXIF reading GPS latitude ref failed:" << filePath;
        } else {
            if (entry->format != EXIF_FORMAT_ASCII) {
                qWarning() << "Image EXIF bad GPS latitude ref format:" << filePath;
            } else {
                if (entry->components != 2) {
                    qWarning() << "Image EXIF wrong GPS latitude ref components count:" << filePath;
                } else {
                    latitude_ref = *entry->data;
                    qDebug() << "Image EXIF latitude ref:" << filePath << latitude_ref;
                }
            }
        }

        entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE));
        if (entry == nullptr) {
            qInfo() << "Image EXIF reading GPS longitude failed:" << filePath;
        } else {
            if (entry->format != EXIF_FORMAT_RATIONAL) {
                qWarning() << "Image EXIF bad GPS longitude format:" << filePath;
            } else {
                if (entry->components != 3) {
                    qWarning() << "Image EXIF wrong GPS longitude components count:" << filePath;
                } else {
                    const unsigned char rational_size = exif_format_get_size(EXIF_FORMAT_RATIONAL);
                    ExifRational r;
                    double degrees = 0.0, minutes = 0.0, seconds = 0.0;
                    r = exif_get_rational(entry->data, bo);
                    if (r.denominator != 0)
                        degrees = (double)r.numerator / (double)r.denominator;
                    r = exif_get_rational(entry->data + rational_size, bo);
                    if (r.denominator != 0)
                        minutes = (double)r.numerator / (double)r.denominator;
                    r = exif_get_rational(entry->data + 2*rational_size, bo);
                    if (r.denominator != 0)
                        seconds = (double)r.numerator / (double)r.denominator;
                    longitude = degrees + minutes / 60.0 + seconds / 3600.0;
                    qDebug() << "Image EXIF longitude:" << filePath << longitude;
                }
            }
        }

        entry = exif_content_get_entry(ed->ifd[EXIF_IFD_GPS], static_cast<ExifTag>(EXIF_TAG_GPS_LONGITUDE_REF));
        if (entry == nullptr) {
            qInfo() << "Image EXIF reading GPS longitude ref failed:" << filePath;
        } else {
            if (entry->format != EXIF_FORMAT_ASCII) {
                qWarning() << "Image EXIF bad GPS longitude ref format:" << filePath;
            } else {
                if (entry->components != 2) {
                    qWarning() << "Image EXIF wrong GPS longitude ref components count:" << filePath;
                } else {
                    longitude_ref = *entry->data;
                    qDebug() << "Image EXIF longitude ref:" << filePath << longitude_ref;
                }
            }
        }

        exif_data_free(ed);
    }

    if (latitude != 0.0 && longitude != 0.0 && latitude_ref != 0 && longitude_ref != 0)
    {
        possibleLocation.emplace(Location(latitude * (latitude_ref == 'S' ? -1.0 : 1.0),
                                          longitude * (longitude_ref == 'W' ? -1.0 : 1.0)));
    }

    return possibleLocation;
}
