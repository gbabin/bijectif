// Copyright (C) 2025 gbabin
// SPDX-License-Identifier: GPL-3.0-only

#include "viewer.h"

#include <KDSignalThrottler.h>

#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QImageReader>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrentRun>

using namespace Qt::Literals::StringLiterals;

Viewer::Viewer(MemoryCache &memoryCache,
               QWidget *parent)
    : QWidget(parent)
    , memoryCache(memoryCache)
    , itemLoadingWatcher(new QFutureWatcher<QVariant>(this))
    , loadDebouncer(new KDToolBox::KDSignalDebouncer(this))
    , graphicsScene(new QGraphicsScene(this))
    , graphicsView(new QGraphicsView(graphicsScene, this))
    , pixmapItem(new QGraphicsPixmapItem())
    , messageItem(new QGraphicsSimpleTextItem())
    , scaleFactor(1.0)
    , isFitted(false)
{
    connect(itemLoadingWatcher, &QFutureWatcher<QVariant>::finished,
            this, &Viewer::itemLoaded);

    pixmapItem->setTransformationMode(Qt::SmoothTransformation);

    graphicsView->setBackgroundBrush(Qt::lightGray);
    graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsView->viewport()->installEventFilter(this);

    graphicsScene->addItem(pixmapItem);
    graphicsScene->addItem(messageItem);

    QLayout* layout = new QVBoxLayout(this);
    layout->addWidget(graphicsView);
    setLayout(layout);

    loadDebouncer->setTimeout(250);
    connect(loadDebouncer, &KDToolBox::KDSignalDebouncer::triggered,
            this, &Viewer::loadItem);
}

void Viewer::viewItem(ViewerItemKind kind, const QString& item)
{
    switch (kind) {
    case VI_IMAGE:
        if (item == imagePathToDisplay)
            return;
        imagePathToDisplay = item;
        messageToDisplay.clear();
        break;
    case VI_TEXT:
        if (item == messageToDisplay)
            return;
        imagePathToDisplay.clear();
        messageToDisplay = item;
        break;
    }

    QMetaObject::invokeMethod(loadDebouncer, &KDToolBox::KDSignalDebouncer::throttle);
}

void Viewer::loadItem()
{
    if (! imagePathToDisplay.isEmpty()) {
        const QPixmap possiblePixmap = memoryCache.getFullImage(imagePathToDisplay);
        if (possiblePixmap.isNull()) {
            const QFuture<QVariant> future = QtConcurrent::run([path=imagePathToDisplay, &cache=memoryCache]() {
                QImageReader reader(path);
                reader.setAutoTransform(true);
                const QImage image = reader.read();
                if (image.isNull()) {
                    qWarning() << "Failed to load image in viewer:" << path << ": Error" << reader.error() << reader.errorString();
                    return QVariant(tr("Image preview failed"));
                } else {
                    const QPixmap pixmap = QPixmap::fromImage(image);
                    cache.insertFullImage(path, pixmap);
                    return QVariant::fromValue(pixmap);
                }
            });

            itemLoadingWatcher->setFuture(future);
        } else {
            const QFuture<QVariant> future = QtConcurrent::run([pixmap=possiblePixmap]() {
                return QVariant::fromValue(pixmap);
            });

            itemLoadingWatcher->setFuture(future);
        }
    } else if (! messageToDisplay.isEmpty()) {
        const QFuture<QVariant> future = QtConcurrent::run([message=messageToDisplay]() {
            return QVariant::fromValue(message);
        });

        itemLoadingWatcher->setFuture(future);
    } else {
        const QFuture<QVariant> future = QtConcurrent::run([]() {
            return QVariant();
        });

        itemLoadingWatcher->setFuture(future);
    }
}

void Viewer::itemLoaded()
{
    const QVariant result = itemLoadingWatcher->result();
    const QPixmap pixmap = result.value<QPixmap>();

    if (pixmap.isNull()) {
        const QString message = result.toString();
        messageItem->setText(message);
        pixmapItem->setPixmap(QPixmap());
        graphicsScene->setSceneRect(QRectF());
        graphicsView->resetTransform();
        isFitted = false;
    } else {
        messageItem->setText(QString());
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
            const QWheelEvent *const wheelEvent = static_cast<QWheelEvent *>(event);

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
