#include "SettingsManager.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_settings("InfiniteSight", "Settings")
{
    load();
}

GeneralSettings SettingsManager::general() const {
    return m_general;
}

PerformanceSettings SettingsManager::performance() const {
    return m_performance;
}

AppearanceSettings SettingsManager::appearance() const {
    return m_appearance;
}

void SettingsManager::setGeneral(const GeneralSettings &settings) {
    m_general = settings;
}

void SettingsManager::setPerformance(const PerformanceSettings &settings) {
    m_performance = settings;
}

void SettingsManager::setAppearance(const AppearanceSettings &settings) {
    m_appearance = settings;
}

void SettingsManager::addRecentFile(const QString &filePath) {
    m_general.recentFiles.removeAll(filePath);
    m_general.recentFiles.prepend(filePath);
    while (m_general.recentFiles.size() > m_general.maxRecentFiles) {
        m_general.recentFiles.removeLast();
    }
    save();
}

void SettingsManager::clearRecentFiles() {
    m_general.recentFiles.clear();
    save();
}

void SettingsManager::save() {
    m_settings.setValue("general/defaultWindowState", m_general.defaultWindowState);
    m_settings.setValue("general/showInfoPanel", m_general.showInfoPanel);
    m_settings.setValue("general/recentFiles", m_general.recentFiles);
    m_settings.setValue("general/maxRecentFiles", m_general.maxRecentFiles);
    m_settings.setValue("general/language", m_general.language);

    m_settings.setValue("performance/lazyLoading", m_performance.lazyLoading);
    m_settings.setValue("performance/quickRender", m_performance.quickRender);
    m_settings.setValue("performance/skipExif", m_performance.skipExif);
    m_settings.setValue("performance/cacheSize", m_performance.cacheSize);

    m_settings.setValue("appearance/uiFont", m_appearance.uiFont);
    m_settings.setValue("appearance/uiFontSize", m_appearance.uiFontSize);
    m_settings.setValue("appearance/theme", m_appearance.theme);

    emit settingsChanged();
}

void SettingsManager::load() {
    m_general.defaultWindowState = m_settings.value("general/defaultWindowState", "normal").toString();
    m_general.showInfoPanel = m_settings.value("general/showInfoPanel", true).toBool();
    m_general.recentFiles = m_settings.value("general/recentFiles").toStringList();
    m_general.maxRecentFiles = m_settings.value("general/maxRecentFiles", 5).toInt();
    m_general.language = m_settings.value("general/language", "en_us").toString();

    m_performance.lazyLoading = m_settings.value("performance/lazyLoading", true).toBool();
    m_performance.quickRender = m_settings.value("performance/quickRender", false).toBool();
    m_performance.skipExif = m_settings.value("performance/skipExif", false).toBool();
    m_performance.cacheSize = m_settings.value("performance/cacheSize", 100).toInt();

    m_appearance.uiFont = m_settings.value("appearance/uiFont", "Segoe UI").toString();
    m_appearance.uiFontSize = m_settings.value("appearance/uiFontSize", 10).toInt();
    m_appearance.theme = m_settings.value("appearance/theme", "dark").toString();
}
