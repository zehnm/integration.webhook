TEMPLATE = app

CONFIG += qt warn_on depend_includepath testcase
QT     += core testlib
QT     -= gui

TARGET = tst_jsonpath

INCDIR = $$PWD/../../src
INCLUDEPATH += $$INCDIR

HEADERS += \
    $$INCDIR/jsonpath.h

SOURCES += \
    tst_jsonpath.cpp \
    $$INCDIR/jsonpath.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

DISTFILES += \
    JsonPath.json \
    myStrom-bulb-setcolor-response.json \
    myStrom-bulb-turnon-response.json \
    myStrom-switch-report-response.json \
    myStrom-switch-temp-response.json \
    myStrom-switch-toggle-response.json

RESOURCES += \
    testfiles.qrc
