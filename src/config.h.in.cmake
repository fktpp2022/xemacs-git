#ifndef _SRC_CONFIG_H_
#define _SRC_CONFIG_H_

#define XEMACS 1

#define EMACS_PROGNAME "@EMACS_PROGNAME@"
#define EMACS_DUMP_FILE_NAME "@EMACS_DUMP_FILE_NAME@"
#define SHEBANG_PROGNAME "@SHEBANG_PROGNAME@"
#define EMACS_CONFIGURATION "@EMACS_CONFIGURATION@"
#define EMACS_CONFIG_OPTIONS "@EMACS_CONFIG_OPTIONS@"

#define EMACS_MAJOR_VERSION @EMACS_MAJOR_VERSION@
#define EMACS_MINOR_VERSION @EMACS_MINOR_VERSION@
#define EMACS_BETA_VERSION @EMACS_BETA_VERSION@
#define EMACS_VERSION "@EMACS_VERSION@"
#define XEMACS_CODENAME "@XEMACS_CODENAME@"
#define XEMACS_RELEASE_DATE "@XEMACS_RELEASE_DATE@"

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#define _XOPEN_SOURCE 600
#ifndef _ALL_SOURCE
#define _ALL_SOURCE 1
#endif
#define _GNU_SOURCE 1
#define __EXTENSIONS__ 1

#cmakedefine TYPEOF @TYPEOF@
#cmakedefine STACK_TRACE_EYE_CATCHER @STACK_TRACE_EYE_CATCHER@
#cmakedefine INHIBIT_SITE_LISP 1
/* #undef INHIBIT_SITE_MODULES */

#define HAVE_UNIX_PROCESSES 1
#cmakedefine HAVE_GLIBC 1
#cmakedefine HAVE_LIBMCHECK 1
/* #undef HAVE_MALLOC_WARNING */
/* #undef REL_ALLOC */

#cmakedefine HAVE_ALLOCA 1
#cmakedefine HAVE_ALLOCA_H 1
/* #undef C_ALLOCA */

#cmakedefine HAVE_TTY 1
/* #undef HAVE_MS_WINDOWS */
/* #undef HAVE_MSG_SELECT */
#cmakedefine HAVE_X_WINDOWS 1
#cmakedefine HAVE_FONTCONFIG 1
#cmakedefine HAVE_FCCONFIGGETRESCANINTERVAL 1
#cmakedefine HAVE_FCCONFIGSETRESCANINTERVAL 1
#cmakedefine HAVE_XFT 1
#cmakedefine HAVE_XFT_MENUBARS 1
#cmakedefine HAVE_XFT_TABS 1
#cmakedefine HAVE_XFT_GAUGES 1

#ifdef HAVE_X_WINDOWS
#define _CONST_X_STRING
#endif

/* #undef HAVE_PANGO */
/* #undef HAVE_PANGOXFT */
/* #undef HAVE_GTK */
/* #undef HAVE_GTK2 */
/* #undef HAVE_GTK3 */

#cmakedefine HAVE_XREGISTERIMINSTANTIATECALLBACK 1
#cmakedefine XREGISTERIMINSTANTIATECALLBACK_NONSTANDARD_PROTOTYPE 1
#cmakedefine THIS_IS_X11R6 1
#cmakedefine HAVE_XCONVERTCASE 1
#cmakedefine HAVE_XTREGISTERDRAWABLE 1
#cmakedefine HAVE_BALLOON_HELP 1

/* Headers */
#cmakedefine HAVE_MCHECK_H 1
#cmakedefine HAVE_A_OUT_H 1
#cmakedefine HAVE_ELF_H 1
/* #undef HAVE_CYGWIN_VERSION_H */
#cmakedefine HAVE_FCNTL_H 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_LIBGEN_H 1
#cmakedefine HAVE_LOCALE_H 1
#cmakedefine HAVE_WCHAR_H 1
/* #undef HAVE_MACH_MACH_H */
#cmakedefine HAVE_SYS_PARAM_H 1
/* #undef HAVE_SYS_PSTAT_H */
#cmakedefine HAVE_SYS_RESOURCE_H 1
#cmakedefine HAVE_SYS_TIME_H 1
#cmakedefine HAVE_SYS_TIMEB_H 1
#cmakedefine HAVE_SYS_TIMES_H 1
#cmakedefine HAVE_SYS_UN_H 1
#cmakedefine HAVE_SYS_USER_H 1
#cmakedefine HAVE_SYS_VLIMIT_H 1
#cmakedefine HAVE_ULIMIT_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine HAVE_SYS_WAIT_H 1
#cmakedefine HAVE_LIBINTL_H 1
#cmakedefine HAVE_X11_XLOCALE_H 1

#cmakedefine WORDS_BIGENDIAN 1
#define HAVE_LONG_FILE_NAMES 1
#cmakedefine CLASH_DETECTION 1

#cmakedefine HAVE_DLOPEN 1
#cmakedefine HAVE_DLERROR 1
#cmakedefine HAVE_SHLIB 1

#cmakedefine HAVE_STRSIGNAL 1
#cmakedefine HAVE_TIMEVAL 1
#cmakedefine HAVE_TM_ZONE 1
/* #undef HAVE_TZNAME */
#cmakedefine HAVE_GETLOADAVG 1
#cmakedefine HAVE_H_ERRNO 1
#cmakedefine HAVE_MMAP 1
#cmakedefine HAVE_STRCOLL 1
#cmakedefine HAVE_GETPGRP 1
#cmakedefine GETPGRP_VOID 1
#cmakedefine HAVE_INVERSE_HYPERBOLIC 1

/* Functions */
#cmakedefine HAVE_CBRT 1
#cmakedefine HAVE_CLOSEDIR 1
#cmakedefine HAVE_DUP2 1
#cmakedefine HAVE_EACCESS 1
#cmakedefine HAVE_FMOD 1
#cmakedefine HAVE_FPATHCONF 1
#cmakedefine HAVE_FREXP 1
#cmakedefine HAVE_FSYNC 1
#cmakedefine HAVE_FTIME 1
#cmakedefine HAVE_FTRUNCATE 1
#cmakedefine HAVE_GETADDRINFO 1
#cmakedefine HAVE_GETHOSTNAME 1
#cmakedefine HAVE_GETNAMEINFO 1
#cmakedefine HAVE_GETPAGESIZE 1
#cmakedefine HAVE_GETRLIMIT 1
#cmakedefine HAVE_GETTIMEOFDAY 1
#cmakedefine HAVE_GETCWD 1
#cmakedefine HAVE_LINK 1
#cmakedefine HAVE_LOGB 1
#cmakedefine HAVE_LRAND48 1
/* #undef HAVE_MATHERR */
#cmakedefine HAVE_MKDIR 1
#cmakedefine HAVE_MKTIME 1
#cmakedefine HAVE_PERROR 1
#cmakedefine HAVE_POLL 1
#cmakedefine HAVE_RANDOM 1
#cmakedefine HAVE_READLINK 1
#cmakedefine HAVE_RENAME 1
/* #undef HAVE_RES_INIT */
#cmakedefine HAVE_RMDIR 1
#cmakedefine HAVE_SELECT 1
#cmakedefine HAVE_SETITIMER 1
#cmakedefine HAVE_SETPGID 1
#cmakedefine HAVE_SETSID 1
#cmakedefine HAVE_SIGBLOCK 1
#cmakedefine HAVE_SIGHOLD 1
#cmakedefine HAVE_SIGPROCMASK 1
#cmakedefine HAVE_SNPRINTF 1
#cmakedefine HAVE_STRERROR 1
#cmakedefine HAVE_SYMLINK 1
#cmakedefine HAVE_TZSET 1
#cmakedefine HAVE_ULIMIT 1
#cmakedefine HAVE_UMASK 1
#cmakedefine HAVE_USLEEP 1
#cmakedefine HAVE_VLIMIT 1
#cmakedefine HAVE_VSNPRINTF 1
#cmakedefine HAVE_WAITPID 1
#cmakedefine HAVE_WCSCMP 1
#cmakedefine HAVE_WCSLEN 1
#cmakedefine HAVE_UTIME 1
#cmakedefine HAVE_SIGSETJMP 1

/* PTY functions */
#cmakedefine HAVE_GETPT 1
#cmakedefine HAVE_OPENPTY 1
#cmakedefine HAVE_GRANTPT 1
#cmakedefine HAVE_UNLOCKPT 1
#cmakedefine HAVE_PTSNAME 1
#cmakedefine HAVE_KILLPG 1
#cmakedefine HAVE_TCGETPGRP 1
#cmakedefine HAVE_PTY_H 1

#cmakedefine HAVE_SOCKETS 1
/* #undef HAVE_SOCKADDR_SUN_LEN */
#cmakedefine HAVE_MULTICAST 1
#cmakedefine HAVE_SYSVIPC 1
#cmakedefine HAVE_LOCKF 1
#cmakedefine HAVE_FLOCK 1

#define SYSV_SYSTEM_DIR 1
#cmakedefine HAVE_TERMIOS 1
/* #undef HAVE_TERMIO */
#cmakedefine NO_TERMIO 1
#define SIGNALS_VIA_CHARACTERS 1
#define NLIST_STRUCT 1

#cmakedefine HAVE_U_INT8_T 1
#cmakedefine HAVE_U_INT16_T 1
#cmakedefine HAVE_U_INT32_T 1

/* Image support */
#cmakedefine HAVE_XPM 1
/* #undef HAVE_XFACE */
#cmakedefine HAVE_JPEG 1
#cmakedefine HAVE_TIFF 1
/* #undef HAVE_GIF */
#cmakedefine HAVE_PNG 1
/* #undef HAVE_ZLIB */

/* Sound */
#cmakedefine HAVE_NATIVE_SOUND 1
#cmakedefine SOUNDCARD_H_FILE "@SOUNDCARD_H_FILE@"
/* #undef HAVE_ALSA_SOUND */
/* #undef HAVE_NAS_SOUND */
/* #undef HAVE_ESD_SOUND */

/* Database */
/* #undef HAVE_DBM */
/* #undef HAVE_BERKELEY_DB */
/* #undef HAVE_DATABASE */
/* #undef HAVE_LDAP */
/* #undef HAVE_POSTGRESQL */

/* XIM */
#cmakedefine HAVE_XIM 1
#cmakedefine XIM_XLIB 1
/* #undef XIM_MOTIF */
/* #undef HAVE_CANNA */
/* #undef HAVE_WNN */
/* #undef WNN6 */

/* Toolkit */
#cmakedefine HAVE_XAUTH 1
#cmakedefine HAVE_NCURSES 1
#cmakedefine CURSES_H_FILE "@CURSES_H_FILE@"
#cmakedefine TERM_H_FILE "@TERM_H_FILE@"

/* Debug / Error checking */
#cmakedefine USE_ASSERTIONS 1
#cmakedefine ERROR_CHECK_BYTE_CODE 1
#cmakedefine ERROR_CHECK_DISPLAY 1
#cmakedefine ERROR_CHECK_EXTENTS 1
#cmakedefine ERROR_CHECK_GC 1
#cmakedefine ERROR_CHECK_GLYPHS 1
#cmakedefine ERROR_CHECK_MALLOC 1
#cmakedefine ERROR_CHECK_STRUCTURES 1
#cmakedefine ERROR_CHECK_TEXT 1
#cmakedefine ERROR_CHECK_TYPES 1
#cmakedefine DEBUG_XEMACS 1
#cmakedefine MEMORY_USAGE_STATS 1
/* #undef QUANTIFY */
/* #undef PURIFY */
/* #undef USE_VALGRIND */
/* #undef EXTERNAL_WIDGET */

#cmakedefine USE_GCC 1
/* #undef USE_GPLUSPLUS */
/* #undef USE_UNION_TYPE */

/* #undef HAVE_CDE */
/* #undef HAVE_DRAGNDROP */
#cmakedefine HAVE_WMCOMMAND 1
/* #undef UNICODE_INTERNAL */
#cmakedefine USE_XFONTSET 1

/* Menubar/scrollbar/toolbar/widget toolkit */
#cmakedefine HAVE_MENUBARS 1
#cmakedefine HAVE_SCROLLBARS 1
/* #undef HAVE_DIALOGS */
#cmakedefine HAVE_TOOLBARS 1
/* #undef HAVE_WIDGETS */

/* #undef LWLIB_USES_MOTIF */
/* #undef LWLIB_USES_ATHENA */
#cmakedefine LWLIB_MENUBARS_LUCID 1
/* #undef LWLIB_MENUBARS_MOTIF */
#cmakedefine LWLIB_SCROLLBARS_LUCID 1
/* #undef LWLIB_SCROLLBARS_MOTIF */
/* #undef LWLIB_SCROLLBARS_ATHENA */
/* #undef LWLIB_SCROLLBARS_ATHENA3D */
/* #undef LWLIB_DIALOGS_MOTIF */
/* #undef LWLIB_DIALOGS_ATHENA */
/* #undef LWLIB_DIALOGS_ATHENA3D */
/* #undef LWLIB_TABS_LUCID */
/* #undef LWLIB_WIDGETS_MOTIF */
/* #undef LWLIB_WIDGETS_ATHENA */
/* #undef HAVE_ATHENA_3D */
/* #undef HAVE_ATHENA_I18N */

/* TLS */
/* #undef WITH_TLS */
/* #undef HAVE_NSS */
/* #undef HAVE_GNUTLS */
/* #undef HAVE_OPENSSL */

/* Number types */
/* #undef WITH_NUMBER_TYPES */
/* #undef WITH_GMP */
/* #undef WITH_MPIR */
/* #undef WITH_MP */
/* #undef WITH_OPENSSL_BIGNUM */

/* #undef HAVE_SOCKS */
/* #undef HAVE_DNET */
/* #undef IPV6_CANONICALIZE */
/* #undef HAVE_GPM */

/* Mail */
/* #undef MAIL_USE_POP */
/* #undef KERBEROS */
/* #undef HESIOD */
/* #undef MAIL_LOCK_LOCKF */
/* #undef MAIL_LOCK_FLOCK */
#cmakedefine MAIL_LOCK_DOT 1
/* #undef MAIL_LOCK_LOCKING */
/* #undef MAIL_LOCK_MMDF */
#cmakedefine HAVE_MKSTEMP 1

#cmakedefine PREFIX_USER_DEFINED 1
/* #undef EXEC_PREFIX_USER_DEFINED */
/* #undef MODULEDIR_USER_DEFINED */
/* #undef SITEMODULEDIR_USER_DEFINED */
/* #undef DOCDIR_USER_DEFINED */
/* #undef LISPDIR_USER_DEFINED */
/* #undef EARLY_PACKAGE_DIRECTORIES_USER_DEFINED */
/* #undef LATE_PACKAGE_DIRECTORIES_USER_DEFINED */
/* #undef LAST_PACKAGE_DIRECTORIES_USER_DEFINED */
/* #undef PACKAGE_PATH_USER_DEFINED */
/* #undef SITELISPDIR_USER_DEFINED */
/* #undef ARCHLIBDIR_USER_DEFINED */
/* #undef ETCDIR_USER_DEFINED */
/* #undef INFODIR_USER_DEFINED */
/* #undef INFOPATH_USER_DEFINED */

/* #undef DUMP_IN_EXEC */
/* #undef SUNPRO */
/* #undef USAGE_TRACKING */
/* #undef TOOLTALK */
/* #undef HAVE_TOOLTALK */
/* #undef QUICK_BUILD */
/* #undef BATCH_COMPILER_RUNS */

#cmakedefine HAVE_MODULES 1
#cmakedefine HAVE_SHLIB 1

/* --- End of #undef section. s&m file includes follow. --- */

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

#define config_opsysfile "@XEMACS_CONFIG_OPSYSFILE@"
#ifdef config_opsysfile
#include config_opsysfile
#endif

/* #undef USE_PARAM_H */
#ifdef USE_PARAM_H
# ifndef NOT_C_CODE
#  include <sys/param.h>
# endif
#endif

/* configure does not set config_machfile on x86_64 linux */
/* #undef config_machfile */
#ifdef config_machfile
#include config_machfile
#endif

#if defined (__cplusplus) && !defined (NOT_C_CODE)
}
#endif

#define USER_FULL_NAME pw->pw_gecos
#define XEMACS_WANTS_C_ALLOCA

/* #undef ssize_t */
/* #undef size_t */
/* #undef pid_t */
/* #undef mode_t */
/* #undef off_t */
/* #undef uid_t */
/* #undef gid_t */
/* #undef socklen_t */
/* #undef inline */
/* #undef const */

#define SIZEOF_SHORT @SIZEOF_SHORT@
#define SIZEOF_INT @SIZEOF_INT@
#define SIZEOF_LONG @SIZEOF_LONG@
#define SIZEOF_LONG_LONG @SIZEOF_LONG_LONG@
#define SIZEOF_VOID_P @SIZEOF_VOID_P@
#define SIZEOF_DOUBLE @SIZEOF_DOUBLE@
#define SIZEOF_OFF_T @SIZEOF_OFF_T@

/* Large file support */
#cmakedefine HAVE_FSEEKO 1

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

#ifndef SYSTEM_TYPE
/* #undef SYSTEM_TYPE */
#endif

#if !defined (MC_ALLOC) || 1
# define ALLOC_TYPE_STATS 1
#endif

/* Window system derived defines */
#if defined (HAVE_GTK) || defined (HAVE_X_WINDOWS) || defined (HAVE_MS_WINDOWS)
#define HAVE_WINDOW_SYSTEM
#endif

#if defined (HAVE_MENUBARS) || defined (HAVE_SCROLLBARS) || defined (HAVE_DIALOGS) || defined (HAVE_TOOLBARS) || defined (HAVE_WIDGETS)
#define HAVE_GUI_OBJECTS
#endif

#if defined (HAVE_MENUBARS) || defined (HAVE_DIALOGS)
#define HAVE_POPUPS
#endif

#if defined (HAVE_GTK) || defined (HAVE_X_WINDOWS)
#define HAVE_XLIKE
#endif

#if defined (HAVE_WIDGETS) && (defined (LWLIB_WIDGETS_MOTIF) || defined (LWLIB_WIDGETS_ATHENA))
#define HAVE_X_WIDGETS
#endif

#if defined(HAVE_DIALOGS) && (defined(LWLIB_DIALOGS_MOTIF) || defined(LWLIB_DIALOGS_ATHENA) || defined(LWLIB_DIALOGS_ATHENA3D))
#define HAVE_X_DIALOGS 1
#endif

#ifdef HAVE_X_WINDOWS
#ifndef NeedFunctionPrototypes
#define NeedFunctionPrototypes 1
#endif
#ifndef FUNCPROTO
#define FUNCPROTO 15
#endif
#endif

#if defined (HAVE_X_WINDOWS) || defined (HAVE_TTY) || defined (HAVE_MSG_SELECT) || defined (HAVE_GTK)
#define HAVE_UNIXOID_EVENT_LOOP
#endif

/* Mule */
#ifndef UNICODE_INTERNAL
#define ALLOW_ALGORITHMIC_CONVERSION_TABLES
#endif

/* Do we need to be able to run code compiled by and written for 21.4? */
#define NEED_TO_HANDLE_21_4_CODE 1

/* #undef USE_GNU_MAKE */

/* Basic types */
#ifndef BITS_PER_CHAR
#define BITS_PER_CHAR 8
#endif
#define SHORTBITS (SIZEOF_SHORT * BITS_PER_CHAR)
#define INTBITS (SIZEOF_INT * BITS_PER_CHAR)
#define LONGBITS (SIZEOF_LONG * BITS_PER_CHAR)
#define LONG_LONG_BITS (SIZEOF_LONG_LONG * BITS_PER_CHAR)
#define VOID_P_BITS (SIZEOF_VOID_P * BITS_PER_CHAR)
#define DOUBLE_BITS (SIZEOF_DOUBLE * BITS_PER_CHAR)

/* Inlining */
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

/* Error check aggregation */
#ifdef ERROR_CHECK_ALL
#define ERROR_CHECK_BYTE_CODE 1
#define ERROR_CHECK_DISPLAY 1
#define ERROR_CHECK_EXTENTS 1
#define ERROR_CHECK_GC 1
#define ERROR_CHECK_GLYPHS 1
#define ERROR_CHECK_MALLOC 1
#define ERROR_CHECK_STRUCTURES 1
#define ERROR_CHECK_TEXT 1
#define ERROR_CHECK_TYPES 1
#endif

#if defined (ERROR_CHECK_BYTE_CODE) || defined (ERROR_CHECK_DISPLAY) || defined (ERROR_CHECK_EXTENTS) || defined (ERROR_CHECK_GC) || defined (ERROR_CHECK_GLYPHS) || defined (ERROR_CHECK_MALLOC) || defined (ERROR_CHECK_STRUCTURES) || defined (ERROR_CHECK_TEXT) || defined (ERROR_CHECK_TYPES)
#define ERROR_CHECK_ANY
#endif

#if !defined (USE_GPLUSPLUS)
#define XEMACS_DEFS_NEEDS_INLINE_DECLS
#endif

/* alloca handling */
#ifndef NOT_C_CODE
#if defined (__CYGWIN__)
#include <alloca.h>
#elif defined (__GNUC__)
#ifndef alloca
#define alloca __builtin_alloca
#endif
#elif defined (HAVE_ALLOCA_H)
#include <alloca.h>
#elif ! defined (alloca)
#ifdef C_ALLOCA
#define alloca xemacs_c_alloca
#else
void *alloca ();
#endif
#endif
#endif /* NOT_C_CODE */

#endif /* _SRC_CONFIG_H_ */
