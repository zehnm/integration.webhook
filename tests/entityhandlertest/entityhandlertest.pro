TEMPLATE = app

CONFIG += qt warn_on depend_includepath testcase
QT     += core network testlib
QT     -= gui

TARGET = tst_entityhandler

INTG_LIB_PATH = $$(YIO_SRC)
isEmpty(INTG_LIB_PATH) {
    INTG_LIB_PATH = $$clean_path($$PWD/../../../integrations.library)
} else {
    INTG_LIB_PATH = $$(YIO_SRC)/integrations.library
}

! include($$INTG_LIB_PATH/yio-plugin-lib.pri) {
    error( "Cannot find the yio-plugin-lib.pri file" )
}

INCDIR = $$PWD/../../src
INCLUDEPATH += $$INCDIR

HEADERS += \
    entityhandlerimpl.h \
    $$INCDIR/entityhandler.h \
    $$INCDIR/httpmethod.h \
    $$INCDIR/jsonpath.h \
    $$INCDIR/webhookcommand.h \
    $$INCDIR/webhookentity.h \
    $$INCDIR/webhookrequest.h

SOURCES += \
    tst_entityhandler.cpp \
    $$INCDIR/entityhandler.cpp \
    $$INCDIR/jsonpath.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"
