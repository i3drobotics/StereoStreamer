QT += core gui widgets

DEFINES += WITH_BOOST

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
        stereostreamerviewer.cpp \
        stereostreamersettings.cpp

HEADERS += \
        stereostreamer.h \
        image2string.h \
        stereostreamerviewer.h \
        stereostreamersettings.h

FORMS += \
        stereostreamerviewer.ui \
        stereostreamersettings.ui
