TEMPLATE = subdirs

SUBDIRS += src

CONFIG(debug, debug|release) {
    SUBDIRS += tests
}

DISTFILES += \
    dependencies.cfg \
    setup-example.json \
    README.md
