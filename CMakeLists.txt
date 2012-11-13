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

#Release must be quiet, without debug output

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  message(STATUS "No build type selected, default is Debug")
endif(NOT CMAKE_BUILD_TYPE)

string(TOUPPER ${CMAKE_BUILD_TYPE} IS_CONFIG_RELEASE)

if(NOT IS_CONFIG_RELEASE STREQUAL "DEBUG")
  add_definitions(-DQT_NO_DEBUG_OUTPUT -DQT_NO_WARNING_OUTPUT)
endif()
