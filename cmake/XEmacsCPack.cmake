include(CPack)

set(CPACK_PACKAGE_NAME "xemacs")
set(CPACK_PACKAGE_VERSION_MAJOR "${EMACS_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${EMACS_MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${EMACS_BETA_VERSION}")
set(CPACK_PACKAGE_VERSION "${EMACS_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "XEmacs - The next generation Emacs editor")
set(CPACK_PACKAGE_DESCRIPTION "XEmacs is a highly customizable open source text editor and application development system. It is protected under the GNU General Public License and related to other versions of Emacs, in particular GNU Emacs. Its emphasis is on modern graphical user interface support and an open software development model, similar to Linux.")
set(CPACK_PACKAGE_VENDOR "XEmacs Development Team")
set(CPACK_PACKAGE_CONTACT "xemacs@xemacs.org")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.xemacs.org/")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
if(EXISTS "${CMAKE_SOURCE_DIR}/README")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README")
endif()

set(CPACK_PACKAGE_INSTALL_DIRECTORY "xemacs-${EMACS_VERSION}")
set(CPACK_PACKAGE_EXECUTABLES "xemacs;XEmacs")

set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")
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
  "\\\\.so\\\\."
  "Makefile$"
  "config\\\\.h$"
  "paths\\\\.h$"
  "config\\\\.status$"
  "config\\\\.log$"
  "configure$"
  "autom4te\\\\.cache/"
  "INSTALL$"
  "Installation$"
)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "xemacs-${EMACS_VERSION}")

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CPACK_GENERATOR "TGZ;TBZ2;ZIP")
  
  find_program(DPKG_EXECUTABLE dpkg)
  if(DPKG_EXECUTABLE)
    list(APPEND CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "xemacs${EMACS_MAJOR_VERSION}${EMACS_MINOR_VERSION}")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libx11-6, libxt6")
    set(CPACK_DEBIAN_PACKAGE_SUGGESTS "xemacs-${EMACS_MAJOR_VERSION}${EMACS_MINOR_VERSION}-mule, xemacs${EMACS_MAJOR_VERSION}${EMACS_MINOR_VERSION}-el")
    set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "XEmacs Development Team <xemacs@xemacs.org>")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "XEmacs highly customizable text editor
 XEmacs is a highly customizable open source text editor and
 application development system. It is protected under the GNU General
 Public License and related to other versions of Emacs, in particular
 GNU Emacs. Its emphasis is on modern graphical user interface support
 and an open software development model, similar to Linux.
 .
 This package contains XEmacs binary and support files.  You will
 need to install the package xemacs${EMACS_MAJOR_VERSION}${EMACS_MINOR_VERSION}-el or xemacs${EMACS_MAJOR_VERSION}${EMACS_MINOR_VERSION}-nomule-el
 to get the base lisp files required for XEmacs to run properly.")
  endif()
  
  find_program(RPMBUILD_EXECUTABLE rpmbuild)
  if(RPMBUILD_EXECUTABLE)
    list(APPEND CPACK_GENERATOR "RPM")
    set(CPACK_RPM_PACKAGE_NAME "xemacs")
    set(CPACK_RPM_PACKAGE_VERSION "${EMACS_MAJOR_VERSION}.${EMACS_MINOR_VERSION}.${EMACS_BETA_VERSION}")
    set(CPACK_RPM_PACKAGE_RELEASE "1")
    set(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Editors")
    set(CPACK_RPM_PACKAGE_URL "https://www.xemacs.org/")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "XEmacs is a highly customizable open source text editor and
application development system. It is protected under the GNU General
Public License and related to other versions of Emacs, in particular
GNU Emacs. Its emphasis is on modern graphical user interface support
and an open software development model, similar to Linux.")
    set(CPACK_RPM_PACKAGE_REQUIRES "libX11 >= 1.0, libXt >= 1.0")
    set(CPACK_RPM_PACKAGE_SUGGESTS "xemacs-el, xemacs-mule")
  endif()
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  set(CPACK_GENERATOR "TGZ;TBZ2;ZIP;DragNDrop")
  set(CPACK_DMG_VOLUME_NAME "XEmacs ${EMACS_VERSION}")
  set(CPACK_DMG_FORMAT "UDBZ")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  set(CPACK_GENERATOR "ZIP;NSIS")
  set(CPACK_NSIS_PACKAGE_NAME "XEmacs")
  set(CPACK_NSIS_DISPLAY_NAME "XEmacs ${EMACS_VERSION}")
  set(CPACK_NSIS_PACKAGE_VERSION "${EMACS_VERSION}")
  set(CPACK_NSIS_CONTACT "xemacs@xemacs.org")
  set(CPACK_NSIS_HELP_LINK "https://www.xemacs.org/")
  set(CPACK_NSIS_URL_INFO_ABOUT "https://www.xemacs.org/")
  set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/etc/xemacs.ico")
  set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/etc/xemacs.ico")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
  set(CPACK_NSIS_MODIFY_PATH ON)
endif()

set(CPACK_COMPONENTS_ALL
  Runtime
  Libraries
  Lisp
  Etc
  Documentation
  Development
  Info
)

set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "XEmacs Runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "XEmacs executable and core libraries")
set(CPACK_COMPONENT_RUNTIME_REQUIRED ON)

set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "XEmacs module libraries")
set(CPACK_COMPONENT_LIBRARIES_DEPENDS Runtime)

set(CPACK_COMPONENT_LISP_DISPLAY_NAME "Lisp Files")
set(CPACK_COMPONENT_LISP_DESCRIPTION "XEmacs Lisp source and compiled files")
set(CPACK_COMPONENT_LISP_REQUIRED ON)

set(CPACK_COMPONENT_ETC_DISPLAY_NAME "Support Files")
set(CPACK_COMPONENT_ETC_DESCRIPTION "XEmacs etc directory support files")
set(CPACK_COMPONENT_ETC_REQUIRED ON)

set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "XEmacs man pages and documentation")

set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "Development Files")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION "XEmacs development headers and tools")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS Runtime)

set(CPACK_COMPONENT_INFO_DISPLAY_NAME "Info Manuals")
set(CPACK_COMPONENT_INFO_DESCRIPTION "XEmacs info documentation")
set(CPACK_COMPONENT_INFO_DEPENDS Documentation)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_NSIS_COMPONENT_INSTALL ON)

set(CPACK_STRIP_FILES TRUE)
set(CPACK_SOURCE_STRIP_FILES TRUE)

set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_SET_DESTDIR ON)
