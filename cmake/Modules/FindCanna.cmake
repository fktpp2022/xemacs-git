find_path(CANNA_INCLUDE_DIR
  NAMES canna/ccommon.h ccommon.h
  PATHS /usr/include /usr/local/include
  DOC "Canna include directory"
)

find_library(CANNA_LIBRARY
  NAMES canna
  PATHS /usr/lib /usr/local/lib
  DOC "Canna library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Canna
  DEFAULT_MSG
  CANNA_LIBRARY CANNA_INCLUDE_DIR
)

if(CANNA_FOUND)
  set(CANNA_LIBRARIES ${CANNA_LIBRARY})
  set(CANNA_INCLUDE_DIRS ${CANNA_INCLUDE_DIR})
  
  if(NOT TARGET Canna::Canna)
    add_library(Canna::Canna UNKNOWN IMPORTED)
    set_target_properties(Canna::Canna PROPERTIES
      IMPORTED_LOCATION "${CANNA_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${CANNA_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(CANNA_INCLUDE_DIR CANNA_LIBRARY)
