set(SRC_CORE_SOURCES
  abbrev.c
  alloc.c
  alloca.c
  array.c
  buffer.c
  bytecode.c
  callint.c
  casefiddle.c
  casetab.c
  chartab.c
  cmdloop.c
  cmds.c
  console.c
  console-stream.c
  data.c
  device.c
  dired.c
  doc.c
  doprnt.c
  dumper.c
  editfns.c
  elhash.c
  emacs.c
  eval.c
  events.c
  event-stream.c
  extents.c
  faces.c
  file-coding.c
  fileio.c
  filemode.c
  floatfns.c
  fns.c
  font-lock.c
  frame.c
  gc.c
  general.c
  glyphs.c
  glyphs-widget.c
  gutter.c
  imgproc.c
  indent.c
  insdel.c
  intl.c
  keymap.c
  line-number.c
  lread.c
  lstream.c
  macros.c
  marker.c
  md5.c
  minibuf.c
  fontcolor.c
  opaque.c
  print.c
  process.c
  rangetab.c
  realpath.c
  redisplay.c
  redisplay-output.c
  regex.c
  search.c
  select.c
  sequence.c
  signal.c
  sound.c
  specifier.c
  strftime.c
  symbols.c
  syntax.c
  sysdep.c
  text.c
  tls.c
  undo.c
  unicode.c
  widget.c
  window.c
  inline.c
)

set(SRC_MULE_SOURCES
  mule-ccl.c
  mule-charset.c
  mule-coding.c
)

set(SRC_GUI_SOURCES
  gui.c
)

if(HAVE_MENUBARS)
  list(APPEND SRC_GUI_SOURCES menubar.c)
endif()

if(HAVE_SCROLLBARS)
  list(APPEND SRC_GUI_SOURCES scrollbar.c)
endif()

if(HAVE_DIALOGS)
  list(APPEND SRC_GUI_SOURCES dialog.c)
endif()

if(HAVE_TOOLBARS)
  list(APPEND SRC_GUI_SOURCES toolbar.c)
endif()

set(SRC_X11_SOURCES
  console-x.c
  device-x.c
  event-Xt.c
  frame-x.c
  glyphs-x.c
  fontcolor-x.c
  redisplay-x.c
  select-x.c
  gccache-x.c
  intl-x.c
  EmacsFrame.c
  EmacsShell.c
  EmacsManager.c
)

if(HAVE_X_WINDOWS)
  add_library(top_level_emacs_shell OBJECT EmacsShell-sub.c)
  target_include_directories(top_level_emacs_shell PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(top_level_emacs_shell PRIVATE
    HAVE_CONFIG_H
    emacs
    DEFINE_TOP_LEVEL_EMACS_SHELL
  )

  add_library(transient_emacs_shell OBJECT EmacsShell-sub.c)
  target_include_directories(transient_emacs_shell PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/src"
  )
  target_compile_definitions(transient_emacs_shell PRIVATE
    HAVE_CONFIG_H
    emacs
    DEFINE_TRANSIENT_EMACS_SHELL
  )
endif()

if(HAVE_XFT)
  list(APPEND SRC_X11_SOURCES font-mgr.c)
endif()

if(HAVE_XIM)
  list(APPEND SRC_X11_SOURCES input-method-xlib.c)
endif()

set(SRC_X11_GUI_SOURCES
  gui-x.c
)

if(HAVE_MENUBARS)
  list(APPEND SRC_X11_GUI_SOURCES menubar-x.c)
endif()

if(HAVE_SCROLLBARS)
  list(APPEND SRC_X11_GUI_SOURCES scrollbar-x.c)
endif()

if(HAVE_DIALOGS AND (HAVE_MOTIF_DIALOGS OR HAVE_ATHENA_DIALOGS))
  list(APPEND SRC_X11_GUI_SOURCES dialog-x.c)
endif()

if(HAVE_TOOLBARS)
  list(APPEND SRC_X11_GUI_SOURCES toolbar-x.c toolbar-xlike.c)
endif()

set(SRC_GTK_SOURCES
  console-gtk.c
  device-gtk.c
  event-gtk.c
  frame-gtk.c
  fontcolor-gtk.c
  redisplay-gtk.c
  glyphs-gtk.c
  select-gtk.c
  gtk-xemacs.c
  ui-gtk.c
)

set(SRC_GTK_GUI_SOURCES
  gui-gtk.c
)

if(HAVE_MENUBARS)
  list(APPEND SRC_GTK_GUI_SOURCES menubar-gtk.c)
endif()

if(HAVE_SCROLLBARS)
  list(APPEND SRC_GTK_GUI_SOURCES scrollbar-gtk.c)
endif()

if(HAVE_DIALOGS)
  list(APPEND SRC_GTK_GUI_SOURCES dialog-gtk.c)
endif()

if(HAVE_TOOLBARS)
  list(APPEND SRC_GTK_GUI_SOURCES toolbar-gtk.c toolbar-xlike.c)
endif()

set(SRC_TTY_SOURCES
  console-tty.c
  device-tty.c
  event-tty.c
  frame-tty.c
  fontcolor-tty.c
  redisplay-tty.c
  cm.c
)

set(SRC_MSWINDOWS_SOURCES
  console-msw.c
  device-msw.c
  event-msw.c
  frame-msw.c
  fontcolor-msw.c
  select-msw.c
  redisplay-msw.c
  glyphs-msw.c
)

set(SRC_MSWINDOWS_GUI_SOURCES
  gui-msw.c
)

if(HAVE_MENUBARS)
  list(APPEND SRC_MSWINDOWS_GUI_SOURCES menubar-msw.c)
endif()

if(HAVE_SCROLLBARS)
  list(APPEND SRC_MSWINDOWS_GUI_SOURCES scrollbar-msw.c)
endif()

if(HAVE_DIALOGS)
  list(APPEND SRC_MSWINDOWS_GUI_SOURCES dialog-msw.c)
endif()

if(HAVE_TOOLBARS)
  list(APPEND SRC_MSWINDOWS_GUI_SOURCES toolbar-msw.c)
endif()

set(SRC_WINDOW_SYSTEM_SOURCES
  glyphs-eimage.c
  glyphs-shared.c
)

set(SRC_UNIXOID_EVENT_SOURCES
  event-unixoid.c
)

set(SRC_DATABASE_SOURCES
  database.c
)

set(SRC_SHLIB_SOURCES
  sysdll.c
  emodules.c
)

if(CLASH_DETECTION)
  set(SRC_CLASH_DETECTION_SOURCES filelock.c)
endif()

if(HAVE_GPM)
  set(SRC_GPM_SOURCES gpmevent.c)
endif()

if(HAVE_WNN)
  set(SRC_WNN_SOURCES mule-wnnfns.c)
endif()

if(DEBUG_XEMACS)
  set(SRC_DEBUG_SOURCES tests.c)
endif()

if(HAVE_SETITIMER)
  set(SRC_PROFILE_SOURCES profile.c)
endif()

if(REL_ALLOC)
  set(SRC_RALLOC_SOURCES ralloc.c)
endif()

if(HAVE_MALLOC_WARNING)
  set(SRC_MALLOC_WARNING_SOURCES vm-limit.c)
endif()

if(TOOLTALK)
  set(SRC_TOOLTALK_SOURCES tooltalk.c)
endif()

if(HAVE_BALLOON_HELP)
  set(SRC_BALLOON_HELP_SOURCES balloon_help.c balloon-x.c)
endif()

if(HAVE_NATIVE_SOUND)
  set(SRC_NATIVE_SOUND_SOURCES linuxplay.c miscplay.c)
endif()

set(SRC_NUMBER_SOURCES
  number.c
)

if(WITH_GMP OR WITH_MPIR)
  list(APPEND SRC_NUMBER_SOURCES number-gmp.c)
elseif(WITH_MP)
  list(APPEND SRC_NUMBER_SOURCES number-mp.c)
elseif(WITH_OPENSSL_BIGNUM)
  list(APPEND SRC_NUMBER_SOURCES number-openssl.c)
endif()

set(SRC_PROCESS_SOURCES)
if(HAVE_UNIX_PROCESSES)
  set(SRC_PROCESS_SOURCES process-unix.c)
elseif(HAVE_WIN32_PROCESSES)
  set(SRC_PROCESS_SOURCES process-nt.c)
endif()

# Async IO — libcurl for HTTP/TLS/streaming
# On Debian/Ubuntu the arch-specific multiarch path is not searched by default.
if(NOT CURL_INCLUDE_DIR)
  find_path(CURL_INCLUDE_DIR curl/curl.h
    PATH_SUFFIXES x86_64-linux-gnu aarch64-linux-gnu
  )
endif()
find_package(CURL REQUIRED)
message(STATUS "Found libcurl: ${CURL_VERSION_STRING}")

set(SRC_ASYNC_SOURCES
  async-scheduler.c
  async-http.c
)

set(SRC_ALL_SOURCES
  ${SRC_CORE_SOURCES}
  ${SRC_MULE_SOURCES}
  ${SRC_GUI_SOURCES}
  ${SRC_NUMBER_SOURCES}
  ${SRC_PROCESS_SOURCES}
)

if(HAVE_X_WINDOWS)
  list(APPEND SRC_ALL_SOURCES
    ${SRC_X11_SOURCES}
    ${SRC_X11_GUI_SOURCES}
    ${SRC_WINDOW_SYSTEM_SOURCES}
  )
endif()

if(HAVE_GTK)
  list(APPEND SRC_ALL_SOURCES
    ${SRC_GTK_SOURCES}
    ${SRC_GTK_GUI_SOURCES}
  )
endif()

if(HAVE_TTY)
  list(APPEND SRC_ALL_SOURCES ${SRC_TTY_SOURCES})
endif()

if(HAVE_MS_WINDOWS)
  list(APPEND SRC_ALL_SOURCES
    ${SRC_MSWINDOWS_SOURCES}
    ${SRC_MSWINDOWS_GUI_SOURCES}
  )
endif()

if(HAVE_UNIXOID_EVENT_LOOP)
  list(APPEND SRC_ALL_SOURCES ${SRC_UNIXOID_EVENT_SOURCES})
endif()

# Async scheduler — always built on Unix (requires unixoid event loop)
if(HAVE_UNIXOID_EVENT_LOOP)
  list(APPEND SRC_ALL_SOURCES ${SRC_ASYNC_SOURCES})
endif()

if(HAVE_DATABASE)
  list(APPEND SRC_ALL_SOURCES ${SRC_DATABASE_SOURCES})
endif()

if(HAVE_SHLIB)
  list(APPEND SRC_ALL_SOURCES ${SRC_SHLIB_SOURCES})
endif()

if(SRC_CLASH_DETECTION_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_CLASH_DETECTION_SOURCES})
endif()

if(SRC_GPM_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_GPM_SOURCES})
endif()

if(SRC_WNN_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_WNN_SOURCES})
endif()

if(SRC_DEBUG_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_DEBUG_SOURCES})
endif()

if(SRC_PROFILE_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_PROFILE_SOURCES})
endif()

if(SRC_RALLOC_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_RALLOC_SOURCES})
endif()

if(SRC_MALLOC_WARNING_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_MALLOC_WARNING_SOURCES})
endif()

if(SRC_TOOLTALK_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_TOOLTALK_SOURCES})
endif()

if(SRC_BALLOON_HELP_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_BALLOON_HELP_SOURCES})
endif()

if(SRC_NATIVE_SOUND_SOURCES)
  list(APPEND SRC_ALL_SOURCES ${SRC_NATIVE_SOUND_SOURCES})
endif()

set(SRC_HEADERS
  abbrev.h
  alloc.h
  array.h
  buffer.h
  bytecode.h
  callint.h
  casefiddle.h
  casetab.h
  chartab.h
  cmds.h
  console.h
  console-stream.h
  console-tty.h
  console-x.h
  console-gtk.h
  console-msw.h
  data.h
  database.h
  device.h
  device-tty.h
  device-x.h
  device-gtk.h
  device-msw.h
  dired.h
  doc.h
  doprnt.h
  editfns.h
  elhash.h
  emacs.h
  emacs-ver.h
  eval.h
  events.h
  event-msw.h
  event-tty.h
  event-Xt.h
  event-gtk.h
  event-unixoid.h
  event-stream.h
  extents.h
  extents-impl.h
  faces.h
  file-coding.h
  fileio.h
  filelock.h
  filemode.h
  floatfns.h
  fns.h
  font-lock.h
  fontcolor.h
  font-mgr.h
  frame.h
  frame-tty.h
  frame-x.h
  frame-gtk.h
  frame-msw.h
  gc.h
  gc-prof.h
  gccache-x.h
  general.h
  glyphs.h
  glyphs-eimage.h
  glyphs-shared.h
  glyphs-tty.h
  glyphs-x.h
  glyphs-gtk.h
  glyphs-msw.h
  glyphs-widget.h
  gpmevent.h
  gtk-xemacs.h
  gui.h
  gui-x.h
  gui-gtk.h
  gui-msw.h
  gutter.h
  intl.h
  intl-win32.h
  imgproc.h
  indent.h
  insdel.h
  keymap.h
  line-number.h
  lread.h
  lstream.h
  macros.h
  marker.h
  md5.h
  minibuf.h
  mule-ccl.h
  mule-charset.h
  mule-coding.h
  mule-wnnfns.h
  opaque.h
  print.h
  process.h
  process-unix.h
  process-nt.h
  profile.h
  rangetab.h
  redisplay.h
  redisplay-tty.h
  redisplay-x.h
  redisplay-gtk.h
  redisplay-msw.h
  redisplay-output.h
  regex.h
  search.h
  select.h
  select-tty.h
  select-x.h
  select-gtk.h
  select-msw.h
  sequence.h
  signal.h
  sound.h
  specifier.h
  specifier-impl.h
  strftime.h
  sunpro.h
  symbols.h
  syntax.h
  sysdep.h
  sysdll.h
  text.h
  tooltalk.h
  tls.h
  ui-gtk.h
  undo.h
  unicode.h
  widget.h
  window.h
  win32.h
  xintrinsic.h
  balloon_help.h
  number.h
)

set(SRC_GENERATED_HEADERS
  "${CMAKE_BINARY_DIR}/src/config.h"
  "${CMAKE_BINARY_DIR}/src/paths.h"
)

set(XEMACS_LINK_LIBRARIES
  m
)

if(HAVE_X_WINDOWS)
  list(APPEND XEMACS_LINK_LIBRARIES
    ${X11_LIBRARIES}
    ${X11_Xt_LIB}
  )
  
  if(X11_Xmu_LIB)
    list(APPEND XEMACS_LINK_LIBRARIES ${X11_Xmu_LIB})
  endif()
  
  if(X11_Xext_LIB)
    list(APPEND XEMACS_LINK_LIBRARIES ${X11_Xext_LIB})
  endif()
  
  if(X11_SM_LIB)
    list(APPEND XEMACS_LINK_LIBRARIES ${X11_SM_LIB})
  endif()
  
  if(X11_ICE_LIB)
    list(APPEND XEMACS_LINK_LIBRARIES ${X11_ICE_LIB})
  endif()
  
  if(HAVE_XPM AND X11_Xpm_LIB)
    list(APPEND XEMACS_LINK_LIBRARIES ${X11_Xpm_LIB})
  endif()
endif()

if(HAVE_GTK)
  if(GTK3_FOUND)
    list(APPEND XEMACS_LINK_LIBRARIES ${GTK3_LIBRARIES})
  elseif(GTK2_FOUND)
    list(APPEND XEMACS_LINK_LIBRARIES ${GTK2_LIBRARIES})
  endif()
endif()

if(HAVE_XFT)
  list(APPEND XEMACS_LINK_LIBRARIES ${XFT_LIBRARIES})
endif()

if(HAVE_PNG)
  list(APPEND XEMACS_LINK_LIBRARIES ${PNG_LIBRARIES})
endif()

if(HAVE_JPEG)
  list(APPEND XEMACS_LINK_LIBRARIES ${JPEG_LIBRARIES})
endif()

if(HAVE_TIFF)
  list(APPEND XEMACS_LINK_LIBRARIES ${TIFF_LIBRARIES})
endif()

if(HAVE_GIF)
  list(APPEND XEMACS_LINK_LIBRARIES ${GIF_LIBRARIES})
endif()

if(HAVE_ZLIB)
  list(APPEND XEMACS_LINK_LIBRARIES ${ZLIB_LIBRARIES})
endif()

if(HAVE_DB)
  list(APPEND XEMACS_LINK_LIBRARIES ${BerkeleyDB_LIBRARIES})
endif()

if(HAVE_GDBM)
  list(APPEND XEMACS_LINK_LIBRARIES ${GDBM_LIBRARIES})
endif()

if(HAVE_LDAP)
  list(APPEND XEMACS_LINK_LIBRARIES ${LDAP_LIBRARIES})
endif()

if(HAVE_POSTGRESQL)
  list(APPEND XEMACS_LINK_LIBRARIES ${PostgreSQL_LIBRARIES})
endif()

if(HAVE_ALSA)
  list(APPEND XEMACS_LINK_LIBRARIES ${ALSA_LIBRARIES})
endif()

if(HAVE_GPM)
  list(APPEND XEMACS_LINK_LIBRARIES ${GPM_LIBRARIES})
endif()

if(HAVE_NCURSES)
  list(APPEND XEMACS_LINK_LIBRARIES ${CURSES_LIBRARIES})
endif()

if(HAVE_CANNA)
  list(APPEND XEMACS_LINK_LIBRARIES ${CANNA_LIBRARIES})
endif()

if(HAVE_WNN)
  list(APPEND XEMACS_LINK_LIBRARIES ${WNN_LIBRARIES})
endif()

if(HAVE_SSL)
  list(APPEND XEMACS_LINK_LIBRARIES ${OPENSSL_LIBRARIES})
endif()

if(HAVE_GMP)
  list(APPEND XEMACS_LINK_LIBRARIES ${GMP_LIBRARIES})
endif()

if(DL_LIBRARIES)
  list(APPEND XEMACS_LINK_LIBRARIES ${DL_LIBRARIES})
endif()

if(HAVE_SHLIB AND NOT WIN32)
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -shared")
endif()

add_library(xemacs-core STATIC ${SRC_ALL_SOURCES} ${SRC_GENERATED_HEADERS})

if(TARGET Emacs_ad_h)
  add_dependencies(xemacs-core Emacs_ad_h)
endif()

if(HAVE_X_WINDOWS)
  target_link_libraries(xemacs-core PRIVATE top_level_emacs_shell transient_emacs_shell)
endif()

target_include_directories(xemacs-core PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}/src"
)

if(HAVE_XFT)
  target_include_directories(xemacs-core PUBLIC ${XFT_INCLUDE_DIRS})
endif()

if(CURL_FOUND)
  target_link_libraries(xemacs-core PRIVATE CURL::libcurl)
  target_include_directories(xemacs-core PRIVATE ${CURL_INCLUDE_DIRS})
endif()

target_compile_definitions(xemacs-core PUBLIC
  HAVE_CONFIG_H
  emacs
  $<$<BOOL:${CURL_FOUND}>:HAVE_LIBCURL>
)

# Create .o symlinks in build/src/ so make-docfile.el can find them
set(_symlink_script "${CMAKE_CURRENT_BINARY_DIR}/create-o-symlinks.sh")
file(WRITE "${_symlink_script}"
"#!/bin/sh
cd \"$1\"
for f in CMakeFiles/xemacs-core.dir/*.c.o; do
  base=$(basename \"$f\" .c.o)
  ln -sf \"$f\" \"$base.o\"
done
")
add_custom_command(TARGET xemacs-core POST_BUILD
  COMMAND /bin/sh "${_symlink_script}" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Creating .o symlinks for make-docfile"
)

if(NOT WIN32)
  # Generate a unique dump_id at configure time (matches autoconf's make-dump-id)
  string(RANDOM LENGTH 8 ALPHABET "0123456789abcdef" _dump_id_hex)
  # Convert hex string to unsigned int for C
  math(EXPR XEMACS_DUMP_ID "0x${_dump_id_hex}" OUTPUT_FORMAT DECIMAL)
  set(XEMACS_DUMP_ID_HEX "${_dump_id_hex}" CACHE INTERNAL "Dump ID hex string")

  file(WRITE "${CMAKE_BINARY_DIR}/src/dump-id.c"
    "/* Auto-generated dump ID */\nunsigned int dump_id = ${XEMACS_DUMP_ID}U;\n")

  add_executable(temacs
    "${CMAKE_CURRENT_SOURCE_DIR}/emacs.c"
    "${CMAKE_BINARY_DIR}/src/dump-id.c"
  )
  
  target_include_directories(temacs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/src"
  )
  
  target_compile_definitions(temacs PRIVATE
    HAVE_CONFIG_H
    emacs
  )
  
  target_link_libraries(temacs PRIVATE
    xemacs-core
    ${XEMACS_LINK_LIBRARIES}
  )
  
  if(HAVE_X_WINDOWS)
    target_link_libraries(temacs PRIVATE lw)
  endif()
endif()

add_executable(xemacs
  "${CMAKE_CURRENT_SOURCE_DIR}/emacs.c"
  "${CMAKE_BINARY_DIR}/src/dump-id.c"
)

target_include_directories(xemacs PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_BINARY_DIR}/src"
)

target_compile_definitions(xemacs PRIVATE
  HAVE_CONFIG_H
  emacs
)

target_link_libraries(xemacs PRIVATE
  xemacs-core
  ${XEMACS_LINK_LIBRARIES}
)

if(HAVE_X_WINDOWS)
  target_link_libraries(xemacs PRIVATE lw)
endif()

set_target_properties(xemacs PROPERTIES
  OUTPUT_NAME "${PROGNAME}"
)

# Install binary as xemacs-<version> with xemacs and xemacs-script symlinks
install(PROGRAMS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}"
  DESTINATION ${INST_BINDIR}
  COMPONENT Runtime
  RENAME "${PROGNAME}-${EMACS_VERSION}"
)
install(CODE "
  execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
    \"${PROGNAME}-${EMACS_VERSION}\"
    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INST_BINDIR}/${PROGNAME}\")
  execute_process(COMMAND \${CMAKE_COMMAND} -E create_symlink
    \"${PROGNAME}-${EMACS_VERSION}\"
    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INST_BINDIR}/${SHEBANG_PROGNAME}\")
" COMPONENT Runtime)

# DOC file (generated during dump by make-docfile)
install(FILES "${CMAKE_BINARY_DIR}/lib-src/DOC"
  DESTINATION ${INST_DOCDIR}
  COMPONENT Runtime
  RENAME "DOC"
)

# Dump file - installed with dump-id suffix matching what the binary expects
install(FILES "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EMACS_DUMP_FILE_NAME}"
  DESTINATION ${INST_ARCHLIBDIR}
  COMPONENT Runtime
  RENAME "${PROGNAME}-${XEMACS_DUMP_ID_HEX}.dmp"
)

# Installation summary
install(FILES "${CMAKE_BINARY_DIR}/Installation"
  DESTINATION ${INST_ARCHLIBDIR}
  COMPONENT Runtime
)

# ============================================================
# pdump build steps (matching autoconf's update-elc + dump flow)
#
# Deep-copy lisp/ into the build tree so that byte-compiled .elc
# files, auto-autoloads, custom-load, and finder-inf.el are written
# here instead of polluting the source tree.  This avoids conflicts
# when an autoconf build shares the same source directory.
# etc/ and info/ are read-only so symlinks are safe for those.
# ============================================================

# For out-of-tree builds: copy lisp/ and create symlinks for etc/ and info/
# at BUILD TIME (not configure time). This ensures:
# 1. The build directory is properly set up before pdump steps run
# 2. Multiple out-of-tree builds can share the same source
# 3. Partial rebuilds work correctly

if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  # Out-of-tree build: set up build directory at build time
  
  add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/lisp/.copied"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/lisp"
      "${CMAKE_BINARY_DIR}/lisp"
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/lisp/.copied"
    COMMENT "Copying lisp/ to build tree"
    VERBATIM
  )

  # Create symlinks for read-only directories at build time
  set(_symlink_outputs "")
  foreach(_dir etc info)
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/${_dir}"
      COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_SOURCE_DIR}/${_dir}"
        "${CMAKE_BINARY_DIR}/${_dir}"
      COMMENT "Creating symlink: ${_dir}"
      VERBATIM
    )
    list(APPEND _symlink_outputs "${CMAKE_BINARY_DIR}/${_dir}")
  endforeach()

  # These files are dependencies for UPDATE-ELC-2-DONE in out-of-tree builds
  set(_build_tree_files "${CMAKE_BINARY_DIR}/lisp/.copied" ${_symlink_outputs})
else()
  # In-source build: directories already exist, no dependencies needed
  set(_build_tree_files "")
endif()

set(LISP_DIR "${CMAKE_BINARY_DIR}/lisp")
set(SRC_LISP_DIR "${CMAKE_SOURCE_DIR}/lisp")

set(XEMACS_BATCH
  "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}"
  -no-packages -no-configured-paths -batch
)

# Steps 1-2 use build-tree lisp paths so that .elc files are written
# to the build tree instead of polluting the source tree. The
# XEMACS_SOURCE_DIRECTORY environment variable tells loadup.el and
# update-elc.el where the real source tree lives (for finding C files
# during make-docfile processing).

# Step 1: Byte-compile .el files needed for dumping (pre-dump)
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/src/NEEDTODUMP"
  DEPENDS xemacs ${_build_tree_files}
  COMMAND ${CMAKE_COMMAND} -E env "XEMACS_SOURCE_DIRECTORY=${CMAKE_SOURCE_DIR}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}" -nd
    -no-packages -no-configured-paths -batch
    -l "${LISP_DIR}/update-elc.el"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Byte-compiling lisp files (update-elc)"
)

# Step 2: Dump
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/${EMACS_DUMP_FILE_NAME}"
  DEPENDS "${CMAKE_BINARY_DIR}/src/NEEDTODUMP"
  COMMAND ${CMAKE_COMMAND} -E env "XEMACS_SOURCE_DIRECTORY=${CMAKE_SOURCE_DIR}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}" -nd
    -no-packages -no-configured-paths -batch
    -l "${LISP_DIR}/loadup.el" dump
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Dumping ${EMACS_DUMP_FILE_NAME}"
)

# Step 3: Copy dump file next to binary
add_custom_target(dump
  DEPENDS "${CMAKE_BINARY_DIR}/${EMACS_DUMP_FILE_NAME}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/${EMACS_DUMP_FILE_NAME}"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EMACS_DUMP_FILE_NAME}"
  COMMENT "Dump complete"
)

# The dump pipeline needs the Installation file (loaded by version.el)
add_dependencies(dump generate_installation)

# Step 4: Post-dump byte-compilation of all remaining .el files
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/src/UPDATE-ELC-2-DONE"
  DEPENDS dump ${_build_tree_files}
  COMMAND ${XEMACS_BATCH} -no-autoloads
    -l update-elc-2.el -f batch-update-elc-2 "${LISP_DIR}"
  COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_BINARY_DIR}/src/UPDATE-ELC-2-DONE"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Post-dump byte-compilation (update-elc-2)"
)

# Step 5: Build finder keyword database
add_custom_command(
  OUTPUT "${LISP_DIR}/finder-inf.el"
  DEPENDS "${CMAKE_BINARY_DIR}/src/UPDATE-ELC-2-DONE"
  COMMAND ${XEMACS_BATCH}
    -eval "(setq finder-compile-keywords-quiet t)"
    -eval "(setq lisp-directory \"${LISP_DIR}\")"
    -l finder -f finder-compile-keywords
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Building finder database"
  VERBATIM
)

# Step 6: Check for load-path shadows
add_custom_target(load-shadows
  DEPENDS "${CMAKE_BINARY_DIR}/src/UPDATE-ELC-2-DONE"
  COMMAND ${XEMACS_BATCH} -f list-load-path-shadows
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Testing for Lisp shadows"
)

# Top-level target matching autoconf's "build-the-mofo"
add_custom_target(build-all ALL
  DEPENDS "${LISP_DIR}/finder-inf.el" load-shadows
)
