# XEmacsCPackAppBundle.cmake - macOS .app Bundle Support
#
# This module provides CMake functions for building, installing, and
# packaging XEmacs as a macOS .app bundle.  It is intended for use on
# macOS only and is conditionally included by XEmacsCPack.cmake.
#
include_guard(GLOBAL)
#
# Public interface:
#   xemacs_setup_app_bundle()            -- generate .app bundle structure
#
# Cache variables:
#   XEMACS_APP_BUNDLE_ENABLED  -- ON/OFF, gate the whole feature
#   XEMACS_APP_BUNDLE_NAME     -- app bundle name without .app suffix (default: XEmacs)
#
# After xemacs_setup_app_bundle() runs:
#   XEMACS_APP_BUNDLE_DIR      -- absolute path to the .app in the build tree
#   XEMACS_APP_BUNDLE_ID       -- CFBundleIdentifier

# ---------------------------------------------------------------------------
# Cache defaults
# ---------------------------------------------------------------------------
if(DEFINED XEMACS_BUILD_APP_BUNDLE AND NOT DEFINED XEMACS_APP_BUNDLE_ENABLED)
  set(XEMACS_APP_BUNDLE_ENABLED "${XEMACS_BUILD_APP_BUNDLE}" CACHE BOOL "Enable macOS .app bundle support" FORCE)
endif()
if(NOT DEFINED XEMACS_APP_BUNDLE_ENABLED)
  if(APPLE)
    set(XEMACS_APP_BUNDLE_ENABLED ON CACHE BOOL "Enable macOS .app bundle support")
  else()
    set(XEMACS_APP_BUNDLE_ENABLED OFF CACHE BOOL "Enable macOS .app bundle support")
  endif()
endif()

if(NOT DEFINED XEMACS_APP_BUNDLE_NAME)
  set(XEMACS_APP_BUNDLE_NAME "XEmacs" CACHE STRING "Name of the .app bundle")
endif()

# ---------------------------------------------------------------------------
# Helper: generate an .icns file from an SVG source using sips + iconutil
# ---------------------------------------------------------------------------
function(xemacs_generate_icon_from_svg)
  cmake_parse_arguments(
    SVG "" "SOURCE;OUTPUT" "" ${ARGN}
  )
  if(NOT SVG_SOURCE OR NOT SVG_OUTPUT)
    return()
  endif()

  find_program(_sips_path sips)
  find_program(_iconutil_path iconutil)
  if(NOT _sips_path OR NOT _iconutil_path)
    message(WARNING "sips or iconutil not found - icon generation skipped")
    return()
  endif()

  get_filename_component(_src_name "${SVG_SOURCE}" NAME_WE)
  set(_iconset_dir "${CMAKE_BINARY_DIR}/${_src_name}.iconset")
  file(MAKE_DIRECTORY "${_iconset_dir}")

  set(_sizes 16 32 64 128 256 512 1024)
  set(_all_outputs "")

  foreach(_size IN LISTS _sizes)
    # Regular resolution
    set(_out_png "${_iconset_dir}/icon_${_size}x${_size}.png")
    add_custom_command(
      OUTPUT "${_out_png}"
      COMMAND ${_sips_path} -s format png -z ${_size} ${_size} "${SVG_SOURCE}" --out "${_out_png}"
      DEPENDS "${SVG_SOURCE}"
      COMMENT "Generating icon_${_size}x${_size}.png"
      VERBATIM
    )
    list(APPEND _all_outputs "${_out_png}")

    # Retina @2x
    math(EXPR _retina "${_size} * 2")
    set(_out_retina "${_iconset_dir}/icon_${_size}x${_size}@2x.png")
    add_custom_command(
      OUTPUT "${_out_retina}"
      COMMAND ${_sips_path} -s format png -z ${_retina} ${_retina} "${SVG_SOURCE}" --out "${_out_retina}"
      DEPENDS "${SVG_SOURCE}"
      COMMENT "Generating icon_${_size}x${_size}@2x.png"
      VERBATIM
    )
    list(APPEND _all_outputs "${_out_retina}")
  endforeach()

  # Assemble .icns from iconset
  add_custom_command(
    OUTPUT "${SVG_OUTPUT}"
    COMMAND ${_iconutil_path} -c icns "${_iconset_dir}" -o "${SVG_OUTPUT}"
    DEPENDS ${_all_outputs}
    COMMENT "Creating ${_src_name}.icns"
    VERBATIM
  )
endfunction()

# ---------------------------------------------------------------------------
# Main function: set up the .app bundle structure for XEmacs
#
# Follows the macOS Application Bundle specification:
#   Contents/MacOS/     -- all executables and the dump file
#   Contents/Resources/ -- all non-code resources (icons, lib data, share data)
# ---------------------------------------------------------------------------
function(xemacs_setup_app_bundle)
  if(NOT APPLE)
    message(STATUS "XEmacs .app bundle: skipping (not on macOS)")
    return()
  endif()

  if(NOT XEMACS_APP_BUNDLE_ENABLED)
    message(STATUS "XEmacs .app bundle: disabled (XEMACS_APP_BUNDLE_ENABLED=OFF)")
    return()
  endif()

  message(STATUS "Setting up XEmacs .app bundle")

  # ---- Bundle directory layout (macOS spec) ----
  set(_app_name "${XEMACS_APP_BUNDLE_NAME}.app")
  set(_app_path "${CMAKE_BINARY_DIR}/${_app_name}")
  set(_contents_dir "${_app_path}/Contents")
  set(_macos_dir  "${_contents_dir}/MacOS")       # all executables
  set(_resources_dir "${_contents_dir}/Resources") # all non-code data

  # Bundle metadata variables (used by Info.plist.in configure_file via @VARIABLE@)
  set(XEMACS_BUNDLE_ID     "org.xemacs.xemacs")
  set(XEMACS_DISPLAY_NAME  "XEmacs")
  set(XEMACS_EXECUTABLE    "${PROGNAME}")
  set(XEMACS_VERSION       "${EMACS_VERSION}")
  set(XEMACS_SHORT_VERSION "${EMACS_MAJOR_VERSION}.${EMACS_MINOR_VERSION}")
  if(DEFINED CPACK_PACKAGE_DESCRIPTION_SUMMARY)
    set(XEMACS_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
  else()
    set(XEMACS_DESCRIPTION "XEmacs - The next generation Emacs editor")
  endif()
  set(XEMACS_COPYRIGHT     "Copyright 1995-2025 XEmacs Foundation")
  set(XEMACS_ICON_FILE     "XEmacs.icns")
  set(XEMACS_CATEGORY      "public.app-category.productivity")
  set(XEMACS_MIN_SYSTEM_VERSION "10.13")

  # ---- Collect data sources ----
  set(_lisp_src "${CMAKE_BINARY_DIR}/lisp")
  set(_etc_src "${CMAKE_SOURCE_DIR}/etc")
  # Internal paths under Resources/ (maintains the same tree structure the binary expects)
  set(_archlib_dest "${_resources_dir}/lib/xemacs-${EMACS_VERSION}/${EMACS_CONFIGURATION}")
  set(_share_dest   "${_resources_dir}/share")

  # ---- Create all directories ----
  file(MAKE_DIRECTORY
    "${_macos_dir}"
    "${_resources_dir}"
  )

  # ---- Generate Info.plist ----
  configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/Info.plist.in"
    "${_contents_dir}/Info.plist"
    @ONLY
  )

  # ---- Generate PkgInfo ----
  file(WRITE "${_contents_dir}/PkgInfo" "APPL????" )

  # ---- Icon generation from SVG ----
  set(_icns_path "${_resources_dir}/${XEMACS_ICON_FILE}")
  set(_svg_source "${CMAKE_SOURCE_DIR}/etc/xemacs-icon.svg")
  if(EXISTS "${_svg_source}")
    xemacs_generate_icon_from_svg(
      SOURCE "${_svg_source}"
      OUTPUT "${_icns_path}"
    )
  else()
    message(WARNING "xemacs-icon.svg not found at ${_svg_source} - icon generation skipped")
  endif()

  # ---- Build staging script ----
  set(_stamp_file "${CMAKE_BINARY_DIR}/XEmacs.app.stamp")
  set(_build_script "${CMAKE_BINARY_DIR}/build_xemacs_app.sh")

  file(WRITE "${_build_script}" "#!/bin/sh\nset -e\n")

  # Create all directories
  file(APPEND "${_build_script}" "mkdir -p \"${_macos_dir}\" \"${_resources_dir}\" \"${_archlib_dest}\" \"${_share_dest}/xemacs-${EMACS_VERSION}\" \"${_share_dest}/man/man1\"\n")

  # ---- Copy all executables into MacOS/ ----
  # Main binary (build tree name is "xemacs", versioned name inside bundle)
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}\" \"${_macos_dir}/${PROGNAME}-${EMACS_VERSION}\"\n")
  endif()
  # Symlinks for unversioned names
  file(APPEND "${_build_script}" "ln -sf \"${PROGNAME}-${EMACS_VERSION}\" \"${_macos_dir}/${PROGNAME}\"\n")
  file(APPEND "${_build_script}" "ln -sf \"${PROGNAME}-${EMACS_VERSION}\" \"${_macos_dir}/${SHEBANG_PROGNAME}\"\n")

  # Client/server utilities
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/etags")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/etags\" \"${_macos_dir}/\"\n")
  endif()
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gnuclient")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gnuclient\" \"${_macos_dir}/\"\n")
  endif()
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gnuserv")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gnuserv\" \"${_macos_dir}/\"\n")
  endif()
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ctags")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ctags\" \"${_macos_dir}/\"\n")
  endif()

  # Dump file alongside the binary so pdump_file_try() finds it on first search
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xemacs.dmp")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xemacs.dmp\" \"${_macos_dir}/xemacs.dmp\"\n")
  endif()

  # ---- Copy data files into Resources/ ----
  # Lisp files
  if(IS_DIRECTORY "${_lisp_src}")
    file(APPEND "${_build_script}" "cp -R \"${_lisp_src}/.\" \"${_share_dest}/xemacs-${EMACS_VERSION}/lisp/\"\n")
  endif()
  # Etc (data files, tutorials, unicode, icons, themes)
  if(IS_DIRECTORY "${_etc_src}")
    file(APPEND "${_build_script}" "cp -R \"${_etc_src}/.\" \"${_share_dest}/xemacs-${EMACS_VERSION}/etc/\"\n")
  endif()
  # Archlib contents (DOC, hexl, movemail, versioned dump)
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xemacs.dmp")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xemacs.dmp\" \"${_archlib_dest}/xemacs-${XEMACS_DUMP_ID_HEX}.dmp\"\n")
  endif()
  if(EXISTS "${CMAKE_BINARY_DIR}/lib-src/DOC")
    file(APPEND "${_build_script}" "cp \"${CMAKE_BINARY_DIR}/lib-src/DOC\" \"${_archlib_dest}/DOC\"\n")
  endif()
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hexl")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hexl\" \"${_archlib_dest}/\"\n")
  endif()
  if(EXISTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/movemail")
    file(APPEND "${_build_script}" "cp \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/movemail\" \"${_archlib_dest}/\"\n")
  endif()
  # Man pages
  if(IS_DIRECTORY "${_etc_src}")
    file(GLOB _man_pages "${_etc_src}/*.1")
    if(_man_pages)
      foreach(_man IN LISTS _man_pages)
        get_filename_component(_man_name "${_man}" NAME)
        file(APPEND "${_build_script}" "cp \"${_man}\" \"${_share_dest}/man/man1/${_man_name}\"\n")
      endforeach()
    endif()
  endif()

  # Touch stamp file
  file(APPEND "${_build_script}" "touch \"${_stamp_file}\"\n")

  # ---- Compatibility symlinks for Lisp runtime path resolution ----
  # The Lisp runtime (paths-find-emacs-roots) walks up from MacOS/ to Contents/
  # and checks for lib/xemacs-VER/ and lisp/ + etc/ directories.  Since the
  # actual data lives under Resources/, we create symlinks at the Contents/
  # level so both the macOS spec and the runtime path-finding are satisfied.
  file(APPEND "${_build_script}" "ln -sf \"Resources/lib\" \"${_contents_dir}/lib\"\n")
  file(APPEND "${_build_script}" "ln -sf \"Resources/share\" \"${_contents_dir}/share\"\n")
  # The exec-directory search looks for lib-src/ at the root level; point it
  # into MacOS/ where all executables now live.
  # Note: Contents/lib-src symlink is omitted.  The runtime searches for
  # `lib-src` at the root to distinguish installed vs in-tree builds.  Since
  # the bundle is always installed, the absence of `lib-src` is the correct
  # signal -- the runtime falls back to configure-* paths (which are set by
  # the invocation-root search against Contents/).

  file(CHMOD "${_build_script}" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

  # ---- Single custom command for the app assembly ----
  add_custom_command(
    OUTPUT "${_stamp_file}"
    COMMAND "${_build_script}"
    DEPENDS "${_build_script}"
      "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROGNAME}"
    COMMENT "Building XEmacs .app bundle"
    VERBATIM
  )

  # ---- Custom targets ----
  add_custom_target(xemacs_macos_icons
    DEPENDS "${_icns_path}"
    COMMENT "Generating macOS icon"
  )

  add_custom_target(xemacs-app-bundle
    ALL
    DEPENDS "${_stamp_file}"
    COMMENT "Building XEmacs .app bundle"
  )
  add_dependencies(xemacs-app-bundle xemacs_macos_icons)

  # ---- Install the .app bundle ----
  install(DIRECTORY "${_app_path}/"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/Applications"
    COMPONENT Runtime
  )

  # ---- Export variables for CPack DMG integration ----
  set(XEMACS_APP_BUNDLE_DIR "${_app_path}" PARENT_SCOPE)
  set(XEMACS_APP_BUNDLE_ID  "${XEMACS_BUNDLE_ID}" PARENT_SCOPE)
  set(XEMACS_APP_BUNDLE_NAME "${XEMACS_APP_BUNDLE_NAME}" PARENT_SCOPE)

  message(STATUS "  Bundle ID: ${XEMACS_BUNDLE_ID}")
  message(STATUS "  Display Name: ${XEMACS_DISPLAY_NAME}")
  message(STATUS "  App path: ${_app_path}")
endfunction()