#ifndef _SRC_CONFIG_H_
#define _SRC_CONFIG_H_

#cmakedefine XEMACS 1

#cmakedefine EMACS_PROGNAME "@PROGNAME@"
#cmakedefine EMACS_DUMP_FILE_NAME "@EMACS_DUMP_FILE_NAME@"
#cmakedefine SHEBANG_PROGNAME "@SHEBANG_PROGNAME@"

#cmakedefine EMACS_CONFIGURATION "@EMACS_CONFIGURATION@"
#cmakedefine EMACS_CONFIG_OPTIONS "@EMACS_CONFIG_OPTIONS@"

#cmakedefine EMACS_MAJOR_VERSION @EMACS_MAJOR_VERSION@
#cmakedefine EMACS_MINOR_VERSION @EMACS_MINOR_VERSION@
#cmakedefine EMACS_BETA_VERSION @EMACS_BETA_VERSION@
#cmakedefine EMACS_VERSION "@EMACS_VERSION@"
#cmakedefine XEMACS_CODENAME "@XEMACS_CODENAME@"
#cmakedefine XEMACS_RELEASE_DATE "@XEMACS_RELEASE_DATE@"

#cmakedefine config_opsysfile "@XEMACS_CONFIG_OPSYSFILE@"
#cmakedefine config_machfile "@XEMACS_CONFIG_MACHFILE@"

#cmakedefine01 USE_GCC
#cmakedefine01 USE_GPLUSPLUS
#cmakedefine USE_UNION_TYPE @USE_UNION_TYPE@

#cmakedefine01 HAVE_UNIX_PROCESSES
#cmakedefine01 HAVE_GLIBC

#cmakedefine01 REL_ALLOC

#cmakedefine01 HAVE_TTY
#cmakedefine01 HAVE_NCURSES
#cmakedefine01 HAVE_GPM

#cmakedefine01 HAVE_MS_WINDOWS
#cmakedefine01 HAVE_X_WINDOWS
#cmakedefine01 HAVE_X11
#cmakedefine HAVE_GTK
#cmakedefine HAVE_GTK2
#cmakedefine HAVE_GTK3
#cmakedefine HAVE_PANGO
#cmakedefine HAVE_PANGOXFT

#cmakedefine01 HAVE_XFT
#cmakedefine01 HAVE_FONTCONFIG

#cmakedefine01 HAVE_XIM
#cmakedefine01 HAVE_CANNA
#cmakedefine01 HAVE_WNN
#cmakedefine01 WNN6

#cmakedefine01 HAVE_XPM
#cmakedefine01 HAVE_XFACE
#cmakedefine01 HAVE_JPEG
#cmakedefine01 HAVE_TIFF
#cmakedefine01 HAVE_GIF
#cmakedefine01 HAVE_PNG
#cmakedefine01 HAVE_ZLIB

#cmakedefine01 HAVE_SOUND
#cmakedefine01 HAVE_NATIVE_SOUND
#cmakedefine01 HAVE_ALSA_SOUND
#cmakedefine01 HAVE_NAS_SOUND
#cmakedefine01 HAVE_ESD_SOUND

#cmakedefine01 HAVE_DATABASE
#cmakedefine01 HAVE_DBM
#cmakedefine01 HAVE_BERKELEY_DB
#cmakedefine01 HAVE_LDAP
#cmakedefine01 HAVE_POSTGRESQL

#cmakedefine01 CLASH_DETECTION

#cmakedefine01 HAVE_SHLIB
#cmakedefine01 HAVE_DLOPEN
#cmakedefine01 HAVE_MODULES

#cmakedefine WITH_TLS
#cmakedefine HAVE_OPENSSL
#cmakedefine HAVE_GNUTLS
#cmakedefine HAVE_NSS

#cmakedefine WITH_NUMBER_TYPES
#cmakedefine WITH_GMP
#cmakedefine WITH_MPIR
#cmakedefine WITH_MP
#cmakedefine WITH_OPENSSL_BIGNUM

#cmakedefine01 HAVE_MENUBARS
#cmakedefine01 HAVE_SCROLLBARS
#cmakedefine01 HAVE_DIALOGS
#cmakedefine01 HAVE_TOOLBARS
#cmakedefine01 HAVE_WIDGETS

#cmakedefine01 LWLIB_USES_MOTIF
#cmakedefine01 LWLIB_USES_ATHENA
#cmakedefine01 LWLIB_MENUBARS_LUCID
#cmakedefine01 LWLIB_MENUBARS_MOTIF
#cmakedefine01 LWLIB_SCROLLBARS_LUCID
#cmakedefine01 LWLIB_SCROLLBARS_MOTIF
#cmakedefine01 LWLIB_SCROLLBARS_ATHENA
#cmakedefine01 LWLIB_SCROLLBARS_ATHENA3D
#cmakedefine01 LWLIB_DIALOGS_MOTIF
#cmakedefine01 LWLIB_DIALOGS_ATHENA
#cmakedefine01 LWLIB_DIALOGS_ATHENA3D
#cmakedefine01 LWLIB_TABS_LUCID
#cmakedefine01 LWLIB_WIDGETS_MOTIF
#cmakedefine01 LWLIB_WIDGETS_ATHENA

#cmakedefine01 HAVE_ATHENA_3D
#cmakedefine01 HAVE_ATHENA_I18N

#cmakedefine01 HAVE_LUCID_WIDGETS
#cmakedefine01 HAVE_LUCID_MENUBARS
#cmakedefine01 HAVE_LUCID_SCROLLBARS
#cmakedefine01 HAVE_LUCID_DIALOGS
#cmakedefine01 HAVE_MOTIF_WIDGETS
#cmakedefine01 HAVE_MOTIF_MENUBARS
#cmakedefine01 HAVE_MOTIF_SCROLLBARS
#cmakedefine01 HAVE_MOTIF_DIALOGS
#cmakedefine01 HAVE_ATHENA_WIDGETS
#cmakedefine01 HAVE_ATHENA_MENUBARS
#cmakedefine01 HAVE_ATHENA_SCROLLBARS
#cmakedefine01 HAVE_ATHENA_DIALOGS
#cmakedefine HAVE_GTK_WIDGETS
#cmakedefine HAVE_GTK_MENUBARS
#cmakedefine HAVE_GTK_SCROLLBARS
#cmakedefine HAVE_GTK_DIALOGS
#cmakedefine HAVE_GNOME_DIALOGS
#cmakedefine01 HAVE_MSW_WIDGETS
#cmakedefine01 HAVE_MSW_MENUBARS
#cmakedefine01 HAVE_MSW_SCROLLBARS
#cmakedefine01 HAVE_MSW_DIALOGS

#cmakedefine01 HAVE_GUI_OBJECTS
#cmakedefine01 HAVE_POPUPS
#cmakedefine01 HAVE_WINDOW_SYSTEM
#cmakedefine01 HAVE_XLIKE
#cmakedefine01 HAVE_X_WIDGETS

#if HAVE_DIALOGS && (LWLIB_DIALOGS_MOTIF || LWLIB_DIALOGS_ATHENA || LWLIB_DIALOGS_ATHENA3D)
#define HAVE_X_DIALOGS 1
#endif

#cmakedefine01 HAVE_UNIXOID_EVENT_LOOP

#cmakedefine01 HAVE_WMCOMMAND
#cmakedefine01 USE_XFONTSET
#cmakedefine01 UNICODE_INTERNAL

#cmakedefine01 HAVE_BALLOON_HELP
#cmakedefine01 HAVE_DRAGNDROP
#cmakedefine01 HAVE_CDE
#cmakedefine01 EXTERNAL_WIDGET

#cmakedefine01 HAVE_TOOLTALK
#cmakedefine01 TOOLTALK

#cmakedefine01 HAVE_SOCKS
#cmakedefine01 HAVE_DNET

#cmakedefine01 IPV6_CANONICALIZE

#cmakedefine01 MAIL_USE_POP
#cmakedefine01 KERBEROS
#cmakedefine01 HESIOD
#cmakedefine01 MAIL_LOCK_LOCKF
#cmakedefine01 MAIL_LOCK_FLOCK
#cmakedefine01 MAIL_LOCK_DOT
#cmakedefine01 MAIL_LOCK_LOCKING
#cmakedefine01 MAIL_LOCK_MMDF

#cmakedefine01 HAVE_MKSTEMP

#cmakedefine01 INHIBIT_SITE_LISP
#cmakedefine01 INHIBIT_SITE_MODULES

#cmakedefine01 DUMP_IN_EXEC

#cmakedefine01 SUNPRO
#cmakedefine01 USAGE_TRACKING

#cmakedefine01 DEBUG_XEMACS
#cmakedefine01 USE_ASSERTIONS
#cmakedefine01 MEMORY_USAGE_STATS
#cmakedefine01 QUANTIFY
#cmakedefine01 PURIFY
#cmakedefine01 USE_VALGRIND
#cmakedefine01 QUICK_BUILD
#cmakedefine01 BATCH_COMPILER_RUNS

#cmakedefine ERROR_CHECK_BYTE_CODE
#cmakedefine ERROR_CHECK_DISPLAY
#cmakedefine ERROR_CHECK_EXTENTS
#cmakedefine ERROR_CHECK_GC
#cmakedefine ERROR_CHECK_GLYPHS
#cmakedefine ERROR_CHECK_MALLOC
#cmakedefine ERROR_CHECK_STRUCTURES
#cmakedefine ERROR_CHECK_TEXT
#cmakedefine ERROR_CHECK_TYPES

#define SIZEOF_SHORT @SIZEOF_SHORT@
#define SIZEOF_INT @SIZEOF_INT@
#define SIZEOF_LONG @SIZEOF_LONG@
#define SIZEOF_LONG_LONG @SIZEOF_LONG_LONG@
#define SIZEOF_VOID_P @SIZEOF_VOID_P@
#define SIZEOF_DOUBLE @SIZEOF_DOUBLE@
#define SIZEOF_OFF_T @SIZEOF_OFF_T@

#cmakedefine01 WORDS_BIGENDIAN
#cmakedefine01 HAVE_FSEEKO
#cmakedefine01 HAVE_SNPRINTF

#cmakedefine01 HAVE_ALLOCA
#cmakedefine01 HAVE_ALLOCA_H
#cmakedefine01 C_ALLOCA

#cmakedefine01 HAVE_SOCKETS
#cmakedefine01 HAVE_SOCKADDR_SUN_LEN
#cmakedefine01 HAVE_MULTICAST
#cmakedefine01 HAVE_SYSVIPC
#cmakedefine01 HAVE_LOCKF
#cmakedefine01 HAVE_FLOCK

#cmakedefine01 HAVE_TERMIOS
#cmakedefine01 HAVE_TERMIO

#cmakedefine01 HAVE_SIGSETJMP

#cmakedefine01 USE_GNU_MAKE
#cmakedefine01 NEED_TO_HANDLE_21_4_CODE

#cmakedefine inline @inline@
#cmakedefine const @const@

#cmakedefine ssize_t @ssize_t@
#cmakedefine size_t @size_t@
#cmakedefine pid_t @pid_t@
#cmakedefine mode_t @mode_t@
#cmakedefine off_t @off_t@
#cmakedefine uid_t @uid_t@
#cmakedefine gid_t @gid_t@
#cmakedefine socklen_t @socklen_t@

#cmakedefine TYPEOF @TYPEOF@
#cmakedefine STACK_TRACE_EYE_CATCHER "@STACK_TRACE_EYE_CATCHER@"
#cmakedefine OS_RELEASE @OS_RELEASE@

#cmakedefine PREFIX_USER_DEFINED @PREFIX_USER_DEFINED@
#cmakedefine EXEC_PREFIX_USER_DEFINED @EXEC_PREFIX_USER_DEFINED@
#cmakedefine MODULEDIR_USER_DEFINED @MODULEDIR_USER_DEFINED@
#cmakedefine SITEMODULEDIR_USER_DEFINED @SITEMODULEDIR_USER_DEFINED@
#cmakedefine DOCDIR_USER_DEFINED @DOCDIR_USER_DEFINED@
#cmakedefine LISPDIR_USER_DEFINED @LISPDIR_USER_DEFINED@
#cmakedefine EARLY_PACKAGE_DIRECTORIES_USER_DEFINED @EARLY_PACKAGE_DIRECTORIES_USER_DEFINED@
#cmakedefine LATE_PACKAGE_DIRECTORIES_USER_DEFINED @LATE_PACKAGE_DIRECTORIES_USER_DEFINED@
#cmakedefine LAST_PACKAGE_DIRECTORIES_USER_DEFINED @LAST_PACKAGE_DIRECTORIES_USER_DEFINED@
#cmakedefine PACKAGE_PATH_USER_DEFINED @PACKAGE_PATH_USER_DEFINED@
#cmakedefine SITELISPDIR_USER_DEFINED @SITELISPDIR_USER_DEFINED@
#cmakedefine ARCHLIBDIR_USER_DEFINED @ARCHLIBDIR_USER_DEFINED@
#cmakedefine ETCDIR_USER_DEFINED @ETCDIR_USER_DEFINED@
#cmakedefine INFODIR_USER_DEFINED @INFODIR_USER_DEFINED@
#cmakedefine INFOPATH_USER_DEFINED @INFOPATH_USER_DEFINED@

#cmakedefine USE_PARAM_H @USE_PARAM_H@

#ifndef BITS_PER_CHAR
#define BITS_PER_CHAR 8
#endif
#define SHORTBITS (SIZEOF_SHORT * BITS_PER_CHAR)
#define INTBITS (SIZEOF_INT * BITS_PER_CHAR)
#define LONGBITS (SIZEOF_LONG * BITS_PER_CHAR)
#define LONG_LONG_BITS (SIZEOF_LONG_LONG * BITS_PER_CHAR)
#define VOID_P_BITS (SIZEOF_VOID_P * BITS_PER_CHAR)
#define DOUBLE_BITS (SIZEOF_DOUBLE * BITS_PER_CHAR)

#ifdef __cplusplus
# define INLINE_HEADER inline static 
#elif defined (FORCE_INLINE_FUNCTION_DEFINITION)
#  define INLINE_HEADER
#elif (defined ( __STDC_VERSION__) &&  __STDC_VERSION__ >= 199901L) \
  || ! defined (__GNUC__) || ! defined(emacs)
# define INLINE_HEADER inline static
#else
# define INLINE_HEADER inline extern
#endif

#ifdef FORCE_INLINE_FUNCTION_DEFINITION
# define MODULE_API_INLINE_HEADER MODULE_API
#else
# define MODULE_API_INLINE_HEADER INLINE_HEADER
#endif

#define DECLARE_INLINE_HEADER(header) \
  INLINE_HEADER header ; INLINE_HEADER header

#define DECLARE_INLINE_MODULE_API(header) \
  MODULE_API_INLINE_HEADER header ; MODULE_API_INLINE_HEADER header

#ifndef NOT_C_CODE
# if defined (__cplusplus)
#  define EXTERN_C extern "C"
# else
#  define EXTERN_C extern
# endif
#endif

#ifdef __GNUC__
#define enum_field(enumeration_type) enum enumeration_type
#else
#define enum_field(enumeration_type) unsigned int
#endif

#ifdef HAVE_SIGSETJMP
# define SETJMP(x) sigsetjmp (x, 0)
# define LONGJMP(x, y) siglongjmp (x, y)
# define JMP_BUF sigjmp_buf
#else
# define SETJMP(x) setjmp (x)
# define LONGJMP(x, y) longjmp (x, y)
# define JMP_BUF jmp_buf
#endif

#define USE_C_FONT_LOCK

#endif
