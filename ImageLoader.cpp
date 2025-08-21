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
    // ģ����ؽ���
    emit loadProgress(10);

    // �����ӳ٣�ģ����ع���
    QThread::msleep(100);
    emit loadProgress(30);

    // ����ͼ��
    QPixmap pixmap(m_filePath);
    emit loadProgress(70);

    // �ٴ��ӳ�
    QThread::msleep(100);

    if (pixmap.isNull()) {
        emit loadError(tr("Failed to load image: %1").arg(m_filePath));
        return;
    }

    emit loadProgress(100);
    emit imageLoaded(pixmap, m_filePath);
}