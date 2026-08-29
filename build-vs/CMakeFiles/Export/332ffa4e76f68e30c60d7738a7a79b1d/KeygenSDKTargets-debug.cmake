#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "KeygenSDK::KeygenSDK" for configuration "Debug"
set_property(TARGET KeygenSDK::KeygenSDK APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(KeygenSDK::KeygenSDK PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/KeygenSDK.lib"
  )

list(APPEND _cmake_import_check_targets KeygenSDK::KeygenSDK )
list(APPEND _cmake_import_check_files_for_KeygenSDK::KeygenSDK "${_IMPORT_PREFIX}/lib/KeygenSDK.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
