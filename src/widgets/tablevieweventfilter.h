// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef TABLEVIEWEVENTFILTER_H
#define TABLEVIEWEVENTFILTER_H

#include <QObject>

class TableViewEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit TableViewEventFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // TABLEVIEWEVENTFILTER_H
