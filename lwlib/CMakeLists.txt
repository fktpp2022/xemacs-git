set(LWLIB_CORE_SOURCES
  lwlib.c
  lwlib-utils.c
  lwlib-colors.c
  lwlib-fonts.c
)

set(LWLIB_LUCID_SOURCES
  lwlib-Xlw.c
  xlwmenu.c
  xlwscrollbar.c
  xlwtabs.c
  xlwgcs.c
)

set(LWLIB_MOTIF_SOURCES
  lwlib-Xm.c
)

set(LWLIB_ATHENA_SOURCES
  lwlib-Xaw.c
  xlwradio.c
  xlwcheckbox.c
  xlwgauge.c
)

set(LWLIB_SOURCES ${LWLIB_CORE_SOURCES})

if(HAVE_LUCID_WIDGETS OR NOT HAVE_MOTIF_WIDGETS AND NOT HAVE_ATHENA_WIDGETS AND NOT HAVE_GTK_WIDGETS)
  list(APPEND LWLIB_SOURCES ${LWLIB_LUCID_SOURCES})
endif()

if(HAVE_MOTIF_WIDGETS)
  list(APPEND LWLIB_SOURCES ${LWLIB_MOTIF_SOURCES})
endif()

if(HAVE_ATHENA_WIDGETS)
  list(APPEND LWLIB_SOURCES ${LWLIB_ATHENA_SOURCES})
endif()

set(LWLIB_HEADERS
  lwlib.h
  lwlib-internal.h
  lwlib-utils.h
  lwlib-colors.h
  lwlib-fonts.h
  lwlib-Xlw.h
  lwlib-Xm.h
  lwlib-Xaw.h
  xlwmenu.h
  xlwmenuP.h
  xlwscrollbar.h
  xlwscrollbarP.h
  xlwtabs.h
  xlwtabsP.h
  xlwradio.h
  xlwradioP.h
  xlwcheckbox.h
  xlwcheckboxP.h
  xlwgauge.h
  xlwgaugeP.h
  xlwgcs.h
)

add_library(lw STATIC ${LWLIB_SOURCES} ${LWLIB_HEADERS})

target_include_directories(lw BEFORE PRIVATE
  "${CMAKE_BINARY_DIR}/lwlib"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}"
  "${CMAKE_SOURCE_DIR}"
)

if(HAVE_XFT)
  target_include_directories(lw PRIVATE ${XFT_INCLUDE_DIRS})
endif()

target_compile_definitions(lw PRIVATE
  HAVE_CONFIG_H
  emacs
)

if(HAVE_X_WINDOWS)
  target_link_libraries(lw PRIVATE ${X11_LIBRARIES})
endif()

if(HAVE_XT)
  target_link_libraries(lw PRIVATE ${X11_Xt_LIB})
endif()

if(HAVE_XMU)
  target_link_libraries(lw PRIVATE ${X11_Xmu_LIB})
endif()

if(HAVE_XPM)
  if(X11_Xpm_LIB)
    target_link_libraries(lw PRIVATE ${X11_Xpm_LIB})
  endif()
endif()

if(HAVE_XFT)
  target_link_libraries(lw PRIVATE ${XFT_LIBRARIES})
endif()

install(TARGETS lw
  ARCHIVE DESTINATION ${XEMACS_LIBDIR}
  COMPONENT Development
)

install(FILES ${LWLIB_HEADERS}
  DESTINATION ${XEMACS_LIBDIR}/include/lwlib
  COMPONENT Development
)
