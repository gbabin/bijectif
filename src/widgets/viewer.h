// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#ifndef VIEWER_H
#define VIEWER_H

#include <QFutureWatcher>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>

namespace KDToolBox {
    class KDSignalDebouncer;
}

class Viewer : public QWidget
{
    Q_OBJECT

public:
    explicit Viewer(QWidget *parent = nullptr);

public Q_SLOTS:
    void viewImage(const QString& path);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void displayImage();

    QFutureWatcher<QImage>* imageReaderWatcher;
    KDToolBox::KDSignalDebouncer* loadDebouncer;
    QGraphicsScene* graphicsScene;
    QGraphicsView* graphicsView;
    QGraphicsPixmapItem* pixmapItem;
    qreal scaleFactor;
    bool isFitted;
    QString displayedPath;

private Q_SLOTS:
    void loadImage();
    void imageLoaded();
};

#endif // VIEWER_H
