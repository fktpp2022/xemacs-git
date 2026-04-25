# XEmacs feature detection - matches autoconf configure baseline
# ============================================================

# --- Type sizes ---
check_type_size("short" SIZEOF_SHORT)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("void*" SIZEOF_VOID_P)
check_type_size("double" SIZEOF_DOUBLE)
check_type_size("off_t" SIZEOF_OFF_T)

include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN_RESULT)
if(WORDS_BIGENDIAN_RESULT)
  set(WORDS_BIGENDIAN 1)
endif()

# --- Header checks ---
check_include_file("locale.h" HAVE_LOCALE_H)
check_include_file("mcheck.h" HAVE_MCHECK_H)
check_include_file("a_out.h" HAVE_A_OUT_H)
check_include_file("elf.h" HAVE_ELF_H)
check_include_file("cygwin/version.h" HAVE_CYGWIN_VERSION_H)
check_include_file("fcntl.h" HAVE_FCNTL_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("libgen.h" HAVE_LIBGEN_H)
check_include_file("wchar.h" HAVE_WCHAR_H)
check_include_file("mach/mach.h" HAVE_MACH_MACH_H)
check_include_file("sys/param.h" HAVE_SYS_PARAM_H)
check_include_file("sys/pstat.h" HAVE_SYS_PSTAT_H)
check_include_file("sys/resource.h" HAVE_SYS_RESOURCE_H)
check_include_file("sys/time.h" HAVE_SYS_TIME_H)
check_include_file("sys/timeb.h" HAVE_SYS_TIMEB_H)
check_include_file("sys/times.h" HAVE_SYS_TIMES_H)
check_include_file("sys/un.h" HAVE_SYS_UN_H)
check_include_file("sys/user.h" HAVE_SYS_USER_H)
check_include_file("sys/vlimit.h" HAVE_SYS_VLIMIT_H)
check_include_file("ulimit.h" HAVE_ULIMIT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("sys/wait.h" HAVE_SYS_WAIT_H)
check_include_file("libintl.h" HAVE_LIBINTL_H)
check_include_file("alloca.h" HAVE_ALLOCA_H)
check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_include_file("pty.h" HAVE_PTY_H)
check_include_file("sys/pty.h" HAVE_SYS_PTY_H)
check_include_file("sys/ptyio.h" HAVE_SYS_PTYIO_H)
check_include_file("libutil.h" HAVE_LIBUTIL_H)
check_include_file("util.h" HAVE_UTIL_H)
check_include_file("stropts.h" HAVE_STROPTS_H)
check_include_file("sys/soundcard.h" HAVE_SYS_SOUNDCARD_H)
check_include_file("machine/soundcard.h" HAVE_MACHINE_SOUNDCARD_H)
check_include_file("linux/soundcard.h" HAVE_LINUX_SOUNDCARD_H)

# --- Struct/type checks ---
check_c_source_compiles("
  #include <sys/time.h>
  int main(void) { struct timeval tv; return 0; }
" HAVE_TIMEVAL)

check_c_source_compiles("
  #include <time.h>
  int main(void) { struct tm t; (void) t.tm_zone; return 0; }
" HAVE_TM_ZONE)

check_c_source_compiles("
  #include <time.h>
  int main(void) { extern char *tzname[]; (void) tzname[0]; return 0; }
" HAVE_TZNAME)

check_c_source_compiles("
  #include <netdb.h>
  int main(void) { int x = h_errno; return x; }
" HAVE_H_ERRNO)

# u_int types
check_c_source_compiles("
  #include <sys/types.h>
  int main(void) { u_int8_t x = 0; return x; }
" HAVE_U_INT8_T)
check_c_source_compiles("
  #include <sys/types.h>
  int main(void) { u_int16_t x = 0; return x; }
" HAVE_U_INT16_T)
check_c_source_compiles("
  #include <sys/types.h>
  int main(void) { u_int32_t x = 0; return x; }
" HAVE_U_INT32_T)

# --- Alloca detection ---
check_c_source_compiles("
  #include <alloca.h>
  int main(void) { char *p = (char*)alloca(10); return p != 0; }
" HAVE_ALLOCA_WITH_H)
if(HAVE_ALLOCA_WITH_H)
  set(HAVE_ALLOCA 1)
else()
  check_c_source_compiles("
    int main(void) { char *p = (char*)__builtin_alloca(10); return p != 0; }
  " HAVE_ALLOCA_BUILTIN)
  if(HAVE_ALLOCA_BUILTIN)
    set(HAVE_ALLOCA 1)
  endif()
endif()

# --- Function checks ---
check_function_exists(cbrt HAVE_CBRT)
check_function_exists(closedir HAVE_CLOSEDIR)
check_function_exists(dup2 HAVE_DUP2)
check_function_exists(eaccess HAVE_EACCESS)
check_function_exists(fmod HAVE_FMOD)
check_function_exists(fpathconf HAVE_FPATHCONF)
check_function_exists(frexp HAVE_FREXP)
check_function_exists(fsync HAVE_FSYNC)
check_function_exists(ftime HAVE_FTIME)
check_function_exists(ftruncate HAVE_FTRUNCATE)
check_function_exists(getaddrinfo HAVE_GETADDRINFO)
check_function_exists(gethostname HAVE_GETHOSTNAME)
check_function_exists(getnameinfo HAVE_GETNAMEINFO)
check_function_exists(getpagesize HAVE_GETPAGESIZE)
check_function_exists(getrlimit HAVE_GETRLIMIT)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(getcwd HAVE_GETCWD)
check_function_exists(link HAVE_LINK)
check_function_exists(logb HAVE_LOGB)
check_function_exists(lrand48 HAVE_LRAND48)
check_function_exists(mkdir HAVE_MKDIR)
check_function_exists(mktime HAVE_MKTIME)
check_function_exists(perror HAVE_PERROR)
check_function_exists(poll HAVE_POLL)
check_function_exists(random HAVE_RANDOM)
check_function_exists(readlink HAVE_READLINK)
check_function_exists(rename HAVE_RENAME)
check_function_exists(rmdir HAVE_RMDIR)
check_function_exists(select HAVE_SELECT)
check_function_exists(setitimer HAVE_SETITIMER)
check_function_exists(setpgid HAVE_SETPGID)
check_function_exists(setsid HAVE_SETSID)
check_function_exists(sigblock HAVE_SIGBLOCK)
check_function_exists(sighold HAVE_SIGHOLD)
check_function_exists(sigprocmask HAVE_SIGPROCMASK)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strsignal HAVE_STRSIGNAL)
check_function_exists(symlink HAVE_SYMLINK)
check_function_exists(tzset HAVE_TZSET)
check_function_exists(ulimit HAVE_ULIMIT)
check_function_exists(umask HAVE_UMASK)
check_function_exists(usleep HAVE_USLEEP)
check_function_exists(vlimit HAVE_VLIMIT)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(waitpid HAVE_WAITPID)
check_function_exists(wcscmp HAVE_WCSCMP)
check_function_exists(wcslen HAVE_WCSLEN)
check_function_exists(utime HAVE_UTIME)
check_function_exists(mkstemp HAVE_MKSTEMP)
check_function_exists(fseeko HAVE_FSEEKO)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(strcoll HAVE_STRCOLL)
check_function_exists(getloadavg HAVE_GETLOADAVG)
check_function_exists(lockf HAVE_LOCKF)
check_function_exists(flock HAVE_FLOCK)
check_function_exists(dlerror HAVE_DLERROR)

# PTY functions
check_function_exists(getpt HAVE_GETPT)
check_function_exists(grantpt HAVE_GRANTPT)
check_function_exists(unlockpt HAVE_UNLOCKPT)
check_function_exists(ptsname HAVE_PTSNAME)
check_function_exists(killpg HAVE_KILLPG)
check_function_exists(tcgetpgrp HAVE_TCGETPGRP)
check_function_exists(openpty HAVE_OPENPTY)

# getpgrp
check_function_exists(getpgrp HAVE_GETPGRP)
if(HAVE_GETPGRP)
  check_c_source_compiles("
    #include <unistd.h>
    int main(void) { pid_t p = getpgrp(); (void) p; return 0; }
  " GETPGRP_VOID)
endif()

# sigsetjmp
check_c_source_compiles("
  #include <setjmp.h>
  int main(void) { sigjmp_buf buf; sigsetjmp(buf, 0); return 0; }
" HAVE_SIGSETJMP)

# inverse hyperbolic
check_c_source_compiles("
  #include <math.h>
  int main(void) { double x = acosh(1.0); x = asinh(0.0); x = atanh(0.0); return (int)x; }
" HAVE_INVERSE_HYPERBOLIC)

# --- Termios / Termio ---
check_include_file("termios.h" HAVE_TERMIOS_H)
if(HAVE_TERMIOS_H)
  set(HAVE_TERMIOS 1)
  set(NO_TERMIO 1)
endif()

# --- Sockets ---
check_c_source_compiles("
  #include <sys/types.h>
  #include <sys/socket.h>
  int main(void) { int s = socket(AF_INET, SOCK_STREAM, 0); return s; }
" HAVE_SOCKETS)

# Multicast
check_c_source_compiles("
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  int main(void) { struct ip_mreq m; m.imr_multiaddr.s_addr = 0; return 0; }
" HAVE_MULTICAST)

# SysV IPC
check_c_source_compiles("
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/shm.h>
  int main(void) { shmget(0, 0, 0); return 0; }
" HAVE_SYSVIPC)

# --- dlopen ---
check_function_exists(dlopen HAVE_DLOPEN_IN_LIBC)
if(HAVE_DLOPEN_IN_LIBC)
  set(HAVE_DLOPEN 1)
else()
  check_library_exists(dl dlopen "" HAVE_DLOPEN_IN_LIBDL)
  if(HAVE_DLOPEN_IN_LIBDL)
    set(HAVE_DLOPEN 1)
    set(DL_LIBRARIES "dl")
  endif()
endif()

if(HAVE_DLOPEN)
  set(HAVE_SHLIB 1)
endif()

# --- Long file names ---
set(HAVE_LONG_FILE_NAMES 1)

# --- Misc system features ---
set(SYSV_SYSTEM_DIR 1)
set(NLIST_STRUCT 1)
set(SIGNALS_VIA_CHARACTERS 1)

# --- X11 ---
if(XEMACS_WITH_X11)
  find_package(X11)
  if(X11_FOUND)
    set(HAVE_X_WINDOWS 1)
    set(HAVE_X11 1)
    set(THIS_IS_X11R6 1)
    if(X11_Xt_FOUND)
      set(HAVE_XT 1)
    endif()
    if(X11_Xmu_FOUND)
      set(HAVE_XMU 1)
    endif()
    if(X11_Xext_FOUND)
      set(HAVE_XEXT 1)
    endif()
    if(X11_Xpm_FOUND)
      set(HAVE_XPM 1)
    endif()
    if(X11_SM_FOUND)
      set(HAVE_X11_SM 1)
    endif()
    if(X11_Xau_FOUND)
      set(HAVE_XAUTH 1)
    endif()

    # XConvertCase
    set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})
    check_function_exists(XConvertCase HAVE_XCONVERTCASE)
    check_function_exists(XtRegisterDrawable HAVE_XTREGISTERDRAWABLE)

    # XRegisterIMInstantiateCallback
    check_c_source_compiles("
      #include <X11/Xlib.h>
      int main(void) {
        XRegisterIMInstantiateCallback(NULL, NULL, NULL, NULL, NULL, NULL);
        return 0;
      }
    " HAVE_XREGISTERIMINSTANTIATECALLBACK)
    if(HAVE_XREGISTERIMINSTANTIATECALLBACK)
      set(XREGISTERIMINSTANTIATECALLBACK_NONSTANDARD_PROTOTYPE 1)
    endif()

    check_include_file("X11/Xlocale.h" HAVE_X11_XLOCALE_H)
    check_include_file("X11/Xfuncproto.h" HAVE_X11_XFUNCPROTO_H)
    unset(CMAKE_REQUIRED_LIBRARIES)

    set(HAVE_BALLOON_HELP 1)
    set(HAVE_WMCOMMAND 1)

    message(STATUS "X11 found: ${X11_LIBRARIES}")
  else()
    message(WARNING "X11 not found")
    set(XEMACS_WITH_X11 OFF)
  endif()
endif()

# --- XIM ---
if(XEMACS_WITH_XIM AND HAVE_X_WINDOWS)
  set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})
  check_c_source_compiles("
    #include <X11/Xlib.h>
    int main(void) { XOpenIM(NULL, NULL, NULL, NULL); return 0; }
  " HAVE_XIM)
  if(HAVE_XIM)
    set(XIM_XLIB 1)
  endif()
  unset(CMAKE_REQUIRED_LIBRARIES)
endif()

# --- Image libraries ---
if(XEMACS_WITH_JPEG)
  find_package(JPEG)
  if(JPEG_FOUND)
    set(HAVE_JPEG 1)
  endif()
endif()

if(XEMACS_WITH_PNG)
  find_package(PNG)
  if(PNG_FOUND)
    set(HAVE_PNG 1)
  endif()
endif()

if(XEMACS_WITH_TIFF)
  find_package(TIFF)
  if(TIFF_FOUND)
    set(HAVE_TIFF 1)
  endif()
endif()

if(XEMACS_WITH_GIF)
  check_include_file("gif_lib.h" HAVE_GIF_LIB_H)
  if(HAVE_GIF_LIB_H)
    check_library_exists(gif DGifOpenFileName "" HAVE_LIBGIF)
    if(HAVE_LIBGIF)
      set(HAVE_GIF 1)
      set(GIF_LIBRARIES "gif")
    endif()
  endif()
endif()

if(XEMACS_WITH_ZLIB)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    set(HAVE_ZLIB 1)
  endif()
endif()

# --- Sound ---
if(XEMACS_WITH_SOUND)
  # Native sound (OSS)
  if(HAVE_SYS_SOUNDCARD_H)
    set(HAVE_NATIVE_SOUND 1)
    set(SOUNDCARD_H_FILE "sys/soundcard.h")
  elseif(HAVE_LINUX_SOUNDCARD_H)
    set(HAVE_NATIVE_SOUND 1)
    set(SOUNDCARD_H_FILE "linux/soundcard.h")
  elseif(HAVE_MACHINE_SOUNDCARD_H)
    set(HAVE_NATIVE_SOUND 1)
    set(SOUNDCARD_H_FILE "machine/soundcard.h")
  endif()
endif()

# --- ncurses / TTY ---
if(XEMACS_WITH_TTY AND XEMACS_WITH_NCURSES)
  find_package(Curses)
  if(CURSES_FOUND)
    set(HAVE_NCURSES 1)
    set(CURSES_H_FILE "curses.h")
    set(TERM_H_FILE "term.h")
    message(STATUS "Found ncurses: ${CURSES_LIBRARIES}")
  endif()
endif()

if(XEMACS_WITH_TTY)
  set(HAVE_TTY 1)
endif()

# --- MULE ---
if(XEMACS_WITH_MULE)
  # Configure does NOT set UNICODE_INTERNAL - it uses old Mule representation
  # set(UNICODE_INTERNAL 1)  -- intentionally NOT set to match configure
  message(STATUS "MULE (multilingual extension) support: enabled")
endif()

# --- Modules / dlopen ---
if(XEMACS_WITH_MODULES AND HAVE_DLOPEN)
  set(HAVE_MODULES 1)
endif()

# --- Clash detection ---
if(XEMACS_WITH_CLASH_DETECTION)
  set(CLASH_DETECTION 1)
endif()

# --- Mail locking ---
if(XEMACS_MAIL_LOCKING STREQUAL "dot")
  set(MAIL_LOCK_DOT 1)
elseif(XEMACS_MAIL_LOCKING STREQUAL "lockf")
  set(MAIL_LOCK_LOCKF 1)
elseif(XEMACS_MAIL_LOCKING STREQUAL "flock")
  set(MAIL_LOCK_FLOCK 1)
elseif(XEMACS_MAIL_LOCKING STREQUAL "locking")
  set(MAIL_LOCK_LOCKING 1)
elseif(XEMACS_MAIL_LOCKING STREQUAL "mmdf")
  set(MAIL_LOCK_MMDF 1)
endif()

# --- Unixoid event loop ---
if(HAVE_X_WINDOWS OR HAVE_TTY OR HAVE_GTK)
  set(HAVE_UNIXOID_EVENT_LOOP 1)
endif()

# --- Session management ---
if(HAVE_X_WINDOWS AND HAVE_X11_SM)
  set(HAVE_SESSION 1)
endif()

# --- Debug / Error checking ---
if(XEMACS_WITH_DEBUG)
  set(DEBUG_XEMACS 1)
endif()

if(XEMACS_WITH_ASSERTIONS)
  set(USE_ASSERTIONS 1)
endif()

if(XEMACS_WITH_MEMORY_USAGE_STATS)
  set(MEMORY_USAGE_STATS 1)
endif()

if(XEMACS_WITH_ERROR_CHECKING)
  set(ERROR_CHECK_TYPES 1)
  set(ERROR_CHECK_GC 1)
  set(ERROR_CHECK_MALLOC 1)
  set(ERROR_CHECK_BYTE_CODE 1)
  set(ERROR_CHECK_DISPLAY 1)
  set(ERROR_CHECK_EXTENTS 1)
  set(ERROR_CHECK_GLYPHS 1)
  set(ERROR_CHECK_STRUCTURES 1)
  set(ERROR_CHECK_TEXT 1)
endif()

# --- Menubars / Scrollbars / Widgets ---
if(XEMACS_WITH_MENUBARS)
  set(HAVE_MENUBARS 1)
endif()
if(XEMACS_WITH_SCROLLBARS)
  set(HAVE_SCROLLBARS 1)
endif()
if(XEMACS_WITH_TOOLBARS)
  set(HAVE_TOOLBARS 1)
endif()

# Dialogs and Widgets require Athena or Motif on X11 - configure found neither
# So we do NOT enable them unless the library is actually found
if(XEMACS_WITH_DIALOGS)
  # Only enable if we have a toolkit that supports dialogs
  # Lucid alone doesn't provide dialogs without Athena
  set(HAVE_DIALOGS_REQUESTED 1)
endif()
if(XEMACS_WITH_WIDGETS)
  set(HAVE_WIDGETS_REQUESTED 1)
endif()

# Lucid toolkit setup
if(HAVE_X_WINDOWS)
  set(NEED_LUCID 1)
  if(HAVE_MENUBARS)
    set(LWLIB_MENUBARS_LUCID 1)
  endif()
  if(HAVE_SCROLLBARS)
    set(LWLIB_SCROLLBARS_LUCID 1)
  endif()
  # LWLIB_TABS_LUCID and HAVE_DIALOGS/HAVE_WIDGETS require Athena
  # Configure did NOT enable them, so we don't either
endif()

# --- Canna / Wnn ---
if(XEMACS_WITH_CANNA)
  check_include_file("canna/ccommon.h" HAVE_CANNA_CCOMMON_H)
  if(HAVE_CANNA_CCOMMON_H)
    check_library_exists("canna" "RkInitialize" "" HAVE_LIBCANNA)
    if(HAVE_LIBCANNA)
      set(HAVE_CANNA 1)
      set(CANNA_LIBRARIES "canna")
    endif()
  endif()
endif()

if(XEMACS_WITH_WNN)
  check_include_file("wnn/wnnlib.h" HAVE_WNN_WNNLIB_H)
  if(HAVE_WNN_WNNLIB_H)
    check_library_exists("wnn" "jd_string_to_romaji" "" HAVE_LIBWNN)
    if(HAVE_LIBWNN)
      set(HAVE_WNN 1)
      set(WNN_LIBRARIES "wnn")
    endif()
  endif()
endif()

# --- TLS ---
if(XEMACS_WITH_TLS)
  find_package(OpenSSL)
  if(OPENSSL_FOUND)
    set(WITH_TLS 1)
    set(HAVE_OPENSSL 1)
  endif()
endif()

# --- Xft / Fontconfig ---
# Xft sub-options force Xft emacs on (matching autoconf behavior)
if(XEMACS_WITH_XFT_MENUBARS OR XEMACS_WITH_XFT_TABS OR XEMACS_WITH_XFT_GAUGES)
  if(NOT XEMACS_WITH_XFT)
    message(STATUS "Forcing XEMACS_WITH_XFT=ON because Xft widget options are enabled")
    set(XEMACS_WITH_XFT ON)
  endif()
endif()

# XFS and Xft menubars are incompatible (matching autoconf)
if(XEMACS_WITH_XFS AND XEMACS_WITH_XFT_MENUBARS)
  message(FATAL_ERROR "XFS and Xft in the menubars are incompatible!")
endif()

if(XEMACS_WITH_XFT AND HAVE_X_WINDOWS)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(XFT xft)
  if(XFT_FOUND)
    set(HAVE_XFT 1)
    set(HAVE_FONTCONFIG 1)
    # font-mgr.c uses fontconfig directly, so we need both Xft and fontconfig
    pkg_check_modules(FONTCONFIG fontconfig)
    set(XFT_LIBRARIES ${XFT_LINK_LIBRARIES} ${FONTCONFIG_LINK_LIBRARIES})
    set(XFT_INCLUDE_DIRS ${XFT_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})

    # Check for FcConfigGetRescanInterval / FcConfigSetRescanInterval
    set(CMAKE_REQUIRED_LIBRARIES ${FONTCONFIG_LINK_LIBRARIES})
    set(CMAKE_REQUIRED_INCLUDES ${FONTCONFIG_INCLUDE_DIRS})
    check_function_exists(FcConfigGetRescanInterval HAVE_FCCONFIGGETRESCANINTERVAL)
    check_function_exists(FcConfigSetRescanInterval HAVE_FCCONFIGSETRESCANINTERVAL)
    unset(CMAKE_REQUIRED_LIBRARIES)
    unset(CMAKE_REQUIRED_INCLUDES)

    # Xft widget sub-options
    if(XEMACS_WITH_XFT_MENUBARS)
      set(HAVE_XFT_MENUBARS 1)
    endif()
    if(XEMACS_WITH_XFT_TABS)
      set(HAVE_XFT_TABS 1)
    endif()
    if(XEMACS_WITH_XFT_GAUGES)
      set(HAVE_XFT_GAUGES 1)
    endif()

    message(STATUS "Xft found: ${XFT_LIBRARIES}")
  else()
    message(FATAL_ERROR "Unable to find Xft for XEMACS_WITH_XFT")
  endif()
elseif(XEMACS_WITH_FONTCONFIG)
  find_package(Fontconfig)
  if(Fontconfig_FOUND)
    set(HAVE_FONTCONFIG 1)
  endif()
endif()

# --- X FontSet (XFS) ---
if(XEMACS_WITH_XFS AND HAVE_X_WINDOWS)
  set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES})
  check_function_exists(XmbDrawString HAVE_XMBDRAWSTRING)
  unset(CMAKE_REQUIRED_LIBRARIES)
  if(HAVE_XMBDRAWSTRING AND XEMACS_WITH_MENUBARS_TYPE STREQUAL "lucid")
    set(USE_XFONTSET 1)
    message(STATUS "X FontSet (XFS) support enabled")
  else()
    message(WARNING "XFS requires XmbDrawString and Lucid menubars")
  endif()
endif()

if(XEMACS_WITH_XFACE)
  check_include_file("compface.h" HAVE_COMPFACE_H)
  if(HAVE_COMPFACE_H)
    check_library_exists("compface" "uncompface" "" HAVE_LIBCOMPFACE)
    if(HAVE_LIBCOMPFACE)
      set(HAVE_XFACE 1)
      set(COMPFACE_LIBRARIES "compface")
    endif()
  endif()
endif()

# --- libmcheck ---
if(HAVE_MCHECK_H)
  check_library_exists(mcheck mcheck "" HAVE_LIBMCHECK_LIB)
  if(HAVE_LIBMCHECK_LIB)
    set(HAVE_LIBMCHECK 1)
    set(MCHECK_LIBRARIES "mcheck")
  endif()
endif()

# --- GCC warning flags ---
if(USE_GCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-switch -Wundef -Wsign-compare -Wno-char-subscripts -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -std=gnu99")
endif()

# --- Build type ---
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# --- OS/Machine name for messages ---
if(XEMACS_CONFIG_OPSYSFILE)
  string(REGEX REPLACE "^s/([^/]+)\\.h$" "\\1" OPSYS_NAME "${XEMACS_CONFIG_OPSYSFILE}")
else()
  set(OPSYS_NAME "${XEMACS_OPSYS}")
endif()

if(XEMACS_CONFIG_MACHFILE)
  string(REGEX REPLACE "^m/([^/]+)\\.h$" "\\1" MACHINE_NAME "${XEMACS_CONFIG_MACHFILE}")
else()
  set(MACHINE_NAME "${XEMACS_MACHINE}")
endif()

message(STATUS "System: ${MACHINE_NAME}-${OPSYS_NAME}")
