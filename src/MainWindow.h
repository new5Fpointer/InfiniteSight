#pragma once

#include "ImageLoader.h"
#include "SettingsManager.h"
#include "CustomMenu.h"
#include <QAction>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFrame>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>

class ZoomableGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void handleFileDrop(const QStringList &paths);

public slots:
    void onImageLoaded(const QPixmap &pixmap, const QString &filePath, const QString &jobId);
    void onInfoReady(const ImageInfo &info, const QString &jobId);
    void onProgress(int value);
    void applySettings();

private slots:
    void openImage();
    void toggleInfoPanel(bool visible);
    void openSettings();
    void switchTheme(const QString &theme);

    void zoomIn();
    void zoomOut();
    void actualSize();
    void fitToWindow();
    void toggleFitActualSize();
    void rotateImage(int angle);
    void mirrorImage();
    void navigateFolderImage(int direction);
    void jumpToImage(int index);

    void onMinimize();
    void onMaximize();
    void onClose();
    void updateMaximizeIcon();
    void showCustomMenu();
    void toggleFullscreen();
    void hideBottomBarAnimated();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUi();
    void createMenus();
    void createToolBar();
    void createTitleBar();
    void createBottomBar();
    void startImageLoading(const QString &filePath);
    void stopCurrentLoading();
    void resetCanvas();
    void initFolderRoaming(const QString &imagePath);
    void updateRoamStatus();
    void applyStyleSheet();
    QIcon themedIcon(const QString &name);
    void refreshToolBarIcons();
    void updateTitleBarTitle();
    void updateBottomBarInfo();

    SettingsManager *m_settingsManager;

    QWidget *m_titleBar;
    QLabel *m_titleIcon;
    QLabel *m_titleLabel;
    QPushButton *m_menuBtn;
    QPushButton *m_pinBtn;
    QPushButton *m_minBtn;
    QPushButton *m_maxBtn;
    QPushButton *m_closeBtn;
    QPoint m_dragPos;
    bool m_dragging;
    QTimer *m_bottomBarTimer;

    QWidget *m_bottomBar;
    QWidget *m_infoContainer;
    QWidget *m_centerContainer;
    QLabel *m_fileInfoLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_fileDimensionLabel;
    QLabel *m_fileFormatLabel;
    QPushButton *m_prevBtn;
    QLabel *m_pageLabel;
    QLineEdit *m_pageEdit;
    QPushButton *m_nextBtn;
    QPushButton *m_fitBtn;
    QPushButton *m_zoomCombo;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_zoomInBtn;
    QPushButton *m_rotateLeftBtn;
    QPushButton *m_rotateRightBtn;
    QPushButton *m_copyBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_fullscreenBtn;
    bool m_bottomBarInLayout = false;
    bool m_isFitToWindow = true;

    QSplitter *m_splitter;
    ZoomableGraphicsView *m_graphicsView;
    QGraphicsScene *m_graphicsScene;
    QGraphicsPixmapItem *m_pixmapItem;

    QDockWidget *m_infoDock;
    QTreeWidget *m_infoTree;
    QProgressBar *m_progressBar;
    QLabel *m_loadingLabel;
    QLabel *m_roamLabel;

    CustomMenu *m_customMenu = nullptr;
    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    QMenu *m_settingsMenu;
    QAction *m_openAction;
    QAction *m_exitAction;
    QAction *m_infoToggle;
    QAction *m_settingsAction;
    QAction *m_darkAction;
    QAction *m_lightAction;

    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    QAction *m_actualSizeAction;
    QAction *m_fitWindowAction;
    QAction *m_rotateLeftAction;
    QAction *m_rotateRightAction;
    QAction *m_mirrorAction;
    QAction *m_prevImageAction;
    QAction *m_nextImageAction;

    QThread *m_loaderThread;
    ImageLoader *m_imageLoader;

    QString m_currentImagePath;
    QString m_currentJobId;
    double m_scaleFactor;
    QStringList m_currentFolderImages;
    int m_currentFolderIndex;
    int m_imageWidth;
    int m_imageHeight;
    qint64 m_fileSize;
};
