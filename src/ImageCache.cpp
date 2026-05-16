#include "ImageCache.h"
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QFileInfo>

QString ImageCache::cacheDir() {
    QString path = QDir::tempPath() + "/InfiniteSight_cache";
    QDir().mkpath(path);
    return path;
}

QString ImageCache::cacheKey(const QString &filePath, int maxEdge) {
    QByteArray hash = QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex();
    return QString("%1_%2.jpg").arg(QString(hash)).arg(maxEdge);
}

bool ImageCache::isVeryLarge(const QString &filePath, qint64 thresholdBytes) {
    QFileInfo fi(filePath);
    return fi.exists() && fi.size() > thresholdBytes;
}

QPixmap ImageCache::loadThumbnail(const QString &filePath, int maxEdge) {
    QString cacheFile = cacheDir() + "/" + cacheKey(filePath, maxEdge);

    if (!QFile::exists(cacheFile)) {
        QPixmap source(filePath);
        if (source.isNull()) return QPixmap();

        QPixmap thumbnail = source.scaled(maxEdge, maxEdge,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation);
        thumbnail.save(cacheFile, "JPEG", 90);
        return thumbnail;
    }

    return QPixmap(cacheFile);
}

QPixmap ImageCache::loadTile(const QString &filePath, int x, int y, int w, int h) {
    QPixmap source(filePath);
    if (source.isNull()) return QPixmap();

    return source.copy(x, y, qMin(w, source.width() - x), qMin(h, source.height() - y));
}
