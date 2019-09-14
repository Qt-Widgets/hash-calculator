#include "widget.h"
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QFont>
#include <QMessageBox>
#include <QTranslator>
#ifndef Q_OS_WINDOWS
#include <QIcon>
#endif

int main(int argc, char *argv[]) {
    // Force Qt to use desktop OpenGL to avoid depending on ANGLE libraries.
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("Hash Calculator"));
#ifndef Q_OS_WINDOWS
    QCoreApplication::setApplicationVersion(QLatin1String("1.0.0"));
#endif
    QCoreApplication::setOrganizationName(QLatin1String("wangwenx190"));
    QCoreApplication::setOrganizationDomain(
        QLatin1String("wangwenx190.github.io"));
    QFont font;
    font.setPointSize(12);
    QGuiApplication::setFont(font);
    QTranslator translator;
    if (translator.load(QLocale(), QLatin1String("hashcalculator"),
                        QLatin1String("_"), QLatin1String(":/i18n"))) {
        QCoreApplication::installTranslator(&translator);
    }
    QCommandLineParser commandLineParser;
    commandLineParser.setApplicationDescription(QCoreApplication::translate(
        "main", "A simple tool to compute hash value for files."));
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
    const QCommandLineOption hashFileOption(
        QLatin1String("hash-file"),
        QCoreApplication::translate(
            "main", "Verify all files listed in the given <hash file>."),
        QCoreApplication::translate("main", "hash file"));
    commandLineParser.addOption(hashFileOption);
    const QCommandLineOption hashAlgorithmOption(
        QLatin1String("algorithm"),
        QCoreApplication::translate(
            "main",
            "Use the given <hash algorithm> to verify the given files."),
        QCoreApplication::translate("main", "hash algorithm"));
    commandLineParser.addOption(hashAlgorithmOption);
    commandLineParser.process(application);
    const QString hashFilePath =
        commandLineParser.value(hashFileOption).trimmed();
    const QString hashAlgorithm =
        commandLineParser.value(hashAlgorithmOption).trimmed();
    Widget widget;
#ifndef Q_OS_WINDOWS
    widget.setWindowIcon(QIcon(QLatin1String(":/hashcalculator.svg")));
#endif
    widget.show();
    if (!hashFilePath.isEmpty() && !hashAlgorithm.isEmpty()) {
        if (!QFileInfo::exists(hashFilePath) ||
            !QFileInfo(hashFilePath).isFile()) {
            QMessageBox::critical(
                &widget,
                QCoreApplication::translate("main", "Invalid hash file"),
                QCoreApplication::translate("main",
                                            "The hash file you choose does not "
                                            "exist or is not a file."));
        } else {
            widget.verifyHashFile(hashFilePath, hashAlgorithm, true);
        }
    }
    return QApplication::exec();
}
