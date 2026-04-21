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

#cmakedefine HAVE_TTY
#cmakedefine HAVE_NCURSES
#cmakedefine HAVE_GPM

#cmakedefine HAVE_MS_WINDOWS
#cmakedefine HAVE_X_WINDOWS
#cmakedefine HAVE_X11
#cmakedefine HAVE_GTK
#cmakedefine HAVE_GTK2
#cmakedefine HAVE_GTK3
#cmakedefine HAVE_PANGO
#cmakedefine HAVE_PANGOXFT

#cmakedefine HAVE_XFT
#cmakedefine HAVE_FONTCONFIG

#cmakedefine HAVE_XIM
#cmakedefine HAVE_CANNA
#cmakedefine HAVE_WNN
#cmakedefine WNN6

#cmakedefine HAVE_XPM
#cmakedefine HAVE_XFACE
#cmakedefine HAVE_JPEG
#cmakedefine HAVE_TIFF
#cmakedefine HAVE_GIF
#cmakedefine HAVE_PNG
#cmakedefine HAVE_ZLIB

#cmakedefine HAVE_SOUND
#cmakedefine HAVE_NATIVE_SOUND
#cmakedefine HAVE_ALSA_SOUND
#cmakedefine HAVE_NAS_SOUND
#cmakedefine HAVE_ESD_SOUND

#cmakedefine HAVE_DATABASE
#cmakedefine HAVE_DBM
#cmakedefine HAVE_BERKELEY_DB
#cmakedefine HAVE_LDAP
#cmakedefine HAVE_POSTGRESQL

#cmakedefine01 CLASH_DETECTION

#cmakedefine HAVE_SHLIB
#cmakedefine HAVE_DLOPEN
#cmakedefine HAVE_MODULES

#cmakedefine WITH_TLS
#cmakedefine HAVE_OPENSSL
#cmakedefine HAVE_GNUTLS
#cmakedefine HAVE_NSS

#cmakedefine WITH_NUMBER_TYPES
#cmakedefine WITH_GMP
#cmakedefine WITH_MPIR
#cmakedefine WITH_MP
#cmakedefine WITH_OPENSSL_BIGNUM

#cmakedefine HAVE_MENUBARS
#cmakedefine HAVE_SCROLLBARS
#cmakedefine HAVE_DIALOGS
#cmakedefine HAVE_TOOLBARS
#cmakedefine HAVE_WIDGETS

#cmakedefine LWLIB_USES_MOTIF
#cmakedefine LWLIB_USES_ATHENA
#cmakedefine LWLIB_MENUBARS_LUCID
#cmakedefine LWLIB_MENUBARS_MOTIF
#cmakedefine LWLIB_SCROLLBARS_LUCID
#cmakedefine LWLIB_SCROLLBARS_MOTIF
#cmakedefine LWLIB_SCROLLBARS_ATHENA
#cmakedefine LWLIB_SCROLLBARS_ATHENA3D
#cmakedefine LWLIB_DIALOGS_MOTIF
#cmakedefine LWLIB_DIALOGS_ATHENA
#cmakedefine LWLIB_DIALOGS_ATHENA3D
#cmakedefine LWLIB_TABS_LUCID
#cmakedefine LWLIB_WIDGETS_MOTIF
#cmakedefine LWLIB_WIDGETS_ATHENA

#cmakedefine HAVE_ATHENA_3D
#cmakedefine HAVE_ATHENA_I18N

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
#cmakedefine HAVE_MSW_WIDGETS
#cmakedefine HAVE_MSW_MENUBARS
#cmakedefine HAVE_MSW_SCROLLBARS
#cmakedefine HAVE_MSW_DIALOGS

#cmakedefine HAVE_GUI_OBJECTS
#cmakedefine HAVE_POPUPS
#cmakedefine HAVE_WINDOW_SYSTEM
#cmakedefine HAVE_XLIKE
#cmakedefine HAVE_X_WIDGETS

#if defined(HAVE_DIALOGS) && (defined(LWLIB_DIALOGS_MOTIF) || defined(LWLIB_DIALOGS_ATHENA) || defined(LWLIB_DIALOGS_ATHENA3D))
#define HAVE_X_DIALOGS 1
#endif

#if defined (HAVE_X_WINDOWS) || defined (HAVE_TTY) || defined (HAVE_MSG_SELECT) || defined (HAVE_GTK)
#define HAVE_UNIXOID_EVENT_LOOP
#endif

#cmakedefine HAVE_WMCOMMAND
#cmakedefine USE_XFONTSET
#cmakedefine UNICODE_INTERNAL

#cmakedefine HAVE_BALLOON_HELP
#cmakedefine HAVE_DRAGNDROP
#cmakedefine HAVE_CDE
#cmakedefine EXTERNAL_WIDGET

#cmakedefine HAVE_TOOLTALK
#cmakedefine TOOLTALK

#cmakedefine HAVE_SOCKS
#cmakedefine HAVE_DNET

#cmakedefine IPV6_CANONICALIZE

#cmakedefine MAIL_USE_POP
#cmakedefine KERBEROS
#cmakedefine HESIOD
#cmakedefine MAIL_LOCK_LOCKF
#cmakedefine MAIL_LOCK_FLOCK
#cmakedefine MAIL_LOCK_DOT
#cmakedefine MAIL_LOCK_LOCKING
#cmakedefine MAIL_LOCK_MMDF

#cmakedefine HAVE_MKSTEMP

#cmakedefine INHIBIT_SITE_LISP
#cmakedefine INHIBIT_SITE_MODULES

#cmakedefine DUMP_IN_EXEC

#cmakedefine SUNPRO
#cmakedefine USAGE_TRACKING

#cmakedefine DEBUG_XEMACS
#cmakedefine USE_ASSERTIONS
#cmakedefine MEMORY_USAGE_STATS
#cmakedefine QUANTIFY
#cmakedefine PURIFY
#cmakedefine USE_VALGRIND
#cmakedefine QUICK_BUILD
#cmakedefine BATCH_COMPILER_RUNS

#cmakedefine ERROR_CHECK_BYTE_CODE
#cmakedefine ERROR_CHECK_DISPLAY
#cmakedefine ERROR_CHECK_EXTENTS
#cmakedefine ERROR_CHECK_GC
#cmakedefine ERROR_CHECK_GLYPHS
#cmakedefine ERROR_CHECK_MALLOC
#cmakedefine ERROR_CHECK_STRUCTURES
#cmakedefine ERROR_CHECK_TEXT
#cmakedefine ERROR_CHECK_TYPES

#if !defined (MC_ALLOC) || 1
# define ALLOC_TYPE_STATS 1
#endif

#ifndef XCDECL
#define XCDECL
#endif

#ifndef SIGTYPE
#define SIGTYPE void XCDECL
#define SIGRETURN return
#endif

#if defined (__cplusplus) && !defined (NOT_C_CODE)
extern "C" {
#endif

#ifdef config_opsysfile
#include config_opsysfile
#endif

#ifdef USE_PARAM_H
# ifndef NOT_C_CODE
#  include <sys/param.h>
# endif
#endif

#ifdef config_machfile
#include config_machfile
#endif

#if defined (__cplusplus) && !defined (NOT_C_CODE)
}
#endif

#define USER_FULL_NAME pw->pw_gecos
#define XEMACS_WANTS_C_ALLOCA

#define SIZEOF_SHORT @SIZEOF_SHORT@
#define SIZEOF_INT @SIZEOF_INT@
#define SIZEOF_LONG @SIZEOF_LONG@
#define SIZEOF_LONG_LONG @SIZEOF_LONG_LONG@
#define SIZEOF_VOID_P @SIZEOF_VOID_P@
#define SIZEOF_DOUBLE @SIZEOF_DOUBLE@
#define SIZEOF_OFF_T @SIZEOF_OFF_T@

#cmakedefine01 WORDS_BIGENDIAN
#cmakedefine HAVE_FSEEKO
#cmakedefine HAVE_SNPRINTF
#cmakedefine HAVE_STRERROR
#cmakedefine HAVE_STRSIGNAL
#cmakedefine HAVE_READLINK
#cmakedefine HAVE_GETCWD
#cmakedefine HAVE_GETTIMEOFDAY
#cmakedefine HAVE_MKDIR
#cmakedefine HAVE_RMDIR
#cmakedefine HAVE_SYS_TIMES_H

#ifdef HAVE_FSEEKO
# define OFF_T off_t
# define FSEEK(stream, offset, whence) fseeko (stream, offset, whence)
# define FTELL(stream) ftello (stream)
#else
# if defined (_MSC_VER) && (_MSC_VER > 1200)
#  define OFF_T INT_64_BIT
#  define SIZEOF_OFF_T 8
#  define FSEEK(stream, offset, whence) _fseeki64 (stream, offset, whence)
#  define FTELL(stream) _ftelli64 (stream)
# else
#  define OFF_T long
#  define FSEEK(stream, offset, whence) fseek (stream, offset, whence)
#  define FTELL(stream) ftell (stream)
# endif
#endif

#cmakedefine HAVE_ALLOCA
#cmakedefine HAVE_ALLOCA_H
#cmakedefine HAVE_LOCALE_H
#cmakedefine01 C_ALLOCA

#cmakedefine HAVE_SOCKETS
#cmakedefine HAVE_SOCKADDR_SUN_LEN
#cmakedefine01 HAVE_MULTICAST
#cmakedefine01 HAVE_SYSVIPC
#cmakedefine HAVE_LOCKF
#cmakedefine HAVE_FLOCK

#cmakedefine HAVE_TERMIOS
#cmakedefine01 HAVE_TERMIO

#cmakedefine HAVE_TIMEVAL
#cmakedefine HAVE_TM_ZONE
#cmakedefine HAVE_TZNAME
#cmakedefine HAVE_SELECT
#cmakedefine HAVE_GETPGRP
#cmakedefine GETPGRP_VOID

#cmakedefine HAVE_SIGSETJMP

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
