TEMPLATE = app
TARGET = $$qtLibraryTarget(hashcalculator)
DESTDIR = bin
QT += gui widgets
HEADERS += $$files(*.h)
SOURCES += $$files(*.cpp)
FORMS += $$files(*.ui)
win32 {
    RC_FILE = hashcalculator.rc
    shared: CONFIG += windeployqt
}
unix: RESOURCES += hashcalculator.qrc
static {
    QTPLUGIN.iconengines = -
    QTPLUGIN.imageformats = -
}
