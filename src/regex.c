/* Extended regular expression matching and search library,
   version 0.12, extended for XEmacs.
   (Implements POSIX draft P10003.2/D11.2, except for
   internationalization features.)

   Copyright (C) 1993, 1994, 1995, 2000, 2023 Free Software Foundation, Inc.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 1995, 2001, 2002, 2003, 2005, 2010 Ben Wing.

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

/* TODO:
   - detect nasty infinite loops like "\\(\\)+?ab" when matching "ac".
   - structure the opcode space into opcode+flag.
   - merge with glic's regex.[ch]

   That's it for now    -sm */

/* Synched up with: FSF 19.29. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

/* XEmacs addition */
#ifdef REL_ALLOC
#define REGEX_REL_ALLOC /* may be undefined below */
#endif

/* XEmacs: define this to add in a speedup for patterns anchored at
   the beginning of a line.  Keep the ifdefs so that it's easier to
   tell where/why this code has diverged from v19. */
#define REGEX_BEGLINE_CHECK

/* XEmacs: the current mmap-based ralloc handles small blocks very
   poorly, so we disable it here. */

#if defined (HAVE_MMAP) || defined (DOUG_LEA_MALLOC)
# undef REGEX_REL_ALLOC
#endif

/* The `emacs' switch turns on certain matching commands
   that make sense only in Emacs. */
#ifdef emacs

#include "lisp.h"
#include "buffer.h"
#include "casetab.h"
#include "syntax.h"

#if (defined (DEBUG_XEMACS) && !defined (DEBUG))
#define DEBUG
#endif

#define RE_TRANSLATE_1(ch) TRT_TABLE_OF (translate, (Ichar) ch)
#define TRANSLATE_P(tr) (!NILP (tr))

/* Converts the pointer to the char to BEG-based offset from the start.	 */
#define PTR_TO_OFFSET(d) (MATCHING_IN_FIRST_STRING			\
			  ? (d) - string1 : (d) - (string2 - size1))

#else  /* not emacs */

#include <stdlib.h>
#include <sys/types.h>
#include <stddef.h> /* needed for ptrdiff_t under Solaris */
#include <string.h>

#include "compiler.h"   /* Get compiler-specific definitions like UNUSED */

#define ABORT abort

/* If we are not linking with Emacs proper,
   we can't use the relocating allocator
   even if config.h says that we can.  */
#undef REGEX_REL_ALLOC

/* defined in lisp.h */
#ifdef REGEX_MALLOC
#ifndef DECLARE_NOTHING
#define DECLARE_NOTHING struct nosuchstruct
#endif
#endif

#define itext_ichar(str)				((Ichar) (str)[0])
#define itext_ichar_fmt(str, fmt, object)		((Ichar) (str)[0])
#define itext_ichar_ascii_fmt(str, fmt, object)	((Ichar) (str)[0])
#define itext_ichar_eql(str, ch)                (((Ichar) (str)[0]) == (ch))

#if (LONGBITS > INTBITS)
# define EMACS_INT long
#else
# define EMACS_INT int
#endif

typedef int Ichar;

#define INC_IBYTEPTR(p) ((p)++)
#define INC_IBYTEPTR_FMT(p, fmt) ((p)++)
#define DEC_IBYTEPTR(p) ((p)--)
#define DEC_IBYTEPTR_FMT(p, fmt) ((p)--)
#define MAX_ICHAR_LEN 1
#define itext_ichar_len(ptr) 1
#define itext_ichar_len_fmt(ptr, fmt) 1

/* Define the syntax stuff for \<, \>, etc.  */

/* This must be nonzero for the wordchar and notwordchar pattern
   commands in re_match_2.  */
#ifndef Sword
#define Sword 1
#endif

#ifdef SYNTAX_TABLE

extern char *re_syntax_table;

#else /* not SYNTAX_TABLE */

/* How many characters in the character set.  */
#define CHAR_SET_SIZE 256

static char re_syntax_table[CHAR_SET_SIZE];

static void
init_syntax_once (void)
{
  static int done = 0;

  if (!done)
    {
      const char *word_syntax_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

      memset (re_syntax_table, 0, sizeof (re_syntax_table));

      while (*word_syntax_chars)
	re_syntax_table[(unsigned int) (*word_syntax_chars++)] = Sword;

      done = 1;
    }
}

#endif /* SYNTAX_TABLE */

#define SYNTAX(ignored, c) re_syntax_table[c]
#undef SYNTAX_FROM_CACHE
#define SYNTAX_FROM_CACHE SYNTAX

#define RE_TRANSLATE_1(c) translate[(unsigned char) (c)]
#define TRANSLATE_P(tr) tr

#endif /* emacs */

/* This is for other GNU distributions with internationalized messages.  */
#if defined (I18N3) && (defined (HAVE_LIBINTL_H) || defined (_LIBC))
# include <libintl.h>
#else
# define gettext(msgid) (msgid)
#endif


/* Get the interface, including the syntax bits.  */
#include "regex.h"

/* isalpha etc. are used for the character classes.  */
#include <ctype.h>

#ifndef emacs /* For the emacs build, we need these in the header. */

/* 1 if C is an ASCII character.  */
#define ISASCII(c) ((c) < 0200)

/* 1 if C is a unibyte character.  */
#define ISUNIBYTE(c) 0

#ifdef isblank
# define ISBLANK(c) isblank (c)
#else
# define ISBLANK(c) ((c) == ' ' || (c) == '\t')
#endif
#ifdef isgraph
# define ISGRAPH(c) isgraph (c)
#else
# define ISGRAPH(c) (isprint (c) && !isspace (c))
#endif

/* Solaris defines ISPRINT so we must undefine it first.  */
#undef ISPRINT
#define ISPRINT(c) isprint (c)
#define ISDIGIT(c) isdigit (c)
#define ISALNUM(c) isalnum (c)
#define ISALPHA(c) isalpha (c)
#define ISCNTRL(c) iscntrl (c)
#define ISLOWER(c) islower (c)
#define ISPUNCT(c) ispunct (c)
#define ISSPACE(c) isspace (c)
#define ISUPPER(c) isupper (c)
#define ISXDIGIT(c) isxdigit (c)

#define ISWORD(c) ISALPHA (c)

#ifdef _tolower
# define TOLOWER(c) _tolower (c)
#else
# define TOLOWER(c) tolower (c)
#endif

#endif /* emacs */

#ifndef NULL
#define NULL (void *)0
#endif

/* We remove any previous definition of `SIGN_EXTEND_CHAR',
   since ours (we hope) works properly with all combinations of
   machines, compilers, `char' and `unsigned char' argument types.
   (Per Bothner suggested the basic approach.)  */
#undef SIGN_EXTEND_CHAR
#if __STDC__
#define SIGN_EXTEND_CHAR(c) ((signed char) (c))
#else  /* not __STDC__ */
/* As in Harbison and Steele.  */
#define SIGN_EXTEND_CHAR(c) ((((unsigned char) (c)) ^ 128) - 128)
#endif

/* Should we use malloc or alloca?  If REGEX_MALLOC is not defined, we
   use `alloca' instead of `malloc'.  This is because using malloc in
   re_search* or re_match* could cause memory leaks when C-g is used in
   Emacs; also, malloc is slower and causes storage fragmentation.  On
   the other hand, malloc is more portable, and easier to debug.

   Because we sometimes use alloca, some routines have to be macros,
   not functions -- `alloca'-allocated space disappears at the end of the
   function it is called in.  */

#ifndef emacs
#define ALLOCA alloca
#define xmalloc malloc
#define xrealloc realloc
#define xfree free
#endif

#ifdef emacs
#define ALLOCA_GARBAGE_COLLECT()		\
do						\
{						\
  if (need_to_check_c_alloca)			\
    xemacs_c_alloca (0);			\
} while (0)
#elif defined (C_ALLOCA)
#define ALLOCA_GARBAGE_COLLECT() alloca (0)
#else
#define ALLOCA_GARBAGE_COLLECT()
#endif

#ifndef emacs
/* So we can use just it to conditionalize on */
#undef ERROR_CHECK_MALLOC
#endif

#ifdef ERROR_CHECK_MALLOC
/* When REL_ALLOC, malloc() is problematic because it could potentially
   cause all rel-alloc()ed data -- including buffer text -- to be relocated.
   We deal with this by checking for such relocation whenever we have
   executed a statement that may call malloc() -- or alloca(), which may
   end up calling malloc() in some circumstances -- and recomputing all
   of our string pointers in re_match_2_internal() and re_search_2().
   However, if malloc() or alloca() happens and we don't know about it,
   we could still be screwed.  So we set up a system where we indicate all
   places where we are prepared for malloc() or alloca(), and in any
   other circumstances, calls to those functions (from anywhere inside of
   XEmacs!) will ABORT().  We do this even when REL_ALLOC is not defined
   so that we catch these problems sooner, since many developers and beta
   testers will not be running with REL_ALLOC. */
int regex_malloc_disallowed;
#define BEGIN_REGEX_MALLOC_OK() regex_malloc_disallowed = 0
#define END_REGEX_MALLOC_OK() regex_malloc_disallowed = 1
#define UNBIND_REGEX_MALLOC_CHECK() unbind_to (depth)
#else
#define BEGIN_REGEX_MALLOC_OK()
#define END_REGEX_MALLOC_OK()
#define UNBIND_REGEX_MALLOC_CHECK()
#endif


#ifdef REGEX_MALLOC

#define REGEX_ALLOCATE xmalloc
#define REGEX_REALLOCATE(source, osize, nsize) xrealloc (source, nsize)
#define REGEX_FREE xfree

#else /* not REGEX_MALLOC  */

/* Emacs already defines alloca, sometimes.  */
#ifndef alloca

/* Make alloca work the best possible way.  */
#ifdef __GNUC__
#define alloca __builtin_alloca
#elif defined (__DECC) /* XEmacs: added next 3 lines, similar to config.h.in */
#include <alloca.h>
#pragma intrinsic(alloca)
#else /* not __GNUC__ */
#if HAVE_ALLOCA_H
#include <alloca.h>
#else /* not __GNUC__ or HAVE_ALLOCA_H */
#ifndef _AIX /* Already did AIX, up at the top.  */
void *alloca ();
#endif /* not _AIX */
#endif /* HAVE_ALLOCA_H */
#endif /* __GNUC__ */

#endif /* not alloca */

#define REGEX_ALLOCATE ALLOCA
#define REGEX_REALLOCATE(source, osize, nsize)				\
  (char *) (memmove (ALLOCA (nsize), source, osize))

/* No need to do anything to free, after alloca.
   Do nothing!  But inhibit gcc warning.  */
#define REGEX_FREE(arg,type) ((void)0)

#endif /* REGEX_MALLOC */

/* Define how to allocate the failure stack.  */

#ifdef REGEX_REL_ALLOC
#define REGEX_ALLOCATE_STACK(size)				\
  r_alloc ((unsigned char **) &failure_stack_ptr, (size))
#define REGEX_REALLOCATE_STACK(source, osize, nsize)		\
  r_re_alloc ((unsigned char **) &failure_stack_ptr, (nsize))
#define REGEX_FREE_STACK(ptr)					\
  r_alloc_free ((unsigned char **) &failure_stack_ptr)

#else /* not REGEX_REL_ALLOC */

#ifdef REGEX_MALLOC

#define REGEX_ALLOCATE_STACK xmalloc
#define REGEX_REALLOCATE_STACK(source, osize, nsize) xrealloc (source, nsize)
#define REGEX_FREE_STACK(arg) xfree (arg)

#else /* not REGEX_MALLOC */

#define REGEX_ALLOCATE_STACK ALLOCA

#define REGEX_REALLOCATE_STACK(source, osize, nsize)			\
   REGEX_REALLOCATE (source, osize, nsize)
/* No need to explicitly free anything.  */
#define REGEX_FREE_STACK(arg)

#endif /* REGEX_MALLOC */
#endif /* REGEX_REL_ALLOC */


/* True if `size1' is non-NULL and PTR is pointing anywhere inside
   `string1' or just past its end.  This works if PTR is NULL, which is
   a good thing.  */
#define FIRST_STRING_P(ptr) 					\
  (size1 && string1 <= (ptr) && (ptr) <= string1 + size1)

/* (Re)Allocate N items of type T using malloc, or fail.  */
#define TALLOC(n, t) ((t *) xmalloc ((n) * sizeof (t)))
#define RETALLOC(addr, n, t) ((addr) = (t *) xrealloc (addr, (n) * sizeof (t)))
#define REGEX_TALLOC(n, t) ((t *) REGEX_ALLOCATE ((n) * sizeof (t)))

#define BYTEWIDTH 8 /* In bits.  */

#define STREQ(s1, s2) (strcmp (s1, s2) == 0)

#undef MAX
#undef MIN
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Type of source-pattern and string chars.  */
typedef const unsigned char re_char;

typedef char re_bool;
#define false 0
#define true 1


#ifdef emacs

Lisp_Object Vthe_lisp_rangetab;

void
vars_of_regex (void)
{
  Vthe_lisp_rangetab = Fmake_range_table (Qstart_closed_end_closed);
  staticpro (&Vthe_lisp_rangetab);
}

/* Convert an offset from the start of the logical text string formed by
   concatenating the two strings together into a byte position in the
   Lisp buffer or string that the text represents.  Knows that
   when handling buffer text, the "string" we're passed in is always
   BYTE_BUF_BEGV - BYTE_BUF_ZV. */

static Bytexpos
offset_to_bytexpos (Lisp_Object lispobj, int off)
{
  if (STRINGP (lispobj))
    return (Bytexpos) off;
  else if (BUFFERP (lispobj))
    return off + BYTE_BUF_BEGV (XBUFFER (lispobj));
  else
    return 0;
}

#ifdef REL_ALLOC

/* ORIG_BUFTEXT is the address of BYTE_BUF_BEG (XBUFFER (lispobj)) as of
   entry to re_match_2_internal(), or as of last call to
   RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS() when a relocation was done.
   LISPOBJ is the Lisp object (if any) from which the string to be searched
   is taken.

   If LISPOBJ is a buffer, return a relocation offset to be added to all
   pointers to string data so that they will be accurate again, after an
   allocation or reallocation that potentially relocated the buffer data. */
static inline Bytecount
offset_post_relocation (Lisp_Object lispobj, const Ibyte *orig_buftext)
{
  if (!BUFFERP (lispobj))
    return 0;
  return (BYTE_BUF_BYTE_ADDRESS (XBUFFER (lispobj),
				 BYTE_BUF_BEG (XBUFFER (lispobj))) -
	  orig_buftext);
}

#endif /* REL_ALLOC */

#ifdef ERROR_CHECK_MALLOC

/* NOTE that this can run malloc() so you need to adjust afterwards. */

static int
bind_regex_malloc_disallowed (int value)
{
  /* Tricky, because the act of binding can run malloc(). */
  int old_regex_malloc_disallowed = regex_malloc_disallowed;
  int depth;
  regex_malloc_disallowed = 0;
  depth = record_unwind_protect_restoring_int (&regex_malloc_disallowed,
					       old_regex_malloc_disallowed);
  regex_malloc_disallowed = value;
  return depth;
}

#endif /* ERROR_CHECK_MALLOC */

#endif /* emacs */


/* These are the command codes that appear in compiled regular
   expressions.  Some opcodes are followed by argument bytes.  A
   command code can specify any interpretation whatsoever for its
   arguments.  Zero bytes may appear in the compiled regular expression.  */

typedef enum
{
  no_op = 0,

  /* Succeed right away--no more backtracking.  */
  succeed,

        /* Followed by one byte giving n, then by n literal bytes.  */
  exactn,

        /* Matches any (more or less) character.  */
  anychar,

        /* Matches any one char belonging to specified set.  First
           following byte is number of bitmap bytes.  Then come bytes
           for a bitmap saying which chars are in.  Bits in each byte
           are ordered low-bit-first.  A character is in the set if its
           bit is 1.  A character too large to have a bit in the map is
           automatically not in the set.  */
  charset,

        /* Same parameters as charset, but match any character that is
           not one of those specified.  */
  charset_not,

        /* Start remembering the text that is matched, for storing in a
           register.  Followed by two bytes with the internal register number,
           in the range 1 to the pattern buffer's re_ngroups field. */
  start_memory,

        /* Stop remembering the text that is matched and store it in a memory
           register.  Followed by two bytes with the internal register number,
           in the range 1 to the pattern buffer's re_ngroups field. */
  stop_memory,

        /* Match a duplicate of something remembered. Followed by two bytes
           containing the unsigned internal register number.  */
  duplicate,

        /* Fail unless at beginning of line.  */
  begline,

        /* Fail unless at end of line.  */
  endline,

        /* Succeeds if at beginning of buffer (if emacs) or at beginning
           of string to be matched (if not).  */
  begbuf,

        /* Analogously, for end of buffer/string.  */
  endbuf,

        /* Followed by two byte relative address to which to jump.  */
  jump,

	/* Same as jump, but marks the end of an alternative.  */
  jump_past_alt,

        /* Followed by two-byte relative address of place to resume at
           in case of failure.  */
  on_failure_jump,

	/* Like `on_failure_jump', except that it assumes that the pattern
	   following it is mutually exclusive with the pattern at the
	   destination of the jump: if one matches something, the other won't
	   match at all.  Always used via `on_failure_jump_smart'.

           XEmacs: GNU uses their on_failure_keep_string_jump instead of this,
           which pushes entries onto the register failure stack needlessly. */
  on_failure_jump_exclusive,

	/* Just like `on_failure_jump', except that it checks that we don't get
	   stuck in an infinite loop (matching an empty string
	   indefinitely).  */
  on_failure_jump_loop,

        /* A smart `on_failure_jump' used for greedy * and + operators.
	   regex_compile() analyses the loop before which it is put and if the
	   loop does not require backtracking, it changes it to
	   `on_failure_jump_exclusive', else it just defaults to changing
	   itself into `on_failure_jump_loop'.  */
  on_failure_jump_smart,

        /* Followed by two-byte relative address and two-byte number n.
           After matching N times, jump to the address upon failure.  */
  succeed_n,

        /* Followed by two-byte relative address, and two-byte number n.
           Jump to the address N times, then fail.  */
  jump_n,

        /* Set the following two-byte relative address to the
           subsequent two-byte number.  The address *includes* the two
           bytes of number.  */
  set_number_at,

  wordchar,	/* Matches any word-constituent character.  */
  notwordchar,	/* Matches any char that is not a word-constituent.  */

  wordbeg,	/* Succeeds if at word beginning.  */
  wordend,	/* Succeeds if at word end.  */

  wordbound,	/* Succeeds if at a word boundary.  */
  notwordbound	/* Succeeds if not at a word boundary.  */

#ifdef emacs
  ,before_dot,	/* Succeeds if before point.  */
  at_dot,	/* Succeeds if at point.  */
  after_dot,	/* Succeeds if after point.  */

	/* Matches any character whose syntax is specified.  Followed by
           a byte which contains a syntax code, e.g., Sword.  */
  syntaxspec,

	/* Matches any character whose syntax is not that specified.  */
  notsyntaxspec,

    /* need extra stuff to be able to properly work with XEmacs/Mule
       characters (which may take up more than one byte) */

  charset_mule, /* Matches any character belonging to specified set.
		    The set is stored in "unified range-table
		    format"; see rangetab.c.  Unlike the `charset'
		    opcode, this can handle arbitrary characters.
		    NOTE: This has nothing to do with the `charset' object,
		    despite its name. */

  charset_mule_not,  /* Same parameters as charset_mule, but match any
			character that is not one of those specified.  */

  /* 97/2/17 jhod: The following two were merged back in from the Mule
     2.3 code to enable some language specific processing */
  categoryspec,     /* Matches entries in the character category tables */
  notcategoryspec    /* The opposite of the above */
#endif /* emacs */

} re_opcode_t;

/* Common operations on the compiled pattern.  */

/* Store NUMBER in little-endian format in two contiguous bytes starting at
   DESTINATION.  */

#define STORE_NUMBER(destination, number)				\
  do {									\
    (destination)[0] = (number) & 0377;					\
    (destination)[1] = (number) >> 8;					\
  } while (0)

/* Like STORE_NUMBER(), but check that the destination addresses are between
   PSTART and PEND, assumed to be available within re_match_2_internal(). */
#define STORE_MATCH_NUMBER(destination, number)				\
  do {									\
    if (destination + 1 >= pend || destination < pstart)		\
      {									\
	FREE_VARIABLES ();						\
	return -2;							\
      }									\
    STORE_NUMBER (destination, number);					\
  } while (0)

/* Same as STORE_NUMBER, except increment DESTINATION to
   the byte after where the number is stored.  Therefore, DESTINATION
   must be an lvalue.  */

#define STORE_MATCH_NUMBER_AND_INCR(destination, number)		\
  do {									\
    STORE_MATCH_NUMBER (destination, number);				\
    (destination) += 2;							\
  } while (0)

/* Put into DESTINATION a little-endian number stored in two contiguous bytes
   starting at SOURCE.  */

#define EXTRACT_NUMBER(destination, source)				\
  ((destination) = extract_number (source))

static inline int
extract_number (re_char *source)
{
  int leading_byte = SIGN_EXTEND_CHAR (source[1]);
  return ((unsigned) leading_byte << 8) + (int) ((unsigned char) (source[0]));
}

/* Similar to EXTRACT_NUMBER, but treat the two bytes at SOURCE as an unsigned
   sixteen bit little-endian value; convert this to a non-negative signed
   integer before assigning it to DESTINATION.

   GNU get away without this--they use EXTRACT_NUMBER() instead, relying on
   the fact that decrementing a negative MCNT within the set_number_at,
   jump_n, and succeed_n handling silently wraps with two's complement, giving
   the desired behaviour.

   That would make our debugging significantly harder. Also, optimizing
   compilers have a habit of munging silent wrapping of two's complement,
   since the standards do not guarantee that it is preserved. This approach
   with extract_nonnegative() is cheap (it will be made inline) and
   comprehensible for anyone debugging. */
#define EXTRACT_NONNEGATIVE(destination, source)     \
  ((destination) = extract_nonnegative (source))

static inline int
extract_nonnegative (re_char *source)
{
  int leading_byte = ((unsigned char) source[1]);
  return (leading_byte << 8) + (int) (((unsigned char) (source[0])));
}

/* Same as EXTRACT_NUMBER, except increment SOURCE to after the number.
   SOURCE must be an lvalue.  */

#define EXTRACT_NUMBER_AND_INCR(destination, source)			\
  ((destination) = extract_number_and_incr ((re_char **) (&source)))

static int
extract_number_and_incr (re_char **source)
{
  int num = extract_number (*source);
  *source += 2;
  return num;
}

/* Similar to EXTRACT_NUMBER_AND_INCR, but treat the two bytes at SOURCE
   as an unsigned sixteen bit little-endian value. */
#define EXTRACT_NONNEGATIVE_AND_INCR(destination, source)               \
  ((destination) = extract_nonnegative_and_incr ((re_char **) (&source)))

static int
extract_nonnegative_and_incr (re_char **source)
{
  int num = extract_nonnegative (*source);
  *source += 2;
  return num;
}

/* We use standard I/O for debugging.  */
#include <stdio.h>

/* If DEBUG is defined, Regex prints many voluminous messages about what
   it is doing (if the variable `debug' is nonzero).  If linked with the
   main program in `iregex.c', you can enter patterns and strings
   interactively.  And if linked with the main program in `main.c' and
   the other test files, you can run the already-written tests.  */

#if defined (DEBUG)

#ifndef emacs
/* XEmacs provides its own version of assert() */
/* It is useful to test things that ``must'' be true when debugging.  */
#include <assert.h>
#endif

extern int debug_regexps;

#define DEBUG_RUNTIME_FLAGS debug_regexps

#define DEBUG_PRINT_COMPILED_PATTERN(p, s, e) do {		\
    if (DEBUG_RUNTIME_FLAGS)					\
      print_partial_compiled_pattern (s, e); } while (0)
#define DEBUG_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2) do {	\
    if (DEBUG_RUNTIME_FLAGS)					\
      print_double_string (w, s1, sz1, s2, sz2); } while (0)
#define DEBUG_FAIL_PRINT_COMPILED_PATTERN(p, s, e) do {		\
    if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT)		\
      print_partial_compiled_pattern (s, e); } while (0)
#define DEBUG_FAIL_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2)	\
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT)	 \
      print_double_string (w, s1, sz1, s2, sz2); } while (0)
#define DEBUG_MATCH_PRINT_COMPILED_PATTERN(p, s, e) do {	\
    if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_MATCHING)		\
      print_partial_compiled_pattern (s, e); } while (0)
#define DEBUG_MATCH_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2)	\
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_MATCHING)		\
      print_double_string (w, s1, sz1, s2, sz2); } while (0)

/* Print the fastmap in human-readable form.  */

static void
print_fastmap (char *fastmap)
{
  int was_a_range = 0;
  int i = 0;

  while (i < (1 << BYTEWIDTH))
    {
      if (fastmap[i++])
	{
	  was_a_range = 0;
          putchar (i - 1);
          while (i < (1 << BYTEWIDTH)  &&  fastmap[i])
            {
              was_a_range = 1;
              i++;
            }
	  if (was_a_range)
            {
              putchar ('-');
              putchar (i - 1);
            }
        }
    }
  putchar ('\n');
}


/* Print a compiled pattern string in human-readable form, starting at
   the START pointer into it and ending just before the pointer END.  */

static void
print_partial_compiled_pattern (re_char *start, re_char *end)
{
  int mcnt, mcnt2;
  re_char *p = start;
  re_char *pend = end;

  if (start == NULL)
    {
      puts ("(null)");
      return;
    }

  /* Loop over pattern commands.  */
  while (p < pend)
    {
      printf ("%zd:\t", (Bytecount)(p - start));

      switch ((re_opcode_t) *p++)
	{
        case no_op:
          printf ("/no_op");
          break;

	case succeed:
          printf ("/succeed");
          break;

	case exactn:
	  mcnt = *p++;
          printf ("/exactn/%d", mcnt);
          while (mcnt--)
	    {
	      putchar ('/');
	      putchar (*p++);
            }
          break;

	case start_memory:
	  EXTRACT_NONNEGATIVE_AND_INCR (mcnt, p);
          printf ("/start_memory/%d", mcnt);
          break;

	case stop_memory:
	  EXTRACT_NONNEGATIVE_AND_INCR (mcnt, p);
	  printf ("/stop_memory/%d/", mcnt);
          break;

	case duplicate:
	  EXTRACT_NONNEGATIVE_AND_INCR (mcnt, p);
	  printf ("/duplicate/%d", mcnt);
	  break;

	case anychar:
	  printf ("/anychar");
	  break;

	case charset:
        case charset_not:
          {
            REGISTER int c, last = -100;
	    REGISTER int in_range = 0;

	    printf ("/charset [%s",
	            (re_opcode_t) *(p - 1) == charset_not ? "^" : "");

            assert (p + *p < pend);

            for (c = 0; c < 256; c++)
	      if (((unsigned char) (c / 8) < *p)
		  && (p[1 + (c/8)] & (1 << (c % 8))))
		{
		  /* Are we starting a range?  */
		  if (last + 1 == c && ! in_range)
		    {
		      putchar ('-');
		      in_range = 1;
		    }
		  /* Have we broken a range?  */
		  else if (last + 1 != c && in_range)
		    {
		      putchar (last);
		      in_range = 0;
		    }

		  if (! in_range)
		    putchar (c);

		  last = c;
              }

	    if (in_range)
	      putchar (last);

	    putchar (']');

	    p += 1 + *p;
	  }
	  break;

#ifdef emacs
	case charset_mule:
        case charset_mule_not:
          {
	    int nentries, i;

	    printf ("/charset_mule [%s",
	            (re_opcode_t) *(p - 1) == charset_mule_not ? "^" : "");
	    printf (" flags: 0x%02x ", *p++);
	    nentries = unified_range_table_nentries ((void *) p);
	    for (i = 0; i < nentries; i++)
	      {
		EMACS_INT first, last;
		Lisp_Object dummy_val;

		unified_range_table_get_range ((void *) p, i, &first,
                                               &last, &dummy_val);
		if (first < 0x80)
		  putchar (first);
		else
		  printf ("(0x%zx)", (Bytecount)first);
		if (first != last)
		  {
		    putchar ('-');
		    if (last < 0x80)
		      putchar (last);
		    else
		      printf ("(0x%zx)", (Bytecount)last);
		  }
	      }
	    putchar (']');
	    p += unified_range_table_bytes_used ((void *) p);
	  }
	  break;
#endif

	case begline:
	  printf ("/begline");
          break;

	case endline:
          printf ("/endline");
          break;

	case on_failure_jump:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/on_failure_jump to %zd", (Bytecount)(p + mcnt - start));
          break;

	case on_failure_jump_exclusive:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/on_failure_jump_exclusive to %zd",
		  (Bytecount)(p + mcnt - start));
 	  break;

	case on_failure_jump_loop:
 	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/on_failure_jump_loop to %zd",
		  (Bytecount)(p + mcnt - start));
 	  break;
 
 	case on_failure_jump_smart:
 	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/on_failure_jump_smart to %zd",
		  (Bytecount)(p + mcnt - start));
 	  break;

        case jump_past_alt:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/jump_past_alt to %zd", (Bytecount)(p + mcnt - start));
	  break;

        case jump:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
  	  printf ("/jump to %zd", (Bytecount)(p + mcnt - start));
	  break;

        case succeed_n:
          EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  printf ("/succeed_n to %zd, ", (Bytecount)(p + mcnt - start));
          EXTRACT_NONNEGATIVE_AND_INCR (mcnt2, p);
	  printf ("%d times", mcnt2);
          break;

        case jump_n:
          EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  printf ("/jump_n to %zd, ", (Bytecount)(p + mcnt - start));
          EXTRACT_NONNEGATIVE_AND_INCR (mcnt2, p);
	  printf ("%d times", mcnt2);
          break;

        case set_number_at:
          EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  printf ("/set_number_at location %zd",
		  (Bytecount)(p + mcnt - start));
          EXTRACT_NONNEGATIVE_AND_INCR (mcnt2, p);
	  printf (" to %d", mcnt2);
          break;

        case wordbound:
	  printf ("/wordbound");
	  break;

	case notwordbound:
	  printf ("/notwordbound");
          break;

	case wordbeg:
	  printf ("/wordbeg");
	  break;

	case wordend:
	  printf ("/wordend");

#ifdef emacs
	case before_dot:
	  printf ("/before_dot");
          break;

	case at_dot:
	  printf ("/at_dot");
          break;

	case after_dot:
	  printf ("/after_dot");
          break;

	case syntaxspec:
          printf ("/syntaxspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
          break;

	case notsyntaxspec:
          printf ("/notsyntaxspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
	  break;

/* 97/2/17 jhod Mule category patch */
	case categoryspec:
	  printf ("/categoryspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
	  break;

	case notcategoryspec:
	  printf ("/notcategoryspec");
	  mcnt = *p++;
	  printf ("/%d", mcnt);
	  break;
/* end of category patch */
#endif /* emacs */

	case wordchar:
	  printf ("/wordchar");
          break;

	case notwordchar:
	  printf ("/notwordchar");
          break;

	case begbuf:
	  printf ("/begbuf");
          break;

	case endbuf:
	  printf ("/endbuf");
          break;

        default:
          printf ("?%d", *(p-1));
	}

      putchar ('\n');
    }

  printf ("%zd:\tend of pattern.\n", (Bytecount)(p - start));
}


static void
print_compiled_pattern (struct re_pattern_buffer *bufp)
{
  re_char *buffer = bufp->buffer;

  print_partial_compiled_pattern (buffer, buffer + bufp->used);
  printf ("%zd bytes used/%zd bytes allocated.\n", bufp->used,
	  bufp->allocated);

  if (bufp->fastmap_accurate && bufp->fastmap)
    {
      printf ("fastmap: ");
      print_fastmap (bufp->fastmap);
    }

  printf ("re_nsub: %zd\t", (Bytecount)bufp->re_nsub);
  printf ("re_ngroups: %zd\t", (Bytecount)bufp->re_ngroups);
  printf ("regs_alloc: %d\t", bufp->regs_allocated);
  printf ("can_be_null: %d\t", bufp->can_be_null);
  printf ("newline_anchor: %d\n", bufp->newline_anchor);
  printf ("no_sub: %d\t", bufp->no_sub);
  printf ("not_bol: %d\t", bufp->not_bol);
  printf ("not_eol: %d\t", bufp->not_eol);
  printf ("syntax: %d\n", bufp->syntax);
  /* Perhaps we should print the translate table?  */
  /* and maybe the category table? */

  if (bufp->external_to_internal_register)
    {
      int i;

      printf ("external_to_internal_register:\n");
      for (i = 0; i <= bufp->re_nsub; i++)
	{
	  if (i > 0)
	    printf (", ");
	  printf ("%d -> %d", i, bufp->external_to_internal_register[i]);
	}
      printf ("\n");
    }
}


static void
print_double_string (re_char *where, re_char *string1, int size1,
		     re_char *string2, int size2)
{
  if (where == NULL)
    printf ("(null)");
  else
    {
      int this_char;

      if (FIRST_STRING_P (where))
        {
          for (this_char = where - string1; this_char < size1; this_char++)
            putchar (string1[this_char]);

          where = string2;
        }

      for (this_char = where - string2; this_char < size2; this_char++)
        putchar (string2[this_char]);
    }
}

#else /* not DEBUG */

#ifndef emacs
#undef assert
#define assert(e) ((void) (1))
#define malloc_checking_assert assert
#endif

#define DEBUG_RUNTIME_FLAGS 0

#define DEBUG_PRINT_COMPILED_PATTERN(p, s, e) \
	(USED (p), USED (s), USED (e))
#define DEBUG_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2) \
	(USED (w), USED (s1), USED (sz1), USED (s2), USED (sz2))
#define DEBUG_FAIL_PRINT_COMPILED_PATTERN(p, s, e) \
	(USED (p), USED (s), USED (e))
#define DEBUG_FAIL_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2) \
	(USED (w), USED (s1), USED (sz1), USED (s2), USED (sz2))
#define DEBUG_MATCH_PRINT_COMPILED_PATTERN(p, s, e) \
	(USED (p), USED (s), USED (e))
#define DEBUG_MATCH_PRINT_DOUBLE_STRING(w, s1, sz1, s2, sz2) \
	(USED (w), USED (s1), USED (sz1), USED (s2), USED (sz2))

#endif /* DEBUG */

#define DEBUG_STATEMENT(e) do { if (DEBUG_RUNTIME_FLAGS) { e; } } while (0)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)

#define DEBUG_PRINT(...)  \
  do { if (DEBUG_RUNTIME_FLAGS) printf (__VA_ARGS__); } while (0)
#define DEBUG_FAIL_PRINT(...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT) \
      printf (__VA_ARGS__); } while (0)
#define DEBUG_MATCH_PRINT(...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_MATCHING) \
      printf (__VA_ARGS__); } while (0)
#define DEBUG_COMPILE_PRINT(...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_COMPILATION) \
      printf (__VA_ARGS__); } while (0)

#elif defined(__GNUC__)

# define DEBUG_PRINT(args...)  \
  do { if (DEBUG_RUNTIME_FLAGS) printf (args ); } while (0)
# define DEBUG_FAIL_PRINT(args...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT) \
      printf (args); } while (0)
# define DEBUG_MATCH_PRINT(args...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_MATCHING) \
      printf (args); } while (0)
#define DEBUG_COMPILE_PRINT(...)  \
  do { if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_COMPILATION) \
      printf (__VA_ARGS__); } while (0)

#else /* defined(__STDC_VERSION__) [...] */

/* Expansion portable to all versions of C, but will provoke warnings regarding
   the result of a comma argument unused for the arguments.  */
# define DEBUG_PRINT	(void)
# define DEBUG_FAIL_PRINT (void)
# define DEBUG_MATCH_PRINT (void)
# define DEBUG_COMPILE_PRINT (void)

#endif


/* Set by `re_set_syntax' to the current regexp syntax to recognize.  Can
   also be assigned to arbitrarily: each pattern buffer stores its own
   syntax, so it can be changed between regex compilations.  */
/* This has no initializer because initialized variables in Emacs
   become read-only after dumping.  */
reg_syntax_t re_syntax_options;


/* Specify the precise syntax of regexps for compilation.  This provides
   for compatibility for various utilities which historically have
   different, incompatible syntaxes.

   The argument SYNTAX is a bit mask comprised of the various bits
   defined in regex.h.  We return the old syntax.  */

reg_syntax_t
re_set_syntax (reg_syntax_t syntax)
{
  reg_syntax_t ret = re_syntax_options;

  re_syntax_options = syntax;
  return ret;
}

/* This table gives an error message for each of the error codes listed
   in regex.h.  Obviously the order here has to be same as there.
   POSIX doesn't require that we do anything for REG_NOERROR,
   but why not be nice?  */

static const char *re_error_msgid[] =
{
  "Success",					/* REG_NOERROR */
  "No match",					/* REG_NOMATCH */
  "Invalid regular expression",			/* REG_BADPAT */
  "Invalid collation character",		/* REG_ECOLLATE */
  "Invalid character class name",		/* REG_ECTYPE */
  "Trailing backslash",				/* REG_EESCAPE */
  "Invalid back reference",			/* REG_ESUBREG */
  "Unmatched [ or [^",				/* REG_EBRACK */
  "Unmatched ( or \\(",				/* REG_EPAREN */
  "Unmatched \\{",				/* REG_EBRACE */
  "Invalid content of \\{\\}",			/* REG_BADBR */
  "Invalid range end",				/* REG_ERANGE */
  "Memory exhausted",				/* REG_ESPACE */
  "Invalid preceding regular expression",	/* REG_BADRPT */
  "Premature end of regular expression",	/* REG_EEND */
  "Regular expression too big",			/* REG_ESIZE */
  "Unmatched ) or \\)",				/* REG_ERPAREN */
  "Invalid content of \\{\\}, repetitions too big", /* REG_ESIZEBR  */
#ifdef emacs
  "Invalid syntax designator",			/* REG_ESYNTAX */
  "Ranges may not span charsets",		/* REG_ERANGESPAN */
  "Invalid category designator",		/* REG_ECATEGORY */
#endif
};

/* Avoiding alloca during matching, to placate r_alloc.  */

/* About these various flags:

   MATCH_MAY_ALLOCATE indicates that it's OK to do allocation in the
   searching and matching functions.  In this case, we use local variables
   to hold the values allocated.  If not, we use *global* variables, which
   are pre-allocated.  NOTE: XEmacs ***MUST*** run with MATCH_MAY_ALLOCATE,
   because the regexp routines may get called reentrantly as a result of
   QUIT processing (e.g. under Windows: re_match -> QUIT -> quit_p -> drain
   events -> process WM_INITMENU -> call filter -> re_match; see stack
   trace in signal.c), so we cannot have any global variables (unless we do
   lots of trickiness including some unwind-protects, which isn't worth it
   at this point).

   REL_ALLOC means that the relocating allocator is in use, for buffers
   and such.  REGEX_REL_ALLOC means that we use rel-alloc to manage the
   fail stack, which may grow quite large.  REGEX_MALLOC means we use
   malloc() in place of alloca() to allocate the fail stack -- only
   applicable if REGEX_REL_ALLOC is not defined.
*/

/* Define MATCH_MAY_ALLOCATE unless we need to make sure that the
   searching and matching functions should not call alloca.  On some
   systems, alloca is implemented in terms of malloc, and if we're
   using the relocating allocator routines, then malloc could cause a
   relocation, which might (if the strings being searched are in the
   ralloc heap) shift the data out from underneath the regexp
   routines. [To clarify: The purpose of rel-alloc is to allow data to
   be moved in memory from one place to another so that all data
   blocks can be consolidated together and excess memory released back
   to the operating system.  This requires that all the blocks that
   are managed by rel-alloc go at the very end of the program's heap,
   after all regularly malloc()ed data.  malloc(), however, is used to
   owning the end of the heap, so that when more memory is needed, it
   just expands the heap using sbrk().  This is reconciled by using a
   malloc() (such as malloc.c, gmalloc.c, or recent versions of
   malloc() in libc) where the sbrk() call can be replaced with a
   user-specified call -- in this case, to rel-alloc's r_alloc_sbrk()
   routine.  This routine calls the real sbrk(), but then shifts all
   the rel-alloc-managed blocks forward to the end of the heap again,
   so that malloc() gets the memory it needs in the location it needs
   it at.  The regex routines may well have pointers to buffer data as
   their arguments, and buffers are managed by rel-alloc if rel-alloc
   has been enabled, so calling malloc() may potentially screw things
   up badly if it runs out of space and asks for more from the OS.]

   [[Here's another reason to avoid allocation: Emacs processes input
   from X in a signal handler; processing X input may call malloc; if
   input arrives while a matching routine is calling malloc, then
   we're scrod.  But Emacs can't just block input while calling
   matching routines; then we don't notice interrupts when they come
   in.  So, Emacs blocks input around all regexp calls except the
   matching calls, which it leaves unprotected, in the faith that they
   will not malloc.]] This previous paragraph is irrelevant under XEmacs,
   as we *do not* do anything so stupid as process input from within a
   signal handler.

   However, the regexp routines may get called reentrantly as a result of
   QUIT processing (e.g. under Windows: re_match -> QUIT -> quit_p -> drain
   events -> process WM_INITMENU -> call filter -> re_match; see stack
   trace in signal.c), so we cannot have any global variables (unless we do
   lots of trickiness including some unwind-protects, which isn't worth it
   at this point).  Hence we MUST have MATCH_MAY_ALLOCATE defined.

   Also, the first paragraph does not make complete sense to me -- what
   about the use of rel-alloc to handle the fail stacks?  Shouldn't these
   reallocations potentially cause buffer data to be relocated as well?  I
   must be missing something, though -- perhaps the writer above is
   assuming that the failure stack(s) will always be allocated after the
   buffer data, and thus reallocating them with rel-alloc won't move buffer
   data. (In fact, a cursory glance at the code in ralloc.c seems to
   confirm this.) --ben */

/* Normally, this is fine.  */
#define MATCH_MAY_ALLOCATE

/* When using GNU C, we are not REALLY using the C alloca, no matter
   what config.h may say.  So don't take precautions for it.  */
#ifdef __GNUC__
#undef C_ALLOCA
#endif

/* The match routines may not allocate if (1) they would do it with malloc
   and (2) it's not safe for them to use malloc.
   Note that if REL_ALLOC is defined, matching would not use malloc for the
   failure stack, but we would still use it for the register vectors;
   so REL_ALLOC should not affect this.  */

/* XEmacs can handle REL_ALLOC and malloc() OK */
#if !defined (emacs) && (defined (C_ALLOCA) || defined (REGEX_MALLOC)) && defined (REL_ALLOC)
#undef MATCH_MAY_ALLOCATE
#endif

#if !defined (MATCH_MAY_ALLOCATE) && defined (emacs)
#error regex must be handle reentrancy; MATCH_MAY_ALLOCATE must be defined
#endif


/* Registers are set to a sentinel when they haven't yet matched. This
   declaration is ahead of most of the register-specific stuff in this file
   because its value is examined in the failure stack code. */
static unsigned char reg_unset_dummy;
#define REG_UNSET_VALUE (&reg_unset_dummy)
#define REG_UNSET(e) ((e) == REG_UNSET_VALUE)

/* Failure stack declarations and macros; both re_compile_fastmap and
   re_match_2 use a failure stack.  These have to be macros because of
   REGEX_ALLOCATE_STACK.  */


/* Number of failure points for which to initially allocate space
   when matching.  If this number is exceeded, we allocate more
   space, so it is not a hard limit.  */
#ifndef INIT_FAILURE_ALLOC
#define INIT_FAILURE_ALLOC 20
#endif

/* Roughly the maximum number of failure points on the stack.  Would be
   exactly that if always used MAX_FAILURE_SPACE each time we failed.
   This is a variable only so users of regex can assign to it; we never
   change it ourselves.  */
#if defined (MATCH_MAY_ALLOCATE)
/* 4400 was enough to cause a crash on Alpha OSF/1,
   whose default stack limit is 2mb.  */
int re_max_failures = 40000;
#else
int re_max_failures = 4000;
#endif

union fail_stack_elt
{
  re_char *pointer;
  unsigned int integer;
};

typedef union fail_stack_elt fail_stack_elt_t;

typedef struct
{
  fail_stack_elt_t *stack;
  Elemcount size;
  Elemcount avail;		/* Offset of next open position.  */
  Elemcount frame;		/* Offset of the cur constructed frame.  */
} fail_stack_type;

#define PATTERN_STACK_EMPTY()     (fail_stack.avail == 0)
#define FAIL_STACK_EMPTY()     (fail_stack.frame == 0)
#define FAIL_STACK_FULL()      (fail_stack.avail == fail_stack.size)

/* Define macros to initialize and free the failure stack.
   Do `return -2' if the alloc fails.  */

#ifdef MATCH_MAY_ALLOCATE
#define INIT_FAIL_STACK()				\
  do {							\
    fail_stack.stack = (fail_stack_elt_t *)		\
      REGEX_ALLOCATE_STACK (INIT_FAILURE_ALLOC *	\
			    sizeof (fail_stack_elt_t));	\
							\
    if (fail_stack.stack == NULL)			\
      {							\
        UNBIND_REGEX_MALLOC_CHECK ();			\
	return -2;					\
      }							\
							\
    fail_stack.size = INIT_FAILURE_ALLOC;		\
    fail_stack.avail = 0;				\
    fail_stack.frame = 0;				\
  } while (0)

#define RESET_FAIL_STACK()  REGEX_FREE_STACK (fail_stack.stack)
#else
#define INIT_FAIL_STACK()						\
  do {									\
    fail_stack.avail = 0;						\
    fail_stack.frame = 0;						\
  } while (0)

#define RESET_FAIL_STACK()
#endif


/* Double the size of FAIL_STACK, up to approximately `re_max_failures' items.

   Return 1 if succeeds, and 0 if either ran out of memory
   allocating space for it or it was already too large. */

#define DOUBLE_FAIL_STACK(fail_stack)					\
  ((fail_stack).size > re_max_failures * TYPICAL_FAILURE_SIZE		\
   ? 0									\
   : ((fail_stack).stack = (fail_stack_elt_t *)				\
        REGEX_REALLOCATE_STACK ((fail_stack).stack, 			\
          (fail_stack).size * sizeof (fail_stack_elt_t),		\
          ((fail_stack).size << 1) * sizeof (fail_stack_elt_t)),	\
									\
      (fail_stack).stack == NULL					\
      ? 0								\
      : ((fail_stack).size <<= 1, 					\
         1)))

#if !defined (emacs) || !defined (REL_ALLOC)
#define RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS()
#else

/* Update a relocatable pointer to reflect that its associated buffer has
   been relocated. Don't change NULL pointers or registers that have not
   been set. If assertions are turned on, sanity-check the value passed in,
   making sure it reflects buffer data. Appropriate for use both in
   re_match_2_internal() and re_search_2(). */
#define RELOCATE_IF_OK(val) RELOCATE_IF_OK_1 (val, 1)

/* Same as above, but make no assertions about how plausible the value of
   VAL is. */
#define RELOCATE_IF_OK_NO_ASSERT(val) RELOCATE_IF_OK_1 (val, 0)

/* The implementation of RELOCATE_IF_OK() and RELOCATE_IF_OK_NO_ASSERT(). */
#define RELOCATE_IF_OK_1(val, assert_ok)				\
  do									\
    {									\
      if (assert_ok)							\
	{								\
	  malloc_checking_assert					\
	    (((re_char *) (val) >= (re_char *) string1			\
	      && (re_char *) (val) <= (re_char *) string1 + size1)	\
	     || ((re_char *)(val) >= (re_char *) string2		\
		 && (re_char *)(val)					\
		 <= (re_char *) string2 + size2) ||			\
	     (val) == NULL || REG_UNSET ((unsigned char *)(val)));	\
	}								\
      if ((val) != NULL && !REG_UNSET ((unsigned char *)(val)))		\
	{								\
	  (val) += rmdp_offset;						\
	}								\
    } while (0)

/* Within re_match_2_internal(), check whether the current search string
   reflects a Lisp buffer that has just had its text reallocated. If so,
   update the local saved pointer values to reflect the new text
   addresses. The local values in question are the local values within
   re_match_2_internal(), together with regular expression register values
   and those values on the fail stack. */
#define RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS()                      \
  do                                                                    \
    {                                                                   \
      Bytecount rmdp_offset                                             \
        = offset_post_relocation (lispobj, orig_buftext);               \
                                                                        \
      if (rmdp_offset)                                                  \
        {                                                               \
          /* This block will be executed rarely enough that it would be \
             reasonable to make it a non-inline function. However, as a \
             function it would need to take 17 arguments to modify,     \
             which is very unwieldy. */                                 \
          Elemcount ii;							\
                                                                        \
          RELOCATE_IF_OK (d);                                           \
          RELOCATE_IF_OK (dend);                                        \
          RELOCATE_IF_OK (end_match_1);                                 \
          RELOCATE_IF_OK (end_match_2);                                 \
          RELOCATE_IF_OK (match_end);                                   \
                                                                        \
          if (bufp->re_ngroups)                                         \
            {                                                           \
              /* Register zero is managed specially, see the code in    \
                 succeed_label. */                                      \
              for (ii = 1; ii < num_regs; ii++)                         \
                {                                                       \
                  RELOCATE_IF_OK (regstart[ii]);                        \
                  RELOCATE_IF_OK (regend[ii]);                          \
                }                                                       \
            }                                                           \
                                                                        \
            /* Check the relevant elements on the fail stack. */        \
          ii = fail_stack.avail - 1;                                    \
          while (ii >= 0)                                               \
            {                                                           \
              ii--; /* Skip the frame. */				\
                                                                        \
              /* string_place: */                                       \
              RELOCATE_IF_OK (fail_stack.stack[ii].pointer); ii--;      \
                                                                        \
              ii--; /* pattern_place, not relocatable. */               \
            }                                                           \
                                                                        \
          /* We use the following when examining all the other values   \
             for plausibility, which makes it impractical to examine    \
             them for plausibility themselves. */                       \
                                                                        \
          RELOCATE_IF_OK_NO_ASSERT (string1);                           \
          RELOCATE_IF_OK_NO_ASSERT (string2);                           \
          RELOCATE_IF_OK_NO_ASSERT (end1);                              \
          RELOCATE_IF_OK_NO_ASSERT (end2);                              \
                                                                        \
          /* Careful, orig_buftext is a relocatable pointer too. */     \
          RELOCATE_IF_OK_NO_ASSERT (orig_buftext);                      \
        }                                                               \
    } while (0)
#endif /* !defined (emacs) || !defined (REL_ALLOC) */

#if !defined (emacs) || !defined (REL_ALLOC)
#define RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS()
#else
/* Within re_search_2(), check whether the current search string reflects a
   Lisp buffer that has just had its text reallocated. If so, update the
   function-local saved pointer values to reflect the new text
   addresses. There are no regular expression register values to update, nor
   is there a fail stack. */
#define RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS()                       \
do                                                                        \
{                                                                         \
  Bytecount rmdp_offset = offset_post_relocation (lispobj, orig_buftext); \
                                                                          \
  if (rmdp_offset)                                                        \
    {                                                                     \
      RELOCATE_IF_OK (str1);                                              \
      RELOCATE_IF_OK (str2);                                              \
      RELOCATE_IF_OK (string1);                                           \
      RELOCATE_IF_OK (string2);                                           \
      RELOCATE_IF_OK (d);                                                 \
                                                                          \
      /* Careful, orig_buftext is a relocatable pointer too. */           \
      RELOCATE_IF_OK_NO_ASSERT (orig_buftext);                            \
    }                                                                     \
} while (0)

#endif /* emacs */

/* Within re_comppile_fastmap(), push pointer POINTER onto FAIL_STACK.
   Return 1 if able to do so and 0 if ran out of memory allocating
   space to do so.

   re_compile_fastmap() has no access to buffer data, starts with an empty
   fail_stack, and cleans up the values it adds on exit. This means
   considerations of whether pointers are relocatable don't apply. */
#define PUSH_PATTERN_OP(POINTER, FAIL_STACK)				\
  ((FAIL_STACK_FULL ()							\
    && !DOUBLE_FAIL_STACK (FAIL_STACK))					\
   ? 0									\
   : ((FAIL_STACK).stack[(FAIL_STACK).avail++].pointer = POINTER,	\
      1))

#define PUSH_FAILURE_POINTER_1(item)					\
  fail_stack.stack[fail_stack.avail++].pointer = (re_char *) (item)

/* Push a non-relocatable (non-buffer-text) pointer value onto the failure
   stack.  Assumes the variable `fail_stack'. Should only be called from
   within `PUSH_FAILURE_POINT', itself only from within
   re_match_2_internal().
   If assertions are turned on, assert that item is not one of the
   relocatable values we know about. */
#define PUSH_FAILURE_POINTER(item) do {					\
    malloc_checking_assert (NULL == item ||				\
			    !((item >= string1 && item <= end1) ||	\
			      (item >= string2 && item <= end2) ||	\
			      REG_UNSET ((const re_char *)item)));	\
    PUSH_FAILURE_POINTER_1 (item);					\
  } while (0)

/* Push a pointer to buffer text onto the failure stack. If assertions are
   turned on, sanity-check the pointer. Note that no type info is saved, and
   the relevant code in RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS() is
   fragile and dependent on the order of operations in PUSH_FAILURE_POINT(),
   just as the code in POP_FAILURE_POINT is. */
#define PUSH_FAILURE_RELOCATABLE(item) do {				\
    malloc_checking_assert ((item >= string1 && item <= end1) ||	\
			    (item >= string2 && item <= end2) ||	\
			    item == NULL ||				\
			    REG_UNSET ((unsigned char *)item));		\
    PUSH_FAILURE_POINTER_1 (item);					\
  } while (0)

/* This pushes an integer-valued item onto the failure stack.
   Assumes the variable `fail_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.  */
#define PUSH_FAILURE_INT(item)					\
  fail_stack.stack[fail_stack.avail++].integer = (item)

/* Push a fail_stack_elt_t value onto the failure stack.
   Assumes the variable `fail_stack'.  Probably should only
   be called from within `PUSH_FAILURE_POINT'.  */
#define PUSH_FAILURE_ELT(item)					\
  fail_stack.stack[fail_stack.avail++] =  (item)

/* These three POP... operations complement the three PUSH... operations.
   All assume that `fail_stack' is nonempty.  */
#define POP_FAILURE_POINTER() fail_stack.stack[--fail_stack.avail].pointer

#define POP_PATTERN_OP POP_FAILURE_POINTER

#ifdef emacs
static inline re_char *
pop_failure_relocatable_1 (fail_stack_type *fail_stack_ptr, re_char *string1,
			   re_char *end1, re_char *string2, re_char *end2)
{
  re_char *item = fail_stack_ptr->stack[fail_stack_ptr->avail - 1].pointer;
  malloc_checking_assert ((item >= string1 && item <= end1) ||
			  (item >= string2 && item <= end2) ||
			  item == NULL || REG_UNSET ((unsigned char *)item));
  fail_stack_ptr->avail -= 1;
  return item;
}
#define POP_FAILURE_RELOCATABLE()		\
  pop_failure_relocatable_1 (&fail_stack, string1, end1, string2, end2)
#else
#define POP_FAILURE_RELOCATABLE POP_FAILURE_POINTER 
#endif    

#define POP_FAILURE_INT() fail_stack.stack[--fail_stack.avail].integer
#define POP_FAILURE_ELT() fail_stack.stack[--fail_stack.avail]

#define NUM_FAILURE_ITEMS 3

/* Used to examine the stack (to detect infinite loops).  */
#define FAILURE_PAT(h) fail_stack.stack[(h) - 1].pointer
#define FAILURE_STR(h) (fail_stack.stack[(h) - 2].pointer)
#define NEXT_FAILURE_HANDLE(h) fail_stack.stack[(h) - 3].integer
#define TOP_FAILURE_HANDLE() fail_stack.frame

#define ENSURE_FAIL_STACK(space) do {					\
    while (REMAINING_AVAIL_SLOTS <= space)				\
      {									\
	BEGIN_REGEX_MALLOC_OK ();					\
	if (!DOUBLE_FAIL_STACK (fail_stack))				\
	  {								\
	    END_REGEX_MALLOC_OK ();					\
	    UNBIND_REGEX_MALLOC_CHECK ();				\
	    return -2;							\
	  }								\
	END_REGEX_MALLOC_OK ();						\
	RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();			\
	DEBUG_MATCH_PRINT ("\n  Doubled stack; size now: %zd\n",	\
		      (fail_stack).size);				\
	DEBUG_MATCH_PRINT ("	 slots available: %zd\n",		\
		      REMAINING_AVAIL_SLOTS);				\
      }                                                                 \
  } while (0)

/* Push register NUM onto the stack.  */
#define PUSH_FAILURE_REG(num)						\
do {									\
  ENSURE_FAIL_STACK (3);						\
  DEBUG_MATCH_PRINT ("    Push reg %d (spanning %p -> %p)\n",		\
		num, regstart[num], regend[num]);			\
  PUSH_FAILURE_RELOCATABLE (regstart[num]);				\
  PUSH_FAILURE_RELOCATABLE (regend[num]);				\
  PUSH_FAILURE_INT (num);						\
} while (0)

/* Pop a saved register off the stack.  */
#define POP_FAILURE_REG()						\
do {									\
  regnum_t reg = POP_FAILURE_INT ();					\
  regend[reg] = POP_FAILURE_RELOCATABLE ();				\
  regstart[reg] = POP_FAILURE_RELOCATABLE ();				\
  DEBUG_MATCH_PRINT ("     Pop reg %d (spanning %p -> %p)\n",		\
	       reg, regstart[reg], regend[reg]);			\
} while (0)

/* Check that we are not stuck in an infinite loop.  */
#define CHECK_INFINITE_LOOP(pat_cur, string_place)			\
do {									\
  int failure = TOP_FAILURE_HANDLE();					\
  /* Check for infinite matching loops */				\
  while (failure > 0 &&							\
	 (FAILURE_STR (failure) == string_place				\
	  || FAILURE_STR (failure) == NULL))				\
    {									\
      assert (FAILURE_PAT (failure) >= pstart                           \
	      && FAILURE_PAT (failure) <= pstart + bufp->used);         \
      if (FAILURE_PAT (failure) == pat_cur)				\
	goto fail;							\
      DEBUG_MATCH_PRINT ("  Other pattern: %p\n",			\
			 FAILURE_PAT (failure));			\
      failure = NEXT_FAILURE_HANDLE(failure);				\
    }									\
  DEBUG_MATCH_PRINT ("  Other string: %p\n", FAILURE_STR (failure));	\
} while (0)
    
/* Push the information about the state we will need if we ever fail back to
   it.  Requires variables fail_stack, regstart, regend, and num_regs be
   declared.

   Does `return -2' if runs out of memory. 

   In practical terms, only to be called from within re_match_2_internal. */

#define PUSH_FAILURE_POINT(pattern_place, string_place)			\
do {									\
  /* Must be int, so when we don't save any registers, the arithmetic	\
     of 0 + -1 isn't done as unsigned.  */				\
  DEBUG_STATEMENT ((failure_id++, nfailure_points_pushed++));		\
  									\
  ENSURE_FAIL_STACK (NUM_FAILURE_ITEMS);				\
									\
  if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT)			\
    {									\
      DEBUG_FAIL_PRINT ("\nPUSH_FAILURE_POINT #%d:\n", failure_id);	\
      DEBUG_FAIL_PRINT ("  Before push, next avail: %zd\n",		\
			 (Bytecount) (fail_stack).avail);		\
      DEBUG_FAIL_PRINT ("                     size: %zd\n",		\
			 (Bytecount) (fail_stack).size);		\
      									\
      DEBUG_FAIL_PRINT ("  Push frame index: %zd\n",			\
			 fail_stack.frame);				\
      DEBUG_FAIL_PRINT ("  Push string %p: `", (void *) string_place);	\
      if (string_place == REG_UNSET_VALUE)				\
	{								\
	  DEBUG_FAIL_PRINT ("(unset)");					\
	}								\
      else								\
	{								\
	  DEBUG_PRINT_DOUBLE_STRING (string_place, string1, size1,	\
				     string2, size2);			\
	}								\
      DEBUG_MATCH_PRINT ("'\n  Push pattern %p: ", pattern_place);	\
      DEBUG_PRINT_COMPILED_PATTERN (bufp, pattern_place, pend);		\
    }									\
									\
  PUSH_FAILURE_INT (fail_stack.frame);					\
  PUSH_FAILURE_RELOCATABLE (string_place);				\
  PUSH_FAILURE_POINTER (pattern_place);					\
									\
  /* Close the frame by moving the frame pointer past it.  */		\
  fail_stack.frame = fail_stack.avail;					\
} while (0)

/* Estimate the size of data pushed by a typical failure stack entry.
   An estimate is all we need, because all we use this for
   is to choose a limit for how big to make the failure stack.
   XEmacs: This is actually exact, which is no harm. */
#define TYPICAL_FAILURE_SIZE ((Bytecount) (NUM_FAILURE_ITEMS            \
                                           * sizeof (fail_stack_elt_t)))

/* How many items can still be added to the stack without overflowing it.  */
#define REMAINING_AVAIL_SLOTS ((fail_stack).size - (fail_stack).avail)

/* How many items can still be added to the stack without overflowing it.  */
#define REMAINING_AVAIL_SLOTS ((fail_stack).size - (fail_stack).avail)


/* Pops what PUSH_FAIL_STACK pushes.

   We restore into the following parameters, all of which should be lvalues:
     STR -- the saved data position.
     PAT -- the saved pattern position.
     REGSTART, REGEND -- arrays of string positions.

   Also assumes the variables `fail_stack' and (if debugging), `bufp',
   `pend', `string1', `size1', `string2', and `size2'.  */

#define POP_FAILURE_POINT(str, pat)					\
do {									\
  assert (!FAIL_STACK_EMPTY());						\
									\
  /* Remove failure points and point to how many regs pushed.  */	\
									\
  if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_FAILURE_POINT)			\
    {									\
      DEBUG_FAIL_PRINT ("POP_FAILURE_POINT:\n");			\
      DEBUG_FAIL_PRINT ("  Before pop, next avail: %zd\n",		\
			 (Bytecount) fail_stack.avail);			\
      DEBUG_FAIL_PRINT ("                    size: %zd\n",		\
			 (Bytecount) fail_stack.size);			\
    }									\
									\
  /* Pop the saved registers.  */					\
  while (fail_stack.frame < fail_stack.avail)				\
    POP_FAILURE_REG ();							\
									\
  pat = (unsigned char *) POP_FAILURE_POINTER ();			\
  DEBUG_MATCH_PRINT ("  Popping pattern %p: ", pat);			\
  DEBUG_PRINT_COMPILED_PATTERN (bufp, pat, pend);			\
									\
  str = POP_FAILURE_RELOCATABLE ();					\
									\
  DEBUG_MATCH_PRINT ("  Popping string %p: `", str);			\
  if (str == REG_UNSET_VALUE)						\
    {									\
      DEBUG_FAIL_PRINT ("(unset)");					\
    }									\
  else									\
    {									\
      DEBUG_PRINT_DOUBLE_STRING (str, string1, size1,			\
				 string2, size2);			\
    }									\
									\
  fail_stack.frame = POP_FAILURE_INT ();				\
  DEBUG_MATCH_PRINT ("'\n  Popping  frame index: %zd\n",		\
		     fail_stack.frame);					\
									\
  assert (fail_stack.avail >= 0);					\
  assert (fail_stack.frame <= fail_stack.avail);			\
									\
  DEBUG_STATEMENT (nfailure_points_popped++);				\
} while (0) /* POP_FAILURE_POINT */


/* Subroutine declarations and macros for regex_compile.  */

/* Fetch the next character in the uncompiled pattern---translating it
   if necessary.  */
#define PATFETCH(c)							\
  do {									\
    PATFETCH_RAW (c);							\
    c = RE_TRANSLATE (c);						\
  } while (0)

/* Fetch the next character in the uncompiled pattern, with no
   translation.  */
#define PATFETCH_RAW(c)							\
  do {if (p == pend) return REG_EEND;					\
    assert (p < pend);							\
    c = itext_ichar (p); 						\
    INC_IBYTEPTR (p);							\
  } while (0)

/* Go backwards one character in the pattern.  */
#define PATUNFETCH DEC_IBYTEPTR (p)

/* If `translate' is non-null, return translate[D], else just D.  We
   cast the subscript to translate because some data is declared as
   `char *', to avoid warnings when a string constant is passed.  But
   when we use a character as a subscript we must make it unsigned.  */
#define RE_TRANSLATE(d) \
  (TRANSLATE_P (translate) ? RE_TRANSLATE_1 (d) : (d))

/* Macros for outputting the compiled pattern into `buffer'.  */

/* If the buffer isn't allocated when it comes in, use this.  */
#define INIT_BUF_SIZE  32

/* Make sure we have at least N more bytes of space in buffer.  */
#define GET_BUFFER_SPACE(n)						\
    while (buf_end - bufp->buffer + (n) > (ptrdiff_t) bufp->allocated)	\
      EXTEND_BUFFER ()

/* Make sure we have one more byte of buffer space and then add C to it.  */
#define BUF_PUSH(c)							\
  do {									\
    GET_BUFFER_SPACE (1);						\
    *buf_end++ = (unsigned char) (c);					\
  } while (0)


/* Ensure we have two more bytes of buffer space and then append C1 and C2.  */
#define BUF_PUSH_2(c1, c2)						\
  do {									\
    GET_BUFFER_SPACE (2);						\
    *buf_end++ = (unsigned char) (c1);					\
    *buf_end++ = (unsigned char) (c2);					\
  } while (0)


/* As with BUF_PUSH_2, except for three bytes.  */
#define BUF_PUSH_3(c1, c2, c3)						\
  do {									\
    GET_BUFFER_SPACE (3);						\
    *buf_end++ = (unsigned char) (c1);					\
    *buf_end++ = (unsigned char) (c2);					\
    *buf_end++ = (unsigned char) (c3);					\
  } while (0)


/* Store a jump with opcode OP at LOC to location TO.  We store a
   relative address offset by the three bytes the jump itself occupies.  */
#define STORE_JUMP(op, loc, to) \
  store_op1 (op, loc, (to) - (loc) - 3)

/* Likewise, for a two-argument jump.  */
#define STORE_JUMP2(op, loc, to, arg) \
  store_op2 (op, loc, (to) - (loc) - 3, arg)

/* Like `STORE_JUMP', but for inserting.  Assume `buf_end' is the
   buffer end.  */
#define INSERT_JUMP(op, loc, to) \
  insert_op1 (op, loc, (to) - (loc) - 3, buf_end)

/* Like `STORE_JUMP2', but for inserting.  Assume `buf_end' is the
   buffer end.  */
#define INSERT_JUMP2(op, loc, to, arg) \
  insert_op2 (op, loc, (to) - (loc) - 3, arg, buf_end)

/* Extend the buffer by twice its current size via realloc and
   reset the pointers that pointed into the old block to point to the
   correct places in the new one.  If extending the buffer results in it
   being larger than RE_MAX_BUF_SIZE, then flag memory exhausted.  */
#define EXTEND_BUFFER()							 \
  do {									 \
    re_char *old_buffer = bufp->buffer;					 \
    if (bufp->allocated == RE_MAX_BUF_SIZE)				 \
      return REG_ESIZE;							 \
    bufp->allocated <<= 1;						 \
    if (bufp->allocated > RE_MAX_BUF_SIZE)				 \
      bufp->allocated = RE_MAX_BUF_SIZE;				 \
    bufp->buffer =							 \
      (unsigned char *) xrealloc (bufp->buffer, bufp->allocated);	 \
    if (bufp->buffer == NULL)						 \
      return REG_ESPACE;						 \
    /* If the buffer moved, move all the pointers into it.  */		 \
    if (old_buffer != bufp->buffer)					 \
      {									 \
        buf_end = (buf_end - old_buffer) + bufp->buffer;		 \
        begalt = (begalt - old_buffer) + bufp->buffer;			 \
        if (fixup_alt_jump)						 \
          fixup_alt_jump = (fixup_alt_jump - old_buffer) + bufp->buffer; \
        if (laststart)							 \
          laststart = (laststart - old_buffer) + bufp->buffer;		 \
        if (pending_exact)						 \
          pending_exact = (pending_exact - old_buffer) + bufp->buffer;	 \
      }									 \
  } while (0)

#define INIT_REG_TRANSLATE_SIZE 5

/* Macros for the compile stack.  */

/* Since offsets can go either forwards or backwards, this type needs to be
   able to hold values from -(RE_MAX_BUF_SIZE - 1) to RE_MAX_BUF_SIZE - 1.  */
typedef int pattern_offset_t;

typedef struct
{
  pattern_offset_t begalt_offset;
  pattern_offset_t fixup_alt_jump;
  pattern_offset_t laststart_offset;
  regnum_t regnum;
} compile_stack_elt_t;

typedef struct
{
  compile_stack_elt_t *stack;
  int size;
  int avail;			/* Offset of next open position.  */
} compile_stack_type;

#define INIT_COMPILE_STACK_SIZE 32

#define COMPILE_STACK_EMPTY  (compile_stack.avail == 0)
#define COMPILE_STACK_FULL  (compile_stack.avail == compile_stack.size)

/* The next available element.  */
#define COMPILE_STACK_TOP (compile_stack.stack[compile_stack.avail])

/* Set the bit for character C in a bit vector.  */
#define SET_LIST_BIT(c)				\
  (buf_end[((unsigned char) (c)) / BYTEWIDTH]	\
   |= 1 << (((unsigned char) c) % BYTEWIDTH))

/* Size of bitmap of charset P in bytes.  P is a start of charset,
   i.e. *P is (re_opcode_t) charset or (re_opcode_t) charset_not.  */
#define CHARSET_BITMAP_SIZE(p) ((p)[1] & 0x7F)

#ifdef emacs

/* Set the "bit" for character C in a range table. */
#define SET_RANGETAB_BIT(c) put_range_table (rtab, c, c, Qt)

/* Parse the longest number we can, but don't produce a bignum, that can't
   correspond to anything we're interested in and would needlessly complicate
   code. Also avoid the silent overflow issues of the non-emacs code below.
   If the string at P is not exhausted, leave P pointing at the next
   (probable-)non-digit byte encountered. */
#define GET_UNSIGNED_NUMBER(num) do \
    {                                                                   \
      Ibyte *_gus_numend = NULL;                                        \
      Lisp_Object _gus_numno;                                           \
      /* most-positive-fixnum on 32 bit XEmacs is 10 decimal digits,    \
         nine will keep us in fixnum territory no matter our            \
         architecture */                                                \
      Bytecount limit = min (pend - p, 9);                              \
                                                                        \
      /* Require that any digits are ASCII. We already require that     \
         the user type ASCII in order to type {,(,|, etc, and there is  \
         the potential for security holes in the future if we allow     \
         non-ASCII digits to specify groups in regexps and other        \
         code that parses regexps is not aware of this. */              \
      _gus_numno = parse_integer (p, &_gus_numend, limit, 10, 1,        \
                                  Vdigit_fixnum_ascii);                 \
      if (FIXNUMP (_gus_numno) && XREALFIXNUM (_gus_numno) >= 0)        \
        {                                                               \
          num = XREALFIXNUM (_gus_numno);                               \
          p = _gus_numend;                                              \
        }                                                               \
    } while (0)
#else
/* Get the next unsigned number in the uncompiled pattern.  */
#define GET_UNSIGNED_NUMBER(num) 					\
  { if (p != pend)							\
     {									\
       int _gun_do_unfetch = 1;                                         \
       PATFETCH (c); 							\
       while (ISDIGIT (c)) 						\
         { 								\
           if (num < 0)							\
              num = 0;							\
           num = num * 10 + c - '0'; 					\
           if (p == pend) 						\
             {                                                          \
               _gun_do_unfetch = 0;                                     \
               break; 							\
             }                                                          \
           PATFETCH (c);						\
         } 								\
       if (_gun_do_unfetch)                                             \
         {                                                              \
           /* Make sure P points to the next non-digit character. */    \
           PATUNFETCH;                                                  \
         }                                                              \
     }                                                                  \
  }
#endif

/* Map a string to the char class it names (if any). BEG points to the string
   to be parsed and LIMIT is the length, in bytes, of that string.

   XEmacs; this only handles the NAME part of the [:NAME:] specification of a
   character class name. The GNU emacs version of this function attempts to
   handle the string from [: onwards, and is called re_wctype_parse. Our
   approach means the function doesn't need to be called with every character
   class encountered.

   LENGTH would be a Bytecount if this function didn't need to be compiled
   also for executables that don't include lisp.h

   Return RECC_ERROR if STRP doesn't match a known character class. */
re_wctype_t
re_wctype (const unsigned char *beg, int limit)
{
  /* Sort tests in the length=five case by frequency the classes to minimize
     number of times we fail the comparison.  The frequencies of character class
     names used in Emacs sources as of 2016-07-27:

     $ find \( -name \*.c -o -name \*.el \) -exec grep -h '\[:[a-z]*:]' {} + |
           sed 's/]/]\n/g' |grep -o '\[:[a-z]*:]' |sort |uniq -c |sort -nr
         213 [:alnum:]
         104 [:alpha:]
          62 [:space:]
          39 [:digit:]
          36 [:blank:]
          26 [:word:]
          26 [:upper:]
          21 [:lower:]
          10 [:xdigit:]
          10 [:punct:]
          10 [:ascii:]
           4 [:nonascii:]
           4 [:graph:]
           2 [:print:]
           2 [:cntrl:]
           1 [:ff:]

     If you update this list, consider also updating chain of or'ed conditions
     in execute_charset_mule function. */

  switch (limit) {
  case 4:
    if (!memcmp (beg, "word", 4))      return RECC_WORD;
    break;
  case 5:
    if (!memcmp (beg, "alnum", 5))     return RECC_ALNUM;
    if (!memcmp (beg, "alpha", 5))     return RECC_ALPHA;
    if (!memcmp (beg, "space", 5))     return RECC_SPACE;
    if (!memcmp (beg, "digit", 5))     return RECC_DIGIT;
    if (!memcmp (beg, "blank", 5))     return RECC_BLANK;
    if (!memcmp (beg, "upper", 5))     return RECC_UPPER;
    if (!memcmp (beg, "lower", 5))     return RECC_LOWER;
    if (!memcmp (beg, "punct", 5))     return RECC_PUNCT;
    if (!memcmp (beg, "ascii", 5))     return RECC_ASCII;
    if (!memcmp (beg, "graph", 5))     return RECC_GRAPH;
    if (!memcmp (beg, "print", 5))     return RECC_PRINT;
    if (!memcmp (beg, "cntrl", 5))     return RECC_CNTRL;
    break;
  case 6:
    if (!memcmp (beg, "xdigit", 6))    return RECC_XDIGIT;
    break;
  case 7:
    if (!memcmp (beg, "unibyte", 7))   return RECC_UNIBYTE;
    break;
  case 8:
    if (!memcmp (beg, "nonascii", 8))  return RECC_NONASCII;
    break;
  case 9:
    if (!memcmp (beg, "multibyte", 9)) return RECC_MULTIBYTE;
    break;
  }

  return RECC_ERROR;
}

/* True if CH is in the char class CC.  */
int
re_iswctype (int ch, re_wctype_t cc
             RE_ISWCTYPE_ARG_DECL)
{
  switch (cc)
    {
    case RECC_ALNUM: return ISALNUM (ch) != 0;
    case RECC_ALPHA: return ISALPHA (ch) != 0;
    case RECC_BLANK: return ISBLANK (ch) != 0;
    case RECC_CNTRL: return ISCNTRL (ch) != 0;
    case RECC_DIGIT: return ISDIGIT (ch) != 0;
    case RECC_GRAPH: return ISGRAPH (ch) != 0;
    case RECC_PRINT: return ISPRINT (ch) != 0;
    case RECC_PUNCT: return ISPUNCT (ch) != 0;
    case RECC_SPACE: return ISSPACE (ch) != 0;
#ifdef emacs
    case RECC_UPPER: 
      return NILP (lispbuf->case_fold_search) ? ISUPPER (ch) != 0
        : !NOCASEP (lispbuf, ch);
    case RECC_LOWER: 
      return NILP (lispbuf->case_fold_search) ? ISLOWER (ch) != 0
        : !NOCASEP (lispbuf, ch);
#else
    case RECC_UPPER: return ISUPPER (ch) != 0;
    case RECC_LOWER: return ISLOWER (ch) != 0;
#endif
    case RECC_XDIGIT: return ISXDIGIT (ch) != 0;
    case RECC_ASCII: return ISASCII (ch) != 0;
    case RECC_NONASCII: case RECC_MULTIBYTE: return !ISASCII (ch);
    case RECC_UNIBYTE: return ISUNIBYTE (ch) != 0;
    case RECC_WORD: return ISWORD (ch) != 0;
    case RECC_ERROR: return false;
    }
  assert (0);
  return -1;
}

#ifdef emacs

static re_bool
re_wctype_can_match_non_ascii (re_wctype_t cc)
{
  switch (cc)
    {
    case RECC_ASCII:
    case RECC_UNIBYTE:
    case RECC_CNTRL:
    case RECC_DIGIT:
    case RECC_XDIGIT:
    case RECC_BLANK:
      return false;
    default:
      return true;
    }
}

/* Return a bit-pattern to use in the range-table bits to match multibyte
   chars of class CC.  */
static unsigned char
re_wctype_to_bit (re_wctype_t cc)
{
  switch (cc)
    {
    case RECC_PRINT: case RECC_GRAPH:
    case RECC_ALPHA: return BIT_ALPHA;
    case RECC_ALNUM: case RECC_WORD: return BIT_WORD;
    case RECC_LOWER: return BIT_LOWER;
    case RECC_UPPER: return BIT_UPPER;
    case RECC_PUNCT: return BIT_PUNCT;
    case RECC_SPACE: return BIT_SPACE;
    case RECC_MULTIBYTE: case RECC_NONASCII: 
    case RECC_ASCII: case RECC_DIGIT: case RECC_XDIGIT: case RECC_CNTRL:
    case RECC_BLANK: case RECC_UNIBYTE: case RECC_ERROR: return 0;
    default:
      ABORT ();
      return 0;
    }
}

#endif /* emacs */

static void store_op1 (re_opcode_t op, unsigned char *loc, int arg);
static void store_op2 (re_opcode_t op, unsigned char *loc, int arg1, int arg2);
static void insert_op1 (re_opcode_t op, unsigned char *loc, int arg,
			unsigned char *end);
static void insert_op2 (re_opcode_t op, unsigned char *loc, int arg1, int arg2,
			unsigned char *end);
static re_bool at_begline_loc_p (re_char *pattern, re_char *p,
				 reg_syntax_t syntax);
static re_bool at_endline_loc_p (re_char *p, re_char *pend, int syntax);
static re_bool group_in_compile_stack (compile_stack_type compile_stack,
				       regnum_t regnum);
static reg_errcode_t compile_range (re_char **p_ptr, re_char *pend,
				    RE_TRANSLATE_TYPE translate,
				    reg_syntax_t syntax,
				    unsigned char *b);
#ifdef emacs
static reg_errcode_t compile_extended_range (re_char **p_ptr,
					     re_char *pend,
					     RE_TRANSLATE_TYPE translate,
					     reg_syntax_t syntax,
					     Lisp_Object rtab);
reg_errcode_t compile_char_class (re_wctype_t cc, Lisp_Object rtab,
                                  Bitbyte *flags_out);
#endif
static re_bool mutually_exclusive_p (struct re_pattern_buffer *bufp,
				     re_char *p1, re_char *p2);

static re_bool group_match_null_string_p (re_char **p, re_char *end);
static re_bool alt_match_null_string_p (re_char *p, re_char *end);
static re_bool common_op_match_null_string_p (re_char **p, re_char *end);
static int bcmp_translate (re_char *s1, re_char *s2,
			   REGISTER int len, RE_TRANSLATE_TYPE translate
#ifdef emacs
			   , Internal_Format fmt, Lisp_Object lispobj
#endif
			   );
static int re_match_2_internal (struct re_pattern_buffer *bufp,
				re_char *string1, int size1,
				re_char *string2, int size2, int pos,
				struct re_registers *regs, int stop
				RE_LISP_CONTEXT_ARGS_DECL);

#ifndef MATCH_MAY_ALLOCATE

/* If we cannot allocate large objects within re_match_2_internal,
   we make the fail stack and register vectors global.
   The fail stack, we grow to the maximum size when a regexp
   is compiled.
   The register vectors, we adjust in size each time we
   compile a regexp, according to the number of registers it needs.  */

static fail_stack_type fail_stack;

/* Size with which the following vectors are currently allocated.
   That is so we can make them bigger as needed,
   but never make them smaller.  */
static int regs_allocated_size;

static re_char **     regstart, **     regend;
static re_char **best_regstart, **best_regend;

/* Make the register vectors big enough for NUM_REGS registers,
   but don't make them smaller.  */

static
regex_grow_registers (int num_regs)
{
  if (num_regs > regs_allocated_size)
    {
      RETALLOC (regstart,	num_regs, re_char *);
      RETALLOC (regend,		num_regs, re_char *);
      RETALLOC (best_regstart,	num_regs, re_char *);
      RETALLOC (best_regend,	num_regs, re_char *);

      regs_allocated_size = num_regs;
    }
}

#endif /* not MATCH_MAY_ALLOCATE */

/* Adjust on_failure_jump_smart to either on_failure_jump_exclusive or
   on_failure_jump_loop after the entire pattern has been compiled, so that
   on_failure_jump_smart won't be called when matching.

   Doing this in regex_compile is more important for us than for GNU given that
   we copy the pattern to the C stack on entering re_match_2_internal() (copy
   done for the sake of re-entrancy) and so mutually_exclusively_p() would be
   called more often at match time.

   I considered implementing this by saving pointers (or offsets) to the
   on_failure_jump_smart instructions as we place them in regex_compile(),
   which would have fewer issues with new opcodes or changes to opcode arity,
   but given INSERT_JUMP() can move previous instructions that's difficult,
   and this approach is inexpensive of memory.  */
static inline void
fixup_on_failure_jump_smart (struct re_pattern_buffer *bufp)
{
  unsigned char *begalt = bufp->buffer;
  re_char *buf_end = begalt + bufp->used;

  while (begalt < buf_end)
    {
      switch ((re_opcode_t) *begalt++)
	{
	case no_op:
	case anychar:
	case begline:
	case endline:
	case wordbound:
	case notwordbound:
	case wordbeg:
	case wordend:
#ifdef emacs
	case before_dot:
	case at_dot:
	case after_dot:
	case begbuf:
	case endbuf:
#endif
	case wordchar:
	case notwordchar:
	  break;
	  
#ifdef emacs
	case syntaxspec:
	case notsyntaxspec:
	case categoryspec:
	case notcategoryspec:
#endif /* emacs */
	  begalt++;
	  break;

	case start_memory:
	case stop_memory:
	case duplicate:
	case on_failure_jump:
	case on_failure_jump_exclusive:
	case on_failure_jump_loop:
	case jump:
	case jump_past_alt:
	  begalt += 2;
	  break;

	case succeed_n:
	case jump_n:
	case set_number_at:
	  begalt += 4;
	  break;

	case exactn:
	case charset:
	case charset_not:
	  begalt += 1 + *begalt;
	  break;

#ifdef emacs
	case charset_mule:
	case charset_mule_not:
	  begalt++;
	  begalt += unified_range_table_bytes_used ((void *) begalt);
	  break;
#endif
	case on_failure_jump_smart:
	  {
	    int mcnt;

	    EXTRACT_NUMBER_AND_INCR (mcnt, begalt);

	    if (mutually_exclusive_p (bufp, begalt, begalt + mcnt))
	      {
		/* Use a fast `on_failure_jump_exclusive' loop.  */
		DEBUG_COMPILE_PRINT ("  smart exclusive => fast loop.\n");
		*(begalt - 3) = on_failure_jump_exclusive;
	      }
	    else
	      {
		/* Default to a safe `on_failure_jump_loop'.  */
		DEBUG_COMPILE_PRINT ("  smart default => slow loop.\n");
		*(begalt - 3) = on_failure_jump_loop;
	      }
	    break;
	  }
	}
    }
}


/* `regex_compile' compiles PATTERN (of length SIZE) according to SYNTAX.
   Returns one of error codes defined in `regex.h', or zero for success.

   Assumes the `allocated' (and perhaps `buffer') and `translate'
   fields are set in BUFP on entry.

   If it succeeds, results are put in BUFP (if it returns an error, the
   contents of BUFP are undefined):
     `buffer' is the compiled pattern;
     `syntax' is set to SYNTAX;
     `used' is set to the length of the compiled pattern;
     `fastmap_accurate' is zero;
     `re_ngroups' is the number of groups in PATTERN, an internal count.
     `re_nsub' is the highest register number encountered in the expression
               (named non-shy groups may make this larger than re_ngroups).
     `not_bol' and `not_eol' are zero;

   The `fastmap' and `newline_anchor' fields are neither
   examined nor set.  */

/* Insert the `jump' from the end of last alternative to "here".
   The space for the jump has already been allocated. */
#define FIXUP_ALT_JUMP()						\
do {									\
  if (fixup_alt_jump)							\
    STORE_JUMP (jump_past_alt, fixup_alt_jump, buf_end);		\
} while (0)

/* Return, freeing storage we allocated.  */
#define FREE_STACK_RETURN(value)			\
do							\
{							\
  xfree (compile_stack.stack);				\
  return value;						\
} while (0)

static reg_errcode_t
regex_compile (re_char *pattern, int size, reg_syntax_t syntax,
	       struct re_pattern_buffer *bufp)
{
  /* We fetch characters from PATTERN here.  We declare these as int
     (or possibly long) so that chars above 127 can be used as
     array indices.  The macros that fetch a character from the pattern
     make sure to coerce to unsigned char before assigning, so we won't
     get bitten by negative numbers here. */
  /* XEmacs change: used to be unsigned char. */
  REGISTER EMACS_INT c, c1;

  /* A random temporary spot in PATTERN.  */
  re_char *p1;

  /* Points to the end of the buffer, where we should append.  */
  REGISTER unsigned char *buf_end;

  /* Keeps track of unclosed groups.  */
  compile_stack_type compile_stack;

  /* Points to the current (ending) position in the pattern.  */
  re_char *p = pattern;
  re_char *pend = pattern + size;

  /* How to translate the characters in the pattern.  */
  RE_TRANSLATE_TYPE translate = bufp->translate;

  /* Address of the count-byte of the most recently inserted `exactn'
     command.  This makes it possible to tell if a new exact-match
     character can be added to that command or if the character requires
     a new `exactn' command.  */
  unsigned char *pending_exact = 0;

  /* Address of start of the most recently finished expression.
     This tells, e.g., postfix * where to find the start of its
     operand.  Reset at the beginning of groups and alternatives.  */
  unsigned char *laststart = 0;

  /* Address of beginning of regexp, or inside of last group.  */
  unsigned char *begalt;

  /* Place in the uncompiled pattern (i.e., the {) to
     which to go back if the interval is invalid.  */
  re_char *beg_interval;

  /* Address of the place where a forward jump should go to the end of
     the containing expression.  Each alternative of an `or' -- except the
     last -- ends with a forward jump of this sort.  */
  unsigned char *fixup_alt_jump = 0;

  /* Counts open-groups as they are encountered.  Remembered for the
     matching close-group on the compile stack, so the same register
     number is put in the stop_memory as the start_memory.  */
  regnum_t regnum = 0;

#ifdef DEBUG
  if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_COMPILATION)
    {
      int debug_count;

      DEBUG_COMPILE_PRINT ("\nCompiling pattern: ");
      for (debug_count = 0; debug_count < size; debug_count++)
        putchar (pattern[debug_count]);
      putchar ('\n');
    }
#endif /* DEBUG */

  /* Initialize the compile stack.  */
  compile_stack.stack = TALLOC (INIT_COMPILE_STACK_SIZE, compile_stack_elt_t);
  if (compile_stack.stack == NULL)
    return REG_ESPACE;

  compile_stack.size = INIT_COMPILE_STACK_SIZE;
  compile_stack.avail = 0;

  /* Initialize the pattern buffer.  */
  bufp->syntax = syntax;
  bufp->fastmap_accurate = 0;
  bufp->not_bol = bufp->not_eol = 0;

  /* Set `used' to zero, so that if we return an error, the pattern
     printer (for debugging) will think there's no pattern.  We reset it
     at the end.  */
  bufp->used = 0;

  /* Always count groups, whether or not bufp->no_sub is set.  */
  bufp->re_nsub = 0;
  bufp->re_ngroups = 0;

  bufp->warned_about_incompatible_back_references = 0;

  if (bufp->external_to_internal_register == 0)
    {
      bufp->external_to_internal_register_size = INIT_REG_TRANSLATE_SIZE;
      RETALLOC (bufp->external_to_internal_register,
		bufp->external_to_internal_register_size,
		int);
    }

  {
    int i;

    bufp->external_to_internal_register[0] = 0;
    for (i = 1; i < bufp->external_to_internal_register_size; i++)
      bufp->external_to_internal_register[i] = (int) 0xDEADBEEF;
  }

#if !defined (emacs) && !defined (SYNTAX_TABLE)
  /* Initialize the syntax table.  */
   init_syntax_once ();
#endif

  if (bufp->allocated == 0)
    {
      if (bufp->buffer)
	{ /* If zero allocated, but buffer is non-null, try to realloc
             enough space.  This loses if buffer's address is bogus, but
             that is the user's responsibility.  */
          RETALLOC (bufp->buffer, INIT_BUF_SIZE, unsigned char);
        }
      else
        { /* Caller did not allocate a buffer.  Do it for them.  */
          bufp->buffer = TALLOC (INIT_BUF_SIZE, unsigned char);
        }
      if (!bufp->buffer) FREE_STACK_RETURN (REG_ESPACE);

      bufp->allocated = INIT_BUF_SIZE;
    }

  begalt = buf_end = bufp->buffer;

  /* Loop through the uncompiled pattern until we're at the end.  */
  while (p != pend)
    {
      PATFETCH (c);

      switch (c)
        {
        case '^':
          {
            if (/* If at start of pattern, it's an operator.  */
		p == pattern + 1
		/* Otherwise, depends on what's come before.  */
		|| at_begline_loc_p (pattern, p, syntax))
              BUF_PUSH (begline);
            else
              goto normal_char;
          }
          break;


        case '$':
          {
            if (/* If at end of pattern, it's an operator.  */
		p == pend
		/* Otherwise, depends on what's next.  */
                || at_endline_loc_p (p, pend, syntax))
               BUF_PUSH (endline);
             else
               goto normal_char;
           }
           break;


	case '+':
        case '?':
        case '*':
          /* If there is no previous pattern... */
          if (!laststart)
            {
	      goto normal_char;
            }

          {
	    /* true means zero (many) matches are allowed. */
	    re_bool zero_times_ok = false, many_times_ok = false;
	    re_bool greedy = true;

            /* If there is a sequence of repetition chars, collapse it
               down to just one (the right one).  We can't combine
               interval operators with these because of, e.g., `a{2}*',
               which should only match an even number of `a's.	*/

	    for (;;)
              {
		if (c == '?' && (zero_times_ok || many_times_ok))
		  {
		    greedy = false;
		  }
		else
		  {
		    zero_times_ok |= c != '+';
		    many_times_ok |= c != '?';
		  }

		if (p == pend)
		  {
		    break;
		  }

		PATFETCH (c);

                if (c == '*' || c == '+' || c == '?')
		  {
		    (void) 0;
                  }
		else
		  {
		    PATUNFETCH;
		    break;
		  }

                /* If we get here, we found another repeat character.  
		   "*?" and "+?" and "??" are okay (and mean match
		   minimally), but other sequences (such as "*??" and
		   "+++") are rejected (reserved for future use). */
	        if (!greedy || c != '?')
		  {
		    FREE_STACK_RETURN (REG_BADRPT);
		  }
		greedy = false;
	      }

            /* Star, etc. applied to an empty pattern is equivalent
               to an empty pattern.  */
            if (!laststart)
              break;

	    /* Now we know whether or not zero matches is allowed
	       and also whether or not two or more matches is allowed. */
            if (greedy)
              {
                if (many_times_ok)
                  {
		    /* More than one repetition is allowed, so put in at the
		       end a backward relative jump from `buf_end' to before
		       the next jump we're going to put in below (which jumps
		       from laststart to after this jump). */
		    assert (p - 1 > pattern);

		    /* Allocate the space for the jump.  */
		    GET_BUFFER_SPACE (3);
                    STORE_JUMP (jump, buf_end, laststart - 3);
		
		    /* We've added more stuff to the buffer.  */
		    buf_end += 3;
		  }

		/* On failure, jump from laststart to buf_end + 3, which will
		   be the end of the buffer after this jump is inserted.  */
		GET_BUFFER_SPACE (3);
		if (!zero_times_ok)
		  {
		    assert (many_times_ok);
		    INSERT_JUMP (on_failure_jump_smart, buf_end - 3,
				 buf_end + 3);
		    pending_exact = 0;
		    buf_end += 3;
		  }
		else
		  {
		    INSERT_JUMP (many_times_ok ? on_failure_jump_smart
				 : on_failure_jump, laststart, buf_end + 3);
		    pending_exact = 0;
		    buf_end += 3;
		  }
	      }
	    else		/* not greedy */
	      { /* I wish the greedy and non-greedy cases could be merged. */

		if (many_times_ok)
		  {
		    /* The non-greedy multiple match looks like a
		       repeat..until: we only need a conditional jump at the
		       end of the loop */
		    GET_BUFFER_SPACE (3);
		    STORE_JUMP (on_failure_jump, buf_end, laststart);
		    buf_end += 3;
		    if (zero_times_ok)
		      {
			/* The repeat...until naturally matches one or more.
			   To also match zero times, we need to first jump to
			   the end of the loop (its conditional jump). */
			GET_BUFFER_SPACE (3);
			INSERT_JUMP (jump, laststart, buf_end);
			buf_end += 3;
		      }
		  }
		else
		  {
		    /* non-greedy a?? */
		    GET_BUFFER_SPACE (6);
		    INSERT_JUMP (jump, laststart, buf_end + 3);
		    buf_end += 3;
		    INSERT_JUMP (on_failure_jump, laststart, laststart + 6);
		    buf_end += 3;
		  }
	      }
	  }
	  break;

	case '.':
          laststart = buf_end;
          BUF_PUSH (anychar);
          break;

#ifdef emacs
#define MAYBE_START_OVER_WITH_EXTENDED(ch)	\
	  if (ch >= 0x80) do			\
	    {					\
	      goto start_over_with_extended;	\
	    } while (0)
#else
#define MAYBE_START_OVER_WITH_EXTENDED(ch) (void)(ch)
#endif

        case '[':
          {
	    /* XEmacs change: this whole section */
            re_bool had_char_class = false;

            if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

            /* Ensure that we have enough space to push a charset: the
               opcode, the length count, and the bitset; 34 bytes in all.  */
	    GET_BUFFER_SPACE (34);

            laststart = buf_end;

            /* We test `*p == '^' twice, instead of using an if
               statement, so we only need one BUF_PUSH.  */
            BUF_PUSH (*p == '^' ? charset_not : charset);
            if (*p == '^')
              p++;

            /* Remember the first position in the bracket expression.  */
            p1 = p;

            /* Push the number of bytes in the bitmap.  */
            BUF_PUSH ((1 << BYTEWIDTH) / BYTEWIDTH);

            /* Clear the whole map.  */
            memset (buf_end, 0, (1 << BYTEWIDTH) / BYTEWIDTH);

            /* Read in characters and ranges, setting map bits.  */
            for (;;)
              {
                if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

                PATFETCH (c);

		/* Frumble-bumble, we may have found some extended chars.
		   Need to start over, process everything using the general
		   extended-char mechanism, and need to use charset_mule and
		   charset_mule_not instead of charset and charset_not. */
		MAYBE_START_OVER_WITH_EXTENDED (c);

                /* Could be the end of the bracket expression.  If it's
                   not (i.e., when the bracket expression is `[]' so
                   far), the ']' character bit gets set way below.  */
                if (c == ']' && p != p1 + 1)
                  break;

                /* Look ahead to see if it's a range when the last thing
                   was a character class.  */
                if (had_char_class && c == '-' && *p != ']')
                  FREE_STACK_RETURN (REG_ERANGE);

                /* Look ahead to see if it's a range when the last thing
                   was a character: if this is a hyphen not at the
                   beginning or the end of a list, then it's the range
                   operator.  */
                if (c == '-'
                    && !(p - 2 >= pattern && p[-2] == '[')
		    && !(p - 3 >= pattern && p[-3] == '[' && p[-2] == '^')
                    && *p != ']')
                  {
                    reg_errcode_t ret;

		    MAYBE_START_OVER_WITH_EXTENDED (*(unsigned char *)p);

		    ret = compile_range (&p, pend, translate, syntax,
					 buf_end);

                    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
                  }

                else if (p[0] == '-' && p[1] != ']')
                  { /* This handles ranges made up of characters only.  */
                    reg_errcode_t ret;

		    /* Move past the `-'.  */
                    PATFETCH (c1);

		    MAYBE_START_OVER_WITH_EXTENDED (*(unsigned char *)p);

		    ret = compile_range (&p, pend, translate, syntax, buf_end);

                    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
                  }

                /* See if we're at the beginning of a possible character
                   class.  */

                else if (syntax & RE_CHAR_CLASSES && c == '[' && *p == ':')
                  { 
                    re_char *str = p + 1;

                    PATFETCH (c);
                    c1 = 0;

                    /* If pattern is `[[:'.  */
                    if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

                    for (;;)
                      {
		        PATFETCH (c);
                        if ((c == ':' && *p == ']') || p == pend)
                          {
                            break;
                          }

                        c1++;
                      }

                    /* If isn't a word bracketed by `[:' and `:]':
                       undo the ending character, the letters, and leave
                       the leading `:' and `[' (but set bits for them).  */
                    if (c == ':' && *p == ']')
                      {
			re_wctype_t cc = re_wctype (str, c1);
                        int ch;

			if (cc == RECC_ERROR)
			  FREE_STACK_RETURN (REG_ECTYPE);

                        /* Throw away the ] at the end of the character
                           class.  */
                        PATFETCH (c);

                        if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

#ifdef emacs
			if (re_wctype_can_match_non_ascii (cc))
			  {
			    goto start_over_with_extended;
			  }
#endif
			for (ch = 0; ch < (1 << BYTEWIDTH); ++ch)
			  {
			    if (re_iswctype (ch, cc
                                             RE_ISWCTYPE_ARG (current_buffer)))
			      {
				SET_LIST_BIT (ch);
			      }
			  }

                        had_char_class = true;
                      }
                    else
                      {
                        c1++;
                        while (c1--)
                          PATUNFETCH;
                        SET_LIST_BIT ('[');
                        SET_LIST_BIT (':');
                        had_char_class = false;
                      }
                  }
                else
                  {
                    had_char_class = false;
                    SET_LIST_BIT (c);
                  }
              }

            /* Discard any (non)matching list bytes that are all 0 at the
               end of the map.  Decrease the map-length byte too.  */
            while ((int) buf_end[-1] > 0 && buf_end[buf_end[-1] - 1] == 0)
              buf_end[-1]--;
            buf_end += buf_end[-1];
	  }
	  break;

#ifdef emacs
        start_over_with_extended:
          {
            REGISTER Lisp_Object rtab = Qnil;
            Bitbyte flags = 0;
            int bytes_needed = sizeof (flags);
            re_bool had_char_class = false;

            /* There are extended chars here, which means we need to use the
               unified range-table format. */
            if (buf_end[-2] == charset)
              buf_end[-2] = charset_mule;
            else
              buf_end[-2] = charset_mule_not;
            buf_end--;
            p = p1; /* go back to the beginning of the charset, after
                       a possible ^. */
            rtab = Vthe_lisp_rangetab;
            Fclear_range_table (rtab);

            /* Read in characters and ranges, setting map bits.  */
            for (;;)
              {
                if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

                PATFETCH (c);

                /* Could be the end of the bracket expression.  If it's
                   not (i.e., when the bracket expression is `[]' so
                   far), the ']' character bit gets set way below.  */
                if (c == ']' && p != p1 + 1)
                  break;

                /* Look ahead to see if it's a range when the last thing
                   was a character class.  */
                if (had_char_class && c == '-' && *p != ']')
                  FREE_STACK_RETURN (REG_ERANGE);

                /* Look ahead to see if it's a range when the last thing
                   was a character: if this is a hyphen not at the
                   beginning or the end of a list, then it's the range
                   operator.  */
                if (c == '-'
                    && !(p - 2 >= pattern && p[-2] == '[')
                    && !(p - 3 >= pattern && p[-3] == '[' && p[-2] == '^')
                    && *p != ']')
                  {
                    reg_errcode_t ret;

                    ret = compile_extended_range (&p, pend, translate, syntax,
                                                  rtab);

                    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
                  }

                else if (p[0] == '-' && p[1] != ']')
                  { /* This handles ranges made up of characters only.  */
                    reg_errcode_t ret;

                    /* Move past the `-'.  */
                    PATFETCH (c1);
                    
                    ret = compile_extended_range (&p, pend, translate,
                                                  syntax, rtab);
                    if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);
                  }

                /* See if we're at the beginning of a possible character
                   class.  */

                else if (syntax & RE_CHAR_CLASSES && c == '[' && *p == ':')
                  {
                    re_char *str = p + 1;

                    PATFETCH (c);
                    c1 = 0;

                    /* If pattern is `[[:'.  */
                    if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

                    for (;;)
                      {
                        PATFETCH (c);
                        if ((c == ':' && *p == ']') || p == pend)
                          {
                            break;
                          }

                        c1++;
                      }

                    /* If isn't a word bracketed by `[:' and `:]':
                       undo the ending character, the letters, and leave
                       the leading `:' and `[' (but set bits for them).  */
                    if (c == ':' && *p == ']')
                      {
                        re_wctype_t cc = re_wctype (str, c1);
                        reg_errcode_t ret = REG_NOERROR;

                        if (cc == RECC_ERROR)
                          FREE_STACK_RETURN (REG_ECTYPE);

                        /* Throw away the ] at the end of the character
                           class.  */
                        PATFETCH (c);

                        if (p == pend) FREE_STACK_RETURN (REG_EBRACK);

                        ret = compile_char_class (cc, rtab, &flags);

                        if (ret != REG_NOERROR) FREE_STACK_RETURN (ret);

                        had_char_class = true;
                      }
                    else
                      {
                        c1++;
                        while (c1--)
                          PATUNFETCH;
                        SET_RANGETAB_BIT ('[');
                        SET_RANGETAB_BIT (':');
                        had_char_class = false;
                      }
                  }
                else
                  {
                    had_char_class = false;
                    SET_RANGETAB_BIT (c);
                  }
              }

            bytes_needed += unified_range_table_bytes_needed (rtab);
            GET_BUFFER_SPACE (bytes_needed);
            *buf_end++ = flags;
            unified_range_table_copy_data (rtab, buf_end);
            buf_end += unified_range_table_bytes_used (buf_end);
            break;
          }
#endif /* emacs */

        case '\\':
          if (p == pend) FREE_STACK_RETURN (REG_EESCAPE);

          /* Do not translate the character after the \, so that we can
             distinguish, e.g., \B from \b, even if we normally would
             translate, e.g., B to b.  */
          PATFETCH_RAW (c);

          switch (c)
            {
            case '(':
              {
                regnum_t r = 0;
		re_bool shy = 0, named_nonshy = 0;

                if (p != pend && itext_ichar_eql (p, '?'))
                  {
                    INC_IBYTEPTR (p); /* Gobble up the '?'.  */
                    PATFETCH (c); /* Fetch the next character, which may be a
                                     digit. */
                    switch (c)
                      {
                      case ':': /* shy groups */
                        shy = 1;
                        break;

                      case '1': case '2': case '3': case '4':
                      case '5': case '6': case '7': case '8': case '9':
                        PATUNFETCH;
                        GET_UNSIGNED_NUMBER (r);
                        if (itext_ichar_eql (p, ':'))
                          {
                            named_nonshy = 1;
                            INC_IBYTEPTR (p); /* Gobble up the ':'. */
                            break;
                            /* Otherwise, fall through and error. */
                          }

                        /* An explicitly specified regnum must start with
                           non-0. */
                      case '0':
                      default:
                        FREE_STACK_RETURN (REG_BADPAT);
                      }
                  }

                if (!shy)
                  {
                    ++regnum;
                    bufp->re_ngroups++;

                    if (bufp->re_ngroups > MAX_REGNUM)
                      {
                        FREE_STACK_RETURN (REG_ESUBREG);
                      }

                    if (named_nonshy)
                      {
                        if (r < bufp->external_to_internal_register_size)
                          {
                            if (group_in_compile_stack
                                (compile_stack,
                                 bufp->external_to_internal_register[r]))
                              {
                                /* GNU errors in this context, which is
                                   inconsistent; it otherwise has no problem
                                   with named non-shy groups overriding
                                   previously-assigned group numbers. I choose
                                   to error here for consistency with GNU for
                                   those writing code that should target
                                   both. */
                                FREE_STACK_RETURN (REG_ESUBREG);
                              }
                          }

                        if (r > bufp->re_nsub)
                          {
                            bufp->re_nsub = r;
                          }
                      }
                    else
                      {
                        r = ++(bufp->re_nsub);
                      }

		    while (bufp->external_to_internal_register_size <=
			   bufp->re_nsub)
		      {
			int i;
			int old_size =
			  bufp->external_to_internal_register_size;
			bufp->external_to_internal_register_size
                          += max (old_size + 5, bufp->re_nsub + 5);

			RETALLOC (bufp->external_to_internal_register,
				  bufp->external_to_internal_register_size,
				  int);

			for (i = old_size;
			     i < bufp->external_to_internal_register_size; i++)
			  bufp->external_to_internal_register[i] =
			    (int) 0xDEADBEEF;
		      }

                    /* This is explicitly [r] rather than [bufp->re_nsub] for
                       the case that the named nonshy group references an
                       unused register number less than bufp->re_nsub. */
		    bufp->external_to_internal_register[r] =
		      bufp->re_ngroups;
		  }

                if (COMPILE_STACK_FULL)
                  {
                    RETALLOC (compile_stack.stack, compile_stack.size << 1,
                              compile_stack_elt_t);
                    if (compile_stack.stack == NULL) return REG_ESPACE;

                    compile_stack.size <<= 1;
                  }

                /* These are the values to restore when we hit end of this
                   group.  They are all relative offsets, so that if the
                   whole pattern moves because of realloc, they will still
                   be valid.  */
                COMPILE_STACK_TOP.begalt_offset = begalt - bufp->buffer;
                COMPILE_STACK_TOP.fixup_alt_jump
                  = fixup_alt_jump ? fixup_alt_jump - bufp->buffer + 1 : 0;
                COMPILE_STACK_TOP.laststart_offset = buf_end - bufp->buffer;
                COMPILE_STACK_TOP.regnum = shy ? -(bufp->re_ngroups)
		  : bufp->re_ngroups;

                /* Do not push a start_memory for shy groups. */
		if (!shy)
		  {
		    GET_BUFFER_SPACE (3);
		    store_op1 (start_memory, buf_end, bufp->re_ngroups);
		    buf_end += 3;
		  }

                compile_stack.avail++;

                fixup_alt_jump = 0;
                laststart = 0;
                begalt = buf_end;
                pending_exact = 0;
              }
              break;


            case ')':
              if (COMPILE_STACK_EMPTY)
		{
		  FREE_STACK_RETURN (REG_ERPAREN);
		}

	      FIXUP_ALT_JUMP ();

              /* Since we just checked for an empty stack above, this
                 ``can't happen''.  */
              assert (compile_stack.avail != 0);
              {
                /* We don't just want to restore into `regnum', because later
                   groups should continue to be numbered higher, as in
                   `(ab)c(de)' -- the second group is #2.  */
                regnum_t this_group_regnum;

                compile_stack.avail--;
                begalt = bufp->buffer + COMPILE_STACK_TOP.begalt_offset;
                fixup_alt_jump
                  = COMPILE_STACK_TOP.fixup_alt_jump
                    ? bufp->buffer + COMPILE_STACK_TOP.fixup_alt_jump - 1
                    : 0;
                laststart = bufp->buffer + COMPILE_STACK_TOP.laststart_offset;
                this_group_regnum = COMPILE_STACK_TOP.regnum;
		pending_exact = 0;

		/* Don't push a stop_memory for shy groups. */
		if (this_group_regnum > 0)
		  {
		    GET_BUFFER_SPACE (3);
		    store_op1 (stop_memory, buf_end, this_group_regnum);
		    buf_end += 3;
		  }
              }
              break;


            case '|':					/* `\|'.  */
              /* Insert before the previous alternative a jump which
                 jumps to this alternative if the former fails.  */
              GET_BUFFER_SPACE (3);
              INSERT_JUMP (on_failure_jump, begalt, buf_end + 6);
              pending_exact = 0;
              buf_end += 3;

              /* The alternative before this one has a jump after it
                 which gets executed if it gets matched.  Adjust that
                 jump so it will jump to this alternative's analogous
                 jump (put in below, which in turn will jump to the next
                 (if any) alternative's such jump, etc.).  The last such
                 jump jumps to the correct final destination.  A picture:
                          _____ _____
                          |   | |   |
                          |   v |   v
                         a | b   | c

                 If we are at `b', then fixup_alt_jump right now points to a
                 three-byte space after `a'.  We'll put in the jump, set
                 fixup_alt_jump to right after `b', and leave behind three
                 bytes which we'll fill in when we get to after `c'.  */

	      FIXUP_ALT_JUMP ();

              /* Mark and leave space for a jump after this alternative,
                 to be filled in later either by next alternative or
                 when know we're at the end of a series of alternatives.  */
              fixup_alt_jump = buf_end;
              GET_BUFFER_SPACE (3);
              buf_end += 3;

              laststart = 0;
              begalt = buf_end;
              break;


            case '{':
              /* If \{ is a literal.  */
              if (!(syntax & RE_INTERVALS)
                  || (p - 2 == pattern  &&  p == pend))
                goto normal_backslash;

#define BAD_INTERVAL FREE_STACK_RETURN

              {
                /* If got here, then the syntax allows intervals.  */

                /* At least (most) this many matches must be made.  */
                int lower_bound = 0, upper_bound = -1;

                beg_interval = p - 1;

                if (p == pend || itext_ichar_eql (p, '+'))
                  {
                    BAD_INTERVAL (REG_EBRACE);
                  }

                GET_UNSIGNED_NUMBER (lower_bound);

                if (p == pend) BAD_INTERVAL (REG_EBRACE);
                PATFETCH (c);

                if (c == ',')
                  {
                    if (p == pend || itext_ichar_eql (p, '+'))
                      BAD_INTERVAL (REG_EBRACE);

                    GET_UNSIGNED_NUMBER (upper_bound);
                    if (upper_bound < 0) upper_bound = RE_DUP_MAX;

                    if (p == pend) BAD_INTERVAL (REG_EBRACE);
                    PATFETCH (c);
                  }
                else
                  {
                    /* Interval such as `{1}' => match exactly once. */
                    upper_bound = lower_bound;
                  }

                if (lower_bound > upper_bound)
                  {
                    BAD_INTERVAL (REG_EBRACE);
                  }

                if (upper_bound > RE_DUP_MAX)
                  {
                    BAD_INTERVAL (REG_ESIZEBR);
                  }

		if (c != '\\')
		  FREE_STACK_RETURN (REG_BADBR);
		if (p == pend)
		  FREE_STACK_RETURN (REG_EESCAPE);
		PATFETCH (c);

                if (c != '}')
                  {
                    BAD_INTERVAL (REG_EBRACE);
                  }

                /* We just parsed a valid interval.  */

                /* It's invalid to have no preceding RE.  */
                if (!laststart)
                  {
		    goto unfetch_interval;
                  }

                /* If the upper bound is zero, don't want to succeed at
                   all; jump from `laststart' to `b + 3', which will be
                   the end of the buffer after we insert the jump.  */
                 if (upper_bound == 0)
                   {
                     GET_BUFFER_SPACE (3);
                     INSERT_JUMP (jump, laststart, buf_end + 3);
                     buf_end += 3;
                   }

                 /* Otherwise, we have a nontrivial interval.  When
                    we're all done, the pattern will look like:
                      set_number_at <jump count> <upper bound>
                      set_number_at <succeed_n count> <lower bound>
                      succeed_n <after jump addr> <succeed_n count>
                      <body of loop>
                      jump_n <succeed_n addr> <jump count>
                    (The upper bound and `jump_n' are omitted if
                    `upper_bound' is 1, though.)  */
                 else
                   { /* If the upper bound is > 1, we need to insert
                        more at the end of the loop.  */
                     int nbytes = 10 + (upper_bound > 1) * 10;

                     GET_BUFFER_SPACE (nbytes);

                     /* Initialize lower bound of the `succeed_n', even
                        though it will be set during matching by its
                        attendant `set_number_at' (inserted next),
                        because `re_compile_fastmap' needs to know.
                        Jump to the `jump_n' we might insert below.  */
                     INSERT_JUMP2 (succeed_n, laststart,
                                   buf_end + 5 + (upper_bound > 1) * 5,
                                   lower_bound);
                     buf_end += 5;

                     /* Code to initialize the lower bound.  Insert
                        before the `succeed_n'.  The `5' is the last two
                        bytes of this `set_number_at', plus 3 bytes of
                        the following `succeed_n'.  */
                     insert_op2 (set_number_at, laststart, 5, lower_bound, buf_end);
                     buf_end += 5;

                     if (upper_bound > 1)
                       { /* More than one repetition is allowed, so
                            append a backward jump to the `succeed_n'
                            that starts this interval.

                            When we've reached this during matching,
                            we'll have matched the interval once, so
                            jump back only `upper_bound - 1' times.  */
                         STORE_JUMP2 (jump_n, buf_end, laststart + 5,
                                      upper_bound - 1);
                         buf_end += 5;

                         /* The location we want to set is the second
                            parameter of the `jump_n'; that is `b-2' as
                            an absolute address.  `laststart' will be
                            the `set_number_at' we're about to insert;
                            `laststart+3' the number to set, the source
                            for the relative address.  But we are
                            inserting into the middle of the pattern --
                            so everything is getting moved up by 5.
                            Conclusion: (b - 2) - (laststart + 3) + 5,
                            i.e., b - laststart.

                            We insert this at the beginning of the loop
                            so that if we fail during matching, we'll
                            reinitialize the bounds.  */
                         insert_op2 (set_number_at, laststart,
				     buf_end - laststart,
                                     upper_bound - 1, buf_end);
                         buf_end += 5;
                       }
                   }
                pending_exact = 0;
                beg_interval = NULL;
              }
              break;
#undef BAD_INTERVAL

            unfetch_interval:
              /* If an invalid interval, match the characters as literals.  */
               assert (beg_interval);
               p = beg_interval;
               beg_interval = NULL;

               /* normal_char and normal_backslash need `c'.  */
               PATFETCH (c);

	       if (p > pattern  &&  p[-1] == '\\')
		 {
		   goto normal_backslash;
		 }
               goto normal_char;

#ifdef emacs
            /* There is no way to specify the before_dot and after_dot
               operators.  rms says this is ok.  --karl  */
            case '=':
              BUF_PUSH (at_dot);
              break;

            case 's':
              laststart = buf_end;
              PATFETCH (c);
	      /* XEmacs addition */
	      if (c >= 0x80 || syntax_spec_code[c] == 0377)
		FREE_STACK_RETURN (REG_ESYNTAX);
              BUF_PUSH_2 (syntaxspec, syntax_spec_code[c]);
              break;

            case 'S':
              laststart = buf_end;
              PATFETCH (c);
	      /* XEmacs addition */
	      if (c >= 0x80 || syntax_spec_code[c] == 0377)
		FREE_STACK_RETURN (REG_ESYNTAX);
              BUF_PUSH_2 (notsyntaxspec, syntax_spec_code[c]);
              break;

/* 97.2.17 jhod merged in to XEmacs from mule-2.3 */
	    case 'c':
	      laststart = buf_end;
	      PATFETCH_RAW (c);
	      if (c < 32 || c > 127)
		FREE_STACK_RETURN (REG_ECATEGORY);
	      BUF_PUSH_2 (categoryspec, c);
	      break;

	    case 'C':
	      laststart = buf_end;
	      PATFETCH_RAW (c);
	      if (c < 32 || c > 127)
		FREE_STACK_RETURN (REG_ECATEGORY);
	      BUF_PUSH_2 (notcategoryspec, c);
	      break;
/* end of category patch */
#endif /* emacs */


            case 'w':
              laststart = buf_end;
              BUF_PUSH (wordchar);
              break;


            case 'W':
              laststart = buf_end;
              BUF_PUSH (notwordchar);
              break;


            case '<':
              BUF_PUSH (wordbeg);
              break;

            case '>':
              BUF_PUSH (wordend);
              break;

            case 'b':
              BUF_PUSH (wordbound);
              break;

            case 'B':
              BUF_PUSH (notwordbound);
              break;

            case '`':
              BUF_PUSH (begbuf);
              break;

            case '\'':
              BUF_PUSH (endbuf);
              break;

            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
	      {
		regnum_t reg = -1, regint;

                PATUNFETCH;
                GET_UNSIGNED_NUMBER (reg);
		  
                /* Progressively divide down the backreference until we find
                   one that corresponds to an existing register. */
                while (reg > 10 &&
                       (syntax & RE_NO_MULTI_DIGIT_BK_REFS
                        || reg > bufp->re_nsub
                        || (bufp->external_to_internal_register[reg]
                            == (int) 0xDEADBEEF)))

                  {
                    PATUNFETCH;
                    reg /= 10;
                  }

                if (reg > bufp->re_nsub
                    || (bufp->external_to_internal_register[reg]
                        == (int) 0xDEADBEEF))
                  {
                    /* \N with one digit with a non-existing group has always
                       been a syntax error.

                       GNU as of Fr 27 Mär 2020 16:24:07 GMT do not accept
                       multidigit backreferences; if they did there would be
                       an argument for this not being an error for those
                       backreferences that are less than some known named
                       backreference. As it is currently we should error, this
                       will give those writing code for XEmacs better
                       feedback. */
                    FREE_STACK_RETURN (REG_ESUBREG);
                  }

		regint = bufp->external_to_internal_register[reg];
		/* Can't back reference to a subexpression if inside of it.  */
		if (group_in_compile_stack (compile_stack, regint))
		  {
                    /* Check REG, not REGINT. */
                    while (reg > 10)
                      {
                        PATUNFETCH;
                        reg = reg / 10;
                      }
                    goto normal_char;
		  }

#ifdef emacs
		if (reg > 9 &&
		    bufp->warned_about_incompatible_back_references == 0)
		  {
		    bufp->warned_about_incompatible_back_references = 1;
		    warn_when_safe (intern ("regex"), Qinfo,
				    "Back reference \\%d now has new "
				    "semantics in %s", reg, pattern);
		  }
#endif

		laststart = buf_end;
		GET_BUFFER_SPACE (3);
		store_op1 (duplicate, buf_end, regint);
		buf_end += 3;
	      }
              break;


            case '+':
            case '?':
            default:
            normal_backslash:
              /* You might think it would be useful for \ to mean
                 not to translate; but if we don't translate it,
                 it will never match anything.  */
              c = RE_TRANSLATE (c);
              goto normal_char;
            }
          break;


	default:
        /* Expects the character in `c'.  */
	/* `p' points to the location after where `c' came from. */
	normal_char:
	  {
	    /* The following conditional synced to GNU Emacs 22.1.  */
	    /* If no exactn currently being built.  */
	    if (!pending_exact

		/* If last exactn not at current position.  */
		|| pending_exact + *pending_exact + 1 != buf_end

		/* We have only one byte following the exactn for the count. */
		|| *pending_exact >= (1 << BYTEWIDTH) - MAX_ICHAR_LEN

		/* If followed by a repetition operator.
		   If the lookahead fails because of end of pattern, any
		   trailing backslash will get caught later.  */
		|| (p != pend && (*p == '*' || *p == '^' || *p == '+' ||
				  *p == '?'))
		|| ((syntax & RE_INTERVALS)
		    && (p + 1 < pend && (p[0] == '\\' && p[1] == '{'))))
	      {
		/* Start building a new exactn.  */

		laststart = buf_end;

		BUF_PUSH_2 (exactn, 0);
		pending_exact = buf_end - 1;
	      }

#ifndef emacs
	    BUF_PUSH (c);
	    (*pending_exact)++;
#else
	    {
	      Bytecount bt_count;
	      Ibyte tmp_buf[MAX_ICHAR_LEN];
	      int i;

	      bt_count = set_itext_ichar (tmp_buf, c);

	      for (i = 0; i < bt_count; i++)
		{
		  BUF_PUSH (tmp_buf[i]);
		  (*pending_exact)++;
		}
	    }
#endif
	    break;
	  }
        } /* switch (c) */
    } /* while p != pend */


  /* Through the pattern now.  */

  FIXUP_ALT_JUMP ();

  if (!COMPILE_STACK_EMPTY)
    FREE_STACK_RETURN (REG_EPAREN);

  /* If we don't want backtracking, force success
     the first time we reach the end of the compiled pattern.  */
  if (syntax & RE_NO_POSIX_BACKTRACKING)
    BUF_PUSH (succeed);

  xfree (compile_stack.stack);

  /* We have succeeded; set the length of the buffer.  */
  bufp->used = buf_end - bufp->buffer;

  fixup_on_failure_jump_smart (bufp);

#ifdef DEBUG
  if (DEBUG_RUNTIME_FLAGS & RE_DEBUG_COMPILATION)
    {
      DEBUG_PRINT ("\nCompiled pattern: \n");
      print_compiled_pattern (bufp);
    }
#endif /* DEBUG */

#ifndef MATCH_MAY_ALLOCATE
  /* Initialize the failure stack to the largest possible stack.  This
     isn't necessary unless we're trying to avoid calling alloca in
     the search and match routines.  */
  {
    int num_regs = bufp->re_ngroups + 1;

    /* Since DOUBLE_FAIL_STACK refuses to double only if the current size
       is strictly greater than re_max_failures, the largest possible stack
       is 2 * re_max_failures failure points.  */
    if (fail_stack.size < (2 * re_max_failures * TYPICAL_FAILURE_SIZE))
      {
	fail_stack.size = (2 * re_max_failures * TYPICAL_FAILURE_SIZE);

	if (! fail_stack.stack)
	  fail_stack.stack
	    = (fail_stack_elt_t *) xmalloc (fail_stack.size
					    * sizeof (fail_stack_elt_t));
	else
	  fail_stack.stack
	    = (fail_stack_elt_t *) xrealloc (fail_stack.stack,
					     (fail_stack.size
					      * sizeof (fail_stack_elt_t)));
      }

    regex_grow_registers (num_regs);
  }
#endif /* not MATCH_MAY_ALLOCATE */

  return REG_NOERROR;
} /* regex_compile */

/* Subroutines for `regex_compile'.  */

/* Store OP at LOC followed by two-byte integer parameter ARG.  */

static void
store_op1 (re_opcode_t op, unsigned char *loc, int arg)
{
  *loc = (unsigned char) op;
  STORE_NUMBER (loc + 1, arg);
}


/* Like `store_op1', but for two two-byte parameters ARG1 and ARG2.  */

static void
store_op2 (re_opcode_t op, unsigned char *loc, int arg1, int arg2)
{
  *loc = (unsigned char) op;
  STORE_NUMBER (loc + 1, arg1);
  STORE_NUMBER (loc + 3, arg2);
}


/* Copy the bytes from LOC to END to open up three bytes of space at LOC
   for OP followed by two-byte integer parameter ARG.  */

static void
insert_op1 (re_opcode_t op, unsigned char *loc, int arg, unsigned char *end)
{
  REGISTER unsigned char *pfrom = end;
  REGISTER unsigned char *pto = end + 3;

  while (pfrom != loc)
    *--pto = *--pfrom;

  store_op1 (op, loc, arg);
}


/* Like `insert_op1', but for two two-byte parameters ARG1 and ARG2.  */

static void
insert_op2 (re_opcode_t op, unsigned char *loc, int arg1, int arg2,
	    unsigned char *end)
{
  REGISTER unsigned char *pfrom = end;
  REGISTER unsigned char *pto = end + 5;

  while (pfrom != loc)
    *--pto = *--pfrom;

  store_op2 (op, loc, arg1, arg2);
}


/* P points to just after a ^ in PATTERN.  Return true if that ^ comes
   after an alternative or a begin-subexpression.  We assume there is at
   least one character before the ^.  */

static re_bool
at_begline_loc_p (re_char *pattern, re_char *p, reg_syntax_t syntax)
{
  re_char *prev = p - 2;
  re_bool prev_prev_backslash = prev > pattern && prev[-1] == '\\';

  return
       /* After a subexpression?  */
    (*prev == '(' && prev_prev_backslash)
       /* After an alternative?  */
    || (*prev == '|' && (prev_prev_backslash));
}


/* The dual of at_begline_loc_p.  This one is for $.  We assume there is
   at least one character after the $, i.e., `P < PEND'.  */

static re_bool
at_endline_loc_p (re_char *p, re_char *pend, int syntax)
{
  re_char *next = p;
  re_bool next_backslash = *next == '\\';
  re_char *next_next = p + 1 < pend ? p + 1 : 0;

  return
       /* Before a subexpression?  */
       (next_backslash && next_next && *next_next == ')')
       /* Before an alternative?  */
    || (next_backslash && next_next && *next_next == '|');
}


/* Returns true if REGNUM is in one of COMPILE_STACK's elements and
   false if it's not.  */

static re_bool
group_in_compile_stack (compile_stack_type compile_stack, regnum_t regnum)
{
  int this_element;

  for (this_element = compile_stack.avail - 1;
       this_element >= 0;
       this_element--)
    if (compile_stack.stack[this_element].regnum == regnum)
      return true;

  return false;
}

/* Read the ending character of a range (in a bracket expression) from the
   uncompiled pattern *P_PTR (which ends at PEND).  We assume the
   starting character is in `P[-2]'.  (`P[-1]' is the character `-'.)
   Then we set the translation of all bits between the starting and
   ending characters (inclusive) in the compiled pattern B.

   Return an error code.

   We use these short variable names so we can use the same macros as
   `regex_compile' itself.

   Under Mule, this is only called when both chars of the range are
   ASCII. */

static reg_errcode_t
compile_range (re_char **p_ptr, re_char *pend, RE_TRANSLATE_TYPE translate,
	       reg_syntax_t syntax, unsigned char *buf_end)
{
  Ichar this_char;

  re_char *p = *p_ptr;
  int range_start, range_end;

  if (p == pend)
    return REG_ERANGE;

  /* Even though the pattern is a signed `char *', we need to fetch
     with unsigned char *'s; if the high bit of the pattern character
     is set, the range endpoints will be negative if we fetch using a
     signed char *.

     We also want to fetch the endpoints without translating them; the
     appropriate translation is done in the bit-setting loop below.  */
  /* The SVR4 compiler on the 3B2 had trouble with unsigned const char *.  */
  range_start = ((const unsigned char *) p)[-2];
  range_end   = ((const unsigned char *) p)[0];

  /* Have to increment the pointer into the pattern string, so the
     caller isn't still at the ending character.  */
  (*p_ptr)++;

  /* If the start is after the end, the range is empty.  */
  if (range_start > range_end)
    return REG_NOERROR;

  /* Here we see why `this_char' has to be larger than an `unsigned
     char' -- the range is inclusive, so if `range_end' == 0xff
     (assuming 8-bit characters), we would otherwise go into an infinite
     loop, since all characters <= 0xff.  */
  for (this_char = range_start; this_char <= range_end; this_char++)
    {
      SET_LIST_BIT (RE_TRANSLATE (this_char));
    }

  return REG_NOERROR;
}

#ifdef emacs

static reg_errcode_t
compile_extended_range (re_char **p_ptr, re_char *pend,
			RE_TRANSLATE_TYPE translate,
			reg_syntax_t syntax, Lisp_Object rtab)
{
  Ichar this_char, range_start, range_end;
  const Ibyte *p;

  if (*p_ptr == pend)
    return REG_ERANGE;

  p = (const Ibyte *) *p_ptr;
  range_end = itext_ichar (p);
  p--; /* back to '-' */
  DEC_IBYTEPTR (p); /* back to start of range */
  /* We also want to fetch the endpoints without translating them; the
     appropriate translation is done in the bit-setting loop below.  */
  range_start = itext_ichar (p);
  INC_IBYTEPTR (*p_ptr);

  /* If the start is after the end, the range is empty.  */
  if (range_start > range_end)
    return REG_NOERROR;

#ifndef UNICODE_INTERNAL
  /* Can't have ranges spanning different charsets, except maybe for
     ranges entirely within the first 256 chars. (The intent of this is that
     the effect of such a range would be unpredictable, since there is no
     well-defined ordering over charsets and the particular assignment of
     charset ID's is arbitrary.) This does not apply to Unicode, with
     well-defined character values. */

  if ((range_start >= 0x100 || range_end >= 0x100)
      && !EQ (old_mule_ichar_charset (range_start),
	      old_mule_ichar_charset (range_end)))
    return REG_ERANGESPAN;
#endif /* not UNICODE_INTERNAL */

  if (TRANSLATE_P (translate))
    {
      /* #### This might be way inefficient if the range encompasses 10,000
	 chars or something.  To be efficient, you'd have to do something like
	 this:

	 range_table a
	 range_table b;
	 map_char_table (translation table, [range_start, range_end]) of
           lambda (ch, translation):
             put (ch, Qt) in a
             put (translation, Qt) in b
	 invert the range in a and truncate to [range_start, range_end]
	 put the union of a, b in rtab

	 This is to say, we want to map every character that has a translation
	 to its translation, and other characters to themselves.

	 This assumes, as is reasonable in practice, that a translation
	 table maps individual characters to their translation, and does
	 not generally map multiple characters to the same translation.
   */
      for (this_char = range_start; this_char <= range_end; this_char++)
	{
	  SET_RANGETAB_BIT (RE_TRANSLATE (this_char));
	}
    }
  else
    put_range_table (rtab, range_start, range_end, Qt);

  return REG_NOERROR;
}

reg_errcode_t
compile_char_class (re_wctype_t cc, Lisp_Object rtab, Bitbyte *flags_out)
{
  *flags_out |= re_wctype_to_bit (cc);

  switch (cc)
    {
    case RECC_ASCII:
      put_range_table (rtab, 0, 0x7f, Qt);
      break;

    case RECC_XDIGIT:
      put_range_table (rtab, 'a', 'f', Qt);
      put_range_table (rtab, 'A', 'f', Qt);
      /* fallthrough */
    case RECC_DIGIT:
      put_range_table (rtab, '0', '9', Qt);
      break;

    case RECC_BLANK:
      put_range_table (rtab, ' ', ' ', Qt);
      put_range_table (rtab, '\t', '\t', Qt);
      break;

    case RECC_PRINT:
      put_range_table (rtab, ' ', 0x7e, Qt);
      put_range_table (rtab, 0x80, CHAR_CODE_LIMIT, Qt);
      break;

    case RECC_GRAPH:
      put_range_table (rtab, '!', 0x7e, Qt);
      put_range_table (rtab, 0x80, CHAR_CODE_LIMIT, Qt);
      break;

    case RECC_NONASCII:
    case RECC_MULTIBYTE:
      put_range_table (rtab, 0x80, CHAR_CODE_LIMIT, Qt);
      break;

    case RECC_CNTRL:
      put_range_table (rtab, 0x00, 0x1f, Qt);
      break;

    case RECC_UNIBYTE:
      /* Never true in XEmacs. */
      break;

      /* The following all have their own bits in the class_bits argument to
         charset_mule and charset_mule_not, they don't use the range table
         information. */
    case RECC_ALPHA:
    case RECC_WORD:
    case RECC_ALNUM: /* Equivalent to RECC_WORD */
    case RECC_LOWER:
    case RECC_PUNCT:
    case RECC_SPACE:
    case RECC_UPPER:
      break;
    }

  return REG_NOERROR;
}

#endif /* emacs */

/* re_compile_fastmap computes a ``fastmap'' for the compiled pattern in
   BUFP.  A fastmap records which of the (1 << BYTEWIDTH) possible
   characters can start a string that matches the pattern.  This fastmap
   is used by re_search to skip quickly over impossible starting points.

   The caller must supply the address of a (1 << BYTEWIDTH)-byte data
   area as BUFP->fastmap.

   We set the `fastmap', `fastmap_accurate', and `can_be_null' fields in
   the pattern buffer.

   Returns 0 if we succeed, -2 if an internal error.   */

int
re_compile_fastmap (struct re_pattern_buffer *bufp
		    RE_LISP_SHORT_CONTEXT_ARGS_DECL)
{
  int j, k;
#ifdef MATCH_MAY_ALLOCATE
  fail_stack_type fail_stack;
#endif
  /* &&#### this should be changed for 8-bit-fixed, for efficiency.  see
     comment marked with &&#### in re_search_2. */
    
  REGISTER char *fastmap = bufp->fastmap;
  unsigned char *pattern = bufp->buffer;
  Bytecount size = bufp->used;
  re_char *p = pattern;
  REGISTER re_char *pend = pattern + size;

#ifdef REGEX_REL_ALLOC
  /* This holds the pointer to the failure stack, when
     it is allocated relocatably.  */
  fail_stack_elt_t *failure_stack_ptr;
#endif

  /* Assume that each path through the pattern can be null until
     proven otherwise.  We set this false at the bottom of switch
     statement, to which we get only if a particular path doesn't
     match the empty string.  */
  re_bool path_can_be_null = true;

  /* We aren't doing a `succeed_n' to begin with.  */
  re_bool succeed_n_p = false;

#ifdef ERROR_CHECK_MALLOC
  /* The pattern comes from string data, not buffer data.  We don't access
     any buffer data, so we don't have to worry about malloc() (but the
     disallowed flag may have been set by a caller). */
  int depth = bind_regex_malloc_disallowed (0);
#endif

  assert (fastmap != NULL && p != NULL);

  INIT_FAIL_STACK ();
  memset (fastmap, 0, 1 << BYTEWIDTH);  /* Assume nothing's valid.  */
  bufp->fastmap_accurate = 1;	    /* It will be when we're done.  */
  bufp->can_be_null = 0;

  /* The loop below works as follows:
     - It has a working-list kept in the PATTERN_STACK and which basically
       starts by only containing a pointer to the first operation.
     - If the opcode we're looking at is a match against some set of
       chars, then we add those chars to the fastmap and go on to the
       next work element from the worklist (done via `break').
     - If the opcode is a control operator on the other hand, we either
       ignore it (if it's meaningless at this point, such as `start_memory')
       or execute it (if it's a jump).  If the jump has several destinations
       (i.e. `on_failure_jump'), then we push the other destination onto the
       worklist.
     We guarantee termination by ignoring backward jumps (more or less),
     so that `p' is monotonically increasing.  More to the point, we
     never set `p' (or push) anything `<= p1'.  */

  /* If can_be_null is set, then the fastmap will not be used anyway.  */
  while (!bufp->can_be_null)
    {
      /* `p1' is used as a marker of how far back a `on_failure_jump'
	 can go without being ignored.  It is normally equal to `p'
	 (which prevents any backward `on_failure_jump') except right
	 after a plain `jump', to allow patterns such as:
	    0: jump 10
	    3..9: <body>
	    10: on_failure_jump 3
	 as used for the *? operator.  */
      re_char *p1 = p;

      if (p == pend || *p == succeed)
	{
	  /* We have reached the (effective) end of pattern.  */
	  if (!PATTERN_STACK_EMPTY ())
	    {
	      bufp->can_be_null |= path_can_be_null;

	      /* Reset for next path.  */
	      path_can_be_null = true;

	      p = POP_PATTERN_OP ();

	      continue;
	    }
	  else
	    break;
	}

      /* We should never be about to go beyond the end of the pattern.  */
      assert (p < pend);

      switch ((re_opcode_t) *p++)
	{

	case duplicate:
	  /* If the first character has to match a backreference, that means
	     that the group was empty (since it already matched).  Since this
	     is the only case that interests us here, we can assume that the
	     backreference must match the empty string.  */
	  p++;
	  continue;

      /* Following are the cases which match a character.  These end
         with `break'.  */

	case exactn:
          fastmap[p[1]] = 1;
	  break;

        case charset:
	  /* XEmacs: Under Mule, these bit vectors will
	     only contain values for characters below 0x80. */
          for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH)))
              fastmap[j] = 1;
	  break;

	case charset_not:
	  /* Chars beyond end of map must be allowed.  */
#ifdef emacs
	  for (j = *p * BYTEWIDTH; j < 0x80; j++)
            fastmap[j] = 1;
	  /* And all extended characters must be allowed, too. */
	  for (j = 0x80; j < 0x100; j++)
	    if (ibyte_first_byte_p (j))
	      fastmap[j] = 1;
#else /* not emacs */
	  for (j = *p * BYTEWIDTH; j < (1 << BYTEWIDTH); j++)
            fastmap[j] = 1;
#endif /* emacs */

	  for (j = *p++ * BYTEWIDTH - 1; j >= 0; j--)
	    if (!(p[j / BYTEWIDTH] & (1 << (j % BYTEWIDTH))))
              fastmap[j] = 1;
          break;

#ifdef emacs
	case charset_mule:
	  {
	    int nentries;
	    Bitbyte flags = *p++;

	    if (flags)
	      {
                /* We need to consult the syntax table, fastmap won't
                   work. */
                bufp->can_be_null = 1;
		continue;
	      }

	    nentries = unified_range_table_nentries ((void *) p);
	    for (j = 0; j < nentries; j++)
	      {
		EMACS_INT first, last;
		Lisp_Object dummy_val;
		int jj;
		Ibyte strr[MAX_ICHAR_LEN];

		unified_range_table_get_range ((void *) p, j, &first, &last,
					       &dummy_val);
#ifndef UNICODE_INTERNAL
		for (jj = first; jj <= last && jj < 0x80; jj++)
		  fastmap[jj] = 1;
		/* Ranges below 0x100 can span charsets, but there
		   are only two (Control-1 and Latin-1), and
		   either first or last has to be in them. */
		set_itext_ichar (strr, first);
		fastmap[*strr] = 1;
		if (last < 0x100)
		  {
		    set_itext_ichar (strr, last);
		    fastmap[*strr] = 1;
		  }
                else if (CHAR_CODE_LIMIT == last)
                  {
		    /* This is RECC_MULTIBYTE or RECC_NONASCII; true for all
                       non-ASCII characters. */
		    jj = 0x80;
		    while (jj < 0xA0)
		      {
			fastmap[jj++] = 1;
		      }
                  }
#else
		/* Ranges can span charsets.  We depend on the fact that
		   lead bytes are monotonically non-decreasing as
		   character values increase.  @@#### This is a fairly
		   reasonable assumption in general (but DOES NOT WORK in
		   old Mule due to the ordering of private dimension-1
		   chars before official dimension-2 chars), and introduces
		   a dependency on the particular representation. */
		{
		  Ibyte strrlast[MAX_ICHAR_LEN];
		  set_itext_ichar (strr, first);
		  set_itext_ichar (strrlast, min (last, CHAR_CODE_LIMIT - 1));
		  for (jj = *strr; jj <= *strrlast; jj++)
		    fastmap[jj] = 1;
		}
#endif /* not UNICODE_INTERNAL */
	      }
	    /* If it's not a possible first byte, it can't be in the fastmap.
	       In UTF-8, lead bytes are not contiguous with ASCII, so a
	       range spanning the ASCII/non-ASCII boundary will put
	       extraneous bytes in the range [0x80 - 0xBF] in the fastmap. */
	    for (j = 0x80; j < 0x100; j++)
	      if (!ibyte_first_byte_p (j))
		fastmap[j] = 0;
	  }
	  break;

	case charset_mule_not:
	  {
	    int nentries;
	    int smallest_prev = 0;
	    Bitbyte flags = *p++;

	    if (flags)
              {
                /* We need to consult the syntax table, fastmap won't
                   work. */
                bufp->can_be_null = 1;
                continue;
              }

	    nentries = unified_range_table_nentries ((void *) p);
#ifndef UNICODE_INTERNAL
	    for (j = 0; j < nentries; j++)
	      {
		EMACS_INT first, last;
		Lisp_Object dummy_val;
		int jj;

		unified_range_table_get_range ((void *) p, j, &first, &last,
					       &dummy_val);
		for (jj = smallest_prev; jj < first && jj < 0x80; jj++)
		  fastmap[jj] = 1;
		smallest_prev = last + 1;
		if (smallest_prev >= 0x80)
		  break;
	      }

	    /* Also set lead bytes after the end */
	    for (j = smallest_prev; j < 0x80; j++)
	      fastmap[j] = 1;

	    /* Calculating which lead bytes are actually allowed
	       here is rather difficult, so we just punt and allow
	       all of them.
	    */
	    for (j = 0x80; j < 0x100; j++)
	      if (ibyte_first_byte_p (j))
		fastmap[j] = 1;
#else
	    for (j = 0; j < nentries; j++)
	      {
		EMACS_INT first, last;
		/* This denotes a range of lead bytes that are not
		   in the fastmap. */
		int firstlead, lastlead;
		Lisp_Object dummy_val;
		int jj;

		unified_range_table_get_range ((void *) p, j, &first, &last,
					       &dummy_val);
		/* With Unicode-internal, lead bytes that are entirely
		   within the range and not including the beginning or end
		   are definitely not in the fastmap.  Leading bytes that
		   include the beginning or ending characters will be in
		   the fastmap unless the beginning or ending characters
		   are the first or last character, respectively, that uses
		   this lead byte.

		   @@#### WARNING! In order to determine whether we are the
		   first or last character using a lead byte we use and
		   embed in the code some knowledge of how UTF-8 works --
		   at least, the fact that the the first character using a
		   particular lead byte has the minimum-numbered trailing
		   byte in all its trailing bytes, and the last character
		   using a particular lead byte has the maximum-numbered
		   trailing byte in all its trailing bytes.  We abstract
		   away the actual minimum/maximum trailing byte numbers,
		   at least.  We could perhaps do this more portably by
		   just looking at the representation of the character one
		   higher or lower and seeing if the lead byte changes, but
		   you'd run into the problem of invalid characters, e.g.
		   if you're at the edge of the range of surrogates or are
		   the top-most allowed character.
		   */
		if (first < 0x80)
		  firstlead = first;
		else
		  {
		    Ibyte strr[MAX_ICHAR_LEN];
		    Bytecount slen = set_itext_ichar (strr, first);
		    int kk;
		    /* Determine if we're the first character using our
		       leading byte. */
		    for (kk = 1; kk < slen; kk++)
		      if (strr[kk] != FIRST_TRAILING_BYTE)
			{
			  /* If not, this leading byte might occur, so
			     make sure it gets added to the fastmap. */
			  firstlead = *strr + 1;
			  break;
			}
		    /* Otherwise, we're the first character using our
		       leading byte, and we don't need to add the leading
		       byte to the fastmap. (If our range doesn't
		       completely cover the leading byte, it will get added
		       anyway by the code handling the other end of the
		       range.) */
		    firstlead = *strr;
		  }
		if (last < 0x80)
		  lastlead = last;
		else
		  {
		    Ibyte strr[MAX_ICHAR_LEN];
		    Bytecount slen
			    = set_itext_ichar (strr,
					       min (last,
						    CHAR_CODE_LIMIT - 1));
		    int kk;
		    /* Same as above but for the last character using
		       our leading byte. */
		    for (kk = 1; kk < slen; kk++)
		      if (strr[kk] != LAST_TRAILING_BYTE)
			{
			  lastlead = *strr - 1;
			  break;
			}
		    lastlead = *strr;
		  }
		/* Now, FIRSTLEAD and LASTLEAD are set to the beginning and
		   end, inclusive, of a range of lead bytes that cannot be
		   in the fastmap.  Essentially, we want to set all the other
		   bytes to be in the fastmap.  Here we handle those after
		   the previous range and before this one. */
		for (jj = smallest_prev; jj < firstlead; jj++)
		  fastmap[jj] = 1;
		smallest_prev = lastlead + 1;
	      }

	    /* Also set lead bytes after the end of the final range. */
	    for (j = smallest_prev; j < 0x100; j++)
	      fastmap[j] = 1;

	    /* If it's not a possible first byte, it can't be in the fastmap.
	       In UTF-8, lead bytes are not contiguous with ASCII, so a
	       range spanning the ASCII/non-ASCII boundary will put
	       extraneous bytes in the range [0x80 - 0xBF] in the fastmap. */
	    for (j = 0x80; j < 0x100; j++)
	      if (!ibyte_first_byte_p (j))
		fastmap[j] = 0;
#endif /* UNICODE_INTERNAL */
	  }
	  break;
#endif /* emacs */


        case anychar:
	  {
	    int fastmap_newline = fastmap['\n'];

	    /* `.' matches anything ...  */
#ifdef emacs
	    /* "anything" only includes bytes that can be the
	       first byte of a character. */
	    for (j = 0; j < 0x100; j++)
	      if (ibyte_first_byte_p (j))
		fastmap[j] = 1;
#else
	    for (j = 0; j < (1 << BYTEWIDTH); j++)
	      fastmap[j] = 1;
#endif

	    /* ... except perhaps newline.  */
	    if (!(bufp->syntax & RE_DOT_NEWLINE))
	      fastmap['\n'] = fastmap_newline;

	    /* Otherwise, have to check alternative paths.  */
	    break;
	  }

#ifndef emacs
	case wordchar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (ignored, j) == Sword)
	      fastmap[j] = 1;
	  break;

	case notwordchar:
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX (ignored, j) != Sword)
	      fastmap[j] = 1;
	  break;
#else /* emacs */
	case wordchar:
	case notwordchar:
	case wordbound:
	case notwordbound:
	case wordbeg:
	case wordend:
	case notsyntaxspec:
	case syntaxspec:
	  /* This match depends on text properties.  These end with
	     aborting optimizations.  */
	  bufp->can_be_null = 1;
	  continue;
#if 0 /* all of the following code is unused now that the `syntax-table'
	 property exists -- it's trickier to do this than just look in
	 the buffer.  &&#### but we could just use the syntax-cache stuff
	 instead; why don't we? --ben */
	case wordchar:
	  k = (int) Sword;
	  goto matchsyntax;

	case notwordchar:
	  k = (int) Sword;
	  goto matchnotsyntax;
	  
        case syntaxspec:
	  k = *p++;
	matchsyntax:
#ifdef emacs
	  for (j = 0; j < 0x80; j++)
	    if (SYNTAX
		(XCHAR_TABLE (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf)), j) ==
		(enum syntaxcode) k)
	      fastmap[j] = 1;
	  /* @@#### To be correct, we need to set the fastmap for any
	     lead byte any of whose characters can have this syntax code.
	     This is hard to calculate so we just punt for now. */
	  for (j = 0x80; j < 0x100; j++)
	    if (ibyte_first_byte_p (j))
	      fastmap[j] = 1;
#else /* not emacs */
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX
		(XCHAR_TABLE (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf)), j) ==
		(enum syntaxcode) k)
	      fastmap[j] = 1;
#endif /* emacs */
	  break;


	case notsyntaxspec:
	  k = *p++;
	matchnotsyntax:
#ifdef emacs
	  for (j = 0; j < 0x80; j++)
	    if (SYNTAX
		(XCHAR_TABLE
		 (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf)), j) !=
		(enum syntaxcode) k)
	      fastmap[j] = 1;
	  /* @@#### To be correct, we need to set the fastmap for any
	     lead byte all of whose characters do not have this syntax code.
	     This is hard to calculate so we just punt for now. */
	  for (j = 0x80; j < 0x100; j++)
	    if (ibyte_first_byte_p (j))
	      fastmap[j] = 1;
#else /* not emacs */
	  for (j = 0; j < (1 << BYTEWIDTH); j++)
	    if (SYNTAX
		(XCHAR_TABLE
		 (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf)), j) !=
		(enum syntaxcode) k)
	      fastmap[j] = 1;
#endif /* emacs */
	  break;
#endif /* 0 */

/* 97/2/17 jhod category patch */
	case categoryspec:
	case notcategoryspec:
	  bufp->can_be_null = 1;
	  UNBIND_REGEX_MALLOC_CHECK ();
	  return 0;
/* end of category patch */

      /* All cases after this match the empty string.  These end with
         `continue'.  */
	case before_dot:
	case at_dot:
	case after_dot:
          continue;
#endif /* emacs */


        case no_op:
        case begline:
        case endline:
	case begbuf:
	case endbuf:
#ifndef emacs
	case wordbound:
	case notwordbound:
	case wordbeg:
	case wordend:
#endif
          continue;


	case jump_n:
	case jump:
        case jump_past_alt:
          EXTRACT_NUMBER_AND_INCR (j, p);
	  if (j < 0)
	    {
	      /* Backward jumps can only go back to code that we've already
		 visited.  `re_compile' should make sure this is true.  */
	      break;
	    }
	  p += j;
	  switch ((re_opcode_t) *p)
	    {
	    case on_failure_jump:
	    case on_failure_jump_exclusive:
	    case on_failure_jump_loop:
	    case on_failure_jump_smart:
	      p++;
	      break;
	    default:
	      continue;
	    };
	  /* Keep `p1' to allow the `on_failure_jump' we are jumping to
	     to jump back to "just after here".  */
	  /* Fallthrough */

	case on_failure_jump:
	case on_failure_jump_exclusive:
	case on_failure_jump_loop:
	case on_failure_jump_smart:
	handle_on_failure_jump:
          EXTRACT_NUMBER_AND_INCR (j, p);

          /* For some patterns, e.g., `(a?)?', `p+j' here points to the
             end of the pattern.  We don't want to push such a point,
             since when we restore it above, entering the switch will
             increment `p' past the end of the pattern.  We don't need
             to push such a point since we obviously won't find any more
             fastmap entries beyond `pend'.  Such a pattern can match
             the null string, though.  */
	  if (p + j <= p1)
	    {
	      /* Backward jump to be ignored.  */
	      (void) (0);
	    }
	  else if (p + j < pend)
            {
              if (!PUSH_PATTERN_OP (p + j, fail_stack))
		{
		  RESET_FAIL_STACK ();
		  UNBIND_REGEX_MALLOC_CHECK ();
		  return -2;
		}
            }
          else
	    {
	      bufp->can_be_null = 1;
	    }

          if (succeed_n_p)
            {
              EXTRACT_NUMBER_AND_INCR (k, p);	/* Skip the n.  */
              succeed_n_p = false;
	    }

          continue;

	case succeed_n:
          /* Get to the number of times to succeed.  */
          p += 2;

          /* Increment p past the n for when k != 0.  */
          EXTRACT_NONNEGATIVE_AND_INCR (k, p);
          if (k == 0)
	    {
              p -= 4;
  	      succeed_n_p = true;  /* Spaghetti code alert.  */
              goto handle_on_failure_jump;
            }
          continue;


	case set_number_at:
          p += 4;
          continue;


	case start_memory:
        case stop_memory:
	  p += 2;
	  continue;


	default:
          ABORT (); /* We have listed all the cases.  */
        } /* switch *p++ */

      /* Getting here means we have found the possible starting
         characters for one path of the pattern -- and that the empty
         string does not match.  We need not follow this path further.
         Instead, look at the next alternative (remembered on the
         stack), or quit if no more.  The test at the top of the loop
         does these things.  */
      path_can_be_null = false;
      p = pend;
    } /* while p */

  /* Set `can_be_null' for the last path (also the first path, if the
     pattern is empty).  */
  bufp->can_be_null |= path_can_be_null;

  RESET_FAIL_STACK ();
  UNBIND_REGEX_MALLOC_CHECK ();
  return 0;
} /* re_compile_fastmap */

/* Set REGS to hold NUM_REGS registers, storing them in STARTS and
   ENDS.  Subsequent matches using PATTERN_BUFFER and REGS will use
   this memory for recording register information.  STARTS and ENDS
   must be allocated using the malloc library routine, and must each
   be at least NUM_REGS * sizeof (regoff_t) bytes long.

   If NUM_REGS == 0, then subsequent matches should allocate their own
   register data.

   Unless this function is called, the first search or match using
   PATTERN_BUFFER will allocate its own register data, without
   freeing the old data.  */

void
re_set_registers (struct re_pattern_buffer *bufp, struct re_registers *regs,
		  int num_regs, regoff_t *starts, regoff_t *ends)
{
  if (num_regs)
    {
      bufp->regs_allocated = REGS_REALLOCATE;
      regs->num_regs = num_regs;
      regs->start = starts;
      regs->end = ends;
    }
  else
    {
      bufp->regs_allocated = REGS_UNALLOCATED;
      regs->num_regs = 0;
      regs->start = regs->end = (regoff_t *) 0;
    }
}

/* Searching routines.  */

/* Like re_search_2, below, but only one string is specified, and
   doesn't let you say where to stop matching. */

int
re_search (struct re_pattern_buffer *bufp, const char *string, int size,
	   int startpos, int range, struct re_registers *regs
	   RE_LISP_CONTEXT_ARGS_DECL)
{
  return re_search_2 (bufp, NULL, 0, string, size, startpos, range,
		      regs, size RE_LISP_CONTEXT_ARGS);
}

/* Using the compiled pattern in BUFP->buffer, first tries to match the
   virtual concatenation of STRING1 and STRING2, starting first at index
   STARTPOS, then at STARTPOS + 1, and so on.

   STRING1 and STRING2 have length SIZE1 and SIZE2, respectively.

   RANGE is how far to scan while trying to match.  RANGE = 0 means try
   only at STARTPOS; in general, the last start tried is STARTPOS +
   RANGE.

   All sizes and positions refer to bytes (not chars); under Mule, the code
   knows about the format of the text and will only check at positions
   where a character starts.

   With MULE, RANGE is a byte position, not a char position.  The last
   start tried is the character starting <= STARTPOS + RANGE.

   In REGS, return the indices of the virtual concatenation of STRING1
   and STRING2 that matched the entire BUFP->buffer and its contained
   subexpressions.

   Do not consider matching one past the index STOP in the virtual
   concatenation of STRING1 and STRING2.

   We return either the position in the strings at which the match was
   found, -1 if no match, or -2 if error (such as failure
   stack overflow).  */

int
re_search_2 (struct re_pattern_buffer *bufp, const char *str1,
	     int size1, const char *str2, int size2, int startpos,
	     int range, struct re_registers *regs, int stop
	     RE_LISP_CONTEXT_ARGS_DECL)
{
  int val;
  re_char *string1 = (re_char *) str1;
  re_char *string2 = (re_char *) str2;
  REGISTER char *fastmap = bufp->fastmap;
  REGISTER RE_TRANSLATE_TYPE translate = bufp->translate;
  int total_size = size1 + size2;
  int endpos = startpos + range;
#ifdef REGEX_BEGLINE_CHECK
  int anchored_at_begline = 0;
#endif
  re_char *d = NULL;
#ifdef emacs
  Internal_Format fmt = buffer_or_other_internal_format (lispobj);
#ifdef REL_ALLOC
  const Ibyte *orig_buftext =
    BUFFERP (lispobj) ?
    BYTE_BUF_BYTE_ADDRESS (XBUFFER (lispobj),
			   BYTE_BUF_BEG (XBUFFER (lispobj))) :
    0;
#endif
#ifdef ERROR_CHECK_MALLOC
  int depth;
#endif
#endif /* emacs */
  int forward_search_p;

  /* Check for out-of-range STARTPOS.  */
  if (startpos < 0 || startpos > total_size)
    return -1;

  /* Fix up RANGE if it might eventually take us outside
     the virtual concatenation of STRING1 and STRING2.  */
  if (endpos < 0)
    range = 0 - startpos;
  else if (endpos > total_size)
    range = total_size - startpos;

  forward_search_p = range > 0;

  (void) (forward_search_p); /* This is only used with assertions, silence the
                                compiler warning when they're turned off. */

  /* If the search isn't to be a backwards one, don't waste time in a
     search for a pattern that must be anchored.  */
  if (bufp->used > 0 && (re_opcode_t) bufp->buffer[0] == begbuf && range > 0)
    {
      if (startpos > 0)
	return -1;
      else
	{
	  d = ((const unsigned char *)
	       (startpos >= size1 ? string2 - size1 : string1) + startpos);
	  range = itext_ichar_len_fmt (d, fmt);
	}
    }

#ifdef emacs
  /* In a forward search for something that starts with \=.
     don't keep searching past point.  */
  if (bufp->used > 0 && (re_opcode_t) bufp->buffer[0] == at_dot && range > 0)
    {
      if (!BUFFERP (lispobj))
	return -1;
      range = (BYTE_BUF_PT (XBUFFER (lispobj))
	       - BYTE_BUF_BEGV (XBUFFER (lispobj)) - startpos);
      if (range < 0)
	return -1;
    }
#endif /* emacs */

#ifdef ERROR_CHECK_MALLOC
  /* Do this after the above return()s. */
  depth = bind_regex_malloc_disallowed (1);
#endif

  /* Update the fastmap now if not correct already.  */
  BEGIN_REGEX_MALLOC_OK ();
  if (fastmap && !bufp->fastmap_accurate)
    if (re_compile_fastmap (bufp RE_LISP_SHORT_CONTEXT_ARGS) == -2)
      {
	END_REGEX_MALLOC_OK ();
	UNBIND_REGEX_MALLOC_CHECK ();
	return -2;
      }

  END_REGEX_MALLOC_OK ();
  RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS ();

#ifdef REGEX_BEGLINE_CHECK
  {
    long i = 0;

    while (i < bufp->used)
      {
	if (bufp->buffer[i] == start_memory ||
	    bufp->buffer[i] == stop_memory)
	  i += 4;
	else
	  break;
      }
    anchored_at_begline = i < bufp->used && bufp->buffer[i] == begline;
  }
#endif

#ifdef emacs
  BEGIN_REGEX_MALLOC_OK ();
  /* Update the mirror syntax table if it's used and dirty. */
  SYNTAX_CODE (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf), 'a');
  scache = setup_syntax_cache (scache, lispobj, lispbuf,
                               offset_to_bytexpos (lispobj, startpos),
			       1);
  END_REGEX_MALLOC_OK ();
  RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
#endif

  /* Loop through the string, looking for a place to start matching.  */
  for (;;)
    {
#ifdef REGEX_BEGLINE_CHECK
      /* If the regex is anchored at the beginning of a line (i.e. with a
	 ^), then we can speed things up by skipping to the next
	 beginning-of-line.  However, to determine "beginning of line" we
	 need to look at the previous char, so can't do this check if at
	 beginning of either string. (Well, we could if at the beginning of
	 the second string, but it would require additional code, and this
	 is just an optimization.) */
      if (anchored_at_begline && startpos > 0 && startpos != size1)
	{
	  if (range > 0)
	    {
	      /* whose stupid idea was it anyway to make this
		 function take two strings to match?? */
	      int lim = 0;
	      re_char *orig_d;
	      re_char *stop_d;

	      /* Compute limit as below in fastmap code, so we are guaranteed
		 to remain within a single string. */
	      if (startpos < size1 && startpos + range >= size1)
		lim = range - (size1 - startpos);

	      d = ((const unsigned char *)
		   (startpos >= size1 ? string2 - size1 : string1) + startpos);
	      orig_d = d;
	      stop_d = d + range - lim;

	      /* We want to find the next location (including the current
		 one) where the previous char is a newline, so back up one
		 and search forward for a newline. */
	      DEC_IBYTEPTR_FMT (d, fmt);	/* Ok, since startpos != size1. */

	      /* Written out as an if-else to avoid testing `translate'
		 inside the loop.  */
	      if (TRANSLATE_P (translate))
		while (d < stop_d &&
		       RE_TRANSLATE_1 (itext_ichar_fmt (d, fmt, lispobj))
		       != '\n')
		  INC_IBYTEPTR_FMT (d, fmt);
	      else
		while (d < stop_d &&
		       itext_ichar_ascii_fmt (d, fmt, lispobj) != '\n')
		  INC_IBYTEPTR_FMT (d, fmt);

	      /* If we were stopped by a newline, skip forward over it.
		 Otherwise we will get in an infloop when our start position
		 was at begline. */
	      if (d < stop_d)
		INC_IBYTEPTR_FMT (d, fmt);
	      range -= d - orig_d;
	      startpos += d - orig_d;
	      assert (!forward_search_p || range >= 0);
	    }
	  else if (range < 0)
	    {
	      /* We're lazy, like in the fastmap code below */
	      Ichar c;

	      d = ((const unsigned char *)
		   (startpos >= size1 ? string2 - size1 : string1) + startpos);
	      DEC_IBYTEPTR_FMT (d, fmt);
	      c = itext_ichar_fmt (d, fmt, lispobj);
	      c = RE_TRANSLATE (c);
	      if (c != '\n')
		goto advance;
	    }
	}
#endif /* REGEX_BEGLINE_CHECK */

      /* If a fastmap is supplied, skip quickly over characters that
         cannot be the start of a match.  If the pattern can match the
         null string, however, we don't need to skip characters; we want
         the first null string.  */
      if (fastmap && startpos < total_size && !bufp->can_be_null)
	{
	  /* For the moment, fastmap always works as if buffer
	     is in default format, so convert chars in the search strings
	     into default format as we go along, if necessary.

	     &&#### fastmap needs rethinking for 8-bit-fixed so
	     it's faster.  We need it to reflect the raw
	     8-bit-fixed values.  That isn't so hard if we assume
	     that the top 96 bytes represent a single 1-byte
	     charset.  For 16-bit/32-bit stuff it's probably not
	     worth it to make the fastmap represent the raw, due to
	     its nature -- we'd have to use the LSB for the
	     fastmap, and that causes lots of problems with Mule
	     chars, where it essentially wipes out the usefulness
	     of the fastmap entirely. */
	  if (range > 0)	/* Searching forwards.  */
	    {
	      int lim = 0;
	      int irange = range;

              if (startpos < size1 && startpos + range >= size1)
                lim = range - (size1 - startpos);

	      d = ((const unsigned char *)
		   (startpos >= size1 ? string2 - size1 : string1) + startpos);

              /* Written out as an if-else to avoid testing `translate'
                 inside the loop.  */
	      if (TRANSLATE_P (translate))
		{
		  while (range > lim)
		    {
		      re_char *old_d = d;
#ifdef emacs
		      Ibyte tempch[MAX_ICHAR_LEN];
		      Ichar buf_ch =
			RE_TRANSLATE_1 (itext_ichar_fmt (d, fmt, lispobj));
		      set_itext_ichar (tempch, buf_ch);
		      if (fastmap[*tempch])
			break;
#else
		      if (fastmap[(unsigned char) RE_TRANSLATE_1 (*d)])
			break;
#endif /* emacs */
		      INC_IBYTEPTR_FMT (d, fmt);
		      range -= (d - old_d);
		      assert (!forward_search_p || range >= 0);
		    }
		}
#ifdef emacs
	      else if (fmt != FORMAT_DEFAULT)
		{
		  while (range > lim)
		    {
		      re_char *old_d = d;
		      Ibyte tempch[MAX_ICHAR_LEN];
		      Ichar buf_ch = itext_ichar_fmt (d, fmt, lispobj);
		      set_itext_ichar (tempch, buf_ch);
		      if (fastmap[*tempch])
			break;
		      INC_IBYTEPTR_FMT (d, fmt);
		      range -= (d - old_d);
		      assert (!forward_search_p || range >= 0);
		    }
		}
#endif /* emacs */
	      else
		{
		  while (range > lim && !fastmap[*d])
		    {
		      re_char *old_d = d;
		      INC_IBYTEPTR (d);
		      range -= (d - old_d);
                      assert (!forward_search_p || range >= 0);
		    }
		}

	      startpos += irange - range;
	    }
	  else				/* Searching backwards.  */
	    {
	      /* #### It's not clear why we don't just write a loop, like
		 for the moving-forward case.  Perhaps the writer got lazy,
		 since backward searches aren't so common. */
	      d = ((const unsigned char *)
		   (startpos >= size1 ? string2 - size1 : string1) + startpos);
#ifdef emacs
	      {
		Ibyte tempch[MAX_ICHAR_LEN];
		Ichar buf_ch =
		  RE_TRANSLATE (itext_ichar_fmt (d, fmt, lispobj));
		set_itext_ichar (tempch, buf_ch);
		if (!fastmap[*tempch])
		  goto advance;
	      }
#else
	      if (!fastmap[(unsigned char) RE_TRANSLATE (*d)])
		goto advance;
#endif /* emacs */
	    }
	}

      /* If can't match the null string, and that's all we have left, fail.  */
      if (range >= 0 && startpos == total_size && fastmap
          && !bufp->can_be_null)
	{
	  UNBIND_REGEX_MALLOC_CHECK ();
	  return -1;
	}

#ifdef emacs /* XEmacs added, w/removal of immediate_quit */
      if (!no_quit_in_re_search)
	{
	  BEGIN_REGEX_MALLOC_OK ();
	  QUIT;
	  END_REGEX_MALLOC_OK ();
	  RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	}

#endif
      BEGIN_REGEX_MALLOC_OK ();
      val = re_match_2_internal (bufp, string1, size1, string2, size2,
				 startpos, regs, stop
				 RE_LISP_CONTEXT_ARGS);
#ifndef REGEX_MALLOC
      ALLOCA_GARBAGE_COLLECT ();
#endif
      END_REGEX_MALLOC_OK ();
      RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS ();

      if (val >= 0)
	{
	  UNBIND_REGEX_MALLOC_CHECK ();
	  return startpos;
	}

      if (val == -2)
	{
	  UNBIND_REGEX_MALLOC_CHECK ();
	  return -2;
	}

      RE_SEARCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
    advance:
      if (!range)
	break;
      else if (range > 0)
	{
	  Bytecount d_size;
	  d = ((const unsigned char *)
	       (startpos >= size1 ? string2 - size1 : string1) + startpos);
	  d_size = itext_ichar_len_fmt (d, fmt);
	  range -= d_size;
          assert (!forward_search_p || range >= 0);
	  startpos += d_size;
	}
      else
	{
	  Bytecount d_size;
	  /* Note startpos > size1 not >=.  If we are on the
	     string1/string2 boundary, we want to backup into string1. */
	  d = ((const unsigned char *)
	       (startpos > size1 ? string2 - size1 : string1) + startpos);
	  DEC_IBYTEPTR_FMT (d, fmt);
	  d_size = itext_ichar_len_fmt (d, fmt);
	  range += d_size;
          assert (!forward_search_p || range >= 0);
	  startpos -= d_size;
	}
    }
  UNBIND_REGEX_MALLOC_CHECK ();
  return -1;
} /* re_search_2 */


/* Declarations and macros for re_match_2.  */

/* This converts PTR, a pointer into one of the search strings `string1'
   and `string2' into an offset from the beginning of that string.  */
#define POINTER_TO_OFFSET(ptr)			\
  (FIRST_STRING_P (ptr)				\
   ? ((regoff_t) ((ptr) - string1))		\
   : ((regoff_t) ((ptr) - string2 + size1)))

/* Macros for dealing with the split strings in re_match_2.  */

#define MATCHING_IN_FIRST_STRING  (dend == end_match_1)

/* Call before fetching a character with *d.  This switches over to
   string2 if necessary.  */
#define REGEX_PREFETCH()						\
  while (d == dend)						    	\
    {									\
      /* End of string2 => fail.  */					\
      if (dend == end_match_2) 						\
        goto fail;							\
      /* End of string1 => advance to string2.  */ 			\
      d = string2;						        \
      dend = end_match_2;						\
    }


/* Test if at very beginning or at very end of the virtual concatenation
   of `string1' and `string2'.  If only one string, it's `string2'.  */
#define AT_STRINGS_BEG(d) ((d) == (size1 ? string1 : string2) || !size2)
#define AT_STRINGS_END(d) ((d) == end2)

/* XEmacs change:
   If the given position straddles the string gap, return the equivalent
   position that is before or after the gap, respectively; otherwise,
   return the same position. */
#define POS_BEFORE_GAP_UNSAFE(d) ((d) == string2 ? end1 : (d))
#define POS_AFTER_GAP_UNSAFE(d) ((d) == end1 ? string2 : (d))

/* Test if CH is a word-constituent character. (XEmacs change) */
#define WORDCHAR_P(ch)						\
  (SYNTAX (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf), ch) == Sword)

/* Free everything we malloc.  */
#ifdef MATCH_MAY_ALLOCATE
#define FREE_VAR(var,type) if (var) REGEX_FREE (var, type); var = NULL
#define FREE_VARIABLES()						\
  do {									\
    UNBIND_REGEX_MALLOC_CHECK ();					\
    REGEX_FREE_STACK (fail_stack.stack);				\
    FREE_VAR (regstart, re_char **);					\
    FREE_VAR (regend, re_char **);					\
    FREE_VAR (best_regstart, re_char **);				\
    FREE_VAR (best_regend, re_char **);					\
  } while (0)
#else /* not MATCH_MAY_ALLOCATE */
#define FREE_VARIABLES()			\
  do {						\
    UNBIND_REGEX_MALLOC_CHECK ();		\
  } while (0)
#endif /* MATCH_MAY_ALLOCATE */

/* These values must meet several constraints.  They must not be valid register
   values, which means we can use numbers larger than MAX_REGNUM.  And the
   value for the lowest register must be larger than the value for the highest
   register, so we do not try to actually save any registers when none are
   active.  */
#define NO_HIGHEST_ACTIVE_REG (MAX_REGNUM + 1)
#define NO_LOWEST_ACTIVE_REG (NO_HIGHEST_ACTIVE_REG + 1)

/* Optimization routines.  */

/* Jump over non-matching operations.  */
static const re_char *
skip_noops (const re_char *p, const re_char *pend, re_bool memory)
{
  int mcnt;
  while (p < pend)
    {
      switch ((re_opcode_t) *p)
	{
	case start_memory:
	  if (!memory)
	    return p;
	case stop_memory:
	  p += 3; break;
	case no_op:
	  p += 1; break;
	case jump:
	  p += 1;
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  p += mcnt;
	  break;
	default:
	  return p;
	}
    }
  assert (p == pend);
  return p;
}

#ifdef emacs
/* Test if C matches charset op.  *PP points to the byte after the
   charset_mule or charset_mule_not opcode.  When the function finishes, *PP
   will be advanced past that opcode and its arguments.  C is character to
   test. TRANSLATE is the translate table if needed. LISPBUF is the buffer to
   match in, used for the case and syntax tables. */
static inline re_bool
execute_charset_mule (re_char **pp, Ichar c
		      RE_EXECUTE_CHARSET_MULE_ARGS_DECL)
{
  re_char *p = *pp;
  re_bool not_p = (re_opcode_t) (p[-1]) == charset_mule_not;
  Bitbyte class_bits = *p++;

  c = RE_TRANSLATE (c); /* The character to match.  */

  if ((class_bits &&
       ((class_bits & BIT_WORD && ISWORD (c)) /* = ALNUM */
	|| (class_bits & BIT_ALPHA && ISALPHA (c))
	|| (class_bits & BIT_SPACE && ISSPACE (c))
	|| (class_bits & BIT_PUNCT && ISPUNCT (c))
	|| (TRANSLATE_P (translate) ?
	    (class_bits & (BIT_UPPER | BIT_LOWER)
	     && !NOCASEP (lispbuf, c))
	    : ((class_bits & BIT_UPPER && ISUPPER (c))
	       || (class_bits & BIT_LOWER && ISLOWER (c))))))
      || EQ (Qt, unified_range_table_lookup ((void *) p, c, Qnil)))
  {
    not_p = !not_p;
  }

  *pp += unified_range_table_bytes_used ((void *)p)
    + 1 /* Include one for CLASS_BITS. */;

  return not_p;
}

#endif

static inline re_bool
execute_charset_nonmule (re_char **pp, Ichar c, RE_TRANSLATE_TYPE translate)
{
  re_char *p = *pp;
  re_bool not_p = (re_opcode_t) (p[-1]) == charset_not;

  c = RE_TRANSLATE (c); /* The character to match.  */

  /* Cast to `unsigned int' instead of `unsigned char' in case the bit list is
     a full 32 bytes long.  */
  if ((unsigned int) c < (unsigned int) (*p * BYTEWIDTH)
      && p[1 + c / BYTEWIDTH] & (1 << (c % BYTEWIDTH)))
    not_p = !not_p;

  *pp += 1 + *p;

  return not_p;
}

/* Non-zero if "p1 matches something" implies "p2 fails".  */
static re_bool
mutually_exclusive_p (struct re_pattern_buffer *bufp, re_char *p1,
		      re_char *p2)
{
  re_char *pend = bufp->buffer + bufp->used;
  RE_TRANSLATE_TYPE translate = bufp->translate;

  assert (p1 >= bufp->buffer && p1 <= pend
	  && p2 >= bufp->buffer && p2 <= pend);

  /* Skip over open/close-group commands.
     If what follows this loop is a ...+ construct,
     look at what begins its body, since we will have to
     match at least one of that.  */
  p2 = skip_noops (p2, pend, 1);
  /* The same skip can be done for p1, except that skipping over
     start_memory is not a good idea (if there's a group inside
     the loop delimited by on_failure_jump_exclusive, then it
     can't optimize the push away (it still works properly, but
     slightly slower rather than faster)).  */
  p1 = skip_noops (p1, pend, 0);

  /* If we're at the end of the pattern, we can change.  */
  if (p2 == pend)
    {
      switch ((re_opcode_t) *p1)
	{
	case anychar:
	case charset_not:
	case charset:
	case exactn:
	  DEBUG_COMPILE_PRINT ("  End of pattern: fast loop.\n");
	  return 1;
	default:
	  return 0;
	}
    }

  else if ((re_opcode_t) *p2 == exactn
	   || (bufp->newline_anchor && (re_opcode_t) *p2 == endline))
    {
      REGISTER Ichar c
	= *p2 == (re_char) endline ? '\n' : itext_ichar (p2 + 2);

      if ((re_opcode_t) *p1 == exactn)
	{
	  Ichar c1 = itext_ichar (p1 + 2);
	  if (RE_TRANSLATE (c) != RE_TRANSLATE (c1))
	    {
	      DEBUG_COMPILE_PRINT ("  '%c' != '%c' => fast loop.\n", c,
				   itext_ichar (p1 + 2));
	      return 1;
	    }
	}
      else if ((re_opcode_t) *p1 == charset
	       || (re_opcode_t) *p1 == charset_not)
	{
	  re_bool not_p = (re_opcode_t) *p1 == charset_not;
	  re_char *pp = p1 + 1;

	  /* Test if C is listed in charset (or charset_not) at `p1'.  */
	  not_p = execute_charset_nonmule (&pp, c, translate);

	  /* `not_p' is equal to 1 if c would match, which means
	     that we can't change to pop_failure_jump.  */
	  if (!not_p)
	    {
	      DEBUG_COMPILE_PRINT ("	 No match => fast loop.\n");
	      return 1;
	    }
	}
#ifdef emacs
      else if ((re_opcode_t) *p1 == charset_mule
	       || (re_opcode_t) *p1 == charset_mule_not)
	{
	  re_bool not_p = (re_opcode_t) *p1 == charset_mule_not;
	  re_char *pp = p1 + 1;

	  if (*pp)
	    {
	      /* Class bits used, cannot determine this at compile time. */
	      return 0;
	    }

	  /* Test if C is listed in charset_mule (or charset_mule_not) at
	     `p1'.  */
	  not_p = execute_charset_mule (&pp, c
					RE_EXECUTE_CHARSET_MULE_ARGS
					(translate, NULL));
	  /* `not_p' is equal to 1 if c would match, which means
	     that we can't change to pop_failure_jump.  */
	  if (!not_p)
	    {
	      DEBUG_COMPILE_PRINT ("	 No match => fast loop.\n");
	      return 1;
	    }
	}
#endif /* emacs */
      else if ((re_opcode_t) *p1 == anychar
	       && c == '\n' && !(bufp->syntax & RE_DOT_NEWLINE))
	{
	  DEBUG_COMPILE_PRINT ("   . != \\n => fast loop.\n");
	  return 1;
	}
    }
  else if ((re_opcode_t) *p2 == charset || (re_opcode_t) *p2 == charset_not)
    {
      if ((re_opcode_t) *p1 == exactn)
	/* Reuse the code above.  */
	return mutually_exclusive_p (bufp, p2, p1);
      else if (*p1 == *p2)
	{
	  /* Now, we are sure that P2 is not charset_mule or charset_mule_not,
	     for the size of bitmap in P2, `p2[1]' is enough.

	     Since we know that all the characters listed in P2 are ASCII, it
	     is enough to test only bitmap table of P1.  */
	  int idx;
	  /* We win if the charset inside the loop has no overlap with the one
	     after the loop.  */
	  for (idx = 0;
	       (idx < (int) p2[1] && idx < CHARSET_BITMAP_SIZE (p1)); idx++)
	    {
	      if ((p2[2 + idx] & p1[2 + idx]) != 0)
		break;
	    }

	  if (idx == p2[1] || idx == CHARSET_BITMAP_SIZE (p1))
	    {
	      DEBUG_COMPILE_PRINT ("	 No match => fast loop.\n");
	      return 1;
	    }
	}
      else if ((re_opcode_t) *p1 == charset
	       || (re_opcode_t) *p1 == charset_not)
	{
	  int idx;
	  /* We win if the charset_not inside the loop lists
	     every character listed in the charset after.	 */
	  for (idx = 0; idx < (int) p2[1]; idx++)
	    {
	      if (! (p2[2 + idx] == 0
		     || (idx < CHARSET_BITMAP_SIZE (p1)
			 && ((p2[2 + idx] & ~ p1[2 + idx]) == 0))))
		break;

	      if (idx == p2[1])
		{
		  DEBUG_COMPILE_PRINT ("	 No match => fast loop.\n");
		  return 1;
		}
	    }
	}
    }

  /* Safe default.  */
  return 0;
}

/* Matching routines.  */

#ifndef emacs   /* XEmacs never uses this.  */
/* re_match is like re_match_2 except it takes only a single string.  */

int
re_match (struct re_pattern_buffer *bufp, const char *string, int size,
	  int pos, struct re_registers *regs
	  RE_LISP_CONTEXT_ARGS_DECL)
{
  int result = re_match_2_internal (bufp, NULL, 0, (re_char *) string, size,
				    pos, regs, size
				    RE_LISP_CONTEXT_ARGS);
  ALLOCA_GARBAGE_COLLECT ();
  return result;
}
#endif /* not emacs */

/* re_match_2 matches the compiled pattern in BUFP against the
   (virtual) concatenation of STRING1 and STRING2 (of length SIZE1 and
   SIZE2, respectively).  We start matching at POS, and stop matching
   at STOP.

   If REGS is non-null and the `no_sub' field of BUFP is nonzero, we
   store offsets for the substring each group matched in REGS.  See the
   documentation for exactly how many groups we fill.

   We return -1 if no match, -2 if an internal error (such as the
   failure stack overflowing).  Otherwise, we return the length of the
   matched substring.  */

int
re_match_2 (struct re_pattern_buffer *bufp, const char *string1,
	    int size1, const char *string2, int size2, int pos,
	    struct re_registers *regs, int stop
	    RE_LISP_CONTEXT_ARGS_DECL)
{
  int result;

#ifdef emacs
  /* Update the mirror syntax table if it's dirty now, this would otherwise
     cause a malloc() in charset_mule in re_match_2_internal() when checking
     characters' syntax. */
  SYNTAX_CODE (BUFFER_MIRROR_SYNTAX_TABLE (lispbuf), 'a');
  scache = setup_syntax_cache (scache, lispobj, lispbuf,
                               offset_to_bytexpos (lispobj, pos),
			       1);
#endif

  result = re_match_2_internal (bufp, (re_char *) string1, size1,
				(re_char *) string2, size2,
				pos, regs, stop
				RE_LISP_CONTEXT_ARGS);

  ALLOCA_GARBAGE_COLLECT ();
  return result;
}

/* This is a separate function so that we can force an alloca cleanup
   afterwards.  */
static int
re_match_2_internal (struct re_pattern_buffer *bufp, re_char *string1,
		     int size1, re_char *string2, int size2, int pos,
		     struct re_registers *regs, int stop
		     RE_LISP_CONTEXT_ARGS_DECL)
{
  /* General temporaries.  */
  int mcnt;
  int should_succeed; /* XEmacs change */

  /* Just past the end of the corresponding string.  */
  re_char *end1, *end2;

  /* Pointers into string1 and string2, just past the last characters in
     each to consider matching.  */
  re_char *end_match_1, *end_match_2;

  /* Where we are in the data, and the end of the current string.  */
  re_char *d, *dend;

  /* Where we are in the pattern, and the end of the pattern.  */
  unsigned char *p;
  re_char *pstart;
  REGISTER re_char *pend;

  /* We use this to map every character in the string.  */
  RE_TRANSLATE_TYPE translate = bufp->translate;

  /* Failure point stack.  Each place that can handle a failure further
     down the line pushes a failure point on this stack.  It consists of
     regstart and regend for all registers corresponding to
     the subexpressions we're currently inside, plus the number of such
     registers, and, finally, two char *'s.  The first char * is where
     to resume scanning the pattern; the second one is where to resume
     scanning the strings. */
#ifdef MATCH_MAY_ALLOCATE /* otherwise, this is global.  */
  fail_stack_type fail_stack;
#endif

  static int failure_id;
  int nfailure_points_pushed, nfailure_points_popped;

#ifdef REGEX_REL_ALLOC
  /* This holds the pointer to the failure stack, when
     it is allocated relocatably.  */
  fail_stack_elt_t *failure_stack_ptr;
#endif

  /* We fill all the registers internally, independent of what we
     return, for use in backreferences.  The number here includes
     an element for register zero.  */
  regnum_t num_regs = bufp->re_ngroups + 1;

  /* Information on the contents of registers. These are pointers into
     the input strings; they record just what was matched (on this
     attempt) by a subexpression part of the pattern, that is, the
     regnum-th regstart pointer points to where in the pattern we began
     matching and the regnum-th regend points to right after where we
     stopped matching the regnum-th subexpression.  (The zeroth register
     keeps track of what the whole pattern matches.)  */
#ifdef MATCH_MAY_ALLOCATE /* otherwise, these are global.  */
  re_char **regstart, **regend;
#endif

  /* The following record the register info as found in the above
     variables when we find a match better than any we've seen before.
     This happens as we backtrack through the failure points, which in
     turn happens only if we have not yet matched the entire string. */
  int best_regs_set = false;
#ifdef MATCH_MAY_ALLOCATE /* otherwise, these are global.  */
  re_char **best_regstart, **best_regend;
#endif

  /* Logically, this is `best_regend[0]'.  But we don't want to have to
     allocate space for that if we're not allocating space for anything
     else (see below).  Also, we never need info about register 0 for
     any of the other register vectors, and it seems rather a kludge to
     treat `best_regend' differently than the rest.  So we keep track of
     the end of the best match so far in a separate variable.  We
     initialize this to NULL so that when we backtrack the first time
     and need to test it, it's not garbage.  */
  re_char *match_end = NULL;

  /* Counts the total number of registers pushed.  */
  int num_regs_pushed;

  /* 1 if this match ends in the same string (string1 or string2)
     as the best previous match.  */
  re_bool same_str_p;

  /* 1 if this match is the best seen so far.  */
  re_bool best_match_p;

#ifdef emacs
  Internal_Format fmt = buffer_or_other_internal_format (lispobj);
#ifdef REL_ALLOC
  const Ibyte *orig_buftext =
    BUFFERP (lispobj) ?
    BYTE_BUF_BYTE_ADDRESS (XBUFFER (lispobj),
			   BYTE_BUF_BEG (XBUFFER (lispobj))) :
    0;
#endif

#ifdef ERROR_CHECK_MALLOC
  int depth = bind_regex_malloc_disallowed (1);
#endif
#endif /* emacs */

  DEBUG_STATEMENT ((nfailure_points_pushed = 0,
		    nfailure_points_popped = 0,
		    num_regs_pushed = 0));

  DEBUG_MATCH_PRINT ("\n\nEntering re_match_2.\n");

  BEGIN_REGEX_MALLOC_OK ();
  INIT_FAIL_STACK ();
  p = (unsigned char *) ALLOCA (bufp->used);
  END_REGEX_MALLOC_OK ();

  /* re_match_2_internal() modifies the compiled pattern (see the succeed_n,
     jump_n, set_number_at opcodes), make it re-entrant by working on a
     copy. This should also give better locality of reference. */
  memcpy (p, bufp->buffer, bufp->used);
  pstart = (re_char *) p;
  pend = pstart + bufp->used;

#ifdef MATCH_MAY_ALLOCATE
  /* Do not bother to initialize all the register variables if there are
     no groups in the pattern, as it takes a fair amount of time.  If
     there are groups, we include space for register 0 (the whole
     pattern), even though we never use it, since it simplifies the
     array indexing.  We should fix this.  */
  if (bufp->re_ngroups)
    {
      BEGIN_REGEX_MALLOC_OK ();
      regstart       = REGEX_TALLOC (num_regs, re_char *);
      regend         = REGEX_TALLOC (num_regs, re_char *);
      best_regstart  = REGEX_TALLOC (num_regs, re_char *);
      best_regend    = REGEX_TALLOC (num_regs, re_char *);
      END_REGEX_MALLOC_OK ();

      if (!(regstart && regend && best_regstart && best_regend))
        {
          FREE_VARIABLES ();
          return -2;
        }
    }
  else
    {
      /* We must initialize all our variables to NULL, so that
         `FREE_VARIABLES' doesn't try to free them.  */
      regstart = regend = best_regstart = best_regend = NULL;
    }
#endif /* MATCH_MAY_ALLOCATE */

#if defined (emacs) && defined (REL_ALLOC)
  {
    /* If the allocations above (or the call to setup_syntax_cache() in
       re_match_2) caused a rel-alloc relocation, then fix up the data
       pointers */
    Bytecount offset = offset_post_relocation (lispobj, orig_buftext);
    if (offset)
      {
	string1 += offset;
	string2 += offset;
      }
  }
#endif /* defined (emacs) && defined (REL_ALLOC) */

  /* The starting position is bogus.  */
  if (pos < 0 || pos > size1 + size2)
    {
      FREE_VARIABLES ();
      return -1;
    }

  /* Initialize subexpression text positions to our sentinel to mark ones that
     no start_memory/stop_memory has been seen for. Also initialize the
     register information struct.  */
  for (mcnt = 1; mcnt < num_regs; mcnt++)
    {
      regstart[mcnt] = regend[mcnt] = REG_UNSET_VALUE;
    }
  /* We move `string1' into `string2' if the latter's empty -- but not if
     `string1' is null.  */
  if (size2 == 0 && string1 != NULL)
    {
      string2 = string1;
      size2 = size1;
      string1 = 0;
      size1 = 0;
    }
  end1 = string1 + size1;
  end2 = string2 + size2;

  /* Compute where to stop matching, within the two strings.  */
  if (stop <= size1)
    {
      end_match_1 = string1 + stop;
      end_match_2 = string2;
    }
  else
    {
      end_match_1 = end1;
      end_match_2 = string2 + stop - size1;
    }

  /* `p' scans through the pattern as `d' scans through the data.
     `dend' is the end of the input string that `d' points within.  `d'
     is advanced into the following input string whenever necessary, but
     this happens before fetching; therefore, at the beginning of the
     loop, `d' can be pointing at the end of a string, but it cannot
     equal `string2'.  */
  if (size1 > 0 && pos <= size1)
    {
      d = string1 + pos;
      dend = end_match_1;
    }
  else
    {
      d = string2 + pos - size1;
      dend = end_match_2;
    }

  DEBUG_MATCH_PRINT ("The compiled pattern is: \n");
  DEBUG_MATCH_PRINT_COMPILED_PATTERN (bufp, p, pend);
  DEBUG_MATCH_PRINT ("The string to match is: `");
  DEBUG_MATCH_PRINT_DOUBLE_STRING (d, string1, size1, string2, size2);
  DEBUG_MATCH_PRINT ("'\n");

  /* This loops over pattern commands.  It exits by returning from the
     function if the match is complete, or it drops through if the match
     fails at this starting point in the input data.  */
  for (;;)
    {
      DEBUG_MATCH_PRINT ("\n0x%zx: ", (Bytecount) p);
#ifdef emacs /* XEmacs added, w/removal of immediate_quit */
      if (!no_quit_in_re_search)
	{
	  BEGIN_REGEX_MALLOC_OK ();
	  QUIT;
	  END_REGEX_MALLOC_OK ();
	  RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	}
#endif

      if (p == pend)
	{ /* End of pattern means we might have succeeded.  */
          DEBUG_MATCH_PRINT ("end of pattern ... ");

	  /* If we haven't matched the entire string, and we want the
             longest match, try backtracking.  */
          if (d != end_match_2)
	    {
	      same_str_p = (FIRST_STRING_P (match_end)
			    == MATCHING_IN_FIRST_STRING);

	      /* AIX compiler got confused when this was combined
		 with the previous declaration.  */
	      if (same_str_p)
		best_match_p = d > match_end;
	      else
		best_match_p = !MATCHING_IN_FIRST_STRING;

              DEBUG_MATCH_PRINT ("backtracking.\n");

              if (!FAIL_STACK_EMPTY ())
                { /* More failure points to try.  */

                  /* If exceeds best match so far, save it.  */
                  if (!best_regs_set || best_match_p)
                    {
                      best_regs_set = true;
                      match_end = d;

                      DEBUG_MATCH_PRINT ("\nSAVING match as best so far.\n");

                      for (mcnt = 1; mcnt < num_regs; mcnt++)
                        {
                          best_regstart[mcnt] = regstart[mcnt];
                          best_regend[mcnt] = regend[mcnt];
                        }
                    }
                  goto fail;
                }

              /* If no failure points, don't restore garbage.  And if
                 last match is real best match, don't restore second
                 best one. */
              else if (best_regs_set && !best_match_p)
                {
  	        restore_best_regs:
                  /* Restore best match.  It may happen that `dend ==
                     end_match_1' while the restored d is in string2.
                     For example, the pattern `x.*y.*z' against the
                     strings `x-' and `y-z-', if the two strings are
                     not consecutive in memory.  */
                  DEBUG_MATCH_PRINT ("Restoring best registers.\n");

                  d = match_end;
                  dend = ((d >= string1 && d <= end1)
		           ? end_match_1 : end_match_2);

		  for (mcnt = 1; mcnt < num_regs; mcnt++)
		    {
		      regstart[mcnt] = best_regstart[mcnt];
		      regend[mcnt] = best_regend[mcnt];
		    }
                }
            } /* d != end_match_2 */

	succeed_label:
          DEBUG_MATCH_PRINT ("Accepting match.\n");

          /* If caller wants register contents data back, do it.  */
	  {
	    int num_nonshy_regs = bufp->re_nsub + 1;
	    if (regs && !bufp->no_sub)
	      {
		/* Have the register data arrays been allocated?  */
		if (bufp->regs_allocated == REGS_UNALLOCATED)
		  { /* No.  So allocate them with malloc.  We need one
		       extra element beyond `num_regs' for the `-1' marker
		       GNU code uses.  */
		    regs->num_regs = MAX (RE_NREGS, num_nonshy_regs + 1);
		    BEGIN_REGEX_MALLOC_OK ();
		    regs->start = TALLOC (regs->num_regs, regoff_t);
		    regs->end = TALLOC (regs->num_regs, regoff_t);
		    END_REGEX_MALLOC_OK ();
		    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
		    if (regs->start == NULL || regs->end == NULL)
		      {
			FREE_VARIABLES ();
			return -2;
		      }
		    bufp->regs_allocated = REGS_REALLOCATE;
		  }
		else if (bufp->regs_allocated == REGS_REALLOCATE)
		  { /* Yes.  If we need more elements than were already
		       allocated, reallocate them.  If we need fewer, just
		       leave it alone.  */
		    if (regs->num_regs < num_nonshy_regs + 1)
		      {
			regs->num_regs = num_nonshy_regs + 1;
			BEGIN_REGEX_MALLOC_OK ();
			RETALLOC (regs->start, regs->num_regs, regoff_t);
			RETALLOC (regs->end, regs->num_regs, regoff_t);
			END_REGEX_MALLOC_OK ();
			RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
			if (regs->start == NULL || regs->end == NULL)
			  {
			    FREE_VARIABLES ();
			    return -2;
			  }
		      }
		  }
		else
		  {
		    /* The braces fend off a "empty body in an else-statement"
		       warning under GCC when assert expands to nothing.  */
		    assert (bufp->regs_allocated == REGS_FIXED);
		  }

		/* Convert the pointer data in `regstart' and `regend' to
		   indices.  Register zero has to be set differently,
		   since we haven't kept track of any info for it.  */
		if (regs->num_regs > 0)
		  {
		    regs->start[0] = pos;
		    regs->end[0] = (MATCHING_IN_FIRST_STRING
				    ? ((regoff_t) (d - string1))
				    : ((regoff_t) (d - string2 + size1)));
		  }

		/* Map over the NUM_NONSHY_REGS non-shy internal registers.
		   Copy each into the corresponding external register.
		   MCNT indexes external registers. */
		for (mcnt = 1; mcnt < MIN (num_nonshy_regs, regs->num_regs);
		     mcnt++)
		  {
		    int internal_reg = bufp->external_to_internal_register[mcnt];
		    if ((int)0xDEADBEEF == internal_reg 
                        || REG_UNSET (regstart[internal_reg]) ||
			REG_UNSET (regend[internal_reg]))
		      regs->start[mcnt] = regs->end[mcnt] = -1;
		    else
		      {
			regs->start[mcnt] =
			  (regoff_t) POINTER_TO_OFFSET (regstart[internal_reg]);
			regs->end[mcnt] =
			  (regoff_t) POINTER_TO_OFFSET (regend[internal_reg]);
		      }
		  }
	      } /* regs && !bufp->no_sub */

	    /* If we have regs and the regs structure has more elements than
	       were in the pattern, set the extra elements starting with
	       NUM_NONSHY_REGS to -1.  If we (re)allocated the registers,
	       this is the case, because we always allocate enough to have
	       at least one -1 at the end.

	       We do this even when no_sub is set because some applications
	       (XEmacs) reuse register structures which may contain stale
	       information, and permit attempts to access those registers.

	       It would be possible to require the caller to do this, but we'd
	       have to change the API for this function to reflect that, and
	       audit all callers.  Note: as of 2003-04-17 callers in XEmacs
	       do clear the registers, but it's safer to leave this code in
	       because of reallocation.
	    */
	    if (regs && regs->num_regs > 0)
	      for (mcnt = num_nonshy_regs; mcnt < regs->num_regs; mcnt++)
		regs->start[mcnt] = regs->end[mcnt] = -1;
	  }
          DEBUG_MATCH_PRINT ("%u failure points pushed, %u popped (%u remain).\n",
                        nfailure_points_pushed, nfailure_points_popped,
                        nfailure_points_pushed - nfailure_points_popped);
          DEBUG_MATCH_PRINT ("%u registers pushed.\n", num_regs_pushed);

          mcnt = d - pos - (MATCHING_IN_FIRST_STRING
			    ? string1
			    : string2 - size1);

          DEBUG_MATCH_PRINT ("Returning %d from re_match_2.\n", mcnt);

          FREE_VARIABLES ();
          return mcnt;
        }

      /* Otherwise match next pattern command.  */
      switch ((re_opcode_t) *p++)
	{
        /* Ignore these.  Used to ignore the n of succeed_n's which
           currently have n == 0.  */
        case no_op:
          DEBUG_MATCH_PRINT ("EXECUTING no_op.\n");
          break;

	case succeed:
          DEBUG_MATCH_PRINT ("EXECUTING succeed.\n");
	  goto succeed_label;

        /* Match exactly a string of length n in the pattern.  The
           following byte in the pattern defines n, and the n bytes after
           that make up the string to match. (Under Mule, this will be in
           the default internal format.) */
	case exactn:
	  mcnt = *p++;
          DEBUG_MATCH_PRINT ("EXECUTING exactn %d.\n", mcnt);

          /* This is written out as an if-else so we don't waste time
             testing `translate' inside the loop.  */
          if (TRANSLATE_P (translate))
	    {
	      do
		{
#ifdef emacs
		  Bytecount pat_len;

		  REGEX_PREFETCH ();
		  if (RE_TRANSLATE_1 (itext_ichar_fmt (d, fmt, lispobj))
		      != itext_ichar (p))
                    goto fail;

		  pat_len = itext_ichar_len (p);
		  p += pat_len;
		  INC_IBYTEPTR_FMT (d, fmt);
		  
		  mcnt -= pat_len;
#else /* not emacs */
		  REGEX_PREFETCH ();
		  if ((unsigned char) RE_TRANSLATE_1 (*d++) != *p++)
                    goto fail;
		  mcnt--;
#endif
		}
	      while (mcnt > 0);
	    }
	  else
	    {
#ifdef emacs
	      /* If buffer format is default, then we can shortcut and just
		 compare the text directly, byte by byte.  Otherwise, we
		 need to go character by character. */
	      if (fmt != FORMAT_DEFAULT)
		{
		  do
		    {
		      Bytecount pat_len;

		      REGEX_PREFETCH ();
		      if (itext_ichar_fmt (d, fmt, lispobj) !=
			  itext_ichar (p))
			goto fail;

		      pat_len = itext_ichar_len (p);
		      p += pat_len;
		      INC_IBYTEPTR_FMT (d, fmt);
		  
		      mcnt -= pat_len;
		    }
		  while (mcnt > 0);
		}
	      else
#endif
		{
		  do
		    {
		      REGEX_PREFETCH ();
		      if (*d++ != *p++) goto fail;
		      mcnt--;
		    }
		  while (mcnt > 0);
		}
	    }
          break;


        /* Match any character except possibly a newline or a null.  */
	case anychar:
          DEBUG_MATCH_PRINT ("EXECUTING anychar.\n");

          REGEX_PREFETCH ();

          if ((!(bufp->syntax & RE_DOT_NEWLINE) &&
	       RE_TRANSLATE (itext_ichar_fmt (d, fmt, lispobj)) == '\n'))
	    goto fail;

          DEBUG_MATCH_PRINT ("  Matched `%c'.\n", *d);
	  INC_IBYTEPTR_FMT (d, fmt); /* XEmacs change */
	  break;


	case charset:
	case charset_not:
	  {
	    re_bool not_p = (re_opcode_t) *(p - 1) == charset_not;
	    REGISTER Ichar c;

	    REGEX_PREFETCH ();
	    c = itext_ichar_fmt (d, fmt, lispobj);

	    not_p = execute_charset_nonmule ((re_char **) (&p), c, translate);

	    if (!not_p) goto fail;

            INC_IBYTEPTR_FMT (d, fmt); /* XEmacs change */
	    break;
	  }

#ifdef emacs
	case charset_mule:
	case charset_mule_not:
	  {
	    REGISTER Ichar c;
	    re_bool not_p = (re_opcode_t) *(p - 1) == charset_mule_not;

	    REGEX_PREFETCH ();
	    c = itext_ichar_fmt (d, fmt, lispobj);

	    not_p = execute_charset_mule ((re_char **) (&p), c
					  RE_EXECUTE_CHARSET_MULE_ARGS
					  (translate, lispbuf));

	    if (!not_p) goto fail;

	    INC_IBYTEPTR_FMT (d, fmt);
	    break;
	  }
#endif /* emacs */


        /* The beginning of a group is represented by start_memory.  The
           argument is the register number in the next two bytes.  The text
           matched within the group is recorded (in the internal registers data
           structure) under the register number.  */
        case start_memory:
	  {
	    regnum_t regno;

	    EXTRACT_NONNEGATIVE_AND_INCR (regno, p);

	    DEBUG_MATCH_PRINT ("EXECUTING start_memory %d:\n", regno);

	    /* In case we need to undo this operation (via backtracking).  */
	    PUSH_FAILURE_REG (regno);

	    regstart[regno] = d;
	    regend[regno] = REG_UNSET_VALUE; /* probably unnecessary.  -sm  */
	    DEBUG_MATCH_PRINT ("  regstart: %zd\n",
				POINTER_TO_OFFSET (regstart[regno]));
	    break;
	  }

        /* The stop_memory opcode represents the end of a group.  Its argument
           is the same as start_memory's: the register number. */
	case stop_memory:
	  {
	    regnum_t regno;

	    EXTRACT_NONNEGATIVE_AND_INCR (regno, p);

	    DEBUG_MATCH_PRINT ("EXECUTING stop_memory %d:\n", regno);

	    /* Strictly speaking, there should be code such as:

	         assert (REG_UNSET (regend[*p]));
		 PUSH_FAILURE_REGSTOP ((unsigned int)*p);

	       But the only info to be pushed is regend[*p] and it is known to
	       be UNSET, so there really isn't anything to push.  Not pushing
	       anything, on the other hand deprives us from the guarantee that
	       regend[*p] is UNSET since undoing this operation will not reset
	       its value properly.  This is not important since the value will
	       only be read on the next start_memory or at the very end and
	       both events can only happen if this stop_memory is *not*
	       undone.  */
	    regend[regno] = d;
	    DEBUG_MATCH_PRINT ("      regend: %zd\n",
			      POINTER_TO_OFFSET (regend[regno]));
	    break;
	  }

	/* \<number> has been turned into a `duplicate' command which is
           followed by the numeric value of <number> as the register number.
           (Already passed through external-to-internal-register mapping, so
           it refers to the actual group number, not the non-shy-only
           numbering used in the external world.) */
        case duplicate:
	  {
	    REGISTER re_char *d2, *dend2;
	    /* Get which register to match against.  */
	    regnum_t regno;
	    
	    EXTRACT_NONNEGATIVE_AND_INCR (regno, p);
	    DEBUG_MATCH_PRINT ("EXECUTING duplicate %d.\n", regno);

	    /* Can't back reference a group which we've never matched.  */
            if (REG_UNSET (regstart[regno]) || REG_UNSET (regend[regno]))
              goto fail;

            /* Where in input to try to start matching.  */
            d2 = regstart[regno];

            /* Where to stop matching; if both the place to start and
               the place to stop matching are in the same string, then
               set to the place to stop, otherwise, for now have to use
               the end of the first string.  */

            dend2 = ((FIRST_STRING_P (regstart[regno])
		      == FIRST_STRING_P (regend[regno]))
		     ? regend[regno] : end_match_1);
	    for (;;)
	      {
		/* If necessary, advance to next segment in register
                   contents.  */
		while (d2 == dend2)
		  {
		    if (dend2 == end_match_2) break;
		    if (dend2 == regend[regno]) break;

                    /* End of string1 => advance to string2. */
                    d2 = string2;
                    dend2 = regend[regno];
		  }
		/* At end of register contents => success */
		if (d2 == dend2) break;

		/* If necessary, advance to next segment in data.  */
		REGEX_PREFETCH ();

		/* How many characters left in this segment to match.  */
		mcnt = dend - d;

		/* Want how many consecutive characters we can match in
                   one shot, so, if necessary, adjust the count.  */
                if (mcnt > dend2 - d2)
		  mcnt = dend2 - d2;

		/* Compare that many; failure if mismatch, else move
                   past them.  */
		if (TRANSLATE_P (translate)
                    ? bcmp_translate (d, d2, mcnt, translate
#ifdef emacs
				      , fmt, lispobj
#endif
				      )
                    : memcmp (d, d2, mcnt))
		  goto fail;
		d += mcnt, d2 += mcnt;
	      }
	  }
	  break;


        /* begline matches the empty string at the beginning of the string
           (unless `not_bol' is set in `bufp'), and, if
           `newline_anchor' is set, after newlines.  */
	case begline:
          DEBUG_MATCH_PRINT ("EXECUTING begline.\n");

          if (AT_STRINGS_BEG (d))
            {
              if (!bufp->not_bol) break;
            }
          else
	    {
	      re_char *d2 = d;
	      DEC_IBYTEPTR (d2);
	      if (itext_ichar_ascii_fmt (d2, fmt, lispobj) == '\n' &&
		  bufp->newline_anchor)
		break;
	    }
          /* In all other cases, we fail.  */
          goto fail;


        /* endline is the dual of begline.  */
	case endline:
          DEBUG_MATCH_PRINT ("EXECUTING endline.\n");

          if (AT_STRINGS_END (d))
            {
              if (!bufp->not_eol) break;
            }

          /* We have to ``prefetch'' the next character.  */
          else if ((d == end1 ?
		    itext_ichar_ascii_fmt (string2, fmt, lispobj) :
		    itext_ichar_ascii_fmt (d, fmt, lispobj)) == '\n'
                   && bufp->newline_anchor)
            {
              break;
            }
          goto fail;


	/* Match at the very beginning of the data.  */
        case begbuf:
          DEBUG_MATCH_PRINT ("EXECUTING begbuf.\n");
          if (AT_STRINGS_BEG (d))
            break;
          goto fail;


	/* Match at the very end of the data.  */
        case endbuf:
          DEBUG_MATCH_PRINT ("EXECUTING endbuf.\n");
	  if (AT_STRINGS_END (d))
	    break;
          goto fail;


	case on_failure_jump_exclusive:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  DEBUG_MATCH_PRINT ("EXECUTING on_failure_jump_exclusive %d "
			     "(to %p):\n", mcnt, p + mcnt);

	  if (! FAIL_STACK_EMPTY ()
	      && FAILURE_PAT (TOP_FAILURE_HANDLE ()) == (p - 3)
	      && fail_stack.avail == fail_stack.frame)
	    {
	      /* We are trying to push failure F2 onto the stack but there
		 is already a failure F1 pushed from the same instruction.
		 Between F1 and now, something has matched (else this is an
		 improper use of on_failure_jump_exclusive), so that we know
		 that the fail-destination of F1 cannot match, hence we can
		 pop F1 before pushing F2.  Instead of doing this pop/push,
		 we manually turn F1 into F2.
		 `fail_stack.avail == fail_stack.frame' makes sure
		 that popping F1 doesn't involve registers, else
		 this optimization cannot be done so trivially.  */
	      assert ((re_char *) (FAILURE_STR (TOP_FAILURE_HANDLE ()))
		      != d);
	      FAILURE_STR (TOP_FAILURE_HANDLE ()) = d;
	    }
	  else
	    PUSH_FAILURE_POINT (p - 3, d);
	  break;

	case on_failure_jump_smart:
	  assert (0); /* Should have been removed from the pattern by
			 fixup_on_failure_jump_smart(). */
	  /* FALLTHROUGH */
	case on_failure_jump_loop:
	on_failure:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  DEBUG_MATCH_PRINT ("EXECUTING on_failure_jump_loop %d (to %p):\n",
			     mcnt, p + mcnt);

	  CHECK_INFINITE_LOOP (p - 3, d);
	  PUSH_FAILURE_POINT (p - 3, d);
	  break;

	/* Uses of on_failure_jump:

           Each alternative starts with an on_failure_jump that points
           to the beginning of the next alternative.  Each alternative
           except the last ends with a jump that in effect jumps past
           the rest of the alternatives.  (They really jump to the
           ending jump of the following alternative, because tensioning
           these jumps is a hassle.)

           Repeats start with an on_failure_jump that points past both
           the repetition text and either the following jump or
           pop_failure_jump back to this on_failure_jump.  */
	case on_failure_jump:
          EXTRACT_NUMBER_AND_INCR (mcnt, p);
	  DEBUG_MATCH_PRINT ("EXECUTING on_failure_jump %d (to %p):\n",
			     mcnt, p + mcnt);

	  PUSH_FAILURE_POINT (p -3, d);
 	  break;

	/* We need this opcode so we can detect where alternatives end
	   in `group_match_null_string_p' et al.  */
        case jump_past_alt:
          DEBUG_MATCH_PRINT ("EXECUTING jump_past_alt.\n");
        /* Unconditionally jump (without popping any failure points).  */
        case jump:
	unconditional_jump:
	  EXTRACT_NUMBER_AND_INCR (mcnt, p);	/* Get the amount to jump.  */
          DEBUG_MATCH_PRINT ("EXECUTING jump %d ", mcnt);
	  p += mcnt;				/* Do the jump.  */
          DEBUG_MATCH_PRINT ("(to 0x%zx).\n", (Bytecount) p);
	  break;

        /* Have to succeed matching what follows at least n times.
           After that, handle like `on_failure_jump'.  */
        case succeed_n:
          EXTRACT_NONNEGATIVE (mcnt, p + 2);
          DEBUG_MATCH_PRINT ("EXECUTING succeed_n %d.\n", mcnt);

          /* Originally, this is how many times we HAVE to succeed.  */
          if (mcnt)
            {
               mcnt--;
	       p += 2;
               DEBUG_MATCH_PRINT ("  Setting 0x%zx to %d.\n", (Bytecount) p,
                                   mcnt);
               STORE_MATCH_NUMBER_AND_INCR (p, mcnt);
            }
	  else
            {
              DEBUG_MATCH_PRINT ("  Setting two bytes from 0x%zx to no_op.\n",
			    (Bytecount) (p+2));
	      STORE_MATCH_NUMBER (p + 2, no_op);
              goto on_failure;
            }
          break;

        case jump_n:
          EXTRACT_NONNEGATIVE (mcnt, p + 2);
          DEBUG_MATCH_PRINT ("EXECUTING jump_n %d.\n", mcnt);

          /* Originally, this is how many times we CAN jump.  */
          if (mcnt)
            {
               mcnt--;
               STORE_MATCH_NUMBER (p + 2, mcnt);
	       goto unconditional_jump;
            }
          /* If don't have to jump any more, skip over the rest of command.  */
	  else
	    p += 4;
          break;

	case set_number_at:
	  {
	    unsigned char *p2;	/* Location of the counter.  */
            DEBUG_MATCH_PRINT ("EXECUTING set_number_at.\n");

            EXTRACT_NUMBER_AND_INCR (mcnt, p);
	    /* Discard 'const'. Now the pattern is copied to the stack, this
               does not change re-entrancy of re_match_2_internal(). */
	    p2 = (unsigned char *) p + mcnt;
            EXTRACT_NONNEGATIVE_AND_INCR (mcnt, p);

            DEBUG_MATCH_PRINT ("  Setting 0x%zx to %d.\n", (Bytecount) p2,
                                mcnt);
	    STORE_MATCH_NUMBER (p2, mcnt);
            break;
          }

        case wordbound:
          DEBUG_MATCH_PRINT ("EXECUTING wordbound.\n");
	  should_succeed = 1;
	matchwordbound:
	  {
	    /* XEmacs change */
	    /* Straightforward and (I hope) correct implementation. */
	    /* emch1 is the character before d, syn1 is the syntax of
	       emch1, emch2 is the character at d, and syn2 is the
	       syntax of emch2. */
	    Ichar emch1, emch2;
	    int syn1 = 0,
	        syn2 = 0;
	    re_char *d_before, *d_after;
	    int result,
		at_beg = AT_STRINGS_BEG (d),
		at_end = AT_STRINGS_END (d);

	    if (at_beg && at_end)
	      {
		result = 0;
	      }
	    else
	      {
		if (!at_beg)
		  {
		    d_before = POS_BEFORE_GAP_UNSAFE (d);
		    DEC_IBYTEPTR_FMT (d_before, fmt);
		    emch1 = itext_ichar_fmt (d_before, fmt, lispobj);
#ifdef emacs
		    BEGIN_REGEX_MALLOC_OK ();
		    UPDATE_SYNTAX_CACHE (scache,
                                         offset_to_bytexpos
                                         (lispobj, PTR_TO_OFFSET (d_before)));
#endif
		    syn1 = SYNTAX_FROM_CACHE (scache, emch1);
		    END_REGEX_MALLOC_OK ();
		  }
		if (!at_end)
		  {
		    d_after = POS_AFTER_GAP_UNSAFE (d);
		    emch2 = itext_ichar_fmt (d_after, fmt, lispobj);
#ifdef emacs
		    BEGIN_REGEX_MALLOC_OK ();
		    UPDATE_SYNTAX_CACHE_FORWARD (scache,
                                                 offset_to_bytexpos
                                                 (lispobj, PTR_TO_OFFSET (d)));
#endif
		    syn2 = SYNTAX_FROM_CACHE (scache, emch2);
		    END_REGEX_MALLOC_OK ();
		  }
		RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();

		if (at_beg)
		  result = (syn2 == Sword);
		else if (at_end)
		  result = (syn1 == Sword);
		else
		  result = ((syn1 == Sword) != (syn2 == Sword));
	      }

	    if (result == should_succeed)
	      break;
	    goto fail;
	  }

	case notwordbound:
          DEBUG_MATCH_PRINT ("EXECUTING notwordbound.\n");
	  should_succeed = 0;
	  goto matchwordbound;

	case wordbeg:
          DEBUG_MATCH_PRINT ("EXECUTING wordbeg.\n");
	  if (AT_STRINGS_END (d))
	    goto fail;
	  {
	    /* XEmacs: this originally read:

	    if (WORDCHAR_P (d) && (AT_STRINGS_BEG (d) || !WORDCHAR_P (d - 1)))
	      break;

	      */
	    re_char *dtmp = POS_AFTER_GAP_UNSAFE (d);
	    Ichar emch = itext_ichar_fmt (dtmp, fmt, lispobj);
	    int tempres;

	    BEGIN_REGEX_MALLOC_OK ();
#ifdef emacs
	    UPDATE_SYNTAX_CACHE
              (scache, 
               offset_to_bytexpos (lispobj, PTR_TO_OFFSET (d)));
#endif
	    tempres = (SYNTAX_FROM_CACHE (scache, emch) != Sword);
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	    if (tempres)
	      goto fail;
	    if (AT_STRINGS_BEG (d))
	      break;
	    dtmp = POS_BEFORE_GAP_UNSAFE (d);
	    DEC_IBYTEPTR_FMT (dtmp, fmt);
	    emch = itext_ichar_fmt (dtmp, fmt, lispobj);
	    BEGIN_REGEX_MALLOC_OK ();
#ifdef emacs
	    UPDATE_SYNTAX_CACHE_BACKWARD
              (scache,
               offset_to_bytexpos (lispobj, PTR_TO_OFFSET (dtmp)));
#endif
	    tempres = (SYNTAX_FROM_CACHE (scache, emch) != Sword);
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	    if (tempres)
	      break;
	    goto fail;
	  }

	case wordend:
          DEBUG_MATCH_PRINT ("EXECUTING wordend.\n");
	  if (AT_STRINGS_BEG (d))
	    goto fail;
	  {
	    /* XEmacs: this originally read:

	    if (!AT_STRINGS_BEG (d) && WORDCHAR_P (d - 1)
		&& (!WORDCHAR_P (d) || AT_STRINGS_END (d)))
	      break;

	      The or condition is incorrect (reversed).
	      */
	    re_char *dtmp;
	    Ichar emch;
	    int tempres;
#ifdef emacs
	    BEGIN_REGEX_MALLOC_OK ();
	    UPDATE_SYNTAX_CACHE
              (scache,
               offset_to_bytexpos (lispobj, PTR_TO_OFFSET (d)));
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
#endif
	    dtmp = POS_BEFORE_GAP_UNSAFE (d);
	    DEC_IBYTEPTR_FMT (dtmp, fmt);
	    emch = itext_ichar_fmt (dtmp, fmt, lispobj);
	    BEGIN_REGEX_MALLOC_OK ();
	    tempres = (SYNTAX_FROM_CACHE (scache, emch) != Sword);
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	    if (tempres)
	      goto fail;
	    if (AT_STRINGS_END (d))
	      break;
	    dtmp = POS_AFTER_GAP_UNSAFE (d);
	    emch = itext_ichar_fmt (dtmp, fmt, lispobj);
	    BEGIN_REGEX_MALLOC_OK ();
#ifdef emacs
            {
              re_char *next = d;
              INC_IBYTEPTR_FMT (next, fmt);
              UPDATE_SYNTAX_CACHE_FORWARD
                (scache,
                 offset_to_bytexpos (lispobj, PTR_TO_OFFSET (next)));
            }
#endif
	    tempres = (SYNTAX_FROM_CACHE (scache, emch) != Sword);
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	    if (tempres)
	      break;
	    goto fail;
	  }

#ifdef emacs
  	case before_dot:
          DEBUG_MATCH_PRINT ("EXECUTING before_dot.\n");
 	  if (!BUFFERP (lispobj)
	      || (BUF_PTR_BYTE_POS (XBUFFER (lispobj), (unsigned char *) d)
		  >= BUF_PT (XBUFFER (lispobj))))
  	    goto fail;
  	  break;

  	case at_dot:
          DEBUG_MATCH_PRINT ("EXECUTING at_dot.\n");
 	  if (!BUFFERP (lispobj)
	      || (BUF_PTR_BYTE_POS (XBUFFER (lispobj), (unsigned char *) d)
		  != BUF_PT (XBUFFER (lispobj))))
  	    goto fail;
  	  break;

  	case after_dot:
          DEBUG_MATCH_PRINT ("EXECUTING after_dot.\n");
 	  if (!BUFFERP (lispobj)
	      || (BUF_PTR_BYTE_POS (XBUFFER (lispobj), (unsigned char *) d)
		  <= BUF_PT (XBUFFER (lispobj))))
  	    goto fail;
  	  break;

	case syntaxspec:
          DEBUG_MATCH_PRINT ("EXECUTING syntaxspec %d.\n", mcnt);
	  mcnt = *p++;
	  goto matchsyntax;

        case wordchar:
          DEBUG_MATCH_PRINT ("EXECUTING Emacs wordchar.\n");
	  mcnt = (int) Sword;
        matchsyntax:
	  should_succeed = 1;
	matchornotsyntax:
	  {
	    int matches;
	    Ichar emch;

	    REGEX_PREFETCH ();
	    BEGIN_REGEX_MALLOC_OK ();
	    UPDATE_SYNTAX_CACHE
              (scache,
               offset_to_bytexpos (lispobj, PTR_TO_OFFSET (d)));
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();

	    emch = itext_ichar_fmt (d, fmt, lispobj);
	    BEGIN_REGEX_MALLOC_OK ();
	    matches = (SYNTAX_FROM_CACHE (scache, emch) ==
		       (enum syntaxcode) mcnt);
	    END_REGEX_MALLOC_OK ();
	    RE_MATCH_RELOCATE_MOVEABLE_DATA_POINTERS ();
	    INC_IBYTEPTR_FMT (d, fmt);
	    if (matches != should_succeed)
	      goto fail;
	  }
	  break;

	case notsyntaxspec:
          DEBUG_MATCH_PRINT ("EXECUTING notsyntaxspec %d.\n", mcnt);
	  mcnt = *p++;
	  goto matchnotsyntax;

        case notwordchar:
          DEBUG_MATCH_PRINT ("EXECUTING Emacs notwordchar.\n");
	  mcnt = (int) Sword;
        matchnotsyntax:
	  should_succeed = 0;
	  goto matchornotsyntax;

/* 97/2/17 jhod Mule category code patch */
	case categoryspec:
	  should_succeed = 1;
        matchornotcategory:
	  {
	    Ichar emch;

	    mcnt = *p++;
	    REGEX_PREFETCH ();
	    emch = itext_ichar_fmt (d, fmt, lispobj);
	    INC_IBYTEPTR_FMT (d, fmt);
	    if (check_char_in_category (emch, BUFFER_CATEGORY_TABLE (lispbuf),
					mcnt, should_succeed))
	      goto fail;
	  }
	  break;

	case notcategoryspec:
	  should_succeed = 0;
	  goto matchornotcategory;
/* end of category patch */
#else /* not emacs */
	case wordchar:
          DEBUG_MATCH_PRINT ("EXECUTING non-Emacs wordchar.\n");
	  REGEX_PREFETCH ();
          if (!WORDCHAR_P ((int) (*d)))
            goto fail;
          d++;
	  break;

	case notwordchar:
          DEBUG_MATCH_PRINT ("EXECUTING non-Emacs notwordchar.\n");
	  REGEX_PREFETCH ();
          if (!WORDCHAR_P ((int) (*d)))
            goto fail;
          d++;
	  break;
#endif /* emacs */

        default:
          ABORT ();
	}
      continue;  /* Successfully executed one pattern command; keep going.  */


    /* We goto here if a matching operation fails. */
    fail:
      if (!FAIL_STACK_EMPTY ())
	{
	  re_char *str;
	  unsigned char *pat;
	  /* A restart point is known.  Restore to that state.  */
	  DEBUG_MATCH_PRINT ("\nFAIL:\n");
	  POP_FAILURE_POINT (str, pat);

	  assert (pat < pend);

	  switch ((re_opcode_t) *pat++)
	    {
	    case on_failure_jump_exclusive:
	    case on_failure_jump_loop:
	    case on_failure_jump:
	    case succeed_n:
	      d = str;
	      EXTRACT_NUMBER_AND_INCR (mcnt, pat);
	      p = pat + mcnt;
	      break;

	    default:
	      abort();
	    }

	  assert (p >= bufp->buffer && p <= pend);

          if (d >= string1 && d <= end1)
	    dend = end_match_1;
        }
      else
        break;   /* Matching at this starting point really fails.  */
    } /* for (;;) */

  if (best_regs_set)
    goto restore_best_regs;

  FREE_VARIABLES ();

  return -1;         			/* Failure to match.  */
} /* re_match_2_internal */

/* Subroutine definitions for re_match_2.  */


/* We are passed P pointing to a register number after a start_memory.

   Return true if the pattern up to the corresponding stop_memory can
   match the empty string, and false otherwise.

   If we find the matching stop_memory, sets P to point to one past its number.
   Otherwise, sets P to an undefined byte less than or equal to END.

   We don't handle duplicates properly (yet).  */

static re_bool
group_match_null_string_p (re_char **p, re_char *end)
{
  int mcnt;
  /* Point to after the args to the start_memory.  */
  re_char *p1 = *p + 2;

  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and return true or
	 false, as appropriate, when we get to one that can't, or to the
         matching stop_memory.  */

      switch ((re_opcode_t) *p1)
        {
        /* Could be either a loop or a series of alternatives.  */
        case on_failure_jump:
          p1++;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);

          /* If the next operation is not a jump backwards in the
	     pattern.  */

	  if (mcnt >= 0)
	    {
              /* Go through the on_failure_jumps of the alternatives,
                 seeing if any of the alternatives cannot match nothing.
                 The last alternative starts with only a jump,
                 whereas the rest start with on_failure_jump and end
                 with a jump, e.g., here is the pattern for `a|b|c':

                 /on_failure_jump/0/6/exactn/1/a/jump_past_alt/0/6
                 /on_failure_jump/0/6/exactn/1/b/jump_past_alt/0/3
                 /exactn/1/c

                 So, we have to first go through the first (n-1)
                 alternatives and then deal with the last one separately.  */


              /* Deal with the first (n-1) alternatives, which start
                 with an on_failure_jump (see above) that jumps to right
                 past a jump_past_alt.  */

              while ((re_opcode_t) p1[mcnt-3] == jump_past_alt)
                {
                  /* `mcnt' holds how many bytes long the alternative
                     is, including the ending `jump_past_alt' and
                     its number.  */

		  if (!alt_match_null_string_p (p1, p1 + mcnt - 3))
                    return false;

                  /* Move to right after this alternative, including the
		     jump_past_alt.  */
                  p1 += mcnt;

                  /* Break if it's the beginning of an n-th alternative
                     that doesn't begin with an on_failure_jump.  */
                  if ((re_opcode_t) *p1 != on_failure_jump)
                    break;

		  /* Still have to check that it's not an n-th
		     alternative that starts with an on_failure_jump.  */
		  p1++;
                  EXTRACT_NUMBER_AND_INCR (mcnt, p1);
                  if ((re_opcode_t) p1[mcnt-3] != jump_past_alt)
                    {
		      /* Get to the beginning of the n-th alternative.  */
                      p1 -= 3;
                      break;
                    }
                }

              /* Deal with the last alternative: go back and get number
                 of the `jump_past_alt' just before it.  `mcnt' contains
                 the length of the alternative.  */
              EXTRACT_NUMBER (mcnt, p1 - 2);

              if (!alt_match_null_string_p (p1, p1 + mcnt))
                return false;

              p1 += mcnt;	/* Get past the n-th alternative.  */
            } /* if mcnt > 0 */
          break;


        case stop_memory:
	  assert (extract_nonnegative (p1 + 1) == extract_nonnegative (*p));
          *p = p1 + 2;
          return true;


        default:
          if (!common_op_match_null_string_p (&p1, end))
            return false;
        }
    } /* while p1 < end */

  return false;
} /* group_match_null_string_p */


/* Similar to group_match_null_string_p, but doesn't deal with alternatives:
   It expects P to be the first byte of a single alternative and END one
   byte past the last. The alternative can contain groups.  */

static re_bool
alt_match_null_string_p (re_char *p, re_char *end)
{
  int mcnt;
  re_char *p1 = p;

  while (p1 < end)
    {
      /* Skip over opcodes that can match nothing, and break when we get
         to one that can't.  */

      switch ((re_opcode_t) *p1)
        {
	/* It's a loop.  */
        case on_failure_jump:
          p1++;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);
          p1 += mcnt;
          break;

	default:
          if (!common_op_match_null_string_p (&p1, end))
            return false;
        }
    }  /* while p1 < end */

  return true;
} /* alt_match_null_string_p */


/* Deals with the ops common to group_match_null_string_p and
   alt_match_null_string_p.

   Sets P to one after the op and its arguments, if any.  */

static re_bool
common_op_match_null_string_p (re_char **p, re_char *end)
{
  int mcnt;
  re_bool ret;
  regnum_t reg_no;
  re_char *p1 = *p;

  switch ((re_opcode_t) *p1++)
    {
    case no_op:
    case begline:
    case endline:
    case begbuf:
    case endbuf:
    case wordbeg:
    case wordend:
    case wordbound:
    case notwordbound:
#ifdef emacs
    case before_dot:
    case at_dot:
    case after_dot:
#endif
      break;

    case start_memory:
      EXTRACT_NONNEGATIVE_AND_INCR (reg_no, p1);
      assert (reg_no > 0 && reg_no <= MAX_REGNUM);
      ret = group_match_null_string_p (&p1, end);

      USED (reg_no);
#if 0
      /* #### Fix this once we use this code at regexp compile time. */
      /* Have to set this here in case we're checking a group which
         contains a group and a back reference to it. */

      if (REG_MATCH_NULL_STRING_P (reg_info[reg_no]) == MATCH_NULL_UNSET_VALUE)
        REG_MATCH_NULL_STRING_P (reg_info[reg_no]) = ret;

#endif

      if (!ret)
        return false;
      break;

    /* If this is an optimized succeed_n for zero times, make the jump.  */
    case jump:
      EXTRACT_NUMBER_AND_INCR (mcnt, p1);
      if (mcnt >= 0)
        p1 += mcnt;
      else
        return false;
      break;

    case succeed_n:
      /* Get to the number of times to succeed.  */
      p1 += 2;
      EXTRACT_NUMBER_AND_INCR (mcnt, p1);

      if (mcnt == 0)
        {
          p1 -= 4;
          EXTRACT_NUMBER_AND_INCR (mcnt, p1);
          p1 += mcnt;
        }
      else
        return false;
      break;

    case duplicate:
#if 0
      /* #### Fix this once we use this code at regexp compile time. */
      if (!REG_MATCH_NULL_STRING_P (reg_info[*p1]))
        return false;
#endif
      break;

    case set_number_at:
      p1 += 4;

    default:
      /* All other opcodes mean we cannot match the empty string.  */
      return false;
  }

  *p = p1;
  return true;
} /* common_op_match_null_string_p */


/* Return zero if TRANSLATE[S1] and TRANSLATE[S2] are identical for LEN
   bytes; nonzero otherwise.  */

static int
bcmp_translate (re_char *s1, re_char *s2,
		REGISTER int len, RE_TRANSLATE_TYPE translate
#ifdef emacs
		, Internal_Format fmt, Lisp_Object lispobj
#endif
		)
{
  REGISTER re_char *p1 = s1, *p2 = s2;
#ifdef emacs
  re_char *p1_end = s1 + len;
  re_char *p2_end = s2 + len;

  while (p1 != p1_end && p2 != p2_end)
    {
      Ichar p1_ch, p2_ch;

      p1_ch = itext_ichar_fmt (p1, fmt, lispobj);
      p2_ch = itext_ichar_fmt (p2, fmt, lispobj);

      if (RE_TRANSLATE_1 (p1_ch)
	  != RE_TRANSLATE_1 (p2_ch))
	return 1;
      INC_IBYTEPTR_FMT (p1, fmt);
      INC_IBYTEPTR_FMT (p2, fmt);
    }
#else /* not emacs */
  while (len)
    {
      if (RE_TRANSLATE_1 (*p1++) != RE_TRANSLATE_1 (*p2++)) return 1;
      len--;
    }
#endif /* emacs */
  return 0;
}

/* Entry points for GNU code.  */

/* re_compile_pattern is the GNU regular expression compiler: it
   compiles PATTERN (of length SIZE) and puts the result in BUFP.
   Returns 0 if the pattern was valid, otherwise an error string.

   Assumes the `allocated' (and perhaps `buffer') and `translate' fields
   are set in BUFP on entry.

   We call regex_compile to do the actual compilation.  */

const char *
re_compile_pattern (const char *pattern, int length,
		    struct re_pattern_buffer *bufp)
{
  reg_errcode_t ret;

  /* GNU code is written to assume at least RE_NREGS registers will be set
     (and at least one extra will be -1).  */
  bufp->regs_allocated = REGS_UNALLOCATED;

  /* And GNU code determines whether or not to get register information
     by passing null for the REGS argument to re_match, etc., not by
     setting no_sub.  */
  bufp->no_sub = 0;

  /* Match anchors at newline.  */
  bufp->newline_anchor = 1;

  ret = regex_compile ((unsigned char *) pattern, length, re_syntax_options,
		       bufp);

  if (!ret)
    return NULL;
  return gettext (re_error_msgid[(int) ret]);
}

/* regex.c ends here. */
