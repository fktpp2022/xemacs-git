#ifndef _SRC_CONFIG_H_
#define _SRC_CONFIG_H_

/* #undef XEMACS */

/* #undef EMACS_PROGNAME */
#define EMACS_DUMP_FILE_NAME "xemacs.dmp"
#define SHEBANG_PROGNAME "xemacs-script"

#define EMACS_CONFIGURATION "x86_64-unknown-"
/* #undef EMACS_CONFIG_OPTIONS */

#define EMACS_MAJOR_VERSION 21
#define EMACS_MINOR_VERSION 5
#define EMACS_BETA_VERSION 36
#define EMACS_VERSION "21.5-b36"
#define XEMACS_CODENAME "leeks"
#define XEMACS_RELEASE_DATE "2025-06-14"

/* #undef config_opsysfile */
/* #undef config_machfile */

#define USE_GCC 1
#define USE_GPLUSPLUS 0
/* #undef USE_UNION_TYPE */

#define HAVE_UNIX_PROCESSES 0
#define HAVE_GLIBC 1

#define REL_ALLOC 0

#define HAVE_TTY 1
#define HAVE_NCURSES 0
#define HAVE_GPM 0

#define HAVE_MS_WINDOWS 0
#define HAVE_X_WINDOWS 1
#define HAVE_X11 1
/* #undef HAVE_GTK */
/* #undef HAVE_GTK2 */
/* #undef HAVE_GTK3 */
/* #undef HAVE_PANGO */
/* #undef HAVE_PANGOXFT */

#define HAVE_XFT 0
#define HAVE_FONTCONFIG 0

#define HAVE_XIM 0
#define HAVE_CANNA 0
#define HAVE_WNN 0
#define WNN6 0

#define HAVE_XPM 1
#define HAVE_XFACE 0
#define HAVE_JPEG 0
#define HAVE_TIFF 0
#define HAVE_GIF 0
#define HAVE_PNG 0
#define HAVE_ZLIB 0

#define HAVE_SOUND 0
#define HAVE_NATIVE_SOUND 0
#define HAVE_ALSA_SOUND 0
#define HAVE_NAS_SOUND 0
#define HAVE_ESD_SOUND 0

#define HAVE_DATABASE 0
#define HAVE_DBM 0
#define HAVE_BERKELEY_DB 0
#define HAVE_LDAP 0
#define HAVE_POSTGRESQL 0

#define CLASH_DETECTION 0

#define HAVE_SHLIB 0
#define HAVE_DLOPEN 0
#define HAVE_MODULES 0

/* #undef WITH_TLS */
/* #undef HAVE_OPENSSL */
/* #undef HAVE_GNUTLS */
/* #undef HAVE_NSS */

/* #undef WITH_NUMBER_TYPES */
/* #undef WITH_GMP */
/* #undef WITH_MPIR */
/* #undef WITH_MP */
/* #undef WITH_OPENSSL_BIGNUM */

#define HAVE_MENUBARS 1
#define HAVE_SCROLLBARS 1
#define HAVE_DIALOGS 1
#define HAVE_TOOLBARS 1
#define HAVE_WIDGETS 1

#define LWLIB_USES_MOTIF 0
#define LWLIB_USES_ATHENA 0
#define LWLIB_MENUBARS_LUCID 1
#define LWLIB_MENUBARS_MOTIF 0
#define LWLIB_SCROLLBARS_LUCID 1
#define LWLIB_SCROLLBARS_MOTIF 0
#define LWLIB_SCROLLBARS_ATHENA 0
#define LWLIB_SCROLLBARS_ATHENA3D 0
#define LWLIB_DIALOGS_MOTIF 0
#define LWLIB_DIALOGS_ATHENA 0
#define LWLIB_DIALOGS_ATHENA3D 0
#define LWLIB_TABS_LUCID 1
#define LWLIB_WIDGETS_MOTIF 0
#define LWLIB_WIDGETS_ATHENA 0

#define HAVE_ATHENA_3D 0
#define HAVE_ATHENA_I18N 0

#define HAVE_LUCID_WIDGETS 1
#define HAVE_LUCID_MENUBARS 1
#define HAVE_LUCID_SCROLLBARS 1
#define HAVE_LUCID_DIALOGS 1
#define HAVE_MOTIF_WIDGETS 0
#define HAVE_MOTIF_MENUBARS 0
#define HAVE_MOTIF_SCROLLBARS 0
#define HAVE_MOTIF_DIALOGS 0
#define HAVE_ATHENA_WIDGETS 0
#define HAVE_ATHENA_MENUBARS 0
#define HAVE_ATHENA_SCROLLBARS 0
#define HAVE_ATHENA_DIALOGS 0
/* #undef HAVE_GTK_WIDGETS */
/* #undef HAVE_GTK_MENUBARS */
/* #undef HAVE_GTK_SCROLLBARS */
/* #undef HAVE_GTK_DIALOGS */
/* #undef HAVE_GNOME_DIALOGS */
#define HAVE_MSW_WIDGETS 0
#define HAVE_MSW_MENUBARS 0
#define HAVE_MSW_SCROLLBARS 0
#define HAVE_MSW_DIALOGS 0

#define HAVE_GUI_OBJECTS 0
#define HAVE_POPUPS 0
#define HAVE_WINDOW_SYSTEM 0
#define HAVE_XLIKE 0
#define HAVE_X_WIDGETS 0

#if HAVE_DIALOGS && (LWLIB_DIALOGS_MOTIF || LWLIB_DIALOGS_ATHENA || LWLIB_DIALOGS_ATHENA3D)
#define HAVE_X_DIALOGS 1
#endif

#define HAVE_UNIXOID_EVENT_LOOP 0

#define HAVE_WMCOMMAND 0
#define USE_XFONTSET 0
#define UNICODE_INTERNAL 1

#define HAVE_BALLOON_HELP 0
#define HAVE_DRAGNDROP 0
#define HAVE_CDE 0
#define EXTERNAL_WIDGET 0

#define HAVE_TOOLTALK 0
#define TOOLTALK 0

#define HAVE_SOCKS 0
#define HAVE_DNET 0

#define IPV6_CANONICALIZE 0

#define MAIL_USE_POP 0
#define KERBEROS 0
#define HESIOD 0
#define MAIL_LOCK_LOCKF 0
#define MAIL_LOCK_FLOCK 0
#define MAIL_LOCK_DOT 0
#define MAIL_LOCK_LOCKING 0
#define MAIL_LOCK_MMDF 0

#define HAVE_MKSTEMP 0

#define INHIBIT_SITE_LISP 0
#define INHIBIT_SITE_MODULES 0

#define DUMP_IN_EXEC 0

#define SUNPRO 0
#define USAGE_TRACKING 0

#define DEBUG_XEMACS 1
#define USE_ASSERTIONS 0
#define MEMORY_USAGE_STATS 0
#define QUANTIFY 0
#define PURIFY 0
#define USE_VALGRIND 0
#define QUICK_BUILD 0
#define BATCH_COMPILER_RUNS 0

/* #undef ERROR_CHECK_BYTE_CODE */
/* #undef ERROR_CHECK_DISPLAY */
/* #undef ERROR_CHECK_EXTENTS */
/* #undef ERROR_CHECK_GC */
/* #undef ERROR_CHECK_GLYPHS */
/* #undef ERROR_CHECK_MALLOC */
/* #undef ERROR_CHECK_STRUCTURES */
/* #undef ERROR_CHECK_TEXT */
/* #undef ERROR_CHECK_TYPES */

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_VOID_P 8
#define SIZEOF_DOUBLE 8
#define SIZEOF_OFF_T 8

#define WORDS_BIGENDIAN 0
#define HAVE_FSEEKO 0
#define HAVE_SNPRINTF 1

#define HAVE_ALLOCA 0
#define HAVE_ALLOCA_H 0
#define C_ALLOCA 0

#define HAVE_SOCKETS 0
#define HAVE_SOCKADDR_SUN_LEN 0
#define HAVE_MULTICAST 0
#define HAVE_SYSVIPC 0
#define HAVE_LOCKF 0
#define HAVE_FLOCK 0

#define HAVE_TERMIOS 0
#define HAVE_TERMIO 0

#define HAVE_SIGSETJMP 0

#define USE_GNU_MAKE 0
#define NEED_TO_HANDLE_21_4_CODE 0

/* #undef inline */
/* #undef const */

/* #undef ssize_t */
/* #undef size_t */
/* #undef pid_t */
/* #undef mode_t */
/* #undef off_t */
/* #undef uid_t */
/* #undef gid_t */
/* #undef socklen_t */

/* #undef TYPEOF */
/* #undef STACK_TRACE_EYE_CATCHER */
/* #undef OS_RELEASE */

/* #undef PREFIX_USER_DEFINED */
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

/* #undef USE_PARAM_H */

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
