// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "tablevieweventfilter.h"

#include <QEvent>
#include <QKeyEvent>

TableViewEventFilter::TableViewEventFilter(QObject *parent)
    : QObject(parent)
{
}

bool TableViewEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        const QKeyEvent *const keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_End)
            return true;
    }

    return QObject::eventFilter(obj, event);
}
