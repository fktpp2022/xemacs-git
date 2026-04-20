find_path(WNN_INCLUDE_DIR
  NAMES wnn/wnnlib.h wnnlib.h
  PATHS /usr/include /usr/local/include
  DOC "Wnn include directory"
)

find_library(WNN_LIBRARY
  NAMES wnn
  PATHS /usr/lib /usr/local/lib
  DOC "Wnn library"
)

find_library(WNN6_LIBRARY
  NAMES wnn6
  PATHS /usr/lib /usr/local/lib
  DOC "Wnn6 library"
)

include(FindPackageHandleStandardArgs)

if(WNN6_LIBRARY)
  find_package_handle_standard_args(Wnn6
    DEFAULT_MSG
    WNN6_LIBRARY WNN_INCLUDE_DIR
  )
  
  if(WNN6_FOUND)
    set(WNN6_LIBRARIES ${WNN6_LIBRARY})
    set(WNN6_INCLUDE_DIRS ${WNN_INCLUDE_DIR})
  endif()
endif()

find_package_handle_standard_args(Wnn
  DEFAULT_MSG
  WNN_LIBRARY WNN_INCLUDE_DIR
)

if(WNN_FOUND)
  set(WNN_LIBRARIES ${WNN_LIBRARY})
  set(WNN_INCLUDE_DIRS ${WNN_INCLUDE_DIR})
  
  if(NOT TARGET Wnn::Wnn)
    add_library(Wnn::Wnn UNKNOWN IMPORTED)
    set_target_properties(Wnn::Wnn PROPERTIES
      IMPORTED_LOCATION "${WNN_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${WNN_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(WNN_INCLUDE_DIR WNN_LIBRARY WNN6_LIBRARY)
