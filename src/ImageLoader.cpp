#include "ImageLoader.h"
#include "SettingsManager.h"
#include <QFileInfo>
#include <QImageReader>
#include <QDateTime>

ImageLoader::ImageLoader(const QString &filePath,
                         const PerformanceSettings &perfSettings,
                         const QString &jobId,
                         QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
    , m_perfSettings(perfSettings)
    , m_jobId(jobId)
{
}

void ImageLoader::load() {
    if (m_canceled) return;

    QPixmap pixmap;
    bool loaded = pixmap.load(m_filePath);

    if (m_canceled) return;

    if (!loaded || pixmap.isNull()) {
        emit finished(QPixmap(), QString("Error: Failed to load image"), m_jobId);
        return;
    }

    emit progress(30);

    ImageInfo info = collectImageInfo();

    if (m_canceled) return;

    emit progress(70);
    emit finished(pixmap, m_filePath, m_jobId);
    emit infoReady(info, m_jobId);
    emit progress(100);
}

void ImageLoader::cancel() {
    m_canceled = true;
}

ImageInfo ImageLoader::collectImageInfo() {
    ImageInfo info;
    QFileInfo fi(m_filePath);

    info.fileInfo["File Name"] = fi.fileName();
    info.fileInfo["Path"] = fi.absoluteFilePath();
    info.fileInfo["Size"] = QString("%1 KB").arg(fi.size() / 1024.0, 0, 'f', 2);
    info.fileInfo["Modified"] = fi.lastModified().toString("yyyy-MM-dd hh:mm:ss");

    if (m_perfSettings.skipExif) {
        return info;
    }

    QImageReader reader(m_filePath);
    if (reader.canRead()) {
        info.imageInfo["Format"] = reader.format().toUpper();
        info.imageInfo["Dimensions"] = QString("%1 x %2 pixels")
                                           .arg(reader.size().width())
                                           .arg(reader.size().height());
        // physicalSize() is not available in Qt 6.10; DPI info would need QImage
        info.imageInfo["DPI"] = QString("N/A");
    }

    return info;
}
