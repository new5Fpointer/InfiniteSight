#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("InfiniteSight");
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    return app.exec();
}