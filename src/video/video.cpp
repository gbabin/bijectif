// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "video.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/display.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QRegularExpression>

using namespace Qt::Literals::StringLiterals;

std::pair<std::optional<Location>, std::optional<QPixmap>> computeVideoThumbnailLocation(int thumbnailSize, const QString &filePath)
{
    AVFormatContext* formatContext = nullptr;
    const AVDictionaryEntry* metadataTag = nullptr;
    AVCodecContext* codecContext = nullptr;
    enum AVCodecID codecId = AV_CODEC_ID_NONE;
    const AVCodec* codec = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* scaledFrame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsContext = nullptr;
    int videoStreamIndex = -1;
    const AVPacketSideData* sideData = nullptr;
    double rotation = 0.0; // angle in degrees

    std::optional<Location> possibleLocation;
    std::optional<QPixmap> possibleThumbnail;

    av_log_set_level(AV_LOG_ERROR);

    // Open input file
    int ret = avformat_open_input(&formatContext, filePath.toStdString().c_str(), nullptr, nullptr);
    if (ret < 0) {
        qWarning() << "Video loading failed (could not open file):" << filePath;
        goto cleanup;
    }

    // Retrieve stream information
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret < 0) {
        qWarning() << "Video loading failed (could not find stream information):" << filePath;
        goto cleanup;
    }

    // Get the location metadata
    metadataTag = av_dict_get(formatContext->metadata, "location", nullptr, 0);
    if (metadataTag == nullptr) {
        metadataTag = av_dict_get(formatContext->metadata, "com.apple.quicktime.location.ISO6709", nullptr, 0);
    }
    if (metadataTag != nullptr) {
        static const QRegularExpression re(u"^([+-][0-9.]+)([+-][0-9.]+)"_s); // ISO 6709 format
        const QRegularExpressionMatch match = re.match(QString::fromUtf8(metadataTag->value));
        if (match.hasMatch()) {
            bool latitudeOk = false;
            const double latitude = match.captured(1).toDouble(&latitudeOk);
            bool longitudeOk = false;
            const double longitude = match.captured(2).toDouble(&longitudeOk);
            if (latitudeOk && longitudeOk) {
                possibleLocation.emplace(latitude, longitude);
            } else {
                qWarning() << "Video loading location error (latitude or longitude parsing failed):" << filePath;
            }
        } else {
            qWarning() << "Video loading location error (format of location tag not recognized):" << filePath;
        }
    }

    // Find the first video stream
    for (int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        qWarning() << "Video loading failed (could not find video stream):" << filePath;
        goto cleanup;
    }

    // Get the rotation metadata
    sideData = av_packet_side_data_get(formatContext->streams[videoStreamIndex]->codecpar->coded_side_data,
                                       formatContext->streams[videoStreamIndex]->codecpar->nb_coded_side_data,
                                       AV_PKT_DATA_DISPLAYMATRIX);
    if (sideData != nullptr) {
        rotation = std::round(av_display_rotation_get((int32_t*)sideData->data));
    }

    // Find decoder
    codecId = formatContext->streams[videoStreamIndex]->codecpar->codec_id;
    codec = avcodec_find_decoder(codecId);
    if (codec == nullptr) {
        qWarning() << "Video loading failed (could not find decoder):" << filePath << "(codec =" << avcodec_get_name(codecId) << ")";
        goto cleanup;
    }

    // Allocate codec context
    codecContext = avcodec_alloc_context3(codec);
    if (codecContext == nullptr) {
        qWarning() << "Video loading failed (could not allocate codec context):" << filePath;
        goto cleanup;
    }

    // Copy codec parameters to context
    ret = avcodec_parameters_to_context(codecContext, formatContext->streams[videoStreamIndex]->codecpar);
    if (ret < 0) {
        qWarning() << "Video loading failed (could not copy codec parameters to context):" << filePath;
        goto cleanup;
    }

    // Open codec
    ret = avcodec_open2(codecContext, codec, nullptr);
    if (ret < 0) {
        qWarning() << "Video loading failed (could not open codec):" << filePath;
        goto cleanup;
    }

    // Allocate frames and packet
    frame = av_frame_alloc();
    scaledFrame = av_frame_alloc();
    packet = av_packet_alloc();

    if (frame == nullptr || scaledFrame == nullptr || packet == nullptr) {
        qWarning() << "Video loading failed (could not allocate frames or packet):" << filePath;
        goto cleanup;
    }

    // Read frames and find the first video frame
    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            // Send packet to decoder
            ret = avcodec_send_packet(codecContext, packet);
            if (ret < 0) {
                qWarning() << "Video loading failed (could not send packet to decoder):" << filePath;
                break;
            }

            // Receive frame from decoder
            ret = avcodec_receive_frame(codecContext, frame);
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            if (ret < 0) {
                qWarning() << "Video loading failed (could not decode the first frame):" << filePath;
                break;
            }

            // Successfully decoded a frame - this is our first frame

            // Calculate scaled dimensions while maintaining aspect ratio
            const int originalWidth = frame->width;
            const int originalHeight = frame->height;

            int scaledWidth, scaledHeight;
            if (originalWidth > originalHeight) {
                scaledWidth = thumbnailSize;
                scaledHeight = (originalHeight * thumbnailSize) / originalWidth;
            } else {
                scaledHeight = thumbnailSize;
                scaledWidth = (originalWidth * thumbnailSize) / originalHeight;
            }

            // Ensure dimensions are even (some codecs prefer even dimensions)
            scaledWidth = (scaledWidth / 2) * 2;
            scaledHeight = (scaledHeight / 2) * 2;

            // Set up scaling context
            swsContext = sws_getContext(originalWidth, originalHeight, static_cast<AVPixelFormat>(frame->format),
                                        scaledWidth, scaledHeight, AV_PIX_FMT_RGB24,
                                        SWS_BICUBIC, nullptr, nullptr, nullptr);
            if (swsContext == nullptr) {
                qWarning() << "Video loading failed (could not initialize scaling context):" << filePath;
                break;
            }

            // Allocate scaled frame buffer
            scaledFrame->format = AV_PIX_FMT_RGB24;
            scaledFrame->width = scaledWidth;
            scaledFrame->height = scaledHeight;
            ret = av_frame_get_buffer(scaledFrame, 32);
            if (ret < 0) {
                qWarning() << "Video loading failed (could not allocate scaled frame buffer):" << filePath;
                break;
            }

            // Scale the frame
            ret = sws_scale(swsContext,
                            &frame->data[0], &frame->linesize[0],
                            0, originalHeight,
                            &scaledFrame->data[0], &scaledFrame->linesize[0]);
            if (ret < 0) {
                qWarning() << "Video loading failed (could not scale the frame):" << filePath;
                break;
            }

            // Convert to QPixmap
            QImage image(scaledFrame->data[0],
                         scaledWidth, scaledHeight,
                         scaledFrame->linesize[0],
                         QImage::Format_RGB888);
            if (image.isNull()) {
                qWarning() << "Video loading failed (could not create image):" << filePath;
                break;
            }

            if (rotation != 0.0) {
                image = image.transformed(QTransform().rotate(-rotation));
            }

            possibleThumbnail.emplace(QPixmap::fromImage(image));

            break;
        }
        av_packet_unref(packet);
    }

cleanup:
    if (swsContext != nullptr)    sws_freeContext(swsContext);
    if (packet != nullptr)        av_packet_free(&packet);
    if (scaledFrame != nullptr)   av_frame_free(&scaledFrame);
    if (frame != nullptr)         av_frame_free(&frame);
    if (codecContext != nullptr)  avcodec_free_context(&codecContext);
    if (formatContext != nullptr) avformat_close_input(&formatContext);

    return std::make_pair(possibleLocation, possibleThumbnail);
}
