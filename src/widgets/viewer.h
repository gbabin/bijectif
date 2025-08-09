// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VIEWER_H
#define VIEWER_H

#include "memorycache.h"

#include <QCache>
#include <QFutureWatcher>
#include <QGraphicsScene>
#include <QWidget>

namespace KDToolBox {
    class KDSignalDebouncer;
}

enum ViewerItemKind {
    VI_IMAGE,
    VI_TEXT
};

class Viewer : public QWidget
{
    Q_OBJECT

public:
    explicit Viewer(MemoryCache &memoryCache, QWidget *parent = nullptr);

public Q_SLOTS:
    void viewItem(ViewerItemKind kind, const QString& item);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    MemoryCache &memoryCache;
    QFutureWatcher<QVariant>* itemLoadingWatcher;
    KDToolBox::KDSignalDebouncer* loadDebouncer;
    QGraphicsScene* graphicsScene;
    QGraphicsView* graphicsView;
    QGraphicsPixmapItem* pixmapItem;
    QGraphicsSimpleTextItem* messageItem;
    qreal scaleFactor;
    bool isFitted;
    QString imagePathToDisplay;
    QString messageToDisplay;

private Q_SLOTS:
    void loadItem();
    void itemLoaded();
};

#endif // VIEWER_H
