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
  set(CPACK_GENERATOR "TGZ;DragNDrop")
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

# include(CPack) must come AFTER all CPACK_* variables are set
include(CPack)
