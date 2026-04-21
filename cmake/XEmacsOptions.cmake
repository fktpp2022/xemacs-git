option(XEMACS_WITH_X11 "Enable X11 window system support" ON)
option(XEMACS_WITH_GTK "Enable GTK window system support" OFF)
option(XEMACS_WITH_GNOME "Enable GNOME desktop support" OFF)
option(XEMACS_WITH_MSW "Enable MS Windows GUI support" OFF)

option(XEMACS_WITH_MULE "Enable MULE (multilingual extension) support" ON)
option(XEMACS_WITH_TOOLBARS "Enable toolbar support" ON)
option(XEMACS_WITH_WMCOMMAND "Handle WM_COMMAND properly" ON)

option(XEMACS_WITH_MENUBARS "Enable menubar support" ON)
option(XEMACS_WITH_SCROLLBARS "Enable scrollbar support" ON)
option(XEMACS_WITH_DIALOGS "Enable dialog support" ON)
option(XEMACS_WITH_WIDGETS "Enable widget support" OFF)

option(XEMACS_WITH_XFT "Enable Xft font rendering support" OFF)
option(XEMACS_WITH_FONTCONFIG "Enable fontconfig support" OFF)

option(XEMACS_WITH_XPM "Enable XPM image support" ON)
option(XEMACS_WITH_PNG "Enable PNG image support" ON)
option(XEMACS_WITH_JPEG "Enable JPEG image support" ON)
option(XEMACS_WITH_TIFF "Enable TIFF image support" ON)
option(XEMACS_WITH_GIF "Enable GIF image support" ON)
option(XEMACS_WITH_XFACE "Enable X-Face image support" OFF)

option(XEMACS_WITH_SOUND "Enable sound support" ON)
option(XEMACS_WITH_ALSA "Enable ALSA sound support" ON)
option(XEMACS_WITH_OSS "Enable OSS native sound support" ON)
option(XEMACS_WITH_NAS "Enable Network Audio System support" OFF)
option(XEMACS_WITH_ESD "Enable Enlightened Sound Daemon support" OFF)

option(XEMACS_WITH_DATABASE "Enable database support" ON)
option(XEMACS_WITH_BERKELEY_DB "Enable Berkeley DB support" ON)
option(XEMACS_WITH_GDBM "Enable GDBM support" ON)
option(XEMACS_WITH_LDAP "Enable LDAP support" ON)
option(XEMACS_WITH_POSTGRESQL "Enable PostgreSQL support" ON)

option(XEMACS_WITH_XIM "Enable XIM input method support" ON)
option(XEMACS_WITH_CANNA "Enable Canna Japanese input method" OFF)
option(XEMACS_WITH_WNN "Enable Wnn Asian input method" OFF)
option(XEMACS_WITH_WNN6 "Enable Wnn6 Asian input method" OFF)
option(XEMACS_WITH_XFS "Enable X FontSet multilingual menubar support" OFF)

option(XEMACS_WITH_TOOLTALK "Enable ToolTalk IPC protocol support" OFF)
option(XEMACS_WITH_SOCKS "Enable SOCKS proxy support" OFF)
option(XEMACS_WITH_DNET "Enable DECnet support" OFF)
option(XEMACS_WITH_IPV6_CNAME "Try IPv6 information first when canonicalizing host names" OFF)
option(XEMACS_WITH_TLS "Enable TLS/SSL connection support" ON)

option(XEMACS_WITH_ZLIB "Enable zlib compression support" ON)
option(XEMACS_WITH_MODULES "Enable dynamic module support" ON)
option(XEMACS_WITH_BIGNUM "Enable bignum support (gmp/mpir/mp/openssl)" OFF)

option(XEMACS_WITH_REL_ALLOC "Enable relocating allocator for Lisp buffers" OFF)
option(XEMACS_WITH_DUMP_IN_EXEC "Enable dumping into executable" ON)
option(XEMACS_WITH_QUICK_BUILD "Enable quick build (for development)" OFF)

option(XEMACS_WITH_DEBUG "Enable debug build" ON)
option(XEMACS_WITH_ERROR_CHECKING "Enable runtime error checking" ON)
option(XEMACS_WITH_ASSERTIONS "Enable runtime assertions" ON)
option(XEMACS_WITH_MEMORY_USAGE_STATS "Enable LISP memory usage API" ON)

option(XEMACS_WITH_TTY "Enable TTY support" ON)
option(XEMACS_WITH_NCURSES "Enable ncurses library for TTY support" ON)
option(XEMACS_WITH_GPM "Enable GPM mouse support for TTYs" OFF)

option(XEMACS_WITH_CLASH_DETECTION "Enable file lock clash detection" ON)
option(XEMACS_WITH_DRAGNDROP "Enable generic drag and drop API" OFF)
option(XEMACS_WITH_CDE "Enable CDE drag and drop support" OFF)
option(XEMACS_WITH_EXTERNAL_WIDGET "Support XEmacs server for text widgets" OFF)

option(XEMACS_WITH_SITE_LISP "Enable site-lisp directory" ON)
option(XEMACS_WITH_SITE_MODULES "Enable site-modules directory" ON)

set(XEMACS_MAIL_LOCKING "dot" CACHE STRING "Mail locking method (lockf, flock, file, locking, mmdf, pop)")
set_property(CACHE XEMACS_MAIL_LOCKING PROPERTY STRINGS lockf flock file locking mmdf pop)

set(XEMACS_WITH_MENUBARS_TYPE "lucid" CACHE STRING "Menubar widget type (yes, no, lucid, motif, athena, gtk, msw)")
set_property(CACHE XEMACS_WITH_MENUBARS_TYPE PROPERTY STRINGS yes no lucid motif athena gtk msw)

set(XEMACS_WITH_SCROLLBARS_TYPE "lucid" CACHE STRING "Scrollbar widget type (yes, no, lucid, motif, athena, gtk, msw)")
set_property(CACHE XEMACS_WITH_SCROLLBARS_TYPE PROPERTY STRINGS yes no lucid motif athena gtk msw)

set(XEMACS_WITH_DIALOGS_TYPE "lucid" CACHE STRING "Dialog widget type (yes, no, lucid, motif, athena, gtk, msw)")
set_property(CACHE XEMACS_WITH_DIALOGS_TYPE PROPERTY STRINGS yes no lucid motif athena gtk msw)

set(XEMACS_WITH_WIDGETS_TYPE "lucid" CACHE STRING "Widget type (yes, no, lucid, motif, athena, gtk, msw)")
set_property(CACHE XEMACS_WITH_WIDGETS_TYPE PROPERTY STRINGS yes no lucid motif athena gtk msw)

set(EMACS_IS_BETA ON CACHE BOOL "Build as beta version")

option(XEMACS_BUILD_TESTS "Build test programs" OFF)
option(XEMACS_INSTALL_DOC "Install documentation" ON)
option(XEMACS_INSTALL_LISP "Install Lisp files" ON)
option(XEMACS_INSTALL_ETC "Install etc files" ON)
option(XEMACS_INSTALL_INFO "Install info files" ON)
