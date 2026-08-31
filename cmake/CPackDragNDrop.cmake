# CPackDragNDrop.cmake - per-generator overrides for the DragNDrop DMG
#
# cpack iterates the generators in CPACK_GENERATOR (CPack.cmake:73-79); for
# each one it resets CPACK_GENERATOR to the current generator and then
# includes this file (set via CPACK_PROJECT_CONFIG_FILE in XEmacsCPack.cmake).
# The guard below makes it a no-op for every generator except DragNDrop, so
# TGZ, `cmake --install`, and `make` are unaffected.
#
# `cpack -G DragNDrop` produces the macOS drag-install DMG: XEmacs.app at the
# DMG root, a native `Applications -> /Applications` symlink (the drag target),
# and the guided install scripts (Install.command / Uninstall.command / bin/ /
# install-cli.sh / README.txt) staged alongside the app so users can either
# drag the app into /Applications or double-click Install.command.
#
# NOTE: at cpack time only CPACK_* variables are visible (CPackConfig.cmake);
# ordinary project variables (EMACS_VERSION, etc.) are not.  The project
# snapshots them into CPACK_XEMACS_* in XEmacsCPack.cmake before include(CPack).

if(NOT CPACK_GENERATOR STREQUAL "DragNDrop")
  return()
endif()

# Distinct filename: never clobbers the TGZ tarball
# (xemacs-<ver>-<arch>.tar.gz).  Capital X matches the .app name and signals
# "GUI drag DMG" vs the FHS tarball.
set(CPACK_PACKAGE_FILE_NAME "XEmacs-${CPACK_XEMACS_VERSION}-${CPACK_XEMACS_SYSTEM_PROCESSOR}")
set(CPACK_DMG_VOLUME_NAME "XEmacs-${CPACK_XEMACS_VERSION}")
set(CPACK_DMG_FORMAT "UDZO")

# Monolithic install stages everything flat into the staging root instead of
# nesting under a per-component `*-<Component>/` subfolder.  The .app lands at
# <staging>/Applications/XEmacs.app; the pre-build script relocates it to the
# root and removes the FHS `share/` and `lib/` residue (the .app is
# self-contained, so Lisp/Etc/Info/Documentation are redundant on a drag DMG).
# (CPACK_COMPONENTS_ALL is ignored when monolithic is on, so we don't set it.)
set(CPACK_MONOLITHIC_INSTALL ON)

# Pre-build script: runs AFTER the monolithic install populates staging and
# BEFORE DragNDrop adds its Applications symlink.  Relocates the app to the
# staging root, removes the colliding Applications/ dir and FHS residue, and
# stages the guided install scripts.  CPACK_TEMPORARY_DIRECTORY = staging root.
set(CPACK_PRE_BUILD_SCRIPTS "${CPACK_XEMACS_SOURCE_DIR}/cmake/CPackDragNDropStage.cmake")
