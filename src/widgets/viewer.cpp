// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "viewer.h"

#include <KDSignalThrottler.h>

#include <QEvent>
#include <QImageReader>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrentRun>

using namespace Qt::Literals::StringLiterals;

Viewer::Viewer(MemoryCache &memoryCache,
               QWidget *parent)
    : QWidget(parent)
    , memoryCache(memoryCache)
    , imageReaderWatcher(new QFutureWatcher<QPixmap>(this))
    , loadDebouncer(new KDToolBox::KDSignalDebouncer(this))
    , graphicsScene(new QGraphicsScene(this))
    , graphicsView(new QGraphicsView(graphicsScene, this))
    , pixmapItem(new QGraphicsPixmapItem())
    , scaleFactor(1.0)
    , isFitted(false)
{
    connect(imageReaderWatcher, &QFutureWatcher<QPixmap>::finished,
            this, &Viewer::imageLoaded);

    pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    graphicsView->setBackgroundBrush(Qt::lightGray);
    graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsView->viewport()->installEventFilter(this);

    graphicsScene->addItem(pixmapItem);

    QLayout* layout = new QVBoxLayout(this);
    layout->addWidget(graphicsView);
    setLayout(layout);

    loadDebouncer->setTimeout(250);
    connect(loadDebouncer, &KDToolBox::KDSignalDebouncer::triggered,
            this, &Viewer::loadImage);
}

void Viewer::viewImage(const QString& path)
{
    if (displayedPath == path)
        return;

    displayedPath = path;

    QMetaObject::invokeMethod(loadDebouncer, &KDToolBox::KDSignalDebouncer::throttle);
}

void Viewer::loadImage()
{
    if (displayedPath.isNull()) {
        const QFuture<QPixmap> future = QtConcurrent::run([]() {
            return QPixmap();
        });

        imageReaderWatcher->setFuture(future);
    } else {
        const QPixmap possiblePixmap = memoryCache.getFullImage(displayedPath);
        if (possiblePixmap.isNull()) {
            const QFuture<QPixmap> future = QtConcurrent::run([path=displayedPath, &cache=memoryCache]() {
                QImageReader reader(path);
                reader.setAutoTransform(true);
                const QImage image = reader.read();
                if (image.isNull()) {
                    qDebug() << "Failed to load image in viewer:" << path;
                    return QPixmap();
                } else {
                    const QPixmap pixmap = QPixmap::fromImage(image);
                    cache.insertFullImage(path, pixmap);
                    return pixmap;
                }
            });

            imageReaderWatcher->setFuture(future);
        } else {
            const QFuture<QPixmap> future = QtConcurrent::run([pixmap=possiblePixmap]() {
                return pixmap;
            });

            imageReaderWatcher->setFuture(future);
        }
    }
}

void Viewer::imageLoaded()
{
    const QPixmap pixmap = imageReaderWatcher->result();

    if (pixmap.isNull()) {
        pixmapItem->setPixmap(pixmap);
        graphicsScene->setSceneRect(QRectF());
        graphicsView->resetTransform();
        isFitted = false;
    } else {
        pixmapItem->setPixmap(pixmap);
        graphicsScene->setSceneRect(pixmap.rect());
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
        isFitted = true;
        scaleFactor = graphicsView->transform().m11();
    }
}

bool Viewer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        if (! pixmapItem->pixmap().isNull()) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);

            if (wheelEvent->angleDelta().y() > 0) {
                scaleFactor *= 1.25;
            } else {
                scaleFactor *= 0.8;
            }

            graphicsView->setTransform(QTransform::fromScale(scaleFactor,scaleFactor));
            isFitted = false;
        }

        return true;
    }

    return QObject::eventFilter(obj, event);
}

void Viewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (isFitted)
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
}
