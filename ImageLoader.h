#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QObject>
#include <QPixmap>
#include <QString>

class ImageLoader : public QObject
{
    Q_OBJECT

public:
    explicit ImageLoader(const QString& filePath, QObject* parent = nullptr);

public slots:
    void loadImage();

signals:
    void imageLoaded(const QPixmap& pixmap, const QString& filePath);
    void loadProgress(int percent);
    void loadError(const QString& error);

private:
    QString m_filePath;
};

#endif // IMAGELOADER_H