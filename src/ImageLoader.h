#pragma once

#include <QMap>
#include <QObject>
#include <QPixmap>
#include <QVariant>
#include "SettingsManager.h"

struct ImageInfo {
    QMap<QString, QString> fileInfo;
    QMap<QString, QString> imageInfo;
    QMap<QString, QVariant> exifInfo;
    QString error;
};

class ImageLoader : public QObject {
    Q_OBJECT

public:
    explicit ImageLoader(const QString &filePath,
                         const PerformanceSettings &perfSettings,
                         const QString &jobId,
                         QObject *parent = nullptr);

public slots:
    void load();
    void cancel();

signals:
    void finished(const QPixmap &pixmap, const QString &filePath, const QString &jobId);
    void infoReady(const ImageInfo &info, const QString &jobId);
    void progress(int value);

private:
    ImageInfo collectImageInfo();

    QString m_filePath;
    PerformanceSettings m_perfSettings;
    QString m_jobId;
    bool m_canceled = false;
};
