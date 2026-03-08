// Copyright (C) 2026 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "heifexif.h"

extern "C" {
#include <libheif/heif.h>
}

#include <QDebug>

QByteArray getExifBytesHeif(const QString &filePath, const QByteArray &bytes)
{
    struct heif_context* ctx = nullptr;
    heif_error err = heif_error_success;
    heif_image_handle* handle = nullptr;
    int count = 0;
    heif_item_id id = 0;
    size_t size = 0;

    QByteArray buffer;

    ctx = heif_context_alloc();
    if (ctx == nullptr) {
        qWarning() << "Failed to allocate HEIF context";
        goto cleanup;
    }

    err = heif_context_read_from_memory_without_copy(ctx, bytes.constData(), bytes.size(), nullptr);
    if (err.code != heif_error_Ok) {
        qInfo() << "Failed to read HEIF image:" << filePath << err.message;
        goto cleanup;
    }

    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != heif_error_Ok) {
        qWarning() << "Failed to get primary image in HEIF image:" << filePath << err.message;
        goto cleanup;
    }

    count = heif_image_handle_get_number_of_metadata_blocks(handle, "Exif");
    if (count <= 0) {
        qInfo() << "No EXIF metadata found in HEIF image:" << filePath << err.message;
        goto cleanup;
    }

    heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif", &id, 1);
    size = heif_image_handle_get_metadata_size(handle, id);
    buffer.resize(size);
    heif_image_handle_get_metadata(handle, id, buffer.data());
    buffer.remove(0, 4); // EXIF block in HEIF starts with a 4-byte offset

    cleanup:
        if (handle != nullptr) heif_image_handle_release(handle);
        if (ctx != nullptr)    heif_context_free(ctx);

    return buffer;
}
