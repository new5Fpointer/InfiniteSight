#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include "SettingsManager.h"

class SettingsWindow : public QDialog {
    Q_OBJECT

public:
    explicit SettingsWindow(SettingsManager *manager, QWidget *parent = nullptr);

signals:
    void settingsApplied();

private slots:
    void onAccepted();
    void onApply();
    void onRestoreDefaults();
    void updatePreview();

private:
    void setupGeneralTab();
    void setupPerformanceTab();
    void setupAppearanceTab();
    void loadCurrentSettings();
    void applyFromUi();

    SettingsManager *m_manager;

    QTabWidget *m_tabWidget;
    QWidget *m_generalTab;
    QWidget *m_performanceTab;
    QWidget *m_appearanceTab;

    QComboBox *m_windowStateCombo;
    QCheckBox *m_showInfoCheck;
    QSpinBox *m_maxRecentSpin;
    QComboBox *m_languageCombo;
    QCheckBox *m_autoUpdateCheck;
    QCheckBox *m_confirmExitCheck;

    QCheckBox *m_lazyLoadingCheck;
    QCheckBox *m_quickRenderCheck;
    QCheckBox *m_skipExifCheck;
    QSpinBox *m_cacheSizeSpin;

    QComboBox *m_fontCombo;
    QSpinBox *m_fontSizeSpin;
    QComboBox *m_themeCombo;
    QLabel *m_previewLabel;
};
