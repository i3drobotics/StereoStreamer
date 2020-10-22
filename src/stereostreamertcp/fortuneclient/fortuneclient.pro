QT += network widgets
requires(qtConfig(combobox))

# Application name
TARGET = FortuneClient

# Application type
TEMPLATE = app vcapp

CONFIG += warn_on
CONFIG += doc
CONFIG -= debug_and_release
CONFIG -= debug_and_release_target

HEADERS       = client.h
SOURCES       = client.cpp \
                main.cpp

# For building in a release and debug in seperate folders
CONFIG(debug, debug|release) { #debug
    DESTDIR = $$TARGET/debug
    OBJECTS_DIR = $$TARGET/.obj_debug
    MOC_DIR     = $$TARGET/.moc_debug
}else {
    DESTDIR = $$TARGET/release
    OBJECTS_DIR = $$TARGET/.obj
    MOC_DIR     = $$TARGET/.moc
}
RCC_DIR = $$TARGET/.qrc
UI_DIR = $$TARGET/.ui

# Define target
isEmpty(TARGET_EXT) {
    win32 {
        TARGET_CUSTOM_EXT = .exe
    }
    macx {
        TARGET_CUSTOM_EXT = .app
    }
} else {
    TARGET_CUSTOM_EXT = $${TARGET_EXT}
}

# Get full output folder path
DEPLOY_FOLDER = $$OUT_PWD/$$DESTDIR

# Deploy qt
win32 {
    DEPLOY_COMMAND = windeployqt
}
macx {
    DEPLOY_COMMAND = macdeployqt
}

DEPLOY_TARGET = $$shell_quote($$shell_path($${DEPLOY_FOLDER}/$${TARGET}$${TARGET_CUSTOM_EXT}))
QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
