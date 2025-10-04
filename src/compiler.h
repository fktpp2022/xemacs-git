/* Compiler-specific definitions for XEmacs.
   Copyright (C) 1998-1999, 2003 Free Software Foundation, Inc.
   Copyright (C) 1994 Richard Mlynarik.
   Copyright (C) 1995, 1996, 2000-2004, 2005, 2010 Ben Wing.

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

/* Synched up with: not in FSF. */

/* Authorship:

   NOT_REACHED, DOESNT_RETURN, PRINTF_ARGS by Richard Mlynarik, c. 1994.
   RETURN_SANS_WARNING by Martin buchholz, 1998 or 1999.
   Many changes and improvements by Jerry James, 2003.
     Split out of lisp.h, reorganized, and modernized.
     {BEGIN,END}_C_DECLS, NEED_GCC, GCC_VERSION
     ATTRIBUTE_MALLOC, ATTRIBUTE_CONST, ATTRIBUTE_PURE, UNUSED
*/

#ifndef INCLUDED_compiler_h
#define INCLUDED_compiler_h

/* Define min() and max(). (Some compilers put them in strange places that
   won't be referenced by include files used by XEmacs, such as `macros.h'
   under Solaris.) */

#ifndef min
# define min(a,b) (((a) <= (b)) ? (a) : (b))
#endif
#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* Regular C complains about possible clobbering of local vars NOT declared
   as volatile if there's a longjmp() in a function.  C++ complains if such
   vars ARE volatile; or more correctly, sans volatile no problem even when
   you longjmp, avec volatile you get unfixable compile errors like

/src/xemacs/lilfix/src/process-unix.c: In function `void
   unix_send_process(Lisp_Object, lstream*)':
/src/xemacs/lilfix/src/process-unix.c:1577: no matching function for call to `
   Lisp_Object::Lisp_Object(volatile Lisp_Object&)'
/src/xemacs/lilfix/src/lisp-union.h:32: candidates are:
   Lisp_Object::Lisp_Object(const Lisp_Object&)
*/

#ifdef __cplusplus
# define VOLATILE_IF_NOT_CPP
#else
# define VOLATILE_IF_NOT_CPP volatile
#endif

/* Avoid indentation problems when XEmacs sees the curly braces */
#ifndef BEGIN_C_DECLS
# ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS }
# else
#  define BEGIN_C_DECLS
#  define END_C_DECLS
# endif
#endif

/* Guard against older gccs that did not define all of these symbols */
#ifdef __GNUC__
# ifndef __GNUC_MINOR__
#  define __GNUC_MINOR__      0
# endif
# ifndef __GNUC_PATCHLEVEL__
#  define __GNUC_PATCHLEVEL__ 0
# endif
#endif /* __GNUC__ */

/* Simplify testing for specific GCC versions.  For non-GNU compilers,
   GCC_VERSION evaluates to zero. */
#ifndef NEED_GCC
# define NEED_GCC(major,minor,patch) (major * 1000000 + minor * 1000 + patch)
#endif /* NEED_GCC */
#ifndef GCC_VERSION
# ifdef __GNUC__
#  define GCC_VERSION NEED_GCC (__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
# else
#  define GCC_VERSION 0
# endif /* __GNUC__ */
#endif /* GCC_VERSION */

#ifdef _MSC_VER
#define MSC_VERSION _MSC_VER
#else
#define MSC_VERSION 0
#endif

/* GCC < 2.6.0 could only declare one attribute per function.  In that case,
   we define DOESNT_RETURN in preference to PRINTF_ARGS, which is only used
   for checking args against the string spec. */
#ifndef PRINTF_ARGS
# if (GCC_VERSION >= NEED_GCC (2, 6, 0))
#  define PRINTF_ARGS(string_index,first_to_check) \
          __attribute__ ((format (printf, string_index, first_to_check)))
# else
#  define PRINTF_ARGS(string_index,first_to_check)
# endif /* GNUC */
#endif

#ifndef DOESNT_RETURN_TYPE
#ifdef __clang__
# if __has_builtin(__builtin_unreachable)
#  define RETURN_NOT_REACHED(value) __builtin_unreachable()
# endif
# define DOESNT_RETURN_TYPE(rettype) rettype
# define DECLARE_DOESNT_RETURN_TYPE(rettype,decl) rettype decl \
  __attribute__ ((noreturn))
#elif (GCC_VERSION > NEED_GCC (0, 0, 0))
#  if (GCC_VERSION >= NEED_GCC (2, 5, 0))
#   ifndef __INTEL_COMPILER
#    define RETURN_NOT_REACHED(value) DO_NOTHING
#   endif
#   define DOESNT_RETURN_TYPE(rettype) rettype
#   define DECLARE_DOESNT_RETURN_TYPE(rettype,decl) rettype decl \
	   __attribute__ ((noreturn))
#  else /* GCC_VERSION < NEED_GCC (2, 5, 0) */
#   define DOESNT_RETURN_TYPE(rettype) rettype volatile
#   define DECLARE_DOESNT_RETURN_TYPE(rettype,decl) rettype volatile decl
#  endif /* GCC_VERSION >= NEED_GCC (2, 5, 0) */
# elif (MSC_VERSION >= 1200)
/* MSVC 6.0 has a mechanism to declare functions which never return */
#  define DOESNT_RETURN_TYPE(rettype) __declspec(noreturn) rettype
#  define DECLARE_DOESNT_RETURN_TYPE(rettype,decl) \
  __declspec(noreturn) rettype XCDECL decl
#  if (MSC_VERSION >= 1300)
/* VC++ 7 issues warnings about return statements in __declspec(noreturn)
   functions; this problem didn't exist under VC++ 6 */
#   define RETURN_NOT_REACHED(value) DO_NOTHING
#  endif
# else /* not gcc, VC++ */
#  define DOESNT_RETURN_TYPE(rettype) rettype
#  define DECLARE_DOESNT_RETURN_TYPE(rettype,decl) rettype decl
# endif /* GCC_VERSION > NEED_GCC (0, 0, 0) */
#endif /* DOESNT_RETURN_TYPE */
#ifndef DOESNT_RETURN
# define DOESNT_RETURN DOESNT_RETURN_TYPE (void)
# define DECLARE_DOESNT_RETURN(decl) DECLARE_DOESNT_RETURN_TYPE (void, decl)
#endif /* DOESNT_RETURN */

#if defined (__clang__) || GCC_VERSION > NEED_GCC (4, 6, 0)
#define PRAGMA_PUSH_DIAGNOSTICS _Pragma ("GCC diagnostic push")
#define PRAGMA_POP_DIAGNOSTICS _Pragma ("GCC diagnostic pop")
#define PRAGMA_IGNORE_DEPRECATED \
  _Pragma ("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#else
#define PRAGMA_PUSH_DIAGNOSTICS DO_NOTHING
#define PRAGMA_POP_DIAGNOSTICS DO_NOTHING
#define PRAGMA_IGNORE_DEPRECATED DO_NOTHING
#endif /* defined (__clang__) || defined (__GNUC__) */

#if GCC_VERSION > NEED_GCC (12, 0, 0)
#define PRAGMA_IGNORE_DANGLING_POINTER \
  _Pragma ("GCC diagnostic ignored \"-Wdangling-pointer\"")
#else
#define PRAGMA_IGNORE_DANGLING_POINTER DO_NOTHING
#endif

/* Another try to fix SunPro C compiler warnings */
/* "end-of-loop code not reached" */
/* "statement not reached */
#if defined __SUNPRO_C || defined __USLC__
# define RETURN_SANS_WARNINGS if (1) return
# define RETURN_NOT_REACHED(value) DO_NOTHING
#endif

/* More ways to shut up compiler.  This works in Fcommand_loop_1(),
   where there's an infinite loop in a function returning a Lisp object.
*/
#if (defined (_MSC_VER) && MSC_VERSION < 1300) || defined (__SUNPRO_C) || \
  defined (__SUNPRO_CC)
# define DO_NOTHING_DISABLING_NO_RETURN_WARNINGS if (0) return Qnil
#else
# define DO_NOTHING_DISABLING_NO_RETURN_WARNINGS DO_NOTHING
#endif

#ifndef RETURN_NOT_REACHED
# define RETURN_NOT_REACHED(value) return (value)
#endif

#ifndef RETURN_SANS_WARNINGS
# define RETURN_SANS_WARNINGS return
#endif

#ifndef DO_NOTHING
# define DO_NOTHING do {} while (0)
#endif

#ifndef DECLARE_NOTHING
# define DECLARE_NOTHING struct nosuchstruct
#endif

#ifndef ATTRIBUTE_MALLOC
# if (GCC_VERSION >= NEED_GCC (2, 96, 0))
#  define ATTRIBUTE_MALLOC __attribute__ ((__malloc__))
# else
#  define ATTRIBUTE_MALLOC
# endif /* GCC_VERSION >= NEED_GCC (2, 96, 0) */
#endif /* ATTRIBUTE_MALLOC */

#ifndef ATTRIBUTE_PURE
# if (GCC_VERSION >= NEED_GCC (2, 96, 0))
#  define ATTRIBUTE_PURE __attribute__ ((pure))
# else
#  define ATTRIBUTE_PURE
# endif /* GCC_VERSION >= NEED_GCC (2, 96, 0) */
#endif /* ATTRIBUTE_PURE */

#ifndef ATTRIBUTE_CONST
# if (GCC_VERSION >= NEED_GCC (2, 5, 0))
#  define ATTRIBUTE_CONST __attribute__ ((const))
#  define CONST_FUNC
# else
#  define ATTRIBUTE_CONST
#  define CONST_FUNC const
# endif /* GCC_VERSION >= NEED_GCC (2, 5, 0) */
#endif /* ATTRIBUTE_CONST */

/*
   NOTE:  These macros MUST be named UNUSED (exactly) or something
   prefixed with USED_IF_, or DEFUN docstrings will be parsed incorrectly.
   See comments in make_docfile.c (write_c_args).  You'd think that this
   wouldn't happen, but unfortunately we do indeed have some arguments
   of DEFUNs unused for GNU compatibility or because features are missing.

   #### At one time, __attribute__ ((unused)) confused G++.  We don't know
   which versions.  Please report problems and fix conditionals.
   #### A similar issue arose with the Intel CC.  We know that v7 didn't
   work and v9 does.  Let us know if v8 works or not, please.
   See <m34plsmh88.fsf@jerrypc.cs.usu.edu>.
*/
#ifndef UNUSED_ARG
# define UNUSED_ARG(decl) unused_##decl
#endif
#ifndef UNUSED
# if defined(__GNUC__) && (!defined(__INTEL_COMPILER) || __INTEL_COMPILER >= 800)
#  define ATTRIBUTE_UNUSED __attribute__ ((unused))
# else
#  define ATTRIBUTE_UNUSED
# endif
# define UNUSED(decl) UNUSED_ARG (decl) ATTRIBUTE_UNUSED
#endif /* UNUSED */

/* Various macros for params/variables used or unused depending on
   config flags. */

#ifdef UNICODE_INTERNAL
# define USED_IF_UNICODE_INTERNAL(decl) decl
# define USED_IF_OLD_MULE(decl) UNUSED (decl)
#else
# define USED_IF_UNICODE_INTERNAL(decl) UNUSED (decl)
# define USED_IF_OLD_MULE(decl) decl
#endif
#ifdef HAVE_XFT
# define USED_IF_XFT(decl) decl
#else
# define USED_IF_XFT(decl) UNUSED (decl)
#endif
#ifdef HAVE_SCROLLBARS
# define USED_IF_SCROLLBARS(decl) decl
#else
# define USED_IF_SCROLLBARS(decl) UNUSED (decl)
#endif
#ifdef HAVE_TTY
#define USED_IF_TTY(decl) decl
#else
#define USED_IF_TTY(decl) UNUSED (decl)
#endif
#ifdef HAVE_TOOLBARS
#define USED_IF_TOOLBARS(decl) decl
#else
#define USED_IF_TOOLBARS(decl) UNUSED (decl)
#endif

/* Declaration that variable or expression X is "used" to defeat
   "unused variable" warnings.  DON'T DO THIS FOR PARAMETERS IF IT ALL
   POSSIBLE.  Use an UNUSED() or USED_IF_*() declaration on the parameter
   instead.  Don't do this for unused local variables that should really
   just be deleted. */
#define USED(x) ((void) (x))

#if defined (DEBUG_XEMACS) || defined (__cplusplus)
# define REGISTER
#else
# define REGISTER register
#endif

#if defined(HAVE_MS_WINDOWS) && defined(HAVE_SHLIB)
# ifdef EMACS_MODULE
#  define MODULE_API __declspec(dllimport)
# else
#  define MODULE_API __declspec(dllexport)
# endif
#else
# define MODULE_API
#endif

/* Under "strict-aliasing" assumptions, you're not necessarily allowed to
   access the same memory address as two different types.  The proper way
   around that is with a union.  The macros below help out, e.g. the
   definition of XE_MAKEPOINTS(val) is

   ANSI_ALIASING_TYPEDEF (POINTS, POINTS);
   #define XE_MAKEPOINTS(l)       ANSI_ALIASING_CAST (POINTS, l)

   replacing

   BAD!!! #define XE_MAKEPOINTS(l)       (* (POINTS *) &(l))

   On the other hand, if you are just casting from one pointer to the other
   in order to pass a pointer to another function, it's probably OK to just
   trick GCC by inserting an intermediate cast to (void *), to avoid
   warnings about "dereferencing type-punned pointer".  #### I don't know
   how kosher this is, but do strict-aliasing rules really apply across
   functions?

   Note that the input to e.g. VOIDP_CAST must be an lvalue (i.e. not
   &(something)), but the value of the macro is also an lvalue, so in place
   of `(void **) &foo' you could write `& VOIDP_CAST (foo)' if you are
   subsequently dereferencing the value or don't feel comfortable doing a
   trick like `(void **) (void *) &foo'.

   Unfortunately, it does not work to just define the union type on the fly in
   the cast -- otherwise, we could avoid the need for a typedef.  Or rather,
   it does work under gcc but not under Visual C++.

   --ben
 */

#define ANSI_ALIASING_TYPEDEF(name, type) typedef union { char c; type p; } *ANSI_ALIASING_##name
#define ANSI_ALIASING_CAST(name, val) (((ANSI_ALIASING_##name) &(val))->p)
ANSI_ALIASING_TYPEDEF (voidp, void *);
/* VOIDP_CAST: Cast an lvalue to (void *) in a way that is ANSI-aliasing
   safe and will not result in GCC warnings.  The result is still an
   lvalue, so you can assign to it or take its address. */
#define VOIDP_CAST(l)  ANSI_ALIASING_CAST (voidp, l)

/* Support macros for code that uses PREPROCESSOR_ARGC(). */
#define PREPROCESSOR_CAT(X, Y) X ## Y
#define PREPROCESSOR_CONCATENATE(X, Y) PREPROCESSOR_CAT(X, Y)

/* Needed to work around difficulties with __VA_ARGS__ and MSVC. */
#define PREPROCESSOR_EXPAND(X) X

#define PREPROCESSOR_ARGC_COUNTER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                                  _11, _12, _13, _14, _15, _16, _17, _18,  \
                                  _19, _20, _21, _22, _23, _24, _25, _26,  \
                                  _27, _28, _29, _30, _31, _32, _33, _34,  \
                                  _35, _36, _37, _38, _39, _40, _41, _42,  \
                                  _43, _44, _45, _46, _47, _48, _49, _50,  \
                                  _51, _52, _53, _54, _55, _56, _57, _58,  \
                                  _59, _60, _61, _62, _63, N, ...) N

/* Laurent Deniau's technique for counting the number of arguments passed to a
   variadic macro, posted to comp.std.c January 2006,
   dqgm2f$ije$1@sunnews.cern.ch . Introduced to the XEmacs code in incbin.h,
   thank you Dale Weiler. */
#define PREPROCESSOR_ARGC(...) \
  PREPROCESSOR_EXPAND (PREPROCESSOR_ARGC_COUNTER                             \
                       (__VA_ARGS__, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, \
                        53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,  \
                        40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28,  \
                        27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15,  \
                        14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#endif /* INCLUDED_compiler_h */
