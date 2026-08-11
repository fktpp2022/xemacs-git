/* s/ file for Apple Darwin (macOS) systems.
   Based on mach-bsd4-3.h and netbsd.h.
   XEmacs cmake branch fix. */

/* Get most of the BSD stuff */
#include "bsd-common.h"

/* SYSTEM_TYPE should indicate the kind of system you are using.
   It sets the Lisp variable system-type.  */

#undef SYSTEM_TYPE
#define SYSTEM_TYPE "darwin"

/* XEmacs addition: define BSD so code can check for it. */
#ifndef BSD
#define BSD 43
#endif

/* Darwin is based on Mach and FreeBSD. */
#undef SIGTYPE
#define SIGTYPE void XCDECL
#define SIGRETURN return

/* Darwin uses libutil for pty functions. */
#define LIBS_SYSTEM -lutil

/* No soundcard support on Darwin. */
#undef HAVE_SYS_SOUNDCARD_H
#undef HAVE_MACHINE_SOUNDCARD_H
#undef HAVE_LINUX_SOUNDCARD_H

/* Don't send signals to subprocesses by "typing" special chars. */
#undef SIGNALS_VIA_CHARACTERS

/* Darwin/FreeBSD-style process groups. */
#define BSD_PGRPS

/* Use getpgrp() with no args. */
#undef GETPGRP_VOID
#define GETPGRP_VOID

/* Ordinary link is simple and effective. */
#define ORDINARY_LINK

/* Use standard C library, no special start files. */
#undef START_FILES
#undef END_FILES
#undef START_FILES_1
#undef END_FILES_1
#define START_FILES_1
#define END_FILES_1

/* Use standard linking. */
#define LD_SWITCH_SYSTEM
#define LD_SWITCH_SYSTEM_TEMACS
#define LD_SWITCH_SYSTEM_AUX
#define LD_SWITCH_X_SITE_AUX

/* Darwin doesn't have these. */
#undef LIBS_DEBUG
#undef NEED_ERRNO

/* Directory constants */
#ifndef MAXNAMLEN
#define MAXNAMLEN 1024
#endif

/* macOS requires _XOPEN_SOURCE 700+ for NI_MAXHOST etc. */
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700

/* macOS networking - include proper headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <util.h>

/* openpty declaration - macOS doesn't expose in public headers */

/* macOS declares getloadavg in libc but not in any header. */
int getloadavg (double loadavg[], int nelem);

/* macOS deprecated functions */
void *sbrk (int delta);

/* Workaround for macOS header issues */
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV 32
#endif
#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 12
#endif
#ifndef IP_DROP_MEMBERSHIP
#define IP_DROP_MEMBERSHIP 13
#endif
#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 5
#endif
struct ip_mreq {
  struct in_addr imr_multiaddr;
  struct in_addr imr_interface;
};
