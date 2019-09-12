#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    // Force Qt to use desktop OpenGL to avoid depending on ANGLE libraries.
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("Hash Calculator"));
#ifndef Q_OS_WINDOWS
    QCoreApplication::setApplicationVersion(QLatin1String("1.0.0"));
#endif
    QCoreApplication::setOrganizationName(QLatin1String("wangwenx190"));
    QCoreApplication::setOrganizationDomain(
        QLatin1String("wangwenx190.github.io"));
    Widget widget;
    widget.show();
    return QApplication::exec();
}
