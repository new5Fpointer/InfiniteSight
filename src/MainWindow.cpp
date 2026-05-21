#include "MainWindow.h"
#include "ImageCache.h"
#include "SettingsWindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMovie>
#include <QOpenGLWidget>
#include <QScrollBar>
#include <QSplitter>
#include <QStyle>
#include <QTransform>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>

ZoomableGraphicsView::ZoomableGraphicsView(QWidget *parent)
    : QGraphicsView(parent) {
    setAcceptDrops(true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setAlignment(Qt::AlignCenter);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    // setViewport(new QOpenGLWidget(this));
    // if (auto* glWidget = qobject_cast<QOpenGLWidget*>(viewport())) {
    //     glWidget->setFormat([]{
    //         QSurfaceFormat fmt;
    //         fmt.setSamples(4);
    //         fmt.setRenderableType(QSurfaceFormat::OpenGL);
    //         return fmt;
    //     }());
    // }
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
    : QMainWindow(parent), m_settingsManager(new SettingsManager(this)), m_graphicsView(new ZoomableGraphicsView(this)),
      m_graphicsScene(new QGraphicsScene(this)), m_pixmapItem(nullptr), m_progressBar(new QProgressBar(this)),
      m_loadingLabel(new QLabel(this)), m_roamLabel(new QLabel(this)), m_loaderThread(nullptr), m_imageLoader(nullptr),
      m_scaleFactor(1.0), m_currentFolderIndex(-1), m_imageWidth(0), m_imageHeight(0), m_fileSize(0), m_dragging(false) {
    setWindowTitle("InfiniteSight");
    setGeometry(100, 100, 1400, 900);
    setAcceptDrops(true);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupUi();
    createMenus();
    createToolBar();
    createTitleBar();
    createBottomBar();

    m_bottomBarTimer = new QTimer(this);
    m_bottomBarTimer->setSingleShot(true);
    m_bottomBarTimer->setInterval(1500);
    connect(m_bottomBarTimer, &QTimer::timeout, this, &MainWindow::hideBottomBarAnimated);
    m_graphicsView->setMouseTracking(true);
    m_graphicsView->viewport()->setMouseTracking(true);
    m_graphicsView->viewport()->installEventFilter(this);

    applySettings();
    updateMaximizeIcon();
}

MainWindow::~MainWindow() {
    stopCurrentLoading();
}

void MainWindow::setupUi() {
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(36);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(10, 0, 0, 0);
    titleLayout->setSpacing(0);

    m_titleIcon = new QLabel(this);
    m_titleIcon->setFixedSize(20, 20);
    titleLayout->addWidget(m_titleIcon);

    m_titleLabel = new QLabel(tr("InfiniteSight"), this);
    m_titleLabel->setObjectName("titleLabel");
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    auto createTitleBtn = [this](const QString &iconName, const QString &objName = "titleBtn") -> QPushButton * {
        QPushButton *btn = new QPushButton(this);
        btn->setFixedSize(46, 36);
        btn->setIconSize(QSize(14, 14));
        btn->setObjectName(objName);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    m_menuBtn = createTitleBtn("menu");
    connect(m_menuBtn, &QPushButton::clicked, this, [this]() {
        QPoint pos = m_menuBtn->mapToGlobal(QPoint(0, m_menuBtn->height()));
        m_fileMenu->exec(pos);
    });
    titleLayout->addWidget(m_menuBtn);

    m_pinBtn = createTitleBtn("pin");
    connect(m_pinBtn, &QPushButton::clicked, this, [this]() {
        Qt::WindowFlags flags = windowFlags();
        if (flags & Qt::WindowStaysOnTopHint) {
            setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
            m_pinBtn->setIcon(themedIcon("pin"));
        } else {
            setWindowFlags(flags | Qt::WindowStaysOnTopHint);
            m_pinBtn->setIcon(themedIcon("pin-off"));
        }
        show();
    });
    titleLayout->addWidget(m_pinBtn);

    m_minBtn = createTitleBtn("minimize");
    connect(m_minBtn, &QPushButton::clicked, this, &MainWindow::onMinimize);
    titleLayout->addWidget(m_minBtn);

    m_maxBtn = createTitleBtn("maximize");
    connect(m_maxBtn, &QPushButton::clicked, this, &MainWindow::onMaximize);
    titleLayout->addWidget(m_maxBtn);

    m_closeBtn = createTitleBtn("close", "closeBtn");
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::onClose);
    titleLayout->addWidget(m_closeBtn);

    mainLayout->addWidget(m_titleBar);

    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_graphicsView->setScene(m_graphicsScene);
    m_graphicsView->setAlignment(Qt::AlignCenter);
    m_graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

    QWidget *imageContainer = new QWidget(this);
    QVBoxLayout *imageLayout = new QVBoxLayout(imageContainer);
    imageLayout->setAlignment(Qt::AlignCenter);
    imageLayout->setContentsMargins(0, 0, 0, 0);
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

    qDebug().noquote() << "Ready";
}

void MainWindow::createTitleBar() {
}

void MainWindow::createBottomBar() {
    m_bottomBar = new QWidget(this);
    m_bottomBar->setObjectName("bottomBar");
    m_bottomBar->setFixedHeight(40);
    m_bottomBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);

    QHBoxLayout *bottomLayout = new QHBoxLayout(m_bottomBar);
    bottomLayout->setContentsMargins(12, 0, 12, 0);
    bottomLayout->setSpacing(0);

    auto createInfoBlock = [this](const QString &objName) -> QLabel * {
        QLabel *label = new QLabel(this);
        label->setObjectName(objName);
        label->setAlignment(Qt::AlignCenter);
        label->setFixedHeight(24);
        return label;
    };

    m_infoContainer = new QWidget(this);
    m_infoContainer->setObjectName("infoContainer");
    QHBoxLayout *infoLayout = new QHBoxLayout(m_infoContainer);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(6);

    m_fileSizeLabel = createInfoBlock("infoBlock");
    m_fileSizeLabel->setFixedWidth(60);
    infoLayout->addWidget(m_fileSizeLabel);

    m_fileDimensionLabel = createInfoBlock("infoBlock");
    m_fileDimensionLabel->setFixedWidth(80);
    infoLayout->addWidget(m_fileDimensionLabel);

    m_fileFormatLabel = createInfoBlock("infoBlock");
    m_fileFormatLabel->setFixedWidth(50);
    infoLayout->addWidget(m_fileFormatLabel);

    bottomLayout->addWidget(m_infoContainer);

    bottomLayout->addStretch();

    auto createBottomBtn = [this](const QString &iconName, int w = 32) -> QPushButton * {
        QPushButton *btn = new QPushButton(this);
        btn->setFixedSize(w, 28);
        if (!iconName.isEmpty()) {
            btn->setIcon(themedIcon(iconName));
            btn->setIconSize(QSize(16, 16));
        }
        btn->setObjectName("bottomBtn");
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };

    m_centerContainer = new QWidget(m_bottomBar);
    m_centerContainer->setObjectName("centerContainer");
    QHBoxLayout *centerLayout = new QHBoxLayout(m_centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(6);

    m_prevBtn = createBottomBtn("chevron-left");
    connect(m_prevBtn, &QPushButton::clicked, this, [this]() { navigateFolderImage(-1); });
    centerLayout->addWidget(m_prevBtn);

    QWidget *pageContainer = new QWidget(m_centerContainer);
    pageContainer->setObjectName("pageContainer");
    pageContainer->setFixedWidth(60);
    QHBoxLayout *pageLayout = new QHBoxLayout(pageContainer);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    m_pageLabel = new QLabel("0 / 0", pageContainer);
    m_pageLabel->setObjectName("pageLabel");
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_pageLabel->setFixedHeight(24);
    m_pageLabel->setCursor(Qt::PointingHandCursor);
    m_pageLabel->installEventFilter(this);
    pageLayout->addWidget(m_pageLabel);

    m_pageEdit = new QLineEdit(pageContainer);
    m_pageEdit->setObjectName("pageEdit");
    m_pageEdit->setFixedHeight(24);
    m_pageEdit->setAlignment(Qt::AlignCenter);
    m_pageEdit->setVisible(false);
    connect(m_pageEdit, &QLineEdit::editingFinished, this, [this]() {
        QString text = m_pageEdit->text().trimmed();
        m_pageEdit->setVisible(false);
        m_pageLabel->setVisible(true);
        if (m_bottomBarTimer)
            m_bottomBarTimer->start();
        if (text.isEmpty())
            return;
        bool ok;
        int page = text.toInt(&ok);
        if (ok && page > 0) {
            jumpToImage(page - 1);
        }
    });
    connect(m_pageEdit, &QLineEdit::returnPressed, this, [this]() {
        m_pageEdit->clearFocus();
    });
    m_pageEdit->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    pageLayout->addWidget(m_pageEdit);

    centerLayout->addWidget(pageContainer);

    m_nextBtn = createBottomBtn("chevron-right");
    connect(m_nextBtn, &QPushButton::clicked, this, [this]() { navigateFolderImage(1); });
    centerLayout->addWidget(m_nextBtn);

    centerLayout->addSpacing(16);

    m_fitBtn = createBottomBtn("fit-screen");
    m_fitBtn->setToolTip(tr("Fit to Window"));
    connect(m_fitBtn, &QPushButton::clicked, this, &MainWindow::fitToWindow);
    centerLayout->addWidget(m_fitBtn);

    m_zoomCombo = createBottomBtn("", 56);
    m_zoomCombo->setText("100%");
    connect(m_zoomCombo, &QPushButton::clicked, this, &MainWindow::actualSize);
    centerLayout->addWidget(m_zoomCombo);

    m_zoomOutBtn = createBottomBtn("zoom-out");
    m_zoomOutBtn->setToolTip(tr("Zoom Out"));
    connect(m_zoomOutBtn, &QPushButton::clicked, this, &MainWindow::zoomOut);
    centerLayout->addWidget(m_zoomOutBtn);

    m_zoomInBtn = createBottomBtn("zoom-in");
    m_zoomInBtn->setToolTip(tr("Zoom In"));
    connect(m_zoomInBtn, &QPushButton::clicked, this, &MainWindow::zoomIn);
    centerLayout->addWidget(m_zoomInBtn);

    m_rotateLeftBtn = createBottomBtn("rotate-left");
    m_rotateLeftBtn->setToolTip(tr("Rotate left") + " (Ctrl+L)");
    connect(m_rotateLeftBtn, &QPushButton::clicked, this, [this]() { rotateImage(-90); });
    centerLayout->addWidget(m_rotateLeftBtn);

    m_rotateRightBtn = createBottomBtn("rotate-right");
    m_rotateRightBtn->setToolTip(tr("Rotate right") + " (Ctrl+R)");
    connect(m_rotateRightBtn, &QPushButton::clicked, this, [this]() { rotateImage(90); });
    centerLayout->addWidget(m_rotateRightBtn);

    m_copyBtn = createBottomBtn("copy");
    m_copyBtn->setToolTip(tr("Copy Image"));
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        if (m_pixmapItem && !m_pixmapItem->pixmap().isNull()) {
            QApplication::clipboard()->setPixmap(m_pixmapItem->pixmap());
            qDebug().noquote() << tr("Image copied to clipboard");
        }
    });
    centerLayout->addWidget(m_copyBtn);

    m_deleteBtn = createBottomBtn("delete");
    m_deleteBtn->setToolTip(tr("Delete Image"));
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentImagePath.isEmpty())
            return;

        QFile file(m_currentImagePath);
        if (!file.moveToTrash()) {
            qDebug() << "Delete failed:" << file.errorString();
            return;
        }
        qDebug() << "Image deleted";

        int currentIndex = m_currentFolderIndex;
        m_currentFolderImages.removeAt(currentIndex);

        if (m_currentFolderImages.isEmpty()) {
            m_graphicsScene->clear();
            m_pixmapItem = nullptr;
            m_currentImagePath.clear();
            m_currentFolderIndex = -1;
            updateTitleBarTitle();
            updateBottomBarInfo();
        } else {
            if (currentIndex >= m_currentFolderImages.size()) {
                m_currentFolderIndex = m_currentFolderImages.size() - 1;
            } else {
                m_currentFolderIndex = currentIndex;
            }

            startImageLoading(m_currentFolderImages[m_currentFolderIndex]);
        }
    });
    centerLayout->addWidget(m_deleteBtn);

    m_centerContainer->adjustSize();
    m_centerContainer->setFixedSize(m_centerContainer->sizeHint());

    m_fullscreenBtn = createBottomBtn("fullscreen");
    m_fullscreenBtn->setToolTip(tr("Fullscreen") + " (F11)");
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &MainWindow::toggleFullscreen);
    bottomLayout->addWidget(m_fullscreenBtn);

    if (auto *lay = centralWidget()->layout()) {
        lay->addWidget(m_bottomBar);
    }
    m_bottomBarInLayout = true;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (m_bottomBar && isFullScreen()) {
        int barHeight = m_bottomBar->height();
        m_bottomBar->setGeometry(0, height() - barHeight, width(), barHeight);
    }
    if (m_bottomBar && m_centerContainer) {
        int x = (m_bottomBar->width() - m_centerContainer->width()) / 2;
        int y = (m_bottomBar->height() - m_centerContainer->height()) / 2;
        m_centerContainer->move(x, y);
    }
}

void MainWindow::createMenus() {
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setVisible(false);

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
    m_zoomInAction = new QAction(themedIcon("zoom-in"), "", this);
    m_zoomInAction->setToolTip(tr("Zoom In") + " (Ctrl++)");
    m_zoomInAction->setShortcut(QKeySequence("Ctrl++"));
    connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);

    m_zoomOutAction = new QAction(themedIcon("zoom-out"), "", this);
    m_zoomOutAction->setToolTip(tr("Zoom Out") + " (Ctrl+-)");
    m_zoomOutAction->setShortcut(QKeySequence("Ctrl+-"));
    connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);

    m_actualSizeAction = new QAction(themedIcon("actual-size"), "", this);
    m_actualSizeAction->setToolTip(tr("Actual Size") + " (Ctrl+0)");
    m_actualSizeAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(m_actualSizeAction, &QAction::triggered, this, &MainWindow::actualSize);

    m_fitWindowAction = new QAction(themedIcon("fit-screen"), "", this);
    m_fitWindowAction->setToolTip(tr("Fit to Window") + " (Ctrl+1)");
    m_fitWindowAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(m_fitWindowAction, &QAction::triggered, this, &MainWindow::fitToWindow);

    m_rotateLeftAction = new QAction(themedIcon("rotate-left"), "", this);
    m_rotateLeftAction->setToolTip(tr("Rotate left") + " (Ctrl+L)");
    m_rotateLeftAction->setShortcut(QKeySequence("Ctrl+L"));
    connect(m_rotateLeftAction, &QAction::triggered, this, [this]() { rotateImage(-90); });

    m_rotateRightAction = new QAction(themedIcon("rotate-right"), "", this);
    m_rotateRightAction->setToolTip(tr("Rotate right") + " (Ctrl+R)");
    m_rotateRightAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(m_rotateRightAction, &QAction::triggered, this, [this]() { rotateImage(90); });

    m_mirrorAction = new QAction(themedIcon("mirror-horizontal"), "", this);
    m_mirrorAction->setToolTip(tr("Mirror Horizontal") + " (Ctrl+M)");
    m_mirrorAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(m_mirrorAction, &QAction::triggered, this, &MainWindow::mirrorImage);

    m_prevImageAction = new QAction(themedIcon("chevron-left"), "", this);
    m_prevImageAction->setToolTip(tr("Previous Image") + " (Left)");
    m_prevImageAction->setShortcut(QKeySequence("Left"));
    connect(m_prevImageAction, &QAction::triggered, this, [this]() { navigateFolderImage(-1); });

    m_nextImageAction = new QAction(themedIcon("chevron-right"), "", this);
    m_nextImageAction->setToolTip(tr("Next Image") + " (Right)");
    m_nextImageAction->setShortcut(QKeySequence("Right"));
    connect(m_nextImageAction, &QAction::triggered, this, [this]() { navigateFolderImage(1); });
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
        qDebug().noquote() << tr("File not found: %1").arg(filePath);
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
        qDebug().noquote() << tr("Settings applied successfully");
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
        updateBottomBarInfo();
    }
}

void MainWindow::zoomOut() {
    if (m_pixmapItem) {
        m_scaleFactor *= 0.8;
        m_graphicsView->scale(0.8, 0.8);
        updateBottomBarInfo();
    }
}

void MainWindow::actualSize() {
    if (m_pixmapItem) {
        m_graphicsView->resetTransform();
        m_scaleFactor = 1.0;
        updateBottomBarInfo();
    }
}

void MainWindow::fitToWindow() {
    if (m_pixmapItem) {
        m_graphicsView->resetTransform();
        m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
        QTransform transform = m_graphicsView->transform();
        m_scaleFactor = transform.m11();
        updateBottomBarInfo();
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

    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());
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

    jumpToImage(newIndex);
}

void MainWindow::jumpToImage(int index) {
    if (m_currentFolderImages.isEmpty() || index < 0 || index >= m_currentFolderImages.size())
        return;
    if (index == m_currentFolderIndex)
        return;

    stopCurrentLoading();
    resetCanvas();
    startImageLoading(m_currentFolderImages[index]);
    m_currentFolderIndex = index;
}

void MainWindow::startImageLoading(const QString &filePath) {
    m_currentJobId = QUuid::createUuid().toString();
    m_currentImagePath = filePath;

    stopCurrentLoading();

    qDebug().noquote() << tr("Loading: %1...").arg(QFileInfo(filePath).fileName());
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
        qDebug().noquote() << tr("Error loading image");
        return;
    }

    resetCanvas();
    m_graphicsScene->clear();

    m_pixmapItem = new QGraphicsPixmapItem(pixmap);
    m_pixmapItem->setTransformationMode(Qt::SmoothTransformation);
    m_graphicsScene->addItem(m_pixmapItem);
    m_graphicsView->setSceneRect(m_graphicsScene->itemsBoundingRect());
    m_graphicsView->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    m_graphicsView->horizontalScrollBar()->setValue(0);
    m_graphicsView->verticalScrollBar()->setValue(0);

    m_imageWidth = pixmap.width();
    m_imageHeight = pixmap.height();
    m_fileSize = QFileInfo(filePath).size();

    m_loadingLabel->setVisible(false);
    m_progressBar->setVisible(false);
    qDebug().noquote() << tr("Loaded: %1").arg(QFileInfo(filePath).fileName());

    stopCurrentLoading();
    initFolderRoaming(filePath);
    updateRoamStatus();
    updateTitleBarTitle();
    updateBottomBarInfo();
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
    QString titleBarBg = theme == "dark" ? "#2D2D30" : "#F3F3F3";
    QString titleBarText = theme == "dark" ? "#E0E0E0" : "#000000";
    QString bottomBarBg = theme == "dark" ? "#2D2D30" : "#FFFFFF";
    QString bottomBarBgFullscreen = theme == "dark" ? "#E62D2D30" : "#E6FFFFFF";
    QString btnHover = theme == "dark" ? "#3F3F46" : "#E5E5E5";
    QString closeHover = "#E81123";
    QString viewBg = theme == "dark" ? "#1E1E1E" : "#FFFFFF";
    QString pageLabelBorder = theme == "dark" ? "#555555" : "#CCCCCC";

    QString style = QString(
                        "QMainWindow, QDockWidget, QTreeWidget, QScrollArea, QWidget {"
                        "  background-color: %1; color: %2; font-family: '%3'; font-size: %4pt; }"
                        "QMenuBar { background-color: %5; color: %6; border-bottom: 1px solid %7; }"
                        "QMenuBar::item:selected { background-color: %8; }"
                        "QTreeWidget::item:selected { background-color: %9; color: #FFFFFF; }"
                        "QProgressBar { border: 1px solid %7; border-radius: 3px; text-align: center;"
                        "  background-color: %10; color: %2; }"
                        "QProgressBar::chunk { background-color: %9; }"
                        "QScrollBar:vertical { border: none; background: %11; width: 12px; margin: 0px; }"
                        "QScrollBar::handle:vertical { background: %12; border-radius: 6px; min-height: 30px; }"
                        "QScrollBar::handle:vertical:hover { background: %13; }"
                        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { border: none; background: none; height: 0px; }"
                        "QScrollBar:horizontal { border: none; background: %11; height: 12px; margin: 0px; }"
                        "QScrollBar::handle:horizontal { background: %12; border-radius: 6px; min-width: 30px; }"
                        "QScrollBar::handle:horizontal:hover { background: %13; }"
                        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { border: none; background: none; width: 0px; }"
                        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,"
                        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
                        "#titleBar { background-color: %14; border-bottom: 1px solid %7; }"
                        "#titleLabel { color: %15; font-size: 13px; padding-left: 6px; }"
                        "#titleBtn { background-color: transparent; border: none; border-radius: 0px; }"
                        "#titleBtn:hover { background-color: %16; }"
                        "#closeBtn { background-color: transparent; border: none; border-radius: 0px; }"
                        "#closeBtn:hover { background-color: %17; }"
                        "#bottomBar { background-color: %18; border-top: 1px solid %7; }"
                        "#bottomBar[fullscreen=\"true\"] { background-color: %20; border-top: 1px solid %7; }"
                        "#centerContainer { background-color: transparent; }"
                        "#fileInfoLabel { color: %15; font-size: 12px; padding: 0 8px; }"
                        "#pageContainer { background-color: transparent; }"
                        "#pageLabel { color: %15; font-size: 12px; background-color: transparent; border: 1px solid %21; border-radius: 4px; padding: 2px 4px; }"
                        "#pageEdit { color: %15; font-size: 12px; background-color: transparent; border: 1px solid %21; border-radius: 4px; padding: 2px 4px; }"
                        "#bottomBtn { background-color: transparent; border: none; border-radius: 4px; }"
                        "#bottomBtn:hover { background-color: %16; }"
                        "#infoBlock { background-color: %18; color: %15; font-size: 11px; border-radius: 4px; padding: 2px 8px; }"
                        "QGraphicsView { background-color: %19; border: none; }")
                        .arg(bg, text, a.uiFont)
                        .arg(a.uiFontSize)
                        .arg(titleBarBg, menuText, border, selected, accent, progressBg, scrollBg, scrollHandle, scrollHandleHover)
                        .arg(titleBarBg, titleBarText, btnHover, closeHover, bottomBarBg, viewBg)
                        .arg(bottomBarBgFullscreen)
                        .arg(pageLabelBorder);

    setStyleSheet(style);

    if (m_bottomBar) {
        m_bottomBar->setProperty("fullscreen", isFullScreen());
        m_bottomBar->style()->unpolish(m_bottomBar);
        m_bottomBar->style()->polish(m_bottomBar);
    }
}

QIcon MainWindow::themedIcon(const QString &name) {
    QString theme = m_settingsManager->appearance().theme;
    QString path = QString(":/icons/%1/%2.svg").arg(theme, name);
    return QIcon(path);
}

void MainWindow::refreshToolBarIcons() {
    QString theme = m_settingsManager->appearance().theme;
    m_zoomInAction->setIcon(themedIcon("zoom-in"));
    m_zoomOutAction->setIcon(themedIcon("zoom-out"));
    m_actualSizeAction->setIcon(themedIcon("actual-size"));
    m_fitWindowAction->setIcon(themedIcon("fit-screen"));
    m_rotateLeftAction->setIcon(themedIcon("rotate-left"));
    m_rotateRightAction->setIcon(themedIcon("rotate-right"));
    m_mirrorAction->setIcon(themedIcon("mirror-horizontal"));
    m_prevImageAction->setIcon(themedIcon("chevron-left"));
    m_nextImageAction->setIcon(themedIcon("chevron-right"));
    if (m_titleIcon) {
        QPixmap iconPix = QPixmap(QString(":/icons/%1/folder-open.svg").arg(theme)).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_titleIcon->setPixmap(iconPix);
    }
    if (m_menuBtn)
        m_menuBtn->setIcon(themedIcon("menu"));
    if (m_pinBtn) {
        Qt::WindowFlags flags = windowFlags();
        if (flags & Qt::WindowStaysOnTopHint)
            m_pinBtn->setIcon(themedIcon("pin-off"));
        else
            m_pinBtn->setIcon(themedIcon("pin"));
    }
    if (m_minBtn)
        m_minBtn->setIcon(themedIcon("minimize"));
    if (m_maxBtn)
        m_maxBtn->setIcon(themedIcon("maximize"));
    if (m_closeBtn)
        m_closeBtn->setIcon(themedIcon("close"));

    if (m_prevBtn)
        m_prevBtn->setIcon(themedIcon("chevron-left"));
    if (m_nextBtn)
        m_nextBtn->setIcon(themedIcon("chevron-right"));
    if (m_fitBtn)
        m_fitBtn->setIcon(themedIcon("fit-screen"));
    if (m_zoomOutBtn)
        m_zoomOutBtn->setIcon(themedIcon("zoom-out"));
    if (m_zoomInBtn)
        m_zoomInBtn->setIcon(themedIcon("zoom-in"));
    if (m_rotateLeftBtn)
        m_rotateLeftBtn->setIcon(themedIcon("rotate-left"));
    if (m_rotateRightBtn)
        m_rotateRightBtn->setIcon(themedIcon("rotate-right"));
    if (m_copyBtn)
        m_copyBtn->setIcon(themedIcon("copy"));
    if (m_deleteBtn)
        m_deleteBtn->setIcon(themedIcon("delete"));

    if (m_maxBtn)
        updateMaximizeIcon();
}

void MainWindow::updateTitleBarTitle() {
    if (m_currentImagePath.isEmpty()) {
        m_titleLabel->setText(tr("InfiniteSight"));
    } else {
        m_titleLabel->setText(QFileInfo(m_currentImagePath).fileName());
    }
}

void MainWindow::updateBottomBarInfo() {
    if (m_currentImagePath.isEmpty()) {
        if (m_fileInfoLabel)
            m_fileInfoLabel->setText("");
        if (m_fileSizeLabel)
            m_fileSizeLabel->setText("");
        if (m_fileDimensionLabel)
            m_fileDimensionLabel->setText("");
        if (m_fileFormatLabel)
            m_fileFormatLabel->setText("");
        m_pageLabel->setText("0 / 0");
        m_zoomCombo->setText("100%");
        return;
    }

    QFileInfo fi(m_currentImagePath);
    QString ext = fi.suffix().toUpper();
    QString sizeStr;
    if (m_fileSize < 1024) {
        sizeStr = QString("%1B").arg(m_fileSize);
    } else if (m_fileSize < 1024 * 1024) {
        sizeStr = QString("%1K").arg(m_fileSize / 1024.0, 0, 'f', 1);
    } else {
        sizeStr = QString("%1M").arg(m_fileSize / (1024.0 * 1024.0), 0, 'f', 2);
    }

    if (m_fileSizeLabel)
        m_fileSizeLabel->setText(sizeStr);
    if (m_fileDimensionLabel)
        m_fileDimensionLabel->setText(QString("%1x%2").arg(m_imageWidth).arg(m_imageHeight));
    if (m_fileFormatLabel)
        m_fileFormatLabel->setText(ext);

    int curr = m_currentFolderIndex >= 0 ? m_currentFolderIndex + 1 : 1;
    int total = m_currentFolderImages.isEmpty() ? 1 : m_currentFolderImages.size();
    m_pageLabel->setText(QString("%1/%2").arg(curr).arg(total));

    int zoomPercent = qRound(m_scaleFactor * 100);
    m_zoomCombo->setText(QString("%1%").arg(zoomPercent));
}

void MainWindow::onMinimize() {
    showMinimized();
}

void MainWindow::onMaximize() {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    updateMaximizeIcon();
}

void MainWindow::onClose() {
    close();
}

void MainWindow::updateMaximizeIcon() {
    if (m_maxBtn) {
        if (isMaximized()) {
            m_maxBtn->setIcon(themedIcon("restore"));
        } else {
            m_maxBtn->setIcon(themedIcon("maximize"));
        }
    }
}

void MainWindow::toggleFullscreen() {
    if (isFullScreen()) {
        m_bottomBarTimer->stop();
        if (m_bottomBar) {
            m_bottomBar->setVisible(true);
            if (!m_bottomBarInLayout) {
                if (auto *lay = centralWidget()->layout()) {
                    lay->addWidget(m_bottomBar);
                    m_bottomBarInLayout = true;
                }
            }
        }
        showNormal();
        if (m_titleBar)
            m_titleBar->setVisible(true);
        if (m_infoContainer)
            m_infoContainer->setVisible(true);
        if (m_fullscreenBtn) {
            m_fullscreenBtn->setIcon(themedIcon("fullscreen"));
            m_fullscreenBtn->setToolTip(tr("Fullscreen") + " (F11)");
        }
        applyStyleSheet();
    } else {
        if (m_bottomBar) {
            if (m_bottomBarInLayout) {
                if (auto *lay = centralWidget()->layout()) {
                    lay->removeWidget(m_bottomBar);
                    m_bottomBarInLayout = false;
                }
            }
            m_bottomBar->setParent(centralWidget());
            m_bottomBar->setVisible(false);
            m_bottomBar->raise();
        }
        showFullScreen();
        if (m_titleBar)
            m_titleBar->setVisible(false);
        if (m_infoContainer)
            m_infoContainer->setVisible(false);
        if (m_fullscreenBtn) {
            m_fullscreenBtn->setIcon(themedIcon("fullscreen-exit"));
            m_fullscreenBtn->setToolTip(tr("Exit Fullscreen") + " (F11)");
        }
        applyStyleSheet();
    }
}

void MainWindow::hideBottomBarAnimated() {
    if (m_bottomBar && isFullScreen())
        m_bottomBar->setVisible(false);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_pageLabel && event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton && !m_currentFolderImages.isEmpty()) {
            m_pageLabel->setVisible(false);
            m_pageEdit->setText(QString::number(m_currentFolderIndex + 1));
            m_pageEdit->setVisible(true);
            m_pageEdit->setFocus();
            m_pageEdit->selectAll();
            if (m_bottomBarTimer)
                m_bottomBarTimer->stop();
            return true;
        }
    }
    if (event->type() == QEvent::MouseMove && isFullScreen() && m_bottomBar) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        QPoint localPos = mapFromGlobal(me->globalPosition().toPoint());
        int barHeight = m_bottomBar->height();
        int triggerZone = barHeight + 20;
        if (localPos.y() >= height() - triggerZone) {
            if (!m_bottomBar->isVisible()) {
                m_bottomBar->setVisible(true);
                m_bottomBar->raise();
            }
            m_bottomBarTimer->stop();
        } else {
            if (m_bottomBar->isVisible() && !(m_pageEdit && m_pageEdit->isVisible())) {
                m_bottomBarTimer->start();
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
            m_dragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPos);
        event->accept();
        return;
    }
    if (isFullScreen() && m_bottomBar) {
        int barHeight = m_bottomBar->height();
        int triggerZone = barHeight + 20;
        if (event->pos().y() >= height() - triggerZone) {
            if (!m_bottomBar->isVisible()) {
                m_bottomBar->setVisible(true);
                m_bottomBar->raise();
            }
            m_bottomBarTimer->stop();
        } else {
            if (m_bottomBar->isVisible() && !(m_pageEdit && m_pageEdit->isVisible())) {
                m_bottomBarTimer->start();
            }
        }
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
        onMaximize();
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
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
        return;
    }

    QString folder = QFileInfo(m_currentImagePath).dir().dirName();
    int curr = m_currentFolderIndex + 1;
    int total = m_currentFolderImages.size();
    qDebug().noquote() << tr("Image %1 of %2 in %3").arg(curr).arg(total).arg(folder);
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
    } else if (event->key() == Qt::Key_F11) {
        toggleFullscreen();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}
