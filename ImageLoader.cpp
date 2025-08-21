#include "ImageLoader.h"
#include <QPixmap>
#include <QFileInfo>
#include <QThread>

ImageLoader::ImageLoader(const QString& filePath, QObject* parent)
    : QObject(parent), m_filePath(filePath)
{
}

void ImageLoader::loadImage()
{
    // 模拟加载进度
    emit loadProgress(10);

    // 短暂延迟，模拟加载过程
    QThread::msleep(100);
    emit loadProgress(30);

    // 加载图像
    QPixmap pixmap(m_filePath);
    emit loadProgress(70);

    // 再次延迟
    QThread::msleep(100);

    if (pixmap.isNull()) {
        emit loadError(tr("Failed to load image: %1").arg(m_filePath));
        return;
    }

    emit loadProgress(100);
    emit imageLoaded(pixmap, m_filePath);
}