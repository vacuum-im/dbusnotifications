cmake_minimum_required(VERSION 2.8)

project(dbusnotifications-plugin)
set(PLUGIN_NAME dbusnotifications)

set(SOURCES dbuspopuphandler.cpp)
set(HEADERS dbuspopuphandler.h)
set(MOC_HEADERS dbuspopuphandler.h)

if (NOT (UNIX AND NOT APPLE))
  message(FATAL_ERROR "Supported only on *nix")
endif (NOT (UNIX AND NOT APPLE)) 

set(QT_USE_QTDBUS ON)
set(QT_USE_QTWEBKIT OFF)

include_directories("${CMAKE_SOURCE_DIR}")

include("${CMAKE_SOURCE_DIR}/cmake/config.cmake")
include("${CMAKE_SOURCE_DIR}/translations/languages.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/plugin.cmake")

install(DIRECTORY "resources/menuicons" DESTINATION "${INSTALL_RESOURCES}")

