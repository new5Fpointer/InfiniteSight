#pragma once

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QDockWidget>
#include <QTreeWidget>
#include <QProgressBar>
#include <QLabel>
#include <QThread>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include "SettingsManager.h"
#include "ImageLoader.h"

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
    void openRecentFile();
    void clearRecentFiles();
    void toggleInfoPanel(bool visible);
    void openSettings();
    void switchTheme(const QString &theme);

    void zoomIn();
    void zoomOut();
    void actualSize();
    void fitToWindow();
    void rotateImage(int angle);
    void mirrorImage();
    void navigateFolderImage(int direction);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();
    void createMenus();
    void createToolBar();
    void updateRecentFilesMenu();
    void startImageLoading(const QString &filePath);
    void stopCurrentLoading();
    void resetCanvas();
    void initFolderRoaming(const QString &imagePath);
    void updateRoamStatus();
    void applyStyleSheet();
    QIcon themedIcon(const QString &name);
    void refreshToolBarIcons();

    SettingsManager *m_settingsManager;

    QSplitter *m_splitter;
    ZoomableGraphicsView *m_graphicsView;
    QGraphicsScene *m_graphicsScene;
    QGraphicsPixmapItem *m_pixmapItem;

    QDockWidget *m_infoDock;
    QTreeWidget *m_infoTree;
    QProgressBar *m_progressBar;
    QLabel *m_loadingLabel;
    QLabel *m_roamLabel;

    QMenu *m_fileMenu;
    QMenu *m_recentMenu;
    QMenu *m_viewMenu;
    QMenu *m_settingsMenu;
    QAction *m_openAction;
    QAction *m_exitAction;
    QAction *m_infoToggle;
    QAction *m_settingsAction;
    QAction *m_darkAction;
    QAction *m_lightAction;
    QAction *m_clearAction;

    QToolBar *m_toolBar;
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
};
