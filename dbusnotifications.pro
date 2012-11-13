include(qmake/debug.inc)
include(qmake/config.inc)

#Project configuration
TARGET              = dbusnotifications
QT                  = core gui dbus xml
include(dbusnotifications.pri)

#Default progect configuration
include(qmake/plugin.inc)
#Release must be quiet, without debug output
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

#Translation
TRANS_SOURCE_ROOT   = .
include(translations/languages.inc)
