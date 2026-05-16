#pragma once

#include <QPixmap>
#include <QString>

class ImageCache {
public:
    static bool isVeryLarge(const QString &filePath, qint64 thresholdBytes = 256LL << 20);
    static QPixmap loadThumbnail(const QString &filePath, int maxEdge = 4096);
    static QPixmap loadTile(const QString &filePath, int x = 0, int y = 0, int w = 2048, int h = 2048);

private:
    static QString cacheDir();
    static QString cacheKey(const QString &filePath, int maxEdge);
};
