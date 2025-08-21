#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "imageloader.h"
#include <QThread>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QProgressBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QDockWidget>
#include <QTreeWidget>
#include <QStatusBar>

class ImageViewer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void openImage();
    void updateImageInfo(const QString& filePath);
    void showImage(const QPixmap& pixmap);
    void onImageLoaded(const QPixmap& pixmap, const QString& filePath);
    void onLoadProgress(int percent);
    void onLoadError(const QString& error);

private:
    void createMenu();
    void createDockWidget();
    void loadImage(const QString& filePath);

    ImageViewer* imageViewer;
    QDockWidget* infoDock;
    QTreeWidget* infoTree;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QString currentImagePath;
    QThread* loaderThread;
    ImageLoader* imageLoader;
};

#endif // MAINWINDOW_H