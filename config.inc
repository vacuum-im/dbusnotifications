#Path to biulded Vacuum src direcroty
isEmpty(VACUUM_SOURCE_PATH) {
  VACUUM_SOURCE_PATH=$$(VACUUM_SOURCE_PATH)
  isEmpty(VACUUM_SOURCE_PATH) {
    VACUUM_SOURCE_PATH               = ../../vacuum-im
    message( "No custom source path. set to: $$VACUUM_SOURCE_PATH" )
  }
}

isEmpty(VACUUM_BUILD_PATH) {
  VACUUM_BUILD_PATH=$$(VACUUM_BUILD_PATH)
  isEmpty(VACUUM_BUILD_PATH) {
    VACUUM_BUILD_PATH               = ../../vacuum-build
    message( "No custom build path. set to: $$VACUUM_BUILD_PATH" )
  }
}

#Debug Features
CONFIG(debug, debug|release) 
{
  #Path to copy plugin on build
  win32:DLLDESTDIR       = $${VACUUM_BUILD_PATH}/plugins
  unix:DESTDIR           = $${VACUUM_BUILD_PATH}/plugins

  #Path from vacumm to plugin resources
  #DEFINES               += RESOURCES_DIR='\\\"../trivialplugin/resources\\\"'
}

#Should be included from SDK
TARGET_UTILS             = vacuumutils
unix:VERSION_UTILS       = 1.2.0
