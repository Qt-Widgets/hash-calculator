TEMPLATE = app
TARGET = $$qtLibraryTarget(hashcalculator)
DESTDIR = bin
QT += gui widgets concurrent
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
    shared {
        WINDEPLOYQT_OPTIONS = \
            --force --no-translations --no-system-d3d-compiler \
            --no-compiler-runtime --no-angle --no-opengl-sw
        CONFIG += windeployqt
    }
}
unix: RESOURCES += hashcalculator.qrc
static {
    QTPLUGIN.iconengines = -
    QTPLUGIN.imageformats = -
}
