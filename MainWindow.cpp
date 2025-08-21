#include "MainWindow.h"
#include "ImageViewer.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QHeaderView>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), loaderThread(nullptr), imageLoader(nullptr)
{
    setWindowTitle("InfiniteSight - Modern Image Viewer");
    setGeometry(100, 100, 1400, 900);

    // �������벿��
    imageViewer = new ImageViewer(this);
    setCentralWidget(imageViewer);

    // ������Ϣ���
    createDockWidget();

    // �����˵�
    createMenu();

    // ����״̬��
    statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);

    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    statusBar()->addPermanentWidget(progressBar);

    // �����Ϸ�
    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    if (loaderThread && loaderThread->isRunning()) {
        loaderThread->quit();
        loaderThread->wait();
    }
}

void MainWindow::createMenu()
{
    QMenuBar* menuBar = new QMenuBar(this);

    // �ļ��˵�
    QMenu* fileMenu = menuBar->addMenu("File");

    QAction* openAction = new QAction("Open", this);
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
    fileMenu->addAction(openAction);

    QAction* exitAction = new QAction("Exit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);

    setMenuBar(menuBar);
}

void MainWindow::createDockWidget()
{
    infoDock = new QDockWidget("Image Information", this);
    infoDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    infoDock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);

    infoTree = new QTreeWidget();
    infoTree->setHeaderHidden(true);
    infoTree->setColumnWidth(0, 200);
    infoTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    infoDock->setWidget(infoTree);
    addDockWidget(Qt::RightDockWidgetArea, infoDock);
}

void MainWindow::openImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.tif *.webp)"
    );

    if (!filePath.isEmpty()) {
        loadImage(filePath);
    }
}

void MainWindow::loadImage(const QString& filePath)
{
    currentImagePath = filePath;
    statusLabel->setText("Loading: " + QFileInfo(filePath).fileName());
    progressBar->setVisible(true);
    progressBar->setValue(0);

    // ȡ���κ����ڽ��еļ���
    if (loaderThread && loaderThread->isRunning()) {
        loaderThread->quit();
        loaderThread->wait();
        delete loaderThread;
        loaderThread = nullptr;
    }

    if (imageLoader) {
        delete imageLoader;
        imageLoader = nullptr;
    }

    // �����µ��̺߳ͼ�����
    loaderThread = new QThread(this);
    imageLoader = new ImageLoader(filePath);

    imageLoader->moveToThread(loaderThread);

    // �����źźͲ�
    connect(loaderThread, &QThread::started, imageLoader, &ImageLoader::loadImage);
    connect(imageLoader, &ImageLoader::imageLoaded, this, &MainWindow::onImageLoaded);
    connect(imageLoader, &ImageLoader::loadProgress, this, &MainWindow::onLoadProgress);
    connect(imageLoader, &ImageLoader::loadError, this, &MainWindow::onLoadError);

    // �߳̽������Զ�ɾ��
    connect(loaderThread, &QThread::finished, loaderThread, &QObject::deleteLater);
    connect(loaderThread, &QThread::finished, imageLoader, &QObject::deleteLater);

    // �����߳�
    loaderThread->start();
}

void MainWindow::onImageLoaded(const QPixmap& pixmap, const QString& filePath)
{
    if (filePath != currentImagePath) {
        return; // ������صĲ��ǵ�ǰ�����ͼ�������
    }

    showImage(pixmap);
    updateImageInfo(filePath);
    statusLabel->setText("Loaded: " + QFileInfo(filePath).fileName());
    progressBar->setVisible(false);

    // �����̺߳ͼ�����
    if (loaderThread) {
        loaderThread->quit();
        loaderThread->wait();
        loaderThread = nullptr;
        imageLoader = nullptr;
    }
}

void MainWindow::onLoadProgress(int percent)
{
    progressBar->setValue(percent);
}

void MainWindow::onLoadError(const QString& error)
{
    statusLabel->setText(error);
    progressBar->setVisible(false);

    // �����̺߳ͼ�����
    if (loaderThread) {
        loaderThread->quit();
        loaderThread->wait();
        loaderThread = nullptr;
        imageLoader = nullptr;
    }
}

void MainWindow::showImage(const QPixmap& pixmap)
{
    imageViewer->setPixmap(pixmap);
}

void MainWindow::updateImageInfo(const QString& filePath)
{
    infoTree->clear();

    QFileInfo fileInfo(filePath);

    // �ļ���Ϣ
    QTreeWidgetItem* fileInfoItem = new QTreeWidgetItem(infoTree);
    fileInfoItem->setText(0, "File Information");

    QTreeWidgetItem* fileNameItem = new QTreeWidgetItem(fileInfoItem);
    fileNameItem->setText(0, "File Name: " + fileInfo.fileName());

    QTreeWidgetItem* pathItem = new QTreeWidgetItem(fileInfoItem);
    pathItem->setText(0, "Path: " + fileInfo.absoluteFilePath());

    QTreeWidgetItem* sizeItem = new QTreeWidgetItem(fileInfoItem);
    sizeItem->setText(0, "Size: " + QString::number(fileInfo.size() / 1024.0, 'f', 2) + " KB");

    QTreeWidgetItem* modifiedItem = new QTreeWidgetItem(fileInfoItem);
    modifiedItem->setText(0, "Modified: " + fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));

    fileInfoItem->setExpanded(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
    else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (const QUrl& url : urlList) {
            QString filePath = url.toLocalFile();
            QFileInfo fileInfo(filePath);

            QString extension = fileInfo.suffix().toLower();
            if (extension == "png" || extension == "jpg" || extension == "jpeg" ||
                extension == "bmp" || extension == "gif" || extension == "tiff" ||
                extension == "tif" || extension == "webp") {
                loadImage(filePath);
                break; // ֻ���ص�һ����Ч��ͼ���ļ�
            }
        }
    }

    event->acceptProposedAction();
}