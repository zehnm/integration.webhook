TEMPLATE = subdirs

SUBDIRS += src

win32|if(unix:!cross_compile): SUBDIRS += tests

DISTFILES += \
    dependencies.cfg \
    setup-example.json \
    README.md \
    version.txt.in \
    doc/mystrom_switch.json \
    doc/shelly_2.5_roller_shutter.json
