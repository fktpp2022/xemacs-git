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

# On Darwin, dynamically loaded modules are linked as bundles whose undefined
# symbols (intern, signal_error, Qnil, defsubr, ...) are resolved against the
# running xemacs executable at load time.  This mirrors autoconf:
#   aclocal.m4 lines 246-248: xcldf='-bundle', xldf='-bundle -bundle_loader ../../src/xemacs'
# and the -rdynamic on the xemacs binary itself is applied in src/CMakeLists.txt
# (matching configure.ac line 3594: XE_APPEND(-rdynamic, start_flags)).
function(xemacs_apply_darwin_bundle_loader target)
  if(APPLE)
    target_link_options(${target} PRIVATE
      "-bundle_loader" "$<TARGET_FILE:xemacs>"
    )
    # xemacs must be built before modules link, since bundle_loader requires
    # the binary to exist on disk at link time (ld resolves it immediately).
    add_dependencies(${target} xemacs)
  endif()
endfunction()

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

  xemacs_apply_darwin_bundle_loader(base64)

  install(TARGETS base64
    LIBRARY DESTINATION ${INST_MODULEDIR}
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

  xemacs_apply_darwin_bundle_loader(zlib)

  install(TARGETS zlib
    LIBRARY DESTINATION ${INST_MODULEDIR}
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

  xemacs_apply_darwin_bundle_loader(ldap)

  install(TARGETS ldap
    LIBRARY DESTINATION ${INST_MODULEDIR}
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

  xemacs_apply_darwin_bundle_loader(postgresql)

  install(TARGETS postgresql
    LIBRARY DESTINATION ${INST_MODULEDIR}
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

  xemacs_apply_darwin_bundle_loader(canna)

  install(TARGETS canna
    LIBRARY DESTINATION ${INST_MODULEDIR}
    COMPONENT Libraries
  )
endif()

# Module autoloads (generated during build)
install(CODE "
  if(EXISTS \"${CMAKE_CURRENT_BINARY_DIR}/auto-autoloads.el\")
    file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${INST_MODULEDIR}\"
      TYPE FILE FILES \"${CMAKE_CURRENT_BINARY_DIR}/auto-autoloads.el\")
  endif()
  if(EXISTS \"${CMAKE_CURRENT_BINARY_DIR}/auto-autoloads.elc\")
    file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${INST_MODULEDIR}\"
      TYPE FILE FILES \"${CMAKE_CURRENT_BINARY_DIR}/auto-autoloads.elc\")
  endif()
" COMPONENT Runtime)
