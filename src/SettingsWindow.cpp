#include "SettingsWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QPushButton>

SettingsWindow::SettingsWindow(SettingsManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setWindowTitle(tr("Application Settings"));
    setMinimumSize(700, 500);

    m_tabWidget = new QTabWidget(this);

    m_generalTab = new QWidget();
    setupGeneralTab();
    m_tabWidget->addTab(m_generalTab, tr("General"));

    m_performanceTab = new QWidget();
    setupPerformanceTab();
    m_tabWidget->addTab(m_performanceTab, tr("Performance"));

    m_appearanceTab = new QWidget();
    setupAppearanceTab();
    m_tabWidget->addTab(m_appearanceTab, tr("Appearance"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
        QDialogButtonBox::Apply | QDialogButtonBox::RestoreDefaults,
        this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsWindow::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsWindow::onApply);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsWindow::onRestoreDefaults);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_tabWidget);
    layout->addWidget(buttonBox);

    loadCurrentSettings();
}

void SettingsWindow::setupGeneralTab() {
    QVBoxLayout *layout = new QVBoxLayout(m_generalTab);

    QGroupBox *windowGroup = new QGroupBox(tr("Window Behavior"), m_generalTab);
    QFormLayout *windowLayout = new QFormLayout(windowGroup);

    m_windowStateCombo = new QComboBox(windowGroup);
    m_windowStateCombo->addItems({tr("Normal"), tr("Maximized"), tr("Fullscreen")});
    windowLayout->addRow(tr("Default window state:"), m_windowStateCombo);

    m_showInfoCheck = new QCheckBox(tr("Show information panel on startup"), windowGroup);
    windowLayout->addRow(m_showInfoCheck);

    m_maxRecentSpin = new QSpinBox(windowGroup);
    m_maxRecentSpin->setRange(0, 20);
    windowLayout->addRow(tr("Maximum recent files:"), m_maxRecentSpin);

    m_languageCombo = new QComboBox(windowGroup);
    m_languageCombo->addItem("English (en_us)", "en_us");
    m_languageCombo->addItem(u8"简体中文 (zh_cn)", "zh_cn");
    m_languageCombo->addItem(u8"繁體中文 (zh_tw)", "zh_tw");
    windowLayout->addRow(tr("Language:"), m_languageCombo);

    windowGroup->setLayout(windowLayout);
    layout->addWidget(windowGroup);

    QGroupBox *otherGroup = new QGroupBox(tr("Other Settings"), m_generalTab);
    QFormLayout *otherLayout = new QFormLayout(otherGroup);

    m_autoUpdateCheck = new QCheckBox(tr("Check for updates on startup"), otherGroup);
    otherLayout->addRow(m_autoUpdateCheck);

    m_confirmExitCheck = new QCheckBox(tr("Confirm before exiting"), otherGroup);
    otherLayout->addRow(m_confirmExitCheck);

    otherGroup->setLayout(otherLayout);
    layout->addWidget(otherGroup);

    layout->addStretch();
}

void SettingsWindow::setupPerformanceTab() {
    QVBoxLayout *layout = new QVBoxLayout(m_performanceTab);

    QGroupBox *imageGroup = new QGroupBox(tr("Image Processing"), m_performanceTab);
    QFormLayout *imageLayout = new QFormLayout(imageGroup);

    m_lazyLoadingCheck = new QCheckBox(tr("Lazy EXIF parsing (faster image loading)"), imageGroup);
    imageLayout->addRow(m_lazyLoadingCheck);

    m_quickRenderCheck = new QCheckBox(tr("Enable fast rendering mode (lower quality)"), imageGroup);
    imageLayout->addRow(m_quickRenderCheck);

    m_skipExifCheck = new QCheckBox(tr("Skip EXIF parsing (maximum speed)"), imageGroup);
    imageLayout->addRow(m_skipExifCheck);

    imageGroup->setLayout(imageLayout);
    layout->addWidget(imageGroup);

    QGroupBox *cacheGroup = new QGroupBox(tr("Caching"), m_performanceTab);
    QFormLayout *cacheLayout = new QFormLayout(cacheGroup);

    m_cacheSizeSpin = new QSpinBox(cacheGroup);
    m_cacheSizeSpin->setRange(0, 1000);
    m_cacheSizeSpin->setSuffix(" MB");
    cacheLayout->addRow(tr("Image cache size:"), m_cacheSizeSpin);

    cacheGroup->setLayout(cacheLayout);
    layout->addWidget(cacheGroup);

    layout->addStretch();
}

void SettingsWindow::setupAppearanceTab() {
    QVBoxLayout *layout = new QVBoxLayout(m_appearanceTab);

    QGroupBox *fontGroup = new QGroupBox(tr("Font"), m_appearanceTab);
    QFormLayout *fontLayout = new QFormLayout(fontGroup);

    m_fontCombo = new QComboBox(fontGroup);
    m_fontCombo->addItems(QFontDatabase::families());
    fontLayout->addRow(tr("UI Font:"), m_fontCombo);

    m_fontSizeSpin = new QSpinBox(fontGroup);
    m_fontSizeSpin->setRange(8, 20);
    fontLayout->addRow(tr("Font Size:"), m_fontSizeSpin);

    m_themeCombo = new QComboBox(fontGroup);
    m_themeCombo->addItems({tr("Dark"), tr("Light")});
    fontLayout->addRow(tr("Theme:"), m_themeCombo);

    fontGroup->setLayout(fontLayout);
    layout->addWidget(fontGroup);

    QGroupBox *previewGroup = new QGroupBox(tr("Preview"), m_appearanceTab);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);

    m_previewLabel = new QLabel(tr("This is a preview of the UI appearance"), previewGroup);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(80);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_previewLabel);

    previewGroup->setLayout(previewLayout);
    layout->addWidget(previewGroup);

    layout->addStretch();

    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::updatePreview);
}

void SettingsWindow::loadCurrentSettings() {
    GeneralSettings g = m_manager->general();
    PerformanceSettings p = m_manager->performance();
    AppearanceSettings a = m_manager->appearance();

    QStringList states = {"normal", "maximized", "fullscreen"};
    m_windowStateCombo->setCurrentIndex(qMax(0, states.indexOf(g.defaultWindowState)));
    m_showInfoCheck->setChecked(g.showInfoPanel);
    m_maxRecentSpin->setValue(g.maxRecentFiles);

    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).toString() == g.language) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }

    m_lazyLoadingCheck->setChecked(p.lazyLoading);
    m_quickRenderCheck->setChecked(p.quickRender);
    m_skipExifCheck->setChecked(p.skipExif);
    m_cacheSizeSpin->setValue(p.cacheSize);

    m_fontCombo->setCurrentText(a.uiFont);
    m_fontSizeSpin->setValue(a.uiFontSize);
    m_themeCombo->setCurrentIndex(a.theme == "dark" ? 0 : 1);

    updatePreview();
}

void SettingsWindow::applyFromUi() {
    GeneralSettings g;
    QStringList states = {"normal", "maximized", "fullscreen"};
    g.defaultWindowState = states.value(m_windowStateCombo->currentIndex(), "normal");
    g.showInfoPanel = m_showInfoCheck->isChecked();
    g.maxRecentFiles = m_maxRecentSpin->value();
    g.language = m_languageCombo->currentData().toString();

    PerformanceSettings p;
    p.lazyLoading = m_lazyLoadingCheck->isChecked();
    p.quickRender = m_quickRenderCheck->isChecked();
    p.skipExif = m_skipExifCheck->isChecked();
    p.cacheSize = m_cacheSizeSpin->value();

    AppearanceSettings a;
    a.uiFont = m_fontCombo->currentText();
    a.uiFontSize = m_fontSizeSpin->value();
    a.theme = m_themeCombo->currentIndex() == 0 ? "dark" : "light";

    m_manager->setGeneral(g);
    m_manager->setPerformance(p);
    m_manager->setAppearance(a);
    m_manager->save();
}

void SettingsWindow::onAccepted() {
    applyFromUi();
    accept();
    emit settingsApplied();
}

void SettingsWindow::onApply() {
    applyFromUi();
    emit settingsApplied();
}

void SettingsWindow::onRestoreDefaults() {
    m_manager->setGeneral(GeneralSettings());
    m_manager->setPerformance(PerformanceSettings());
    m_manager->setAppearance(AppearanceSettings());
    m_manager->save();
    loadCurrentSettings();
    emit settingsApplied();
}

void SettingsWindow::updatePreview() {
    QString theme = m_themeCombo->currentIndex() == 0 ? "dark" : "light";
    QString bg = theme == "dark" ? "#2D2D30" : "#FFFFFF";
    QString fg = theme == "dark" ? "#E0E0E0" : "#000000";
    QString accent = "#007ACC";

    m_previewLabel->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 2px solid %3; "
        "border-radius: 5px; padding: 10px;"
    ).arg(bg, fg, accent));
}
