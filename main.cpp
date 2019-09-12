#include "widget.h"
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#ifndef Q_OS_WINDOWS
#include <QIcon>
#endif

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
    QCommandLineParser commandLineParser;
    commandLineParser.setApplicationDescription(QCoreApplication::translate(
        "main", "A simple tool to compute hash for files."));
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
    commandLineParser.addPositionalArgument(
        QLatin1String("algorithms"),
        QCoreApplication::translate("main", "Hash algorithms."));
    commandLineParser.addPositionalArgument(
        QLatin1String("files"),
        QCoreApplication::translate("main", "Files to be computed."));
    Widget widget;
#ifndef Q_OS_WINDOWS
    widget.setWindowIcon(QIcon(QLatin1String(":/hashcalculator.svg")));
#endif
    widget.show();
    return QApplication::exec();
}
