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
    , viewScale(1.0)
    , pixmapScale(1.0)
    , pixmapScaleMin(false)
    , pixmapScaleMax(false)
    , pyramid(nullptr)
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

    loadDebouncer->setTimeout(100);
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

    loadDebouncer->throttle();
}

void Viewer::loadItem()
{
    if (! imagePathToDisplay.isEmpty()) {
        const QSharedPointer<const Pyramid> possiblePyramid = memoryCache.getPyramid(imagePathToDisplay);
        if (possiblePyramid.isNull()) {
            const QFuture<QVariant> future = QtConcurrent::run([path=imagePathToDisplay, &cache=memoryCache]() {
                const QSharedPointer<const Pyramid> pyramid = QSharedPointer<const Pyramid>::create(path);
                if (pyramid->isEmpty()) {
                    return QVariant(tr("Image preview failed"));
                } else {
                    cache.insertPyramid(path, pyramid);
                    return QVariant::fromValue(pyramid);
                }
            });

            itemLoadingWatcher->setFuture(future);
        } else {
            const QFuture<QVariant> future = QtConcurrent::run([possiblePyramid]() {
                return QVariant::fromValue(possiblePyramid);
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

    if (result.canConvert<QString>()) {
        const QString message = result.toString();
        messageItem->setText(message);
        pixmapItem->setPixmap(QPixmap());
        graphicsScene->setSceneRect(QRect());
        graphicsView->resetTransform();
        isFitted = false;
        pyramid.clear();
    } else if (result.canConvert<QSharedPointer<const Pyramid>>()) {
        pyramid = result.value<QSharedPointer<const Pyramid>>();

        const QSize viewportSize = graphicsView->viewport()->size();
        const QPair<QPixmap, qreal> scaled = pyramid->getPixmap(viewportSize);

        messageItem->setText(QString());
        pixmapScale = scaled.second;
        pixmapScaleMin = false;
        pixmapScaleMax = false;
        pixmapItem->setPixmap(scaled.first);
        pixmapItem->setTransformationMode(Qt::SmoothTransformation);
        pixmapItem->setTransform(QTransform::fromScale(1.0 / pixmapScale, 1.0 / pixmapScale));

        const QSize originalSize = pyramid->getOriginalSize();
        graphicsScene->setSceneRect(QRect(QPoint(), originalSize));
        graphicsView->fitInView(graphicsScene->sceneRect(), Qt::KeepAspectRatio);
        isFitted = true;
        viewScale = graphicsView->transform().m11();
    } else if (! result.isNull()) {
        qCritical() << "Unsupported viewer item variant type:" << result.typeName();

        messageItem->setText(QString());
        pixmapItem->setPixmap(QPixmap());
        graphicsScene->setSceneRect(QRect());
        graphicsView->resetTransform();
        isFitted = false;
        pyramid.clear();
    }
}

bool Viewer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel)
    {
        if (! pixmapItem->pixmap().isNull()) {
            {
                const QWheelEvent *const wheelEvent = static_cast<QWheelEvent *>(event);

                constexpr qreal zoomFactor = 1.1;

                if (wheelEvent->angleDelta().y() > 0) {
                    viewScale *= zoomFactor;
                } else if (wheelEvent->angleDelta().y() < 0) {
                    viewScale /= zoomFactor;
                } else {
                    return true;
                }
            }

            {
                isFitted = false;

                if (viewScale > 4.0)
                    viewScale = 4.0;

                const QSize viewportSize = graphicsView->viewport()->size();
                const QSize originalSize = pyramid->getOriginalSize();
                const qreal minScale = std::min((qreal)viewportSize.width() / originalSize.width(),
                                                (qreal)viewportSize.height() / originalSize.height());
                if (viewScale < minScale) {
                    viewScale = minScale;
                    isFitted = true;
                }
            }

            if (!pixmapScaleMax && pixmapScale < viewScale) {
                const QPair<QPixmap, qreal> scaled = pyramid->getPixmap(viewScale);

                if (scaled.second == pixmapScale) {
                    pixmapScaleMax = true;
                } else {
                    pixmapScale = scaled.second;
                    pixmapScaleMin = false;
                    pixmapScaleMax = false;
                    pixmapItem->setPixmap(scaled.first);
                    pixmapItem->setTransform(QTransform::fromScale(1.0 / pixmapScale, 1.0 / pixmapScale));
                }
            } else if (!pixmapScaleMin && pixmapScale > viewScale * 2.0) {
                const QPair<QPixmap, qreal> scaled = pyramid->getPixmap(viewScale);

                if (scaled.second == pixmapScale) {
                    pixmapScaleMin = true;
                } else {
                    pixmapScale = scaled.second;
                    pixmapScaleMin = false;
                    pixmapScaleMax = false;
                    pixmapItem->setPixmap(scaled.first);
                    pixmapItem->setTransform(QTransform::fromScale(1.0 / pixmapScale, 1.0 / pixmapScale));
                }
            }

            graphicsView->setTransform(QTransform::fromScale(viewScale,viewScale));
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
