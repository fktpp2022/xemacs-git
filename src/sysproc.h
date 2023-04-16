/*
   Copyright (C) 1995 Free Software Foundation, Inc.
   Copyright (C) 2000, 2002 Ben Wing.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see <http://www.gnu.org/licenses/>. */

/* Synched up with: Not really in FSF. */

#ifndef INCLUDED_sysproc_h_
#define INCLUDED_sysproc_h_

#include "systime.h" /* necessary for sys/resource.h; also gets the
			FD_* defines on some systems. */
#ifndef WIN32_NATIVE
#include <sys/resource.h>
#endif

#ifdef MINGW
#include <../mingw/process.h>
#elif defined (WIN32_NATIVE)
/* <process.h> should not conflict with "process.h", as per ANSI definition.
   This is not true with visual c though. The trick with <../include/process.h>
   works with VC4.2b, 5.0 and 6.0.  Visual Studio 2015 moved process.h to
   <../ucrt/>, so from that version onwards use that. */
#if _MSC_VER >= 1900
#include <../ucrt/process.h>
#else
#include <../include/process.h>
#endif
#endif

#ifdef HAVE_SOCKETS	/* TCP connection support, if kernel can do it */
# ifndef WIN32_NATIVE
#  include <sys/socket.h>
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
# endif
#endif /* HAVE_SOCKETS */

#ifdef WIN32_NATIVE
/* Note: winsock.h already included in systime.h above */
/* map winsock error codes to standard names */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EALREADY
#define EALREADY WSAEALREADY
#endif
#ifndef ENOTSOCK
#define ENOTSOCK WSAENOTSOCK
#endif
#ifndef EDESTADDRREQ
#define EDESTADDRREQ WSAEDESTADDRREQ
#endif
#ifndef EMSGSIZE
#define EMSGSIZE WSAEMSGSIZE
#endif
#ifndef EPROTOTYPE
#define EPROTOTYPE WSAEPROTOTYPE
#endif
#ifndef ENOPROTOOPT
#define ENOPROTOOPT WSAENOPROTOOPT
#endif
#ifndef EPROTONOSUPPORT
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT
#endif
#ifndef ESOCKTNOSUPPORT
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP WSAEOPNOTSUPP
#endif
#ifndef EPFNOSUPPORT
#define EPFNOSUPPORT WSAEPFNOSUPPORT
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif
#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif
#ifndef EADDRNOTAVAIL
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#endif
#ifndef ENETDOWN
#define ENETDOWN WSAENETDOWN
#endif
#ifndef ENETUNREACH
#define ENETUNREACH WSAENETUNREACH
#endif
#ifndef ENETRESET
#define ENETRESET WSAENETRESET
#endif
#ifndef ECONNABORTED
#define ECONNABORTED WSAECONNABORTED
#endif
#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif
#ifndef ENOBUFS
#define ENOBUFS WSAENOBUFS
#endif
#ifndef EISCONN
#define EISCONN WSAEISCONN
#endif
#ifndef ENOTCONN
#define ENOTCONN WSAENOTCONN
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN WSAESHUTDOWN
#endif
#ifndef ETOOMANYREFS
#define ETOOMANYREFS WSAETOOMANYREFS
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif
#ifndef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#endif
#ifndef ELOOP
#define ELOOP WSAELOOP
#endif
/* #define ENAMETOOLONG            WSAENAMETOOLONG */
#ifndef EHOSTDOWN
#define EHOSTDOWN WSAEHOSTDOWN
#endif
#ifndef EHOSTUNREACH
#define EHOSTUNREACH WSAEHOSTUNREACH
#endif
/* #define ENOTEMPTY               WSAENOTEMPTY */
#ifndef EPROCLIM
#define EPROCLIM WSAEPROCLIM
#endif
#ifndef EUSERS
#define EUSERS WSAEUSERS
#endif
#ifndef EDQUOT
#define EDQUOT WSAEDQUOT
#endif
#ifndef ESTALE
#define ESTALE WSAESTALE
#endif
#ifndef EREMOTE
#define EREMOTE WSAEREMOTE
#endif
#undef FROB

#endif /* WIN32_NATIVE */

/* On some systems, e.g. DGUX, inet_addr returns a `struct in_addr'. */
#ifdef HAVE_BROKEN_INET_ADDR
# define IN_ADDR struct in_addr
# define NUMERIC_ADDR_ERROR (numeric_addr.s_addr == -1)
#else
# if (LONGBITS > 32)
#  define IN_ADDR unsigned int
# else
#  define IN_ADDR unsigned long
# endif
# define NUMERIC_ADDR_ERROR (numeric_addr == (IN_ADDR) -1)
#endif

/* Define first descriptor number available for subprocesses.  */
#define FIRST_PROC_DESC 3

#ifdef AIX
#include <sys/select.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>		/* isastream(), I_PUSH */
#endif

#ifdef HAVE_STRTIO_H
#include <strtio.h>		/* TIOCSIGNAL */
#endif

#ifdef HAVE_PTY_H
#include <pty.h>		/* openpty() on Tru64, Linux */
#endif

#ifdef HAVE_LIBUTIL_H
#include <libutil.h>		/* openpty() on FreeBSD */
#endif

#ifdef HAVE_UTIL_H
#include <util.h>		/* openpty() on NetBSD */
#endif

/* The FD_* macros expand to __extension__ forms on glibc-based systems.  Uno
   does not understand such forms, so let's help it out. */
#ifdef UNO
#undef FD_SET
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#undef MAXDESC
#undef SELECT_TYPE
#endif /* UNO */

#ifdef FD_SET

/* We could get this from param.h, but better not to depend on finding that.
   And better not to risk that it might define other symbols used in this
   file.  */
# ifdef FD_SETSIZE
#  define MAXDESC FD_SETSIZE
# else
#  define MAXDESC 64
# endif /* FD_SETSIZE */
# define SELECT_TYPE fd_set

#else /* no FD_SET */

# define MAXDESC 32
# define SELECT_TYPE int

/* Define the macros to access a single-int bitmap of descriptors.  */
# define FD_SET(n, p) (*(p) |= (1 << (n)))
# define FD_CLR(n, p) (*(p) &= ~(1 << (n)))
# define FD_ISSET(n, p) (*(p) & (1 << (n)))
# define FD_ZERO(p) (*(p) = 0)

#endif /* no FD_SET */

int poll_fds_for_input (SELECT_TYPE mask);
int qxe_execve (const Ibyte *filename, Ibyte * const argv[],
		Ibyte * const envp[]);
pid_t qxe_getpid (void);

#endif /* INCLUDED_sysproc_h_ */
