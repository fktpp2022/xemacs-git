# CPackDragNDropStage.cmake - staging relocation for the DragNDrop DMG
#
# Runs as a CPACK_PRE_BUILD_SCRIPT: AFTER the monolithic install populates the
# staging dir and BEFORE DragNDrop adds its Applications -> /Applications
# symlink.  CPACK_TEMPORARY_DIRECTORY is the staging root (set by cpack); with
# CPACK_MONOLITHIC_INSTALL=ON the install() rules stage flat into it, so the
# .app is at <staging>/Applications/XEmacs.app.
#
# This resolves the "File exists" collision: the app install rule stages the
# app as a real Applications/XEmacs.app/ directory, and DragNDrop then tries to
# create an Applications symlink of the same name.  We break the collision by
# moving the app to the staging root and deleting the now-empty Applications/
# dir so DragNDrop's symlink can claim the name.
#
# NOTE: at cpack time only CPACK_* variables are visible; the project
# snapshots paths into CPACK_XEMACS_* (XEmacsCPack.cmake).

set(_stage "${CPACK_TEMPORARY_DIRECTORY}")
set(_app_name "${CPACK_XEMACS_APP_BUNDLE_NAME}.app")
set(_staged_app "${_stage}/Applications/${_app_name}")
set(_root_app   "${_stage}/${_app_name}")

# 1. Relocate app to the DMG root (idempotent: only if still in Applications/).
if(EXISTS "${_staged_app}" AND NOT EXISTS "${_root_app}")
  file(RENAME "${_staged_app}" "${_root_app}")
endif()

# 2. Clear the "Applications" name so DragNDrop's symlink can take it.
#    Also drops any FHS residue (share/, lib/, man/) the install rules may
#    have staged, keeping the drag DMG minimal -- the .app is self-contained.
file(REMOVE_RECURSE "${_stage}/Applications")
file(REMOVE_RECURSE "${_stage}/share")
file(REMOVE_RECURSE "${_stage}/lib")

# 3. Stage the guided install scripts at the DMG root alongside the app.
#    These are generated at configure time (XEmacsCPack.cmake) and exist in
#    the build tree; file(INSTALL ...) preserves exec bits via
#    USE_SOURCE_PERMISSIONS.
file(INSTALL "${CPACK_XEMACS_BINARY_DIR}/dmg-bin/"
  DESTINATION "${_stage}/bin"
  USE_SOURCE_PERMISSIONS)
file(INSTALL "${CPACK_XEMACS_BINARY_DIR}/dmg-install-cli.sh"
  DESTINATION "${_stage}"
  RENAME "install-cli.sh"
  USE_SOURCE_PERMISSIONS)
file(INSTALL "${CPACK_XEMACS_BINARY_DIR}/dmg-install.command"
  DESTINATION "${_stage}"
  RENAME "Install.command"
  USE_SOURCE_PERMISSIONS)
file(INSTALL "${CPACK_XEMACS_BINARY_DIR}/dmg-uninstall.command"
  DESTINATION "${_stage}"
  RENAME "Uninstall.command"
  USE_SOURCE_PERMISSIONS)
if(EXISTS "${CPACK_XEMACS_SOURCE_DIR}/README")
  file(INSTALL "${CPACK_XEMACS_SOURCE_DIR}/README"
    DESTINATION "${_stage}"
    RENAME "README.txt")
endif()
