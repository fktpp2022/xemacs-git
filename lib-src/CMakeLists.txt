set(LIB_SRC_UTILITIES
  b2m
  ebrowse
  emacsclient
  hexl
  movemail
  profile
  rcs2log
  rcs-checkin
  update-game-score
  vcdiff
)

set(LIB_SRC_INTERNAL_UTILITIES
  cvtmail
  digest-doc
  fakemail
  mailfmt
  make-docfile
  ntlib
  sorted-doc
  timer
)

set(LIB_SRC_BUILT_UTILITIES "")
set(LIB_SRC_BUILT_INTERNAL_UTILITIES "")

foreach(util ${LIB_SRC_UTILITIES} ${LIB_SRC_INTERNAL_UTILITIES})
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${util}.c")
    add_executable(${util} ${util}.c)
    target_include_directories(${util} PRIVATE
      "${CMAKE_SOURCE_DIR}/src"
      "${CMAKE_BINARY_DIR}/src"
    )
    target_compile_definitions(${util} PRIVATE
      HAVE_CONFIG_H
    )
    list(FIND LIB_SRC_UTILITIES ${util} _util_index)
    if(_util_index GREATER -1)
      list(APPEND LIB_SRC_BUILT_UTILITIES ${util})
    else()
      list(APPEND LIB_SRC_BUILT_INTERNAL_UTILITIES ${util})
    endif()
  endif()
endforeach()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gnuserv.c")
  add_executable(gnuserv gnuserv.c)
  target_include_directories(gnuserv PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(gnuserv PRIVATE
    HAVE_CONFIG_H
  )
  
  if(HAVE_X_WINDOWS)
    target_link_libraries(gnuserv PRIVATE ${X11_LIBRARIES})
  endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gnudoit.c")
  add_executable(gnudoit gnudoit.c)
  target_include_directories(gnudoit PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(gnudoit PRIVATE
    HAVE_CONFIG_H
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/etags.c")
  add_executable(etags etags.c)
  target_include_directories(etags PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(etags PRIVATE
    HAVE_CONFIG_H
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ctags.c")
  add_executable(ctags ctags.c)
  target_include_directories(ctags PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(ctags PRIVATE
    HAVE_CONFIG_H
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test-distrib.c")
  add_executable(test-distrib test-distrib.c)
  target_include_directories(test-distrib PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(test-distrib PRIVATE
    HAVE_CONFIG_H
  )
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/make-docfile.c")
  target_link_libraries(make-docfile PRIVATE m)
endif()

if(LIB_SRC_BUILT_UTILITIES)
  install(TARGETS ${LIB_SRC_BUILT_UTILITIES}
    RUNTIME DESTINATION ${XEMACS_ARCHLIBDIR}
    COMPONENT Runtime
  )
endif()

if(TARGET gnuserv)
  install(TARGETS gnuserv
    RUNTIME DESTINATION ${XEMACS_BINDIR}
    COMPONENT Runtime
  )
endif()

if(TARGET etags)
  install(TARGETS etags
    RUNTIME DESTINATION ${XEMACS_BINDIR}
    COMPONENT Runtime
  )
endif()

if(TARGET ctags)
  install(TARGETS ctags
    RUNTIME DESTINATION ${XEMACS_BINDIR}
    COMPONENT Runtime
  )
endif()

if(LIB_SRC_BUILT_INTERNAL_UTILITIES)
  install(TARGETS ${LIB_SRC_BUILT_INTERNAL_UTILITIES}
    RUNTIME DESTINATION ${XEMACS_ARCHLIBDIR}
    COMPONENT Runtime
  )
endif()
