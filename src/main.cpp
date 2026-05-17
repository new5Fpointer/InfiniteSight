#include <QApplication>
#include <QSurfaceFormat>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setApplicationName("InfiniteSight");
    app.setOrganizationName("InfiniteSight");

    MainWindow window;
    window.show();

    return app.exec();
}
