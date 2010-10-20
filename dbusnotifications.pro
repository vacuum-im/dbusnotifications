include(config.inc)

#Project Configuration
TARGET              = dbusnotifications
TEMPLATE            = lib
CONFIG             += plugin
QT                  = core dbus gui
isEmpty(VACUUM_BUILD_PATH) {
    LIBS               += -L$$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR
    LIBS               += -l$${TARGET_UTILS}
} else {
    LIBS               += -L$${VACUUM_BUILD_PATH}/src/libs
    LIBS               += -l$${TARGET_UTILS}
}
isEmpty(VACUUM_SOURCE_PATH) {
    DEPENDPATH         += $$INSTALL_PREFIX/include/$$INSTALL_APP_DIR
    INCLUDEPATH        += $$INSTALL_PREFIX/include/$$INSTALL_APP_DIR
} else {
    DEPENDPATH         += $${VACUUM_SOURCE_PATH}/src
    INCLUDEPATH        += $${VACUUM_SOURCE_PATH}/src
}

#SVN Info
SVN_REVISION=$$system(svnversion -n -c .)
win32 {
  exists(svninfo.h):system(del svninfo.h)
  !isEmpty(SVN_REVISION):system(echo $${LITERAL_HASH}define SVN_REVISION \"$$SVN_REVISION\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
} else {
  exists(svninfo.h):system(rm -f svninfo.h)
  !isEmpty(SVN_REVISION):system(echo \\$${LITERAL_HASH}define SVN_REVISION \\\"$${SVN_REVISION}\\\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
}

#Translation
TRANSLATIONS        = ./translations/src/ru_RU/$${TARGET}.ts

#Install

win32 {
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR/plugins
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/resources
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/translations
} else:unix:!macx {
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR/$$INSTALL_APP_DIR/plugins
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/resources
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/doc/$$INSTALL_APP_DIR
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/translations
} else:macx {
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/PlugIns
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR/translations
}

target.path        = $$INSTALL_PLUGINS
resources.path     = $$INSTALL_RESOURCES
resources.files    = ./resources/*
documents.path     = $$INSTALL_DOCUMENTS
documents.files    = ./doc/*
translations.path  = $$INSTALL_TRANSLATIONS
translations.files = ./translations/bin/*

INSTALLS            = target documents translations

include(dbusnotifications.pri)
