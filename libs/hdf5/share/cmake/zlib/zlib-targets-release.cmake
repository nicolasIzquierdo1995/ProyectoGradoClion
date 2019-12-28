#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "zlib-static" for configuration "Release"
set_property(TARGET zlib-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zlib-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libz.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS zlib-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_zlib-static "${_IMPORT_PREFIX}/lib/libz.a" )

# Import target "zlib-shared" for configuration "Release"
set_property(TARGET zlib-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(zlib-shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libz.so.1.2.11"
  IMPORTED_SONAME_RELEASE "libz.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS zlib-shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_zlib-shared "${_IMPORT_PREFIX}/lib/libz.so.1.2.11" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
