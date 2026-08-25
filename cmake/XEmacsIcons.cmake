# Icon generation utilities for macOS .app bundles
#
# This module provides functions to generate macOS icon sets from
# source icons (SVG or PNG) and create .icns files.
#
# Functions:
#   xemacs_generate_macos_icons(SOURCE <svg|png> OUTPUT <dir>)
#     Generates all required icon sizes and packages into .icns
#
# Variables set:
#   XEMACS_ICNS_FILE - Path to generated .icns file
#   XEMACS_ICON_DIRS - List of icon directory build outputs

include(CMakeParseArguments)

# -----------------------------------------------------------------------------
# xemacs_generate_macos_icons
#
# Generate macOS icon resources from an SVG or PNG source.
#
# Arguments:
#   SOURCE <file>   - Input icon file (SVG or PNG)
#   OUTPUT <dir>    - Output directory for generated icons
#
# Example:
#   xemacs_generate_macos_icons(
#     SOURCE "${CMAKE_SOURCE_DIR}/etc/xemacs-icon.svg"
#     OUTPUT "${CMAKE_BINARY_DIR}/macos-icons"
#   )
# -----------------------------------------------------------------------------
function(xemacs_generate_macos_icons)
  cmake_parse_arguments(
    ICON "" "SOURCE;OUTPUT" "" ${ARGN}
  )

  if(NOT ICON_SOURCE)
    message(FATAL_ERROR "xemacs_generate_macos_icons: SOURCE is required")
  endif()
  if(NOT ICON_OUTPUT)
    message(FATAL_ERROR "xemacs_generate_macos_icons: OUTPUT is required")
  endif()

  get_filename_component(SOURCE_NAME "${ICON_SOURCE}" NAME_WE)
  set(ICON_DIR "${ICON_OUTPUT}/${SOURCE_NAME}")
  set(ICNS_FILE "${ICON_OUTPUT}/${SOURCE_NAME}.icns")

  # macOS requires these specific pixel sizes for icon set
  set(ICON_SIZES
    16
    32
    64
    128
    256
    512
    1024
  )

  # Create output directory
  file(MAKE_DIRECTORY "${ICON_DIR}/icon.iconset")

  # Generate PNGs at all required sizes from source
  foreach(SIZE IN LISTS ICON_SIZES)
    set(PNG_FILE "${ICON_DIR}/icon.iconset/icon_${SIZE}x${SIZE}${SIZE}.png")

    # Determine source format and convert appropriately
    get_filename_component(SRC_EXT "${ICON_SOURCE}" EXT)
    string(TOLOWER "${SRC_EXT}" SRC_EXT_LOWER)

    if(SRC_EXT_LOWER MATCHES "\\.svg$")
      # Use sips to convert SVG to PNG at exact size
      # sips handles SVG natively on macOS
      add_custom_command(
        OUTPUT "${PNG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ICON_DIR}/icon.iconset"
        COMMAND sips -s format png -z ${SIZE} ${SIZE} "${ICON_SOURCE}" --out "${PNG_FILE}"
        DEPENDS "${ICON_SOURCE}"
        COMMENT "Generating icon_${SIZE}x${SIZE}.png from ${SOURCE_NAME}.svg"
      )
    else()
      # For PNG sources, resize using sips
      add_custom_command(
        OUTPUT "${PNG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ICON_DIR}/icon.iconset"
        COMMAND sips -s format png -z ${SIZE} ${SIZE} "${ICON_SOURCE}" --out "${PNG_FILE}"
        DEPENDS "${ICON_SOURCE}"
        COMMENT "Generating icon_${SIZE}x${SIZE}.png from ${SOURCE_NAME}${SRC_EXT}"
      )
    endif()

    list(APPEND ICON_PNG_OUTPUTS "${PNG_FILE}")
  endforeach()

  # Also generate @2x versions for Retina display support
  foreach(SIZE IN LISTS ICON_SIZES)
    math(EXPR RETINA_SIZE "${SIZE} * 2")
    set(PNG_FILE "${ICON_DIR}/icon.iconset/icon_${SIZE}x${SIZE}@2x.png")

    get_filename_component(SRC_EXT "${ICON_SOURCE}" EXT)
    string(TOLOWER "${SRC_EXT}" SRC_EXT_LOWER)

    if(SRC_EXT_LOWER MATCHES "\\.svg$")
      add_custom_command(
        OUTPUT "${PNG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ICON_DIR}/icon.iconset"
        COMMAND sips -s format png -z ${RETINA_SIZE} ${RETINA_SIZE} "${ICON_SOURCE}" --out "${PNG_FILE}"
        DEPENDS "${ICON_SOURCE}"
        COMMENT "Generating icon_${SIZE}x${SIZE}@2x.png from ${SOURCE_NAME}.svg"
      )
    else()
      add_custom_command(
        OUTPUT "${PNG_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ICON_DIR}/icon.iconset"
        COMMAND sips -s format png -z ${RETINA_SIZE} ${RETINA_SIZE} "${ICON_SOURCE}" --out "${PNG_FILE}"
        DEPENDS "${ICON_SOURCE}"
        COMMENT "Generating icon_${SIZE}x${SIZE}@2x.png from ${SOURCE_NAME}${SRC_EXT}"
      )
    endif()

    list(APPEND ICON_PNG_OUTPUTS "${PNG_FILE}")
  endforeach()

  # Create the .icns file from the icon set using iconutil
  add_custom_command(
    OUTPUT "${ICNS_FILE}"
    COMMAND iconutil -c icns "${ICON_DIR}/icon.iconset" -o "${ICNS_FILE}"
    DEPENDS ${ICON_PNG_OUTPUTS}
    COMMENT "Creating ${SOURCE_NAME}.icns from icon set"
  )

  # Create a custom target so the icons are built
  add_custom_target(xemacs_macos_icons ALL
    DEPENDS "${ICNS_FILE}"
  )

  # Export variables for use by caller
  set(XEMACS_ICNS_FILE "${ICNS_FILE}" PARENT_SCOPE)
  set(XEMACS_ICON_DIR "${ICON_DIR}" PARENT_SCOPE)
  set(XEMACS_ICON_DIRS "${ICON_DIR}" PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------
# xemacs_add_app_bundle_icon
#
# Add an icon to an existing .app bundle.
#
# Arguments:
#   BUNDLE <path>  - Path to the .app bundle
#   ICNS <file>    - Path to the .icns file to install
#
# Example:
#   xemacs_add_app_bundle_icon(
#     BUNDLE "${APP_BUNDLE}"
#     ICNS "${XEMACS_ICNS_FILE}"
#   )
# -----------------------------------------------------------------------------
function(xemacs_add_app_bundle_icon)
  cmake_parse_arguments(
    APP_ICON "" "BUNDLE;ICNS" "" ${ARGN}
  )

  if(NOT APP_ICON_BUNDLE)
    message(FATAL_ERROR "xemacs_add_app_bundle_icon: BUNDLE is required")
  endif()
  if(NOT APP_ICON_ICNS)
    message(FATAL_ERROR "xemacs_add_app_bundle_icon: ICNS is required")
  endif()

  get_filename_component(ICNS_NAME "${APP_ICON_ICNS}" NAME)

  add_custom_command(
    TARGET xemacs_macos_icons POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${APP_ICON_ICNS}"
      "${APP_ICON_BUNDLE}/Contents/Resources/${ICNS_NAME}"
    COMMENT "Installing ${ICNS_NAME} into ${APP_ICON_BUNDLE}"
    COMMENT "Copying ${ICNS_NAME} to ${APP_ICON_BUNDLE}/Contents/Resources/"
  )
endfunction()
