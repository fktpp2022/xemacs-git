set(MODULES
  base64
  zlib
)

if(HAVE_LDAP)
  list(APPEND MODULES ldap)
endif()

if(HAVE_POSTGRESQL)
  list(APPEND MODULES postgresql)
endif()

if(HAVE_CANNA)
  list(APPEND MODULES canna)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/base64/base64.c")
  add_library(base64 MODULE
    base64/base64.c
  )
  
  target_include_directories(base64 PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(base64 PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  set_target_properties(base64 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "base64"
  )
  
  install(TARGETS base64
    LIBRARY DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/zlib/zlib.c" AND HAVE_ZLIB)
  add_library(zlib MODULE
    zlib/zlib.c
  )
  
  target_include_directories(zlib PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(zlib PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  target_link_libraries(zlib PRIVATE
    ${ZLIB_LIBRARIES}
  )
  
  set_target_properties(zlib PROPERTIES
    PREFIX ""
    OUTPUT_NAME "zlib"
  )
  
  install(TARGETS zlib
    LIBRARY DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ldap/eldap.c" AND HAVE_LDAP)
  add_library(ldap MODULE
    ldap/eldap.c
  )
  
  target_include_directories(ldap PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(ldap PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  target_link_libraries(ldap PRIVATE
    ${LDAP_LIBRARIES}
  )
  
  set_target_properties(ldap PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ldap"
  )
  
  install(TARGETS ldap
    LIBRARY DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/postgresql/postgresql.c" AND HAVE_POSTGRESQL)
  add_library(postgresql MODULE
    postgresql/postgresql.c
  )
  
  target_include_directories(postgresql PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(postgresql PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  target_link_libraries(postgresql PRIVATE
    ${PostgreSQL_LIBRARIES}
  )
  
  set_target_properties(postgresql PROPERTIES
    PREFIX ""
    OUTPUT_NAME "postgresql"
  )
  
  install(TARGETS postgresql
    LIBRARY DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/canna/canna_api.c" AND HAVE_CANNA)
  add_library(canna MODULE
    canna/canna_api.c
  )
  
  target_include_directories(canna PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(canna PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  target_link_libraries(canna PRIVATE
    ${CANNA_LIBRARIES}
  )
  
  set_target_properties(canna PROPERTIES
    PREFIX ""
    OUTPUT_NAME "canna"
  )
  
  install(TARGETS canna
    LIBRARY DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()

install(FILES
  auto-autoloads.el
  DESTINATION ${XEMACS_MODULEDIR}
  COMPONENT Libraries
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/auto-autoloads.elc")
  install(FILES
    auto-autoloads.elc
    DESTINATION ${XEMACS_MODULEDIR}
    COMPONENT Libraries
  )
endif()
