# CPack configuration for XEmacs binary package generation
#
# Usage:
#   cpack -G TGZ       # tar.gz archive
#   cpack -G DEB       # Debian package
#   cpack -G RPM       # RPM package

set(CPACK_PACKAGE_NAME "xemacs")
set(CPACK_PACKAGE_VERSION_MAJOR "${EMACS_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${EMACS_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${EMACS_BETA_VERSION}")
set(CPACK_PACKAGE_VERSION "${EMACS_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XEmacs - The next generation Emacs editor")
set(CPACK_PACKAGE_VENDOR "XEmacs Development Team")
set(CPACK_PACKAGE_CONTACT "xemacs@xemacs.org")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.xemacs.org/")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
if(EXISTS "${CMAKE_SOURCE_DIR}/README")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
endif()

set(CPACK_PACKAGE_FILE_NAME "xemacs-${EMACS_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "xemacs-${EMACS_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES "xemacs;XEmacs")
set(CPACK_STRIP_FILES TRUE)

# Source package settings
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "xemacs-${EMACS_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES
  "/\\\\.git/"
  "/\\\\.trae/"
  "/build/"
  "/cmake-build-*/"
  "~$"
  "\\\\.swp$"
  "\\\\.o$"
  "\\\\.a$"
  "\\\\.so$"
)

# Platform-specific generators
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CPACK_GENERATOR "TGZ")

  find_program(DPKG_EXECUTABLE dpkg)
  if(DPKG_EXECUTABLE)
    list(APPEND CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "xemacs")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libx11-6, libxt6")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
  endif()

  find_program(RPMBUILD_EXECUTABLE rpmbuild)
  if(RPMBUILD_EXECUTABLE)
    list(APPEND CPACK_GENERATOR "RPM")
    set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Editors")
    set(CPACK_RPM_PACKAGE_URL "https://www.xemacs.org/")
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CPACK_GENERATOR "TGZ")
  # Per-generator override file.  cpack includes this for each generator in
  # CPACK_GENERATOR (CPack.cmake:73-79); it self-guards and only applies
  # DragNDrop overrides, so TGZ/install are unaffected.  `cpack -G DragNDrop`
  # produces the drag-install DMG; bare `cpack` still makes the FHS tarball.
  set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/cmake/CPackDragNDrop.cmake")

  # macOS .app bundle support
  include("${CMAKE_SOURCE_DIR}/cmake/XEmacsCPackAppBundle.cmake")
  if(XEMACS_APP_BUNDLE_ENABLED)
    # --- User-level CLI install support -----------------------------------
    # Generate per-tool wrapper scripts and an install-cli.sh at configure
    # time.  These are staged into the drag-install DMG by
    # CPackDragNDropStage.cmake (via CPACK_INSTALL_SCRIPTS) during `cpack -G
    # DragNDrop`.  Each wrapper execs the real binary inside
    # /Applications/XEmacs.app after drag-install.
    set(_xemacs_cli_tools xemacs xemacs-script etags gnuclient gnuserv)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/dmg-bin")
    foreach(_tool ${_xemacs_cli_tools})
      file(WRITE "${CMAKE_BINARY_DIR}/dmg-bin/${_tool}"
"#!/bin/sh
exec \"/Applications/XEmacs.app/Contents/MacOS/${_tool}\" \"\$@\"
")
      file(CHMOD "${CMAKE_BINARY_DIR}/dmg-bin/${_tool}"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)
    endforeach()

    # install-cli.sh: creates symlinks in $HOME/.local/bin (no sudo).
    # Resolves the .app from /Applications first, else falls back to the
    # DMG mount location (this script's own directory).
    file(WRITE "${CMAKE_BINARY_DIR}/dmg-install-cli.sh"
[[#!/bin/sh
# XEmacs command-line tools installer (user-level, no sudo).
# Creates symlinks in $HOME/.local/bin pointing at the binaries inside
# XEmacs.app, whether the app is in /Applications or the DMG is mounted.

TOOLS="xemacs xemacs-script etags gnuclient gnuserv"

# Resolve the .app location: prefer /Applications, else look next to this
# script (the mounted-DMG layout puts install-cli.sh at the DMG root).
APP="/Applications/XEmacs.app"
if [ ! -d "$APP" ]; then
  script_dir=$(cd "$(dirname "$0")" && pwd)
  if [ -d "$script_dir/XEmacs.app" ]; then
    APP="$script_dir/XEmacs.app"
  else
    echo "error: XEmacs.app not found in /Applications or next to this script" >&2
    echo "       Drag XEmacs.app into /Applications, or run this script from" >&2
    echo "       the mounted DMG, then re-run." >&2
    exit 1
  fi
fi

mkdir -p "$HOME/.local/bin" || {
  echo "error: could not create $HOME/.local/bin" >&2
  exit 1
}

installed=0
for tool in $TOOLS; do
  bin="$APP/Contents/MacOS/$tool"
  if [ -e "$bin" ]; then
    ln -sf "$bin" "$HOME/.local/bin/$tool"
    echo "linked $HOME/.local/bin/$tool -> $bin"
    installed=$((installed + 1))
  else
    echo "skip: $bin not found"
  fi
done

case ":$PATH:" in
  *":$HOME/.local/bin:"*)
    echo "note: $HOME/.local/bin is already on your PATH."
    ;;
  *)
    echo
    echo "Add ~/.local/bin to your PATH to use the XEmacs command-line tools."
    echo "  sh/bash:  add 'export PATH=\"$HOME/.local/bin:$PATH\"' to ~/.profile"
    echo "  zsh:      add 'export PATH=\"$HOME/.local/bin:$PATH\"' to ~/.zshrc"
    ;;
esac

echo
echo "Done. $installed tool(s) linked into $HOME/.local/bin."
]])
    file(CHMOD "${CMAKE_BINARY_DIR}/dmg-install-cli.sh"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE)

    # --- Install.command: one-click installer -----------------------------
    # Double-click to copy .app to /Applications, set up CLI symlinks, open app.
    # Also copies Uninstall.command into ~/.local/bin/remove-xemacs for
    # permanent access after the DMG is ejected.
    file(WRITE "${CMAKE_BINARY_DIR}/dmg-install.command"
[[#!/bin/sh
# XEmacs macOS One-Click Installer
# Double-click to install XEmacs.app to /Applications and set up CLI tools.
# ~/.local/bin is on PATH by default on macOS (zsh/bash include it).

set -e

APP="XEmacs.app"
TARGET="/Applications/${APP}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Determine source .app location
if [ -d "$SCRIPT_DIR/${APP}" ]; then
  SRC="$SCRIPT_DIR/${APP}"
elif [ -d "$TARGET" ]; then
  echo "XEmacs is already installed in /Applications."
  SRC="$TARGET"
else
  echo "Error: Could not find ${APP} in this DMG or /Applications." >&2
  exit 1
fi

# Copy to /Applications (ditto preserves symlinks, unlike cp -R)
if [ "$SRC" != "$TARGET" ]; then
  echo "Installing ${APP} to /Applications..."
  ditto "$SRC" "$TARGET"
fi

# Set up CLI symlinks in ~/.local/bin
TOOLS="xemacs xemacs-script etags gnuclient gnuserv"
CLI_DIR="$HOME/.local/bin"
mkdir -p "$CLI_DIR"
installed=0
for tool in $TOOLS; do
  bin="$TARGET/Contents/MacOS/$tool"
  if [ -e "$bin" ]; then
    ln -sf "$bin" "$CLI_DIR/$tool"
    echo "  linked: $CLI_DIR/$tool"
    installed=$((installed + 1))
  fi
done

# Copy uninstaller to ~/.local/bin for permanent access (even after DMG is gone)
UNINSTALL_SRC="$(cd "$(dirname "$0")" && pwd)/Uninstall.command"
if [ -f "$UNINSTALL_SRC" ]; then
  cp "$UNINSTALL_SRC" "$CLI_DIR/remove-xemacs"
  chmod +x "$CLI_DIR/remove-xemacs"
  echo "  installed: $CLI_DIR/remove-xemacs"
fi

open "$TARGET"
osascript -e "display notification \"XEmacs installed ($installed tools)\" with title \"XEmacs\" subtitle \"Ready to use\""
]])
    file(CHMOD "${CMAKE_BINARY_DIR}/dmg-install.command"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE)

    # --- Uninstall.command: smart cleanup ----------------------------------
    # Cleans up CLI symlinks from ~/.local/bin/ regardless of DMG availability.
    # Safe to run even if XEmacs.app has been moved to Trash.
    file(WRITE "${CMAKE_BINARY_DIR}/dmg-uninstall.command"
[[#!/bin/sh
# XEmacs macOS Smart Uninstaller
# Cleans up CLI symlinks from ~/.local/bin/ regardless of DMG availability.
# Safe to run even if XEmacs.app has already been moved to Trash.

APP="XEmacs.app"
CLI_DIR="$HOME/.local/bin"
TOOLS="xemacs xemacs-script etags gnuclient gnuserv"

removed=0
for tool in $TOOLS; do
  link="$CLI_DIR/$tool"
  if [ -L "$link" ]; then
    rm -f "$link"
    echo "  removed: $link"
    removed=$((removed + 1))
  fi
done

# Report .app status
if [ -d "/Applications/${APP}" ]; then
  echo "XEmacs.app is still in /Applications."
elif [ -d "$HOME/.Trash/${APP}" ]; then
  echo "XEmacs.app found in Trash — symlinks already cleaned."
else
  echo "XEmacs.app not found (may already be removed)."
fi

if [ "$removed" -gt 0 ]; then
  osascript -e "display notification \"XEmacs CLI tools removed ($removed)\" with title \"XEmacs\" subtitle \"Cleanup complete\""
else
  osascript -e "display notification \"No XEmacs CLI tools found to remove.\" with title \"XEmacs\" subtitle \"Already clean\""
fi
]])
    file(CHMOD "${CMAKE_BINARY_DIR}/dmg-uninstall.command"
      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                  GROUP_READ GROUP_EXECUTE
                  WORLD_READ WORLD_EXECUTE)

  else()
    # Fallback: use plain TGZ without .app bundle
    message(STATUS "macOS .app bundle disabled, using TGZ only")
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CPACK_GENERATOR "ZIP;NSIS")
endif()

# Components
set(CPACK_COMPONENTS_ALL Runtime Lisp Etc Info Documentation)

set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "XEmacs Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "XEmacs executable, dump, DOC, and support utilities")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)

set(CPACK_COMPONENT_LISP_DISPLAY_NAME "Lisp Files")
set(CPACK_COMPONENT_LISP_DESCRIPTION "XEmacs Lisp source and byte-compiled files")
set(CPACK_COMPONENT_LISP_REQUIRED ON)

set(CPACK_COMPONENT_ETC_DISPLAY_NAME "Support Files")
set(CPACK_COMPONENT_ETC_DESCRIPTION "XEmacs etc directory (tutorials, icons, etc.)")
set(CPACK_COMPONENT_ETC_REQUIRED ON)

set(CPACK_COMPONENT_INFO_DISPLAY_NAME "Info Manuals")
set(CPACK_COMPONENT_INFO_DESCRIPTION "XEmacs info documentation")

set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "Man pages and other documentation")

# Project info needed at cpack time (CPackDragNDrop*.cmake), where ordinary
# project variables (EMACS_VERSION, XEMACS_APP_BUNDLE_NAME, CMAKE_BINARY_DIR)
# are NOT visible -- cpack only reads CPACK_* variables from CPackConfig.cmake.
# CPACK_XEMACS_* carries this data across.
if(APPLE AND XEMACS_APP_BUNDLE_ENABLED)
  set(CPACK_XEMACS_APP_BUNDLE_NAME "${XEMACS_APP_BUNDLE_NAME}")
  set(CPACK_XEMACS_VERSION "${EMACS_VERSION}")
  set(CPACK_XEMACS_SOURCE_DIR "${CMAKE_SOURCE_DIR}")
  set(CPACK_XEMACS_BINARY_DIR "${CMAKE_BINARY_DIR}")
  set(CPACK_XEMACS_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# include(CPack) must come AFTER all CPACK_* variables are set
include(CPack)
