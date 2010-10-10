include(config.inc)

#Project Configuration
TARGET              = dbusnotifications
TEMPLATE            = lib
CONFIG             += plugin
QT                  = core dbus gui
LIBS               += -L$${VACUUM_BUILD_PATH}/src/libs
LIBS               += -l$${TARGET_UTILS}
DEPENDPATH         += $${VACUUM_SOURCE_PATH}/src
INCLUDEPATH        += $${VACUUM_SOURCE_PATH}/src

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

include(dbusnotifications.pri)
