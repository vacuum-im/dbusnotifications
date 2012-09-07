include(qmake/debug.inc)
include(qmake/config.inc)

#Project configuration
TARGET              = dbusnotifications
QT                  = core gui dbus xml
include(dbusnotifications.pri)

#Default progect configuration
include(qmake/plugin.inc)

#Translation
TRANS_SOURCE_ROOT   = .
include(translations/languages.inc)
