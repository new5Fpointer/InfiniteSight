#include "ImageViewer.h"
#include <QWheelEvent>
#include <QResizeEvent>
#include <QtMath>

ImageViewer::ImageViewer(QWidget* parent)
    : QGraphicsView(parent), scaleFactor(1.0)
{
    scene = new QGraphicsScene(this);
    setScene(scene);

    setDragMode(ScrollHandDrag);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignCenter);
    setTransformationAnchor(AnchorUnderMouse);
}

void ImageViewer::setPixmap(const QPixmap& pixmap)
{
    scene->clear();
    pixmapItem = scene->addPixmap(pixmap);
    scene->setSceneRect(pixmap.rect());

    fitToWindow();
    scaleFactor = 1.0;
}

void ImageViewer::fitToWindow()
{
    if (pixmapItem) {
        fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }
}

void ImageViewer::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // 缩放
        double zoomFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            // 放大
            scale(zoomFactor, zoomFactor);
            scaleFactor *= zoomFactor;
        }
        else {
            // 缩小
            scale(1.0 / zoomFactor, 1.0 / zoomFactor);
            scaleFactor /= zoomFactor;
        }
        event->accept();
    }
    else {
        QGraphicsView::wheelEvent(event);
    }
}

void ImageViewer::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (pixmapItem && scaleFactor == 1.0) {
        fitToWindow();
    }
}