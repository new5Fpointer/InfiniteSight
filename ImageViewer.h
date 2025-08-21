#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>

class ImageViewer : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget* parent = nullptr);
    void setPixmap(const QPixmap& pixmap);
    void fitToWindow();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* scene;
    QGraphicsPixmapItem* pixmapItem;
    double scaleFactor;
};

#endif // IMAGEVIEWER_H