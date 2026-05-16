#include "MainWindow.h"
#include "ImageCache.h"
#include "SettingsWindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMovie>
#include <QScrollBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTransform>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent) {
    setAcceptDrops(true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignCenter);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
}

void ZoomableGraphicsView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double zoomInFactor = 1.15;
        double zoomOutFactor = 1.0 / zoomInFactor;
        double scaleFactor = (event->angleDelta().y() > 0) ? zoomInFactor : zoomOutFactor;

        QPointF mousePos = event->position();
        QPointF scenePos = mapToScene(mousePos.toPoint());

        scale(scaleFactor, scaleFactor);

        QPointF newScenePos = mapToScene(mousePos.toPoint());
        QPointF delta = newScenePos - scenePos;
        setTransformationAnchor(QGraphicsView::NoAnchor);
        translate(delta.x(), delta.y());

        event->accept();
    } else {
        int delta = static_cast<int>(event->angleDelta().y() * 0.5);
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
    }
}

void ZoomableGraphicsView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ZoomableGraphicsView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void ZoomableGraphicsView::dropEvent(QDropEvent *event) {
    if (parentWidget()) {
        MainWindow *mainWin = qobject_cast<MainWindow *>(parentWidget()->window());
        if (mainWin) {
            QList<QUrl> urls = event->mimeData()->urls();
            QStringList paths;
            for (const QUrl &url : urls) {
                paths.append(url.toLocalFile());
            }
            mainWin->handleFileDrop(paths);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_settingsManager(new SettingsManager(this)), m_graphicsView(new ZoomableGraphicsView(this)), m_graphicsScene(new QGraphicsScene(this)), m_pixmapItem(nullptr), m_progressBar(new QProgressBar(this)), m_loadingLabel(new QLabel(this)), m_roamLabel(new QLabel(this)), m_loaderThread(nullptr), m_imageLoader(nullptr), m_scaleFactor(1.0), m_currentFolderIndex(-1) {
    setWindowTitle(tr("InfiniteSight - Modern Image Viewer"));
    setGeometry(100, 100, 1400, 900);
    setAcceptDrops(true);

    setupUi();
    createMenus();
    createToolBar();
    applySettings();
}

MainWindow::~MainWindow() {
    stopCurrentLoading();
}

void MainWindow::setupUi() {
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_graphicsView->setScene(m_graphicsScene);
    m_graphicsView->setAlignment(Qt::AlignCenter);
    m_graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

    QWidget *imageContainer = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setAlignment(Qt::AlignCenter);
    imageLayout->addWidget(m_graphicsView);

    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setVisible(false);
    imageLayout->addWidget(m_loadingLabel);

    m_progressBar->setVisible(false);
    m_progressBar->setRange(0, 100);
    imageLayout->addWidget(m_progressBar);

    m_infoDock = new QDockWidget(tr("Image Information"), this);
    m_infoDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_infoTree = new QTreeWidget(this);
    m_infoTree->setHeaderHidden(true);
    m_infoTree->setColumnWidth(0, 200);
    m_infoTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_infoTree->setFont(QFont("Segoe UI", 10));

    m_infoDock->setWidget(m_infoTree);
    addDockWidget(Qt::RightDockWidgetArea, m_infoDock);

    m_splitter->addWidget(imageContainer);
    m_splitter->addWidget(m_infoDock);
    m_splitter->setSizes({1000, 200});
    m_infoDock->setMinimumWidth(150);

    mainLayout->addWidget(m_splitter);
    setCentralWidget(mainWidget);

    m_roamLabel->setObjectName("roam_label");
    statusBar()->addPermanentWidget(m_roamLabel);
    m_roamLabel->setVisible(false);

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::createMenus() {
    QMenuBar *menuBar = new QMenuBar(this);

    m_fileMenu = menuBar->addMenu(tr("&File"));

    m_openAction = new QAction(tr("&Open Image"), this);
    m_openAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openImage);
    m_fileMenu->addAction(m_openAction);

    m_recentMenu = m_fileMenu->addMenu(tr("Recent Files"));
    updateRecentFilesMenu();

    m_fileMenu->addSeparator();

    m_exitAction = new QAction(tr("E&xit"), this);
    m_exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    m_fileMenu->addAction(m_exitAction);

    m_viewMenu = menuBar->addMenu(tr("&View"));

    m_infoToggle = new QAction(tr("&Image Information"), this);
    m_infoToggle->setShortcut(QKeySequence("Ctrl+I"));
    m_infoToggle->setCheckable(true);
    m_infoToggle->setChecked(m_settingsManager->general().showInfoPanel);
    connect(m_infoToggle, &QAction::toggled, this, &MainWindow::toggleInfoPanel);
    m_viewMenu->addAction(m_infoToggle);

    QMenu *themeMenu = m_viewMenu->addMenu(tr("Theme"));
    QActionGroup *themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    m_darkAction = new QAction(tr("Dark"), this);
    m_darkAction->setCheckable(true);
    m_darkAction->setActionGroup(themeGroup);
    connect(m_darkAction, &QAction::triggered, this, [this]() { switchTheme("dark"); });
    themeMenu->addAction(m_darkAction);

    m_lightAction = new QAction(tr("Light"), this);
    m_lightAction->setCheckable(true);
    m_lightAction->setActionGroup(themeGroup);
    connect(m_lightAction, &QAction::triggered, this, [this]() { switchTheme("light"); });
    themeMenu->addAction(m_lightAction);

    QString currentTheme = m_settingsManager->appearance().theme;
    m_darkAction->setChecked(currentTheme == "dark");
    m_lightAction->setChecked(currentTheme == "light");

    m_settingsMenu = menuBar->addMenu(tr("&Settings"));
    m_settingsAction = new QAction(tr("Application Settings"), this);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    m_settingsMenu->addAction(m_settingsAction);

    setMenuBar(menuBar);
}

void MainWindow::createToolBar() {
    m_toolBar = addToolBar(tr("Tools"));
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(20, 20));

    m_zoomInAction = new QAction(themedIcon("zoom-in"), "", this);
    m_zoomInAction->setToolTip(tr("Zoom In") + " (Ctrl++)");
    m_zoomInAction->setShortcut(QKeySequence("Ctrl++"));
    connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    m_toolBar->addAction(m_zoomInAction);

    m_zoomOutAction = new QAction(themedIcon("zoom-out"), "", this);
    m_zoomOutAction->setToolTip(tr("Zoom Out") + " (Ctrl+-)");
    m_zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));
    connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    m_toolBar->addAction(m_zoomOutAction);

    m_actualSizeAction = new QAction(themedIcon("actual-size"), "", this);
    m_actualSizeAction->setToolTip(tr("Actual Size") + " (Ctrl+0)");
    m_actualSizeAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(m_actualSizeAction, &QAction::triggered, this, &MainWindow::actualSize);
    m_toolBar->addAction(m_actualSizeAction);

    m_fitWindowAction = new QAction(themedIcon("fit-screen"), "", this);
    m_fitWindowAction->setToolTip(tr("Fit to Window") + " (Ctrl+1)");
    m_fitWindowAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(m_fitWindowAction, &QAction::triggered, this, &MainWindow::fitToWindow);
    m_toolBar->addAction(m_fitWindowAction);

    m_rotateLeftAction = new QAction(themedIcon("rotate-left"), "", this);
    m_rotateLeftAction->setToolTip(tr("Rotate left") + " (Ctrl+L)");
    m_rotateLeftAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(m_rotateLeftAction, &QAction::triggered, this, [this]() { rotateImage(-90); });
    m_toolBar->addAction(m_rotateLeftAction);

    m_rotateRightAction = new QAction(themedIcon("rotate-right"), "", this);
    m_rotateRightAction->setToolTip(tr("Rotate right") + " (Ctrl+R)");
    m_rotateRightAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(m_rotateRightAction, &QAction::triggered, this, [this]() { rotateImage(90); });
    m_toolBar->addAction(m_rotateRightAction);

    m_mirrorAction = new QAction(themedIcon("mirror-horizontal"), "", this);
    m_mirrorAction->setToolTip(tr("Mirror Horizontal") + " (Ctrl+M)");
    m_mirrorAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(m_mirrorAction, &QAction::triggered, this, &MainWindow::mirrorImage);
    m_toolBar->addAction(m_mirrorAction);

    m_prevImageAction = new QAction(themedIcon("chevron-left"), "", this);
    m_prevImageAction->setToolTip(tr("Previous Image") + " (Left)");
    m_prevImageAction->setShortcut(QKeySequence("Left"));
    connect(m_prevImageAction, &QAction::triggered, this, [this]() { navigateFolderImage(-1); });
    m_toolBar->addAction(m_prevImageAction);

    m_nextImageAction = new QAction(themedIcon("chevron-right"), "", this);
    m_nextImageAction->setToolTip(tr("Next Image") + " (Right)");
    m_nextImageAction->setShortcut(QKeySequence("Right"));
    connect(m_nextImageAction, &QAction::triggered, this, [this]() { navigateFolderImage(1); });
    m_toolBar->addAction(m_nextImageAction);
}

void MainWindow::updateRecentFilesMenu() {
    m_recentMenu->clear();
    QStringList recentFiles = m_settingsManager->general().recentFiles;

    if (recentFiles.isEmpty()) {
        QAction *noFilesAction = new QAction(tr("No recent files"), this);
        noFilesAction->setEnabled(false);
        m_recentMenu->addAction(noFilesAction);
        return;
    }

    for (const QString &filePath : recentFiles) {
        if (QFile::exists(filePath)) {
            QAction *action = new QAction(QFileInfo(filePath).fileName(), this);
            action->setData(filePath);
            connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
            m_recentMenu->addAction(action);
        }
    }

    m_recentMenu->addSeparator();
    m_clearAction = new QAction(tr("Clear Recent Files"), this);
    connect(m_clearAction, &QAction::triggered, this, &MainWindow::clearRecentFiles);
    m_recentMenu->addAction(m_clearAction);
}

void MainWindow::openImage() {
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Image"), "",
                                                    tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp)"));

    if (!filePath.isEmpty()) {
        stopCurrentLoading();
        resetCanvas();
        m_settingsManager->addRecentFile(filePath);
        updateRecentFilesMenu();
        startImageLoading(filePath);
    }
}

void MainWindow::openRecentFile() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    QString filePath = action->data().toString();
    if (QFile::exists(filePath)) {
        stopCurrentLoading();
        resetCanvas();
        startImageLoading(filePath);
    } else {
        statusBar()->showMessage(tr("File not found: %1").arg(filePath), 3000);
    }
}

void MainWindow::clearRecentFiles() {
    m_settingsManager->clearRecentFiles();
    updateRecentFilesMenu();
}

void MainWindow::toggleInfoPanel(bool visible) {
    m_infoDock->setVisible(visible);
    GeneralSettings g = m_settingsManager->general();
    g.showInfoPanel = visible;
    m_settingsManager->setGeneral(g);
    m_settingsManager->save();
}

void MainWindow::openSettings() {
    SettingsWindow dialog(m_settingsManager, this);
    connect(&dialog, &SettingsWindow::settingsApplied, this, &MainWindow::applySettings);
    if (dialog.exec() == QDialog::Accepted) {
        statusBar()->showMessage(tr("Settings applied successfully"));
    }
}

void MainWindow::switchTheme(const QString &theme) {
    AppearanceSettings a = m_settingsManager->appearance();
    a.theme = theme;
    m_settingsManager->setAppearance(a);
    m_settingsManager->save();
    applySettings();
}

void MainWindow::zoomIn() {
    if (m_pixmapItem) {
        m_scaleFactor *= 1.2;
        m_graphicsView->scale(1.2, 1.2);
    }
}

void MainWindow::zoomOut() {
    if (m_pixmapItem) {
        m_scaleFactor *= 0.8;
        m_graphicsView->scale(0.8, 0.8);
    }
}

void MainWindow::actualSize() {
    if (m_pixmapItem) {
        m_graphicsView->resetTransform();
        m_scaleFactor = 1.0;
    }
}

void MainWindow::fitToWindow() {
    if (m_pixmapItem) {
        m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    }
}

void MainWindow::rotateImage(int angle) {
    if (!m_pixmapItem)
        return;

    QPixmap current = m_pixmapItem->pixmap();
    if (current.isNull())
        return;

    QTransform transform;
    transform.rotate(angle);
    QPixmap rotated = current.transformed(transform, Qt::SmoothTransformation);
    m_pixmapItem->setPixmap(rotated);
    m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
}

void MainWindow::mirrorImage() {
    if (!m_pixmapItem)
        return;

    QPixmap current = m_pixmapItem->pixmap();
    if (current.isNull())
        return;

    QTransform transform;
    transform.scale(-1, 1);
    transform.translate(-current.width(), 0);
    QPixmap mirrored = current.transformed(transform, Qt::SmoothTransformation);
    m_pixmapItem->setPixmap(mirrored);
}

void MainWindow::navigateFolderImage(int direction) {
    if (m_currentFolderImages.isEmpty() || m_currentFolderIndex < 0)
        return;

    int newIndex = (m_currentFolderIndex + direction) % m_currentFolderImages.size();
    if (newIndex < 0)
        newIndex += m_currentFolderImages.size();
    if (newIndex == m_currentFolderIndex)
        return;

    stopCurrentLoading();
    resetCanvas();
    startImageLoading(m_currentFolderImages[newIndex]);
    m_currentFolderIndex = newIndex;
}

void MainWindow::startImageLoading(const QString &filePath) {
    m_currentJobId = QUuid::createUuid().toString();
    m_currentImagePath = filePath;

    stopCurrentLoading();

    statusBar()->showMessage(tr("Loading: %1...").arg(QFileInfo(filePath).fileName()));
    m_graphicsScene->clear();
    m_loadingLabel->setVisible(true);
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);

    PerformanceSettings perf = m_settingsManager->performance();
    m_imageLoader = new ImageLoader(filePath, perf, m_currentJobId);
    m_loaderThread = new QThread(this);
    m_imageLoader->moveToThread(m_loaderThread);

    connect(m_loaderThread, &QThread::started, m_imageLoader, &ImageLoader::load);
    connect(m_imageLoader, &ImageLoader::finished, this, &MainWindow::onImageLoaded);
    connect(m_imageLoader, &ImageLoader::infoReady, this, &MainWindow::onInfoReady);
    connect(m_imageLoader, &ImageLoader::progress, this, &MainWindow::onProgress);
    connect(m_loaderThread, &QThread::finished, m_loaderThread, &QObject::deleteLater);

    m_loaderThread->start();
}

void MainWindow::onImageLoaded(const QPixmap &pixmap, const QString &filePath, const QString &jobId) {
    if (jobId != m_currentJobId)
        return;
    if (pixmap.isNull()) {
        statusBar()->showMessage(tr("Error loading image"));
        return;
    }

    resetCanvas();
    m_graphicsScene->clear();

    m_pixmapItem = new QGraphicsPixmapItem(pixmap);
    m_graphicsScene->addItem(m_pixmapItem);
    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());
    m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    m_graphicsView->horizontalScrollBar()->setValue(0);
    m_graphicsView->verticalScrollBar()->setValue(0);

    m_loadingLabel->setVisible(false);
    m_progressBar->setVisible(false);
    statusBar()->showMessage(tr("Loaded: %1").arg(QFileInfo(filePath).fileName()));

    stopCurrentLoading();
    initFolderRoaming(filePath);
    updateRoamStatus();
}

void MainWindow::onInfoReady(const ImageInfo &info, const QString &jobId) {
    if (jobId != m_currentJobId)
        return;

    m_infoTree->clear();

    auto addSection = [this](const QString &title, const QMap<QString, QString> &data) {
        if (data.isEmpty())
            return;
        QTreeWidgetItem *root = new QTreeWidgetItem(m_infoTree, {title});
        for (auto it = data.begin(); it != data.end(); ++it) {
            new QTreeWidgetItem(root, {QString("%1: %2").arg(it.key(), it.value())});
        }
        root->setExpanded(true);
    };

    addSection(tr("File Information"), info.fileInfo);
    addSection(tr("Image Information"), info.imageInfo);

    if (!info.error.isEmpty()) {
        QTreeWidgetItem *errRoot = new QTreeWidgetItem(m_infoTree, {"Error"});
        new QTreeWidgetItem(errRoot, {info.error});
        errRoot->setExpanded(true);
    }
}

void MainWindow::onProgress(int value) {
    m_progressBar->setValue(value);
}

void MainWindow::applySettings() {
    GeneralSettings g = m_settingsManager->general();
    PerformanceSettings p = m_settingsManager->performance();
    AppearanceSettings a = m_settingsManager->appearance();

    if (g.defaultWindowState == "maximized") {
        showMaximized();
    } else if (g.defaultWindowState == "fullscreen") {
        showFullScreen();
    }

    m_infoDock->setVisible(g.showInfoPanel);
    m_infoToggle->setChecked(g.showInfoPanel);

    QFont appFont(a.uiFont, a.uiFontSize);
    QApplication::setFont(appFont);
    statusBar()->setFont(appFont);
    menuBar()->setFont(appFont);

    applyStyleSheet();
    refreshToolBarIcons();

    m_darkAction->setChecked(a.theme == "dark");
    m_lightAction->setChecked(a.theme == "light");
}

void MainWindow::applyStyleSheet() {
    AppearanceSettings a = m_settingsManager->appearance();
    QString theme = a.theme;
    QString bg = theme == "dark" ? "#2D2D30" : "#FFFFFF";
    QString accent = "#007ACC";
    QString text = theme == "dark" ? "#E0E0E0" : "#000000";
    QString menuText = theme == "dark" ? "#CCCCCC" : "#000000";
    QString border = theme == "dark" ? "#1E1E1E" : "#CCCCCC";
    QString selected = theme == "dark" ? "#3F3F46" : "#E0E0E0";
    QString progressBg = theme == "dark" ? "#1E1E1E" : "#FFFFFF";
    QString scrollBg = theme == "dark" ? "#404040" : "#F0F0F0";
    QString scrollHandle = theme == "dark" ? "#606060" : "#C0C0C0";
    QString scrollHandleHover = theme == "dark" ? "#808080" : "#A0A0A0";

    QString style = QString(
                        "QMainWindow, QDockWidget, QTreeWidget, QScrollArea, QWidget {"
                        "  background-color: %1; color: %2; font-family: '%3'; font-size: %4pt; }"
                        "QMenuBar { background-color: %1; color: %5; border-bottom: 1px solid %6; }"
                        "QMenuBar::item:selected { background-color: %7; }"
                        "QTreeWidget::item:selected { background-color: %8; color: #FFFFFF; }"
                        "QProgressBar { border: 1px solid %6; border-radius: 3px; text-align: center;"
                        "  background-color: %9; color: %2; }"
                        "QProgressBar::chunk { background-color: %8; }"
                        "QScrollBar:vertical { border: none; background: %10; width: 12px; margin: 0px; }"
                        "QScrollBar::handle:vertical { background: %11; border-radius: 6px; min-height: 30px; }"
                        "QScrollBar::handle:vertical:hover { background: %12; }"
                        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; height: 0px; }"
                        "QScrollBar:horizontal { border: none; background: %10; height: 12px; margin: 0px; }"
                        "QScrollBar::handle:horizontal { background: %11; border-radius: 6px; min-width: 30px; }"
                        "QScrollBar::handle:horizontal:hover { background: %12; }"
                        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { border: none; background: none; width: 0px; }"
                        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,"
                        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }")
                        .arg(bg, text, a.uiFont)
                        .arg(a.uiFontSize)
                        .arg(menuText, border, selected, accent, progressBg, scrollBg, scrollHandle, scrollHandleHover);

    setStyleSheet(style);
}

QIcon MainWindow::themedIcon(const QString &name) {
    QString theme = m_settingsManager->appearance().theme;
    QString path = QString(":/icons/%1/%2.svg").arg(theme, name);
    return QIcon(path);
}

void MainWindow::refreshToolBarIcons() {
    m_zoomInAction->setIcon(themedIcon("zoom-in"));
    m_zoomOutAction->setIcon(themedIcon("zoom-out"));
    m_actualSizeAction->setIcon(themedIcon("actual-size"));
    m_fitWindowAction->setIcon(themedIcon("fit-screen"));
    m_rotateLeftAction->setIcon(themedIcon("rotate-left"));
    m_rotateRightAction->setIcon(themedIcon("rotate-right"));
    m_mirrorAction->setIcon(themedIcon("mirror-horizontal"));
    m_prevImageAction->setIcon(themedIcon("chevron-left"));
    m_nextImageAction->setIcon(themedIcon("chevron-right"));
}

void MainWindow::stopCurrentLoading() {
    if (m_imageLoader) {
        m_imageLoader->cancel();
    }
    if (m_loaderThread && m_loaderThread->isRunning()) {
        m_loaderThread->quit();
        if (!m_loaderThread->wait(2000)) {
            m_loaderThread->terminate();
            m_loaderThread->wait();
        }
    }
    m_loaderThread = nullptr;
    m_imageLoader = nullptr;
}

void MainWindow::resetCanvas() {
    m_graphicsView->resetTransform();
    m_scaleFactor = 1.0;
    m_graphicsView->horizontalScrollBar()->setValue(0);
    m_graphicsView->verticalScrollBar()->setValue(0);
    m_graphicsScene->clearSelection();
    m_graphicsView->centerOn(0, 0);
    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());
}

void MainWindow::initFolderRoaming(const QString &imagePath) {
    QString folder = QFileInfo(imagePath).absolutePath();
    QDir dir(folder);
    if (!dir.exists())
        return;

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.gif" << "*.tiff" << "*.tif" << "*.webp";
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files);

    QStringList files = dir.entryList();
    files.sort(Qt::CaseInsensitive);

    m_currentFolderImages.clear();
    for (const QString &f : files) {
        m_currentFolderImages.append(dir.absoluteFilePath(f));
    }

    if (m_currentFolderImages.isEmpty()) {
        m_currentFolderIndex = -1;
        return;
    }

    m_currentFolderIndex = m_currentFolderImages.indexOf(imagePath);
    if (m_currentFolderIndex < 0)
        m_currentFolderIndex = 0;
}

void MainWindow::updateRoamStatus() {
    if (m_currentFolderImages.isEmpty() || m_currentFolderIndex < 0) {
        m_roamLabel->setVisible(false);
        return;
    }

    QString folder = QFileInfo(m_currentImagePath).dir().dirName();
    int curr = m_currentFolderIndex + 1;
    int total = m_currentFolderImages.size();
    m_roamLabel->setText(tr("Image %1 of %2 in %3").arg(curr).arg(total).arg(folder));
    m_roamLabel->setVisible(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::handleFileDrop(const QStringList &paths) {
    QStringList exts = {"png", "jpg", "jpeg", "bmp", "gif", "tiff", "tif", "webp"};

    for (const QString &path : paths) {
        QString ext = QFileInfo(path).suffix().toLower();
        if (exts.contains(ext)) {
            stopCurrentLoading();
            resetCanvas();
            m_settingsManager->addRecentFile(path);
            updateRecentFilesMenu();
            startImageLoading(path);
            break;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urls = event->mimeData()->urls();
    QStringList paths;
    for (const QUrl &url : urls) {
        paths.append(url.toLocalFile());
    }
    handleFileDrop(paths);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    stopCurrentLoading();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        navigateFolderImage(-1);
    } else if (event->key() == Qt::Key_Right) {
        navigateFolderImage(1);
    } else {
        QMainWindow::keyPressEvent(event);
    }
}
