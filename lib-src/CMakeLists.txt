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

# gnuslib is a shared support library for gnuserv and gnuclient
set(GNUSERV_COMMON_SOURCES gnuslib.c)
set(GNUSERV_COMMON_INCLUDES
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/src"
)
set(GNUSERV_COMMON_DEFS HAVE_CONFIG_H)
set(GNUSERV_COMMON_LIBS "")
if(HAVE_X_WINDOWS)
  list(APPEND GNUSERV_COMMON_LIBS ${X11_LIBRARIES})
endif()
if(HAVE_XAUTH)
  find_library(XAU_LIBRARY Xau)
  if(XAU_LIBRARY)
    list(APPEND GNUSERV_COMMON_LIBS ${XAU_LIBRARY})
  endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gnuserv.c")
  add_executable(gnuserv gnuserv.c ${GNUSERV_COMMON_SOURCES})
  target_include_directories(gnuserv PRIVATE ${GNUSERV_COMMON_INCLUDES})
  target_compile_definitions(gnuserv PRIVATE ${GNUSERV_COMMON_DEFS})
  target_link_libraries(gnuserv PRIVATE ${GNUSERV_COMMON_LIBS})
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gnuclient.c")
  add_executable(gnuclient gnuclient.c ${GNUSERV_COMMON_SOURCES})
  target_include_directories(gnuclient PRIVATE ${GNUSERV_COMMON_INCLUDES})
  target_compile_definitions(gnuclient PRIVATE ${GNUSERV_COMMON_DEFS})
  target_link_libraries(gnuclient PRIVATE ${GNUSERV_COMMON_LIBS})
endif()

# gnudoit is a shell script, installed separately

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
