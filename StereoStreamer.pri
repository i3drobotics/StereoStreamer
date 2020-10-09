QT += core gui widgets

DEPENDPATH += "$$PWD\src"
INCLUDEPATH += "$$PWD\src"
INCLUDEPATH += "$$PWD\src\widget"
INCLUDEPATH += "$$PWD\src\stereostreamer"
VPATH += $$PWD\src
VPATH += $$PWD\src\widget
VPATH += $$PWD\src\stereostreamer

SOURCES += \
        stereostreamer.cpp \
        image2string.cpp \
        stereostreamerviewer.cpp

HEADERS += \
        stereostreamer.h \
        image2string.h \
        stereostreamerviewer.h

FORMS += stereostreamerviewer.ui \
    $$PWD/src/widget/stereostreamersettings.ui
