#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QMap>

struct GeneralSettings {
    QString defaultWindowState = "normal";
    bool showInfoPanel = true;
    QStringList recentFiles;
    int maxRecentFiles = 5;
    QString language = "en_us";
};

struct PerformanceSettings {
    bool lazyLoading = true;
    bool quickRender = false;
    bool skipExif = false;
    int cacheSize = 100;
};

struct AppearanceSettings {
    QString uiFont = "Segoe UI";
    int uiFontSize = 10;
    QString theme = "dark";
};

class SettingsManager : public QObject {
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);

    GeneralSettings general() const;
    PerformanceSettings performance() const;
    AppearanceSettings appearance() const;

    void setGeneral(const GeneralSettings &settings);
    void setPerformance(const PerformanceSettings &settings);
    void setAppearance(const AppearanceSettings &settings);

    void addRecentFile(const QString &filePath);
    void clearRecentFiles();

    void save();
    void load();

signals:
    void settingsChanged();

private:
    QSettings m_settings;
    GeneralSettings m_general;
    PerformanceSettings m_performance;
    AppearanceSettings m_appearance;
};
