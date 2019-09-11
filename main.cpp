#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication application(argc, argv);
    Widget widget;
    widget.show();
    return QApplication::exec();
}
