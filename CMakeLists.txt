cmake_minimum_required(VERSION 3.16)

project(XEmacs
  VERSION 21.5.36
  DESCRIPTION "XEmacs - The next generation Emacs editor"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

include(CTest)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CMakePackageConfigHelpers)

include("${CMAKE_SOURCE_DIR}/cmake/XEmacsOptions.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/XEmacsOS.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/XEmacsConfigure.cmake")

set(EMACS_MAJOR_VERSION 21)
set(EMACS_MINOR_VERSION 5)
set(EMACS_BETA_VERSION 36)
set(XEMACS_CODENAME "leeks")
set(XEMACS_RELEASE_DATE "2025-06-14")

if(EMACS_IS_BETA)
  set(EMACS_VERSION "${EMACS_MAJOR_VERSION}.${EMACS_MINOR_VERSION}-b${EMACS_BETA_VERSION}")
else()
  set(EMACS_VERSION "${EMACS_MAJOR_VERSION}.${EMACS_MINOR_VERSION}.${EMACS_BETA_VERSION}")
endif()

set(PROGNAME "xemacs")
set(SHEBANG_PROGNAME "xemacs-script")
set(EMACS_DUMP_FILE_NAME "xemacs.dmp")

set(EMACS_CONFIGURATION "${CMAKE_SYSTEM_PROCESSOR}-unknown-${CMAKE_SYSTEM_NAME_LOWER}")
string(TOLOWER "${CMAKE_SYSTEM_NAME}" CMAKE_SYSTEM_NAME_LOWER)

set(inststaticdir "${PROGNAME}")
set(instvardir "${PROGNAME}-${EMACS_VERSION}")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()

set(XEMACS_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(XEMACS_EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(XEMACS_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
set(XEMACS_DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share")
set(XEMACS_DATADIR "${XEMACS_DATAROOTDIR}")
set(XEMACS_STATEDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(XEMACS_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(XEMACS_MANDIR "${XEMACS_DATAROOTDIR}/man")
set(XEMACS_INFODIR "${XEMACS_DATADIR}/${instvardir}/info")

set(XEMACS_LISPDIR "${XEMACS_DATADIR}/${instvardir}/lisp")
set(XEMACS_SITELISPDIR "${XEMACS_DATADIR}/${inststaticdir}/site-lisp")
set(XEMACS_ETCDIR "${XEMACS_DATADIR}/${instvardir}/etc")
set(XEMACS_ARCHLIBDIR "${XEMACS_LIBDIR}/${instvardir}/${EMACS_CONFIGURATION}")
set(XEMACS_DOCDIR "${XEMACS_ARCHLIBDIR}")
set(XEMACS_MODULEDIR "${XEMACS_ARCHLIBDIR}/modules")
set(XEMACS_SITEMODULEDIR "${XEMACS_LIBDIR}/${inststaticdir}/site-modules")

set(XEMACS_LATE_PACKAGES "${XEMACS_DATADIR}/${PROGNAME}")

message(STATUS "XEmacs ${EMACS_VERSION} \"${XEMACS_CODENAME}\"")
message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Configuration: ${EMACS_CONFIGURATION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

configure_file(
  "${CMAKE_SOURCE_DIR}/src/config.h.in.cmake"
  "${CMAKE_BINARY_DIR}/src/config.h"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/src/paths.h.in.cmake"
  "${CMAKE_BINARY_DIR}/src/paths.h"
  @ONLY
)

configure_file(
  "${CMAKE_SOURCE_DIR}/lwlib/config.h.in.cmake"
  "${CMAKE_BINARY_DIR}/lwlib/config.h"
  @ONLY
)

set(GENERATED_HEADERS
  "${CMAKE_BINARY_DIR}/src/config.h"
  "${CMAKE_BINARY_DIR}/src/paths.h"
)

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_BINARY_DIR}/src"
  "${CMAKE_SOURCE_DIR}/lib-src"
  "${CMAKE_BINARY_DIR}/lwlib"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(lib-src)
add_subdirectory(lwlib)
add_subdirectory(src)

if(XEMACS_WITH_MODULES)
  add_subdirectory(modules)
endif()

include("${CMAKE_SOURCE_DIR}/cmake/XEmacsCPack.cmake")

configure_file(
  "${CMAKE_SOURCE_DIR}/cmake/Installation.txt.in"
  "${CMAKE_BINARY_DIR}/Installation.txt"
  @ONLY
)

if(XEMACS_INSTALL_LISP)
  if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/lisp")
    install(DIRECTORY lisp/
      DESTINATION ${XEMACS_LISPDIR}
      COMPONENT Lisp
      FILES_MATCHING
        PATTERN "*.el"
        PATTERN "*.elc"
        PATTERN "*.el.gz"
        PATTERN "*.elc.gz"
      PATTERN "CVS" EXCLUDE
      PATTERN ".git" EXCLUDE
      PATTERN ".svn" EXCLUDE
    )
  endif()
endif()

if(XEMACS_INSTALL_ETC)
  if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/etc")
    install(DIRECTORY etc/
      DESTINATION ${XEMACS_ETCDIR}
      COMPONENT Etc
      PATTERN "CVS" EXCLUDE
      PATTERN ".git" EXCLUDE
      PATTERN ".svn" EXCLUDE
    )
  endif()
endif()

if(XEMACS_INSTALL_INFO)
  if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/info")
    install(DIRECTORY info/
      DESTINATION ${XEMACS_INFODIR}
      COMPONENT Info
      FILES_MATCHING
        PATTERN "*.info"
        PATTERN "*.info-[0-9]"
        PATTERN "*.info-[0-9][0-9]"
      PATTERN "CVS" EXCLUDE
      PATTERN ".git" EXCLUDE
      PATTERN ".svn" EXCLUDE
    )
  endif()
endif()

if(XEMACS_INSTALL_DOC)
  if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/man")
    install(DIRECTORY man/
      DESTINATION ${XEMACS_MANDIR}/man1
      COMPONENT Documentation
      FILES_MATCHING
        PATTERN "*.1"
      PATTERN "CVS" EXCLUDE
      PATTERN ".git" EXCLUDE
      PATTERN ".svn" EXCLUDE
    )
  endif()
  
  install(FILES
    "${CMAKE_BINARY_DIR}/Installation.txt"
    DESTINATION ${XEMACS_DOCDIR}
    COMPONENT Documentation
    RENAME "Installation"
  )
  
  if(EXISTS "${CMAKE_SOURCE_DIR}/INSTALL")
    install(FILES
      "${CMAKE_SOURCE_DIR}/INSTALL"
      DESTINATION ${XEMACS_DOCDIR}
      COMPONENT Documentation
    )
  endif()
  
  if(EXISTS "${CMAKE_SOURCE_DIR}/README")
    install(FILES
      "${CMAKE_SOURCE_DIR}/README"
      DESTINATION ${XEMACS_DOCDIR}
      COMPONENT Documentation
    )
  endif()
  
  if(EXISTS "${CMAKE_SOURCE_DIR}/COPYING")
    install(FILES
      "${CMAKE_SOURCE_DIR}/COPYING"
      DESTINATION ${XEMACS_DOCDIR}
      COMPONENT Documentation
    )
  endif()
  
  if(EXISTS "${CMAKE_SOURCE_DIR}/NEWS")
    install(FILES
      "${CMAKE_SOURCE_DIR}/NEWS"
      DESTINATION ${XEMACS_DOCDIR}
      COMPONENT Documentation
    )
  endif()
endif()

install(DIRECTORY
  DESTINATION ${XEMACS_SITELISPDIR}
  COMPONENT Lisp
)

install(DIRECTORY
  DESTINATION ${XEMACS_SITEMODULEDIR}
  COMPONENT Libraries
)

message(STATUS "")
message(STATUS "========== Configuration Summary ==========")
message(STATUS "XEmacs version: ${EMACS_VERSION}")
message(STATUS "Codename: ${XEMACS_CODENAME}")
message(STATUS "")
message(STATUS "Installation options:")
message(STATUS "  prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  bindir: ${XEMACS_BINDIR}")
message(STATUS "  datadir: ${XEMACS_DATADIR}")
message(STATUS "  libdir: ${XEMACS_LIBDIR}")
message(STATUS "  lispdir: ${XEMACS_LISPDIR}")
message(STATUS "  etcdir: ${XEMACS_ETCDIR}")
message(STATUS "  archlibdir: ${XEMACS_ARCHLIBDIR}")
message(STATUS "")
message(STATUS "Window system:")
if(XEMACS_WITH_X11)
  message(STATUS "  X11: yes")
else()
  message(STATUS "  X11: no")
endif()
if(XEMACS_WITH_GTK)
  message(STATUS "  GTK: yes")
else()
  message(STATUS "  GTK: no")
endif()
message(STATUS "")
message(STATUS "Image formats:")
message(STATUS "  PNG: ${HAVE_PNG}")
message(STATUS "  JPEG: ${HAVE_JPEG}")
message(STATUS "  TIFF: ${HAVE_TIFF}")
message(STATUS "  GIF: ${HAVE_GIF}")
message(STATUS "  XPM: ${HAVE_XPM}")
message(STATUS "")
message(STATUS "===========================================")
message(STATUS "")
