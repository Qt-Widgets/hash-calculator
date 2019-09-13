TEMPLATE = app
TARGET = $$qtLibraryTarget(hashcalculator)
DESTDIR = bin
QT += gui widgets
win32: QT += winextras
HEADERS += $$files(*.h)
SOURCES += $$files(*.cpp)
FORMS += $$files(*.ui)
TRANSLATIONS += \
    hashcalculator_en.ts \
    hashcalculator_zh_CN.ts
CONFIG += lrelease embed_translations
win32 {
    RC_FILE = hashcalculator.rc
    shared: CONFIG += windeployqt
}
unix: RESOURCES += hashcalculator.qrc
static {
    QTPLUGIN.iconengines = -
    QTPLUGIN.imageformats = -
}
