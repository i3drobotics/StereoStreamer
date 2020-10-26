QT += core gui widgets

DEFINES += WITH_BOOST

DEPENDPATH += "$$PWD\src"
INCLUDEPATH += "$$PWD\src"
INCLUDEPATH += "$$PWD\src\widget"
INCLUDEPATH += "$$PWD\src\image2string"
INCLUDEPATH += "$$PWD\src\stereostreamer"
INCLUDEPATH += "$$PWD\src\stereostreamer2"
INCLUDEPATH += "$$PWD\src\stereostreamersingle"
VPATH += $$PWD\src
VPATH += $$PWD\src\widget
VPATH += $$PWD\src\image2string
VPATH += $$PWD\src\stereostreamer
VPATH += $$PWD\src\stereostreamer2
VPATH += $$PWD\src\stereostreamersingle

SOURCES += \
        stereostreamer.cpp \
        stereostreamer2.cpp \
        stereostreamersingle.cpp \
        image2string.cpp \
        stereostreamerviewer.cpp \
        stereostreamersettings.cpp

HEADERS += \
        stereostreamer.h \
        stereostreamer2.h \
        stereostreamersingle.h \
        image2string.h \
        stereostreamerviewer.h \
        stereostreamersettings.h

FORMS += \
        stereostreamerviewer.ui \
        stereostreamersettings.ui
