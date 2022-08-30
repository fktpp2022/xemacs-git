/* String search routines for XEmacs.
   Copyright (C) 1985, 1986, 1987, 1992-1995 Free Software Foundation, Inc.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 2001, 2002, 2005, 2010 Ben Wing.

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

/* Synched up with: FSF 19.29, except for region-cache stuff. */

/* Hacked on for Mule by Ben Wing, December 1994 and August 1995. */

/* This file has been Mule-ized. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "casetab.h"
#include "insdel.h"
#include "opaque.h"
#include "regex.h"
#ifdef REGION_CACHE_NEEDS_WORK
#include "region-cache.h"
#endif
#include "syntax.h"
#include "extents.h"
#include "extents-impl.h"

#define TRANSLATE(table, pos)	\
 (!NILP (table) ? TRT_TABLE_OF (table, (Ichar) pos) : pos)

#define REGEXP_CACHE_SIZE 20

#ifdef DEBUG_XEMACS

/* Used in tests/automated/case-tests.el if available. */
Fixnum debug_searches;

/* Declare as int rather than Bitflags because it's used by regex.c, which
   may be used outside of XEmacs (e.g. etags.c). */
int debug_regexps;
Lisp_Object Vdebug_regexps;

Lisp_Object Qsearch_algorithm_used, Qboyer_moore, Qsimple_search;

Lisp_Object Qcompilation, Qfailure_point, Qmatching;
#endif

Boolint search_error_on_bad_match_data;

/* If the regexp is non-nil, then the buffer contains the compiled form
   of that regexp, suitable for searching.  */
struct regexp_cache
{
  struct regexp_cache *next;
  Lisp_Object regexp;
  struct re_pattern_buffer buf;
  char fastmap[256];
  /* Nonzero means regexp was compiled to do full POSIX backtracking.  */
  char posix;
};

/* The instances of that struct.  */
static struct regexp_cache searchbufs[REGEXP_CACHE_SIZE];

/* The head of the linked list; points to the most recently used buffer.  */
static struct regexp_cache *searchbuf_head;


/* Every call to re_match, etc., must pass &search_regs as the regs
   argument unless you can show it is unnecessary (i.e., if re_match
   is certainly going to be called again before region-around-match
   can be called).

   Since the registers are now dynamically allocated, we need to make
   sure not to refer to the Nth register before checking that it has
   been allocated by checking search_regs.num_regs.

   The regex code keeps track of whether it has allocated the search
   buffer using bits in the re_pattern_buffer.  This means that whenever
   you compile a new pattern, it completely forgets whether it has
   allocated any registers, and will allocate new registers the next
   time you call a searching or matching function.  Therefore, we need
   to call re_set_registers after compiling a new pattern or after
   setting the match registers, so that the regex functions will be
   able to free or re-allocate it properly.  */
static struct re_registers search_regs;

/* A cons of a fixnum (corresponding to search.regs.NUM_REGS) and a Lisp
   vector of extents, or Qnil. These reflect the entries in START and END of
   search_regs as of the last search, allow lazy conversion of Bytebpos to
   Charbpos values, and allow (match-string NUM) to give sane results even if
   the last item searched for was a string (though we don't actually do that
   currently for compatibility reasons; we do give a warning). Only
   initialized after a successful match, which does not need to be a
   successful regexp match.  */
static Lisp_Object Vsearch_registers;

/* An uninterned symbol used by Freplace_match () to communicate where \U, \E,
   \l, etc, were encountered to its output. */
static Lisp_Object Vcase_flag_symbol;

/* Every function that sets the match data _must_ clear unused search
   registers on success.  An unsuccessful search or match _must_ preserve
   the search registers.  The traditional documentation implied that
   any match operation might trash the registers, but in fact failures
   have always preserved the match data (in GNU Emacs as well).  Some
   plausible code depends on this behavior (cf. `w3-configuration-data'
   in library "w3-cfg").

   The ordinary string matches use the zeroth element of the match data in the
   same way that the regexp matches do.

   This function assumes that SEARCH_REGSP reflects a struct re_registers
   corresponding to the most recent regexp or non-regexp search, and that
   SEARCH_OBJ is a string or buffer that was searched in that operation. It
   clears unused search registers, and it updates Vsearch_registers
   accordingly, see the description of that variable.

   This should be called after both regexp and non-regexp successful searches,
   though with non-regexp searches search_regs.{start,end}[0] will need to be
   explicitly initialized. */
static void set_lisp_search_registers (Lisp_Object search_obj,
                                       const struct re_registers *
                                       search_regsp);

/* error condition signalled when regexp compile_pattern fails */

Lisp_Object Qinvalid_regexp;

/* Regular expressions used in forward/backward-word */
Lisp_Object Vforward_word_regexp, Vbackward_word_regexp;

Fixnum warn_about_possibly_incompatible_back_references;

/* range table for use with skip_chars.  Only needed for Mule. */
Lisp_Object Vskip_chars_range_table;

static Bytebpos simple_search (struct buffer *buf, Ibyte *base_pat,
			       Bytecount len, Bytebpos pos, Bytebpos lim,
			       EMACS_INT n, Lisp_Object trt);
static Bytebpos boyer_moore (struct buffer *buf, Ibyte *base_pat,
			     Bytecount len, Bytebpos pos, Bytebpos lim,
			     EMACS_INT n, Lisp_Object trt,
			     Lisp_Object inverse_trt, Ibyte *char_base,
                             int char_base_len);
static Bytebpos search_buffer (struct buffer *buf, Lisp_Object str,
			       Bytebpos pos, Bytebpos lim, EMACS_INT n,
			       Boolint RE, Lisp_Object trt,
			       Lisp_Object inverse_trt, Boolint posix);

static DECLARE_DOESNT_RETURN (matcher_overflow (void));

static DOESNT_RETURN
matcher_overflow ()
{
  stack_overflow ("Stack overflow in regexp matcher", Qunbound);
}

static void
search_warn_or_error (Lisp_Object level, const CIbyte *fmt, ...)
                              
{
  va_list va;
  Lisp_Object obj;

  va_start (va, fmt);
  obj = emacs_vsprintf_string_lisp (fmt, va);
  va_end (va);

  if (search_error_on_bad_match_data)
    {
      signal_error_1 (Qinvalid_state, list1 (obj));
    }
  else
    {
      warn_when_safe_lispobj (Qsearch, level, obj);
    }
}

/* Compile a regexp and signal a Lisp error if anything goes wrong.
   PATTERN is the pattern to compile.
   CP is the place to put the result.
   TRANSLATE is a translation table for ignoring case, or Qnil for none.
   REGP is the structure that says where to store the "register"
   values that will result from matching this pattern.
   If it is 0, we should compile the pattern not to record any
   subexpression bounds.
   POSIX is nonzero if we want full backtracking (POSIX style)
   for this pattern.  0 means backtrack only enough to get a valid match.  */

static int
compile_pattern_1 (struct regexp_cache *cp, Lisp_Object pattern,
		   struct re_registers *UNUSED (regp), Lisp_Object translate,
		   int posix, Error_Behavior errb)
{
  const char *val;
  reg_syntax_t old;

  cp->regexp = Qnil;
  cp->buf.translate = translate;
  cp->posix = posix;
  old = re_set_syntax (RE_SYNTAX_EMACS
		       | (posix ? 0 : RE_NO_POSIX_BACKTRACKING));
  val = (const char *)
    re_compile_pattern ((char *) XSTRING_DATA (pattern),
			XSTRING_LENGTH (pattern), &cp->buf);
  re_set_syntax (old);
  if (val)
    {
      maybe_signal_error (Qinvalid_regexp, 0, build_cistring (val),
			  Qsearch, errb);
      return 0;
    }

  cp->regexp
    = make_string (XSTRING_DATA (pattern), XSTRING_LENGTH (pattern));
  return 1;
}

/* Compile a regexp if necessary, but first check to see if there's one in
   the cache.
   PATTERN is the pattern to compile.
   TRANSLATE is a translation table for ignoring case, or Qnil for none.
   REGP is the structure that says where to store the "register"
   values that will result from matching this pattern.
   If it is 0, we should compile the pattern not to record any
   subexpression bounds.
   POSIX is nonzero if we want full backtracking (POSIX style)
   for this pattern.  0 means backtrack only enough to get a valid match.  */

struct re_pattern_buffer *
compile_pattern (Lisp_Object pattern, struct re_registers *regp,
		 Lisp_Object translate, Lisp_Object UNUSED (searchobj),
		 struct buffer *UNUSED (searchbuf), int posix,
		 Error_Behavior errb)
{
  struct regexp_cache *cp, **cpp;

  for (cpp = &searchbuf_head; ; cpp = &cp->next)
    {
      cp = *cpp;
      /* &&#### once we fix up the fastmap code in regex.c for 8-bit-fixed,
         we need to record and compare the buffer and format, since the
         fastmap will reflect the state of the buffer -- and things get
         more complicated if the buffer has changed formats or (esp.) has
         kept the format but changed its interpretation!  may need to have
         the code that changes the interpretation go through and invalidate
         cache entries for that buffer. */
      if (cp->posix == posix && EQ (cp->buf.translate, translate)
          && internal_equal (cp->regexp, pattern, 0))
	break;

      /* If we're at the end of the cache, compile into the last cell.  */
      if (cp->next == 0)
	{
	  if (!compile_pattern_1 (cp, pattern, regp, translate,
				  posix, errb))
	    return 0;
	  break;
	}
    }

  /* When we get here, cp (aka *cpp) contains the compiled pattern,
     either because we found it in the cache or because we just compiled it.
     Move it to the front of the queue to mark it as most recently used.  */
  *cpp = cp->next;
  cp->next = searchbuf_head;
  searchbuf_head = cp;

  /* Advise the searching functions about the space we have allocated
     for register data.  */
  if (regp)
    re_set_registers (&cp->buf, regp, regp->num_regs, regp->start, regp->end);

  return &cp->buf;
}

/* Error condition used for failing searches */
Lisp_Object Qsearch_failed;

static DECLARE_DOESNT_RETURN (signal_failure (Lisp_Object));

static DOESNT_RETURN
signal_failure (Lisp_Object arg)
{
  for (;;)
    Fsignal (Qsearch_failed, list1 (arg));
}

/* We can normally lazily reuse our saved extents, but extents attached to
   strings are not currently dumpable. Vstrings_to_nuke_extents is, at dump
   time, a weak list of those strings having extent info that the search code
   has encountered. Just before dumping, loadup.el tells #'store-match-data
   that all extent info for objects on this list should be cleared using
   uninit_object_extents(). Once that is done (and thus, also for normal, post
   pdump_load() executions) Vstrings_to_nuke_extents becomes Qnil.

   clear_lisp_search_registers used to eagerly uninit_object_extents() on each
   dump-time call, but that interacts badly with #'save-match-data. */
static Lisp_Object Vstrings_to_nuke_extents;

static void
clear_lisp_search_registers (void)
{
  Elemcount ii;

  for (ii = 0; ii < XVECTOR_LENGTH (XCDR (Vsearch_registers)); ii++)
    {
      if (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[ii]))
        {
          Lisp_Object obj
            = extent_object (XEXTENT (XVECTOR_DATA
                                      (XCDR (Vsearch_registers))[ii]));
          if (purify_flag && STRINGP (obj))
            {
	      XWEAK_LIST_LIST (Vstrings_to_nuke_extents)
		= Fcons (obj, XWEAK_LIST_LIST (Vstrings_to_nuke_extents));
	      Fdetach_extent (XVECTOR_DATA (XCDR (Vsearch_registers))[ii]);
              XVECTOR_DATA (XCDR (Vsearch_registers))[ii] = Qnil;
            }
          else if (EQ (obj, XSYMBOL_NAME (Qsearch))
                   && extent_detached_p
                   (XEXTENT (XVECTOR_DATA
                             (XCDR (Vsearch_registers))[ii])))
            {
              DO_NOTHING; /* Usual case for our lazily-reused extents, no need
                             to do anything. */
            }
          else
            {
              set_extent_endpoints (XEXTENT (XVECTOR_DATA
                                             (XCDR (Vsearch_registers))
                                             [ii]),
                                    0, 0, XSYMBOL_NAME (Qsearch));
              Fdetach_extent (XVECTOR_DATA (XCDR (Vsearch_registers)) [ii]);
              if (ii < 1)
                {
                  Fput (XVECTOR_DATA (XCDR (Vsearch_registers)) [ii],
                        Qcontext, Qunbound);
                }
            }
        }
      else
        {
          XVECTOR_DATA (XCDR (Vsearch_registers))[ii] = Qnil;          
        }
    }
}

static void
set_lisp_search_registers (Lisp_Object search_obj,
                           const struct re_registers *search_regsp)
{
  Elemcount num_regs = search_regsp->num_regs, num_regs_out = -1;
  Lisp_Object registers_vector = XCDR (Vsearch_registers);
  Boolint bufferp = BUFFERP (search_obj), nonnil_seen = 0;

  clear_lisp_search_registers ();

  while (num_regs > 0)
    {
      num_regs--;
      structure_checking_assert (search_regsp->start[num_regs] >= bufferp ?
                                 (search_regsp->end[num_regs] >= bufferp)
                                 : (search_regsp->end[num_regs] < 0));

      if (search_regsp->start[num_regs] >= bufferp)
        {
          if (!nonnil_seen)
            {
              num_regs_out = num_regs + 1;
              nonnil_seen = 1;

              if (num_regs_out > XVECTOR_LENGTH (registers_vector))
                {
                  Lisp_Object new_registers_vector
                    = Fmake_vector (make_fixnum (num_regs_out * 2), Qnil);
                  memcpy (XVECTOR_DATA (new_registers_vector),
                          XVECTOR_DATA (registers_vector),
                          sizeof (Lisp_Object) *
                          XVECTOR_LENGTH (registers_vector));
                  XSETCDR (Vsearch_registers, new_registers_vector);
                  registers_vector = XCDR (Vsearch_registers);
                }
            }

          if (!EXTENTP (XVECTOR_DATA (registers_vector)[num_regs]))
            {
              XVECTOR_DATA (registers_vector)[num_regs]
                = Fmake_extent (Qnil, Qnil, search_obj);
	      set_extent_start_open_p (XEXTENT (XVECTOR_DATA
						(registers_vector)[num_regs]),
				       1);
            }

          set_extent_endpoints (XEXTENT (XVECTOR_DATA (registers_vector)
                                         [num_regs]),
                                search_regsp->start[num_regs],
                                search_regsp->end[num_regs], search_obj);
        }
    }

  if (!bufferp)
    {
      /* Both the regexp and the non-regexp code need to supply (match-string
         0), otherwise this wasn't a successful call, and
         set_lisp_search_registers() shouldn't have been called at all. */
      structure_checking_assert (search_regsp->start[0] >= 0);
      /* Tell Freplace_match what buffer to use for the case, syntax tables if
         needed. Yes, I have thought about the performance impact of this, and
         it doesn't matter; while Fset_extent_property is one of the more
         expensive of the object plist methods, it's still plenty fast, and
         this won't be hit for buffer matches, which are the most costly
         (given the, in general, larger search size of the text searched). */
      Fset_extent_property (XVECTOR_DATA (registers_vector) [0], Qcontext,
                            wrap_buffer (current_buffer));
    }

  XSETCAR (Vsearch_registers, make_fixnum (num_regs_out));
}

static void
canonicalize_lisp_search_registers_for_replace (Lisp_Object match_obj)
{
  Elemcount jj = 0, num_regs = XFIXNUM (XCAR (Vsearch_registers));
  Lisp_Object registers = XCDR (Vsearch_registers);

  while (jj < num_regs)
    {
      if (EXTENTP (XVECTOR_DATA (registers)[jj])
          && !extent_detached_p (XEXTENT (XVECTOR_DATA (registers)[jj])))
        {
          if (!EQ (extent_object (XEXTENT (XVECTOR_DATA (registers)[jj])),
                   match_obj))
            {
              if (STRINGP (match_obj) && 
                  internal_equal (match_obj,
                                  extent_object
                                  (XEXTENT (XVECTOR_DATA (registers)[jj])),
                                  0))
                {
                  DO_NOTHING;
                }
              else
                {
                  search_warn_or_error (Qerror,
                                        "Likely bug: #'replace-match asked to"
                                        " operate on %S, match data reflect "
                                        "%S, which is distinct", match_obj,
                                        extent_object (XEXTENT
                                                       (XVECTOR_DATA
                                                        (registers)[jj])));
                }

              /* This is brutal from a byte-char perspective, but will be
                 rare, and is bug-compatible with the old code. */
              Fset_extent_endpoints
                (XVECTOR_DATA (registers)[jj],
                 Fextent_start_position (XVECTOR_DATA (registers)[jj], Qnil),
                 Fextent_end_position (XVECTOR_DATA (registers)[jj], Qnil),
                 match_obj);
            }
        }
      else if (CONSP (XVECTOR_DATA (registers)[jj]))
        {
          XVECTOR_DATA (registers)[jj]
            /* Canonicalize the fixnum char positions values to extents, and
               error if they are out of range for this match object. */
            = Fmake_extent (XCAR (XVECTOR_DATA (registers)[jj]),
                            XCDR (XVECTOR_DATA (registers)[jj]),
                            match_obj);
          set_extent_start_open_p (XEXTENT (XVECTOR_DATA
                                            (registers)[jj]),
                                   1);
        }
      jj++;
    }
}

/* Discard those internal extents not currently used prior to GC.  Don't
   replace XCDR (Vsearch_registers) with a smaller vector, it is unlikely to
   ever get larger than about 512 and its's not a frob block object anyway,
   less likely to be reused. If the number of registers currently in use is
   less than 16, leave the lower 16 registers as they are, they are more
   likely to be regularly reused than are registers 17-512. */
void
flush_unused_lisp_search_registers (void)
{
  Elemcount ii = XVECTOR_LENGTH (XCDR (Vsearch_registers));
  Elemcount num_regs = XFIXNUM (XCAR (Vsearch_registers));

  num_regs = max (16, num_regs);

  while (--ii >= num_regs)
    {
      if (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[ii]))
        {
          structure_checking_assert
            (extent_detached_p
             (XEXTENT (XVECTOR_DATA (XCDR (Vsearch_registers))[ii])));
          Fdelete_extent (XVECTOR_DATA (XCDR (Vsearch_registers))[ii]);
          XVECTOR_DATA (XCDR (Vsearch_registers))[ii] = Qnil;
        }
    }
}

static Lisp_Object
looking_at_1 (Lisp_Object string, struct buffer *buf, Boolint posix,
	      Boolint nodata)
{
  Lisp_Object val;
  Bytebpos p1, p2;
  Bytecount s1, s2;
  REGISTER int i;
  struct re_pattern_buffer *bufp;
  struct syntax_cache scache_struct;
  struct syntax_cache *scache = &scache_struct;
  
  CHECK_STRING (string);
  bufp = compile_pattern (string, &search_regs,
			  (!NILP (buf->case_fold_search)
			   ? XCASE_TABLE_DOWNCASE (buf->case_table) : Qnil),
			  wrap_buffer (buf), buf, posix, ERROR_ME);

  QUIT;

  /* Get pointers and sizes of the two strings
     that make up the visible portion of the buffer. */

  p1 = BYTE_BUF_BEGV (buf);
  p2 = BYTE_BUF_CEILING_OF (buf, p1);
  s1 = p2 - p1;
  s2 = BYTE_BUF_ZV (buf) - p2;

  /* By making the regex object, regex buffer, and syntax cache arguments to
     re_{search,match}{,_2}, and by having re_match_2_internal() work on
     (and modify) its own copy of the cached compiled pattern, we've removed
     the need to do nasty things to deal with regex reentrancy. */
  i = re_match_2 (bufp, (char *) BYTE_BUF_BYTE_ADDRESS (buf, p1),
		  s1, (char *) BYTE_BUF_BYTE_ADDRESS (buf, p2), s2,
		  BYTE_BUF_PT (buf) - BYTE_BUF_BEGV (buf),
		  nodata ? NULL : &search_regs,
		  BYTE_BUF_ZV (buf) - BYTE_BUF_BEGV (buf), wrap_buffer (buf),
		  buf, scache);

  if (i == -2)
    matcher_overflow ();

  val = (0 <= i ? Qt : Qnil);

  if (nodata || NILP (val))
    {
      return val;
    }
  else
    {
      int num_regs = search_regs.num_regs;
      for (i = 0; i < num_regs; i++)
	if (search_regs.start[i] >= 0)
	  {
	    search_regs.start[i] += BYTE_BUF_BEGV (buf);
	    search_regs.end[i] += BYTE_BUF_BEGV (buf);
	  }
    }
  
  set_lisp_search_registers (wrap_buffer (buf), &search_regs);
  return val;
}

DEFUN ("looking-at", Flooking_at, 1, 2, 0, /*
Return t if text after point matches regular expression REGEXP.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails,
the match data from the previous successful match are preserved.  If you have
no need for the match data, call `looking-at-p' instead, which always
preserves the match data.  See also `save-match-data'.

Optional argument BUFFER defaults to the current buffer.
*/
       (regexp, buffer))
{
  return looking_at_1 (regexp, decode_buffer (buffer, 0), 0, 0);
}

DEFUN ("posix-looking-at", Fposix_looking_at, 1, 2, 0, /*
Return t if text after point matches regular expression REGEXP.

Find the longest match, in accordance with POSIX regular expression rules.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved.  See
`save-match-data'.

The function `looking-at-p' does not modify the match data. Behavior regarding
the longest match is not relevant to it, and so it is an alternative to the
use of `save-match-data' for those contexts where the match data is not
necessary.

Optional argument BUFFER defaults to the current buffer.
*/
       (regexp, buffer))
{
  return looking_at_1 (regexp, decode_buffer (buffer, 0), 1, 0);
}

DEFUN ("looking-at-p", Flooking_at_p, 1, 2, 0, /*
Return t if text after point matches regular expression REGEXP.

This differs from `looking-at' in that it does not modify the match
data on success.  This has the advantage that calls to `looking-at-p'
have fewer side effects, are easier to reason about, and are less
likely to provoke problems with other code when that other code has
not been written with the consideration of modification of the match
data at the forefront of the programmer's mind.  Neither function
modifies the match data on failure.

Optional argument BUFFER defaults to the current buffer.
*/
       (regexp, buffer))
{
  return looking_at_1 (regexp, decode_buffer (buffer, 0), 0, 1);
}

static Lisp_Object
string_match_1 (Lisp_Object regexp, Lisp_Object string, Lisp_Object start,
		struct buffer *buf, Boolint posix)
{
  Bytecount val, bis;
  struct re_pattern_buffer *bufp;

  /* Some FSF junk with running_asynch_code, to preserve the match
     data.  Not necessary because we don't call process filters
     asynchronously (i.e. from within QUIT). */

  CHECK_STRING (regexp);
  CHECK_STRING (string);

  if (NILP (start))
    bis = 0;
  else
    {
      bis = get_string_pos_byte (string, start, GB_NEGATIVE_FROM_END);
    }

  bufp = compile_pattern (regexp, &search_regs,
			  (!NILP (buf->case_fold_search)
			   ? XCASE_TABLE_DOWNCASE (buf->case_table) : Qnil),
			  string, buf, posix, ERROR_ME);
  QUIT;
  {
    struct syntax_cache scache_struct;
    struct syntax_cache *scache = &scache_struct;
  
    /* By making the regex object, regex buffer, and syntax cache arguments
       to re_{search,match}{,_2}, and by having re_match_2_internal() work
       on (and modify) its own copy of the cached compiled pattern, we've
       removed the need to do nasty things to deal with regex reentrancy. */
    val = re_search (bufp, (char *) XSTRING_DATA (string),
		     XSTRING_LENGTH (string), bis,
		     XSTRING_LENGTH (string) - bis,
		     &search_regs, string, buf, scache);
  }
  if (val == -2)
    matcher_overflow ();
  if (val < 0) return Qnil;

  set_lisp_search_registers (string, &search_regs);
  return make_fixnum (string_index_byte_to_char (string, val));
}

DEFUN ("string-match", Fstring_match, 2, 4, 0, /*
Return index of start of first match for REGEXP in STRING, or nil.
If third arg START is non-nil, start search at that index in STRING.
For index of first char beyond the match, do (match-end 0).
`match-end' and `match-beginning' also give indices of substrings
matched by parenthesis constructs in the pattern.

Optional arg BUFFER controls how case folding and syntax and category
lookup is done (according to the value of `case-fold-search' in that buffer
and that buffer's case tables, syntax tables, and category table).  If nil
or unspecified, it defaults *NOT* to the current buffer but instead:

-- the value of `case-fold-search' in the current buffer is still respected
   because of idioms like

      (let ((case-fold-search nil))
         (string-match "^foo.*bar" string))

   but the case, syntax, and category tables come from the standard tables,
   which are accessed through functions `default-{case,syntax,category}-table'
   and serve as the parents of the tables in particular buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails,
the match data from the previous successful match are preserved.  If you have
no need for the match data, call `string-match-p' instead, which always
preserves the match data.  See also `save-match-data'.
*/
       (regexp, string, start, buffer))
{
/* &&#### implement new interp for buffer arg; check code to see if it
   makes more sense than prev */
  return string_match_1 (regexp, string, start, decode_buffer (buffer, 0), 0);
}

DEFUN ("posix-string-match", Fposix_string_match, 2, 4, 0, /*
Return index of start of first match for REGEXP in STRING, or nil.

Find the longest match, in accordance with POSIX regular expression rules.

If third arg START is non-nil, start search at that index in STRING.
For index of first char beyond the match, do (match-end 0).
`match-end' and `match-beginning' also give indices of substrings
matched by parenthesis constructs in the pattern.

Optional arg BUFFER controls how case folding is done (according to
the value of `case-fold-search' in that buffer and that buffer's case
tables) and defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.

The function `string-match-p', which does not modify the match data and does
not differ from `posix-string-match' otherwise (since the distinction between
the longest match and the first match is not available to callers), is an
alternative to the use of `save-match-data' for those contexts where use of
the match data is not necessary.
*/
       (regexp, string, start, buffer))
{
  return string_match_1 (regexp, string, start, decode_buffer (buffer, 0), 1);
}

/* Match REGEXP against RELOC, searching that substring of RELOC
   starting at OFFSET, and return the index of the match, or negative
   on failure.  This does not clobber the match data.  If RELOC is
   Qnil, the text to be examined is taken to be at NONRELOC, an Ibyte
   pointer. */
Bytecount
fast_string_match (Lisp_Object regexp, const Ibyte *nonreloc,
		   Lisp_Object reloc, Bytecount offset,
		   Bytecount length, Boolint case_fold_search,
		   Error_Behavior errb, Boolint no_quit)
{
  Bytecount val;
  Ibyte *newnonreloc = (Ibyte *) nonreloc;
  struct re_pattern_buffer *bufp;
  struct syntax_cache scache_struct;
  struct syntax_cache *scache = &scache_struct;

  bufp = compile_pattern (regexp, 0,
			  (case_fold_search
			   ? XCASE_TABLE_DOWNCASE (Vstandard_case_table)
			   : Qnil),
			  reloc, 0, 0, errb);
  if (!bufp)
    return -1; /* will only do this when errb != ERROR_ME */
  if (!no_quit)
    QUIT;
  else
    no_quit_in_re_search = 1;

  fixup_internal_substring (nonreloc, reloc, offset, &length);

  /* Don't need to protect against GC inside of re_search() due to QUIT;
     QUIT is GC-inhibited. */
  if (!NILP (reloc))
    newnonreloc = XSTRING_DATA (reloc);

  /* By making the regex object, regex buffer, and syntax cache arguments to
     re_{search,match}{,_2}, and by having re_match_2_internal() work on
     (and modify) its own copy of the cached compiled pattern, we've removed
     the need to do nasty things to deal with regex reentrancy. */
  val = re_search (bufp, (char *) newnonreloc + offset, length, 0,
		   length, 0, reloc, 0, scache);

  no_quit_in_re_search = 0;
  return val;
}

Bytecount
fast_lisp_string_match (Lisp_Object regex, Lisp_Object string)
{
  return fast_string_match (regex, 0, string, 0, -1, 0, ERROR_ME, 0);
}

DEFUN ("string-match-p", Fstring_match_p, 2, 4, 0, /*
Return index of start of first match for REGEXP in STRING, or nil.

If third arg START is non-nil, start search at that index in STRING.

Optional arg BUFFER controls how case folding and syntax and category
lookup is done (according to the value of `case-fold-search' in that buffer
and that buffer's case tables, syntax tables, and category table).

This differs from `string-match' in that it does not modify the match
data on success.  This has the advantage that calls to
`string-match-p' have fewer side effects, are easier to reason about,
and are less likely to provoke problems with other code when that
other code has not been written with the consideration of modification
of the match data at the forefront of the programmer's mind.

This function should be also used in those contexts where the programmer would
have used `posix-string-match' but prefers not to have the match data
affected.  There will be no difference in the return value (the start of the
longest match will also be the start of the first match).

Neither function modifies the match data on failure.
*/
       (regexp, string, start, buffer))
{
  Bytecount result, bstart;
  struct buffer *bufp;

  CHECK_STRING (regexp);
  CHECK_STRING (string);
  bstart = (NILP (start)) ? 0 : get_string_pos_byte (string, start,
                                                     GB_NEGATIVE_FROM_END);
  bufp = decode_buffer (buffer, 0);
  result = fast_string_match (regexp, NULL, string, bstart,
                              XSTRING_LENGTH (string) - bstart,
                              !NILP (bufp->case_fold_search), 
                              ERROR_ME, 0);
  return result < 0 ? Qnil
    : make_fixnum (string_index_byte_to_char (string, result + bstart));
}


#ifdef REGION_CACHE_NEEDS_WORK
/* The newline cache: remembering which sections of text have no newlines.  */

/* If the user has requested newline caching, make sure it's on.
   Otherwise, make sure it's off.
   This is our cheezy way of associating an action with the change of
   state of a buffer-local variable.  */
static void
newline_cache_on_off (struct buffer *buf)
{
  if (NILP (buf->cache_long_line_scans))
    {
      /* It should be off.  */
      if (buf->newline_cache)
        {
          free_region_cache (buf->newline_cache);
          buf->newline_cache = 0;
        }
    }
  else
    {
      /* It should be on.  */
      if (buf->newline_cache == 0)
        buf->newline_cache = new_region_cache ();
    }
}
#endif

/* Search in BUF for COUNT instances of the character TARGET between
   START and END.

   If COUNT is positive, search forwards; END must be >= START.
   If COUNT is negative, search backwards for the -COUNTth instance;
      END must be <= START.
   If COUNT is zero, do anything you please; run rogue, for all I care.

   If END is zero, use BEGV or ZV instead, as appropriate for the
   direction indicated by COUNT.

   If we find COUNT instances, set *SHORTAGE to zero, and return the
   position after the COUNTth match.  Note that for reverse motion
   this is not the same as the usual convention for Emacs motion commands.

   If we don't find COUNT instances before reaching END, set *SHORTAGE
   to the number of TARGETs left unfound, and return END.

   If ALLOW_QUIT is non-zero, call QUIT periodically. */

Bytebpos
byte_scan_buffer (struct buffer *buf, Ichar target, Bytebpos st, Bytebpos en,
		  EMACS_INT count, EMACS_INT *shortage, int allow_quit)
{
  Bytebpos lim = en > 0 ? en :
    ((count > 0) ? BYTE_BUF_ZV (buf) : BYTE_BUF_BEGV (buf));

  /* #### newline cache stuff in this function not yet ported */
  assert (count != 0);

  if (shortage)
    *shortage = 0;

  if (count > 0)
    {
#ifdef MULE
      Internal_Format fmt = buf->text->format;
      /* Check for char that's unrepresentable in the buffer -- it
         certainly can't be there. */
      if (!ichar_fits_in_format (target, fmt, wrap_buffer (buf)))
	{
	  *shortage = count;
	  return lim;
	}
      /* Due to the Mule representation of characters in a buffer, we can
	 simply search for characters in the range 0 - 127 directly; for
	 8-bit-fixed, we can do this for all characters.  In other cases,
	 we do it the "hard" way.  Note that this way works for all
	 characters and all formats, but the other way is faster. */
      else if (! (fmt == FORMAT_8_BIT_FIXED ||
		  (fmt == FORMAT_DEFAULT && ichar_ascii_p (target))))
	{
	  Raw_Ichar raw = ichar_to_raw (target, fmt, wrap_buffer (buf));
	  while (st < lim && count > 0)
	    {
	      if (BYTE_BUF_FETCH_CHAR_RAW (buf, st) == raw)
		count--;
	      INC_BYTEBPOS (buf, st);
	    }
	}
      else
#endif
	{
	  Raw_Ichar raw = ichar_to_raw (target, fmt, wrap_buffer (buf));
	  while (st < lim && count > 0)
	    {
	      Bytebpos ceiling;
	      Ibyte *bufptr;

	      ceiling = BYTE_BUF_CEILING_OF (buf, st);
	      ceiling = min (lim, ceiling);
	      bufptr = (Ibyte *) memchr (BYTE_BUF_BYTE_ADDRESS (buf, st),
					   raw, ceiling - st);
	      if (bufptr)
		{
		  count--;
		  st = BYTE_BUF_PTR_BYTE_POS (buf, bufptr) + 1;
		}
	      else
		st = ceiling;
	    }
	}

      if (shortage)
	*shortage = count;
      if (allow_quit)
	QUIT;
      return st;
    }
  else
    {
#ifdef MULE
      Internal_Format fmt = buf->text->format;
      /* Check for char that's unrepresentable in the buffer -- it
         certainly can't be there. */
      if (!ichar_fits_in_format (target, fmt, wrap_buffer (buf)))
	{
	  *shortage = -count;
	  return lim;
	}
      else if (! (fmt == FORMAT_8_BIT_FIXED ||
		  (fmt == FORMAT_DEFAULT && ichar_ascii_p (target))))
	{
	  Raw_Ichar raw = ichar_to_raw (target, fmt, wrap_buffer (buf));
	  while (st > lim && count < 0)
	    {
	      DEC_BYTEBPOS (buf, st);
	      if (BYTE_BUF_FETCH_CHAR_RAW (buf, st) == raw)
		count++;
	    }
	}
      else
#endif
	{
	  Raw_Ichar raw = ichar_to_raw (target, fmt, wrap_buffer (buf));
	  while (st > lim && count < 0)
	    {
	      Bytebpos floorpos;
	      Ibyte *bufptr;
	      Ibyte *floorptr;

	      floorpos = BYTE_BUF_FLOOR_OF (buf, st);
	      floorpos = max (lim, floorpos);
	      /* No memrchr() ... */
	      bufptr = BYTE_BUF_BYTE_ADDRESS_BEFORE (buf, st);
	      floorptr = BYTE_BUF_BYTE_ADDRESS (buf, floorpos);
	      while (bufptr >= floorptr)
		{
		  st--;
		  /* At this point, both ST and BUFPTR refer to the same
		     character.  When the loop terminates, ST will
		     always point to the last character we tried. */
		  if (*bufptr == (Ibyte) raw)
		    {
		      count++;
		      break;
		    }
		  bufptr--;
		}
	    }
	}

      if (shortage)
	*shortage = -count;
      if (allow_quit)
	QUIT;
      if (count)
	return st;
      else
	{
	/* We found the character we were looking for; we have to return
	   the position *after* it due to the strange way that the return
	   value is defined. */
	  INC_BYTEBPOS (buf, st);
	  return st;
	}
    }
}

Charbpos
scan_buffer (struct buffer *buf, Ichar target, Charbpos start, Charbpos end,
	     EMACS_INT count, EMACS_INT *shortage, int allow_quit)
{
  Bytebpos byte_retval;
  Bytebpos byte_start, byte_end;

  byte_start = charbpos_to_bytebpos (buf, start);
  if (end)
    byte_end = charbpos_to_bytebpos (buf, end);
  else
    byte_end = 0;
  byte_retval = byte_scan_buffer (buf, target, byte_start, byte_end, count,
			      shortage, allow_quit);
  return bytebpos_to_charbpos (buf, byte_retval);
}

Bytebpos
byte_find_next_newline_no_quit (struct buffer *buf, Bytebpos from, int count)
{
  return byte_scan_buffer (buf, '\n', from, 0, count, 0, 0);
}

Charbpos
find_next_newline_no_quit (struct buffer *buf, Charbpos from, int count)
{
  return scan_buffer (buf, '\n', from, 0, count, 0, 0);
}

Charbpos
find_next_newline (struct buffer *buf, Charbpos from, int count)
{
  return scan_buffer (buf, '\n', from, 0, count, 0, 1);
}

Bytecount
byte_find_next_ichar_in_string (Lisp_Object str, Ichar target, Bytecount st,
			       EMACS_INT count)
{
  Bytebpos lim = XSTRING_LENGTH (str) -1;
  Ibyte *s = XSTRING_DATA (str);

  assert (count >= 0);

#ifdef MULE
  /* Due to the Mule representation of characters in a buffer,
     we can simply search for characters in the range 0 - 127
     directly.  For other characters, we do it the "hard" way.
     Note that this way works for all characters but the other
     way is faster. */
  if (target >= 128)
    {
      while (st < lim && count > 0)
	{
	  if (string_ichar (str, st) == target)
	    count--;
	  INC_BYTECOUNT (s, st);
	}
    }
  else
#endif
    {
      while (st < lim && count > 0)
	{
	  Ibyte *bufptr = (Ibyte *) memchr (itext_n_addr (s, st),
						(int) target, lim - st);
	  if (bufptr)
	    {
	      count--;
	      st =  (Bytebpos) (bufptr - s) + 1;
	    }
	  else
	    st = lim;
	}
    }
  return st;
}

/* This function synched with FSF 21.1 */
static Lisp_Object
skip_chars (struct buffer *buf, Boolint forwardp, Boolint syntaxp,
	    Lisp_Object string, Lisp_Object lim)
{
  REGISTER Ibyte *p, *pend;
  REGISTER Ichar c;
  /* We store the first 256 chars in an array here and the rest in
     a range table. */
  unsigned char fastmap[256];
  int negate = 0;
  Charbpos limit;
  struct syntax_cache *scache;
  Bitbyte class_bits = 0;
  
  if (NILP (lim))
    limit = forwardp ? BUF_ZV (buf) : BUF_BEGV (buf);
  else
    {
      /* See the commentary on START_POINT, POS below regarding why we haven't
         moved to functioning in byte positions. */
      limit = get_buffer_pos_char (buf, lim, GB_COERCE_RANGE);
    }

  CHECK_STRING (string);
  p = XSTRING_DATA (string);
  pend = p + XSTRING_LENGTH (string);
  memset (fastmap, 0, sizeof (fastmap));

  Fclear_range_table (Vskip_chars_range_table);

  if (p != pend && *p == '^')
    {
      negate = 1;
      p++;
    }

  /* Find the characters specified and set their elements of fastmap.
     If syntaxp, each character counts as itself.
     Otherwise, handle backslashes and ranges specially  */

  while (p != pend)
    {
      c = itext_ichar (p);
      INC_IBYTEPTR (p);
      if (syntaxp)
	{
	  if (c < 0200 && syntax_spec_code[c] < (unsigned char) Smax)
	    fastmap[c] = 1;
	  else
	    invalid_argument ("Invalid syntax designator", make_char (c));
	}
      else
	{
	  if (c == '\\')
	    {
	      if (p == pend) break;
	      c = itext_ichar (p);
	      INC_IBYTEPTR (p);
	    }
	  if (p != pend && *p == '-')
	    {
	      Ichar cend;

	      /* Skip over the dash.  */
	      p++;
	      if (p == pend) break;
	      cend = itext_ichar (p);
	      while (c <= cend && c < 256)
		{
		  fastmap[c] = 1;
		  c++;
		}
	      if (c <= cend)
		Fput_range_table (make_fixnum (c), make_fixnum (cend), Qt,
				  Vskip_chars_range_table);
	      INC_IBYTEPTR (p);
	    }
          else if ('[' == c && p != pend && *p == ':')
            {
              Ibyte *colonp;
              int ch = 0;
              re_wctype_t cc;

              INC_IBYTEPTR (p);

              if (p == pend)
                {
                  fastmap ['['] = fastmap[':'] = 1;
                  break;
                }

              colonp = (Ibyte *) memchr (p, ':', pend - p);
              if (NULL == colonp || (colonp + 1) == pend || colonp[1] != ']')
                {
                  fastmap ['['] = fastmap[':'] = 1;
                  continue;
                }

              cc = re_wctype (p, colonp - p);
              if (cc == RECC_ERROR)
                {
                  invalid_argument ("Invalid character class",
                                    make_string (p, colonp - p));
                }

              for (ch = 0; ch < countof (fastmap); ++ch)
                {
                  if (re_iswctype (ch, cc, buf))
                    {
                      fastmap[ch] = 1;
                    }
                }

              compile_char_class (cc, Vskip_chars_range_table, &class_bits);

              p = colonp + 2;
            }
	  else
	    {
	      if (c < 256)
		fastmap[c] = 1;
	      else
		Fput_range_table (make_fixnum (c), make_fixnum (c), Qt,
				  Vskip_chars_range_table);
	    }
	}
    }

  /* #### Not in FSF 21.1 */
  if (syntaxp && fastmap['-'] != 0)
    fastmap[' '] = 1;

  {
    /* The syntax cache still functions in terms of character positions, so we
       still need START_POINT and POS. And pleasingly we call BOTH_BUF_SET_PT
       () at the end, going beyond the standard of care in avoiding byte to
       char performance issues.  */
    Charbpos start_point = BUF_PT (buf);
    Charbpos pos = start_point;
    Bytebpos pos_byte = BYTE_BUF_PT (buf);

    if (syntaxp)
      {
	scache = setup_buffer_syntax_cache (buf, pos, forwardp ? 1 : -1);
	/* All syntax designators are normal chars so nothing strange
	   to worry about */
	if (forwardp)
	  {
	    if (pos < limit)
	      while (fastmap[(unsigned char)
			     syntax_code_spec
			     [(int) SYNTAX_FROM_CACHE
			      (scache, BYTE_BUF_FETCH_CHAR (buf, pos_byte))]]
                     != negate)
		{
		  pos++;
		  INC_BYTEBPOS (buf, pos_byte);
		  if (pos >= limit)
		    break;
		  UPDATE_SYNTAX_CACHE_FORWARD (scache, pos);
		}
	  }
	else
	  {
	    while (pos > limit)
	      {
		Bytebpos savepos = pos_byte;
		pos--;
		DEC_BYTEBPOS (buf, pos_byte);
		UPDATE_SYNTAX_CACHE_BACKWARD (scache, pos);
		if (fastmap[(unsigned char)
                            syntax_code_spec
                            [(int) SYNTAX_FROM_CACHE
                             (scache, BYTE_BUF_FETCH_CHAR (buf, pos_byte))]]
                    == negate)
		  {
		    pos++;
		    pos_byte = savepos;
		    break;
		  }
	      }
	  }
      }
    else
      {
        struct buffer *lispbuf = buf;

#define CLASS_BIT_CHECK(c)                                              \
        (class_bits && ((class_bits & BIT_ALPHA && ISALPHA (c))         \
                        || (class_bits & BIT_SPACE && ISSPACE (c))      \
                        || (class_bits & BIT_PUNCT && ISPUNCT (c))      \
                        || (class_bits & BIT_WORD && ISWORD (c))        \
                        || (NILP (buf->case_fold_search) ?              \
                            ((class_bits & BIT_UPPER && ISUPPER (c))    \
                             || (class_bits & BIT_LOWER && ISLOWER (c))) \
                            : (class_bits & (BIT_UPPER | BIT_LOWER)     \
                               && !NOCASEP (buf, c)))))
	if (forwardp)
	  {
	    while (pos < limit)
	      {
		Ichar ch = BYTE_BUF_FETCH_CHAR (buf, pos_byte);
                if ((ch < countof (fastmap) ? fastmap[ch]
                     : (CLASS_BIT_CHECK (ch) ||
                        (EQ (Qt, Fget_range_table (make_fixnum (ch),
                                                   Vskip_chars_range_table,
                                                   Qnil)))))
                    != negate)
		  {
		    pos++;
		    INC_BYTEBPOS (buf, pos_byte);
		  }
		else
		  break;
	      }
	  }
	else
	  {
	    while (pos > limit)
	      {
		Bytebpos prev_pos_byte = pos_byte;
		Ichar ch;

		DEC_BYTEBPOS (buf, prev_pos_byte);
		ch = BYTE_BUF_FETCH_CHAR (buf, prev_pos_byte);
                if ((ch < countof (fastmap) ? fastmap[ch]
                     : (CLASS_BIT_CHECK (ch) ||
                        (EQ (Qt, Fget_range_table (make_fixnum (ch),
                                                   Vskip_chars_range_table,
                                                   Qnil)))))
                    != negate)
		  {
		    pos--;
		    pos_byte = prev_pos_byte;
		  }
                else
                  break;
	      }
	  }
      }
    QUIT;
    BOTH_BUF_SET_PT (buf, pos, pos_byte);
    return make_fixnum (BUF_PT (buf) - start_point);
  }
}

DEFUN ("skip-chars-forward", Fskip_chars_forward, 1, 3, 0, /*
Move point forward, stopping before a char not in STRING, or at pos LIMIT.
STRING is like the inside of a `[...]' in a regular expression
except that `]' is never special and `\\' quotes `^', `-' or `\\'.
Thus, with arg "a-zA-Z", this skips letters stopping before first nonletter.
With arg "^a-zA-Z", skips nonletters stopping before first letter.
Returns the distance traveled, either zero or positive.

Optional argument BUFFER defaults to the current buffer.

This function does not modify the match data.
*/
       (string, limit, buffer))
{
  return skip_chars (decode_buffer (buffer, 0), 1, 0, string, limit);
}

DEFUN ("skip-chars-backward", Fskip_chars_backward, 1, 3, 0, /*
Move point backward, stopping after a char not in STRING, or at pos LIMIT.
See `skip-chars-forward' for details.
Returns the distance traveled, either zero or negative.

Optional argument BUFFER defaults to the current buffer.

This function does not modify the match data.
*/
       (string, limit, buffer))
{
  return skip_chars (decode_buffer (buffer, 0), 0, 0, string, limit);
}


DEFUN ("skip-syntax-forward", Fskip_syntax_forward, 1, 3, 0, /*
Move point forward across chars in specified syntax classes.
SYNTAX is a string of syntax code characters.
Stop before a char whose syntax is not in SYNTAX, or at position LIMIT.
If SYNTAX starts with ^, skip characters whose syntax is NOT in SYNTAX.
This function returns the distance traveled, either zero or positive.

Optional argument BUFFER defaults to the current buffer.

This function does not modify the match data.
*/
       (syntax, limit, buffer))
{
  return skip_chars (decode_buffer (buffer, 0), 1, 1, syntax, limit);
}

DEFUN ("skip-syntax-backward", Fskip_syntax_backward, 1, 3, 0, /*
Move point backward across chars in specified syntax classes.
SYNTAX is a string of syntax code characters.
Stop on reaching a char whose syntax is not in SYNTAX, or at position LIMIT.
If SYNTAX starts with ^, skip characters whose syntax is NOT in SYNTAX.
This function returns the distance traveled, either zero or negative.

Optional argument BUFFER defaults to the current buffer.

This function does not modify the match data.
*/
       (syntax, limit, buffer))
{
  return skip_chars (decode_buffer (buffer, 0), 0, 1, syntax, limit);
}


/* Subroutines of Lisp buffer search functions. */

static Lisp_Object
search_command (Lisp_Object string, Lisp_Object limit, Lisp_Object noerror,
		Lisp_Object count, Lisp_Object buffer, int direction,
		Boolint RE, Boolint posix)
{
  REGISTER Bytebpos np;
  Bytebpos lim;
  EMACS_INT n = direction;
  struct buffer *buf;

  if (!NILP (count))
    {
      CHECK_FIXNUM (count);
      n *= XFIXNUM (count);
    }

  buf = decode_buffer (buffer, 0);
  CHECK_STRING (string);
  if (NILP (limit))
    lim = n > 0 ? BYTE_BUF_ZV (buf) : BYTE_BUF_BEGV (buf);
  else
    {
      lim = get_buffer_pos_byte (buf, limit, GB_COERCE_RANGE);
      if (n > 0 ? lim < BYTE_BUF_PT (buf) : lim > BYTE_BUF_PT (buf))
	invalid_argument ("Invalid search limit (wrong side of point)",
			  Qunbound);
    }

  np = search_buffer (buf, string, BYTE_BUF_PT (buf), lim, n, RE,
		      (!NILP (buf->case_fold_search)
		       ? XCASE_TABLE_CANON (buf->case_table)
                       : Qnil),
		      (!NILP (buf->case_fold_search)
		       ? XCASE_TABLE_EQV (buf->case_table)
                       : Qnil), posix);

  if (np <= 0)
    {
      if (NILP (noerror))
	{
	  signal_failure (string);
	  RETURN_NOT_REACHED (Qnil);
	}
      if (!EQ (noerror, Qt))
	{
	  if (lim < BYTE_BUF_BEGV (buf) || lim > BYTE_BUF_ZV (buf))
	    ABORT ();
	  BYTE_BUF_SET_PT (buf, lim);
	  return Qnil;
#if 0 /* This would be clean, but maybe programs depend on
	 a value of nil here.  */
	  np = lim;
#endif
	}
      else
        return Qnil;
    }

  if (np < BYTE_BUF_BEGV (buf) || np > BYTE_BUF_ZV (buf))
    ABORT ();

  BYTE_BUF_SET_PT (buf, np);

  return make_fixnum (BUF_PT (buf));
}

static Boolint
trivial_regexp_p (Lisp_Object regexp)
{
  Ibyte *s = XSTRING_DATA (regexp), *send = s + XSTRING_LENGTH (regexp);
  while (s < send)
    {
      Ichar c = itext_ichar (s);
      INC_IBYTEPTR (s);
      switch (c)
	{
	/* #### howcum ']' doesn't appear here, but ... */
	case '.': case '*': case '+': case '?': case '[': case '^': case '$':
	  return 0;
	case '\\':
          if (s == send)
            {
              return 0; 
            }
          c = itext_ichar (s);
          INC_IBYTEPTR (s);
	  switch (c)
	    {
	    /* ... ')' does appear here?  ('<' and '>' can appear singly.) */
	    /* #### are there other constructs to check? */
	    case '|': case '(': case ')': case '`': case '\'': case 'b':
	    case 'B': case '<': case '>': case 'w': case 'W': case 's':
	    case 'S': case '=': case '{': case '}':
#ifdef MULE
	    /* 97/2/25 jhod Added for category matches */
	    case 'c': case 'C':
#endif /* MULE */
	    case '1': case '2': case '3': case '4': case '5':
	    case '6': case '7': case '8': case '9':
	      return 0;
	    }
	}
    }
  return 1;
}

#ifdef MULE
/* Return non-zero if two characters -- the first represented in
 * Itext format, and the second given as a character -- differ in the
 * non-final bytes of their respective Itext representations. */

inline static Boolint
chars_differ_in_non_final_bytes (const Ibyte *astr, Bytecount alen, Ichar b)
{
  Ibyte bstr[MAX_ICHAR_LEN];
  Bytecount blen = set_itext_ichar (bstr, b);

  /* Are two characters in Itext representation same except
     for the last byte of the representation?  They're the same if the
     lengths are the same and the text up till the final byte (if any)
     of each is the same.  Correspondingly, they're different if either
     the lengths are different or the non-final-byte text is non-zero
     in length and different. (If both strings have the same length and
     the length is 1, then both are the same up till the final byte,
     since they are only a final byte.  We check for this to avoid
     calling memcmp() with zero size. */
  return (alen != blen || (alen > 1 && memcmp (astr, bstr, blen - 1)));
}
#endif

/* Search for the n'th occurrence of STRING in BUF,
   starting at position POS and stopping at position BUFLIM,
   treating PAT as a literal string if RE is false or as
   a regular expression if RE is true.

   If N is positive, searching is forward and BUFLIM must be greater
   than POS.
   If N is negative, searching is backward and BUFLIM must be less
   than POS.

   Returns -x if only N-x occurrences found (x > 0),
   or else the position at the beginning of the Nth occurrence
   (if searching backward) or the end (if searching forward).

   POSIX is nonzero if we want full backtracking (POSIX style)
   for this pattern.  0 means backtrack only enough to get a valid match.  */
static Bytebpos
search_buffer (struct buffer *buf, Lisp_Object string, Bytebpos pos,
	       Bytebpos lim, EMACS_INT n, Boolint RE, Lisp_Object trt,
	       Lisp_Object inverse_trt, Boolint posix)
{
  Bytecount len = XSTRING_LENGTH (string);
  Ibyte *base_pat = XSTRING_DATA (string);
  REGISTER EMACS_INT i, j;
  Bytebpos p1, p2;
  Bytecount s1, s2;

  /* Some FSF junk with running_asynch_code, to preserve the match
     data.  Not necessary because we don't call process filters
     asynchronously (i.e. from within QUIT). */

  /* Searching 0 times means noop---don't move, don't touch registers.  */
  if (n == 0)
    return pos;

  /* Null string is found at starting position.  */
  if (len == 0)
    {
      structure_checking_assert (search_regs.num_regs > 0);

      search_regs.start[0] = search_regs.end[0] = pos;
      search_regs.num_regs = 1;
      set_lisp_search_registers (wrap_buffer (buf), &search_regs);
      return pos;
    }

  if (RE && !trivial_regexp_p (string))
    {
      struct re_pattern_buffer *bufp;

      bufp = compile_pattern (string, &search_regs, trt,
			      wrap_buffer (buf), buf, posix, ERROR_ME);

      /* Get pointers and sizes of the two strings
	 that make up the visible portion of the buffer. */

      p1 = BYTE_BUF_BEGV (buf);
      p2 = BYTE_BUF_CEILING_OF (buf, p1);
      s1 = p2 - p1;
      s2 = BYTE_BUF_ZV (buf) - p2;

      while (n != 0)
	{
	  Bytecount val;
	  struct syntax_cache scache_struct;
	  struct syntax_cache *scache = &scache_struct;
  
	  QUIT;
	  /* By making the regex object, regex buffer, and syntax cache
	     arguments to re_{search,match}{,_2}, and by having
	     re_match_2_internal() work on (and modify) its own copy of the
	     cached compiled pattern, we've removed the need to do nasty
	     things to deal with regex reentrancy. */
	  val = re_search_2 (bufp,
			     (char *) BYTE_BUF_BYTE_ADDRESS (buf, p1), s1,
			     (char *) BYTE_BUF_BYTE_ADDRESS (buf, p2), s2,
                             pos - BYTE_BUF_BEGV (buf), lim - pos, &search_regs,
			     n > 0 ? lim - BYTE_BUF_BEGV (buf) :
			     pos - BYTE_BUF_BEGV (buf), wrap_buffer (buf),
			     buf, scache);

	  if (val == -2)
	    {
	      matcher_overflow ();
	    }
	  if (val >= 0)
	    {
	      int num_regs = search_regs.num_regs;
	      j = BYTE_BUF_BEGV (buf);
	      for (i = 0; i < num_regs; i++)
		if (search_regs.start[i] >= 0)
		  {
		    search_regs.start[i] += j;
		    search_regs.end[i] += j;
		  }

	      /* Set pos to the new position. */
	      pos = n > 0 ? search_regs.end[0] : search_regs.start[0];
              set_lisp_search_registers (wrap_buffer (buf), &search_regs);
	    }
	  else
	    return (n > 0 ? 0 - n : n);
	  if (n > 0) n--; else n++;
	}
      return pos;
    }
  else				/* non-RE case */
    {
      int char_base_len = -1;
      Ibyte char_base[MAX_ICHAR_LEN];
      int boyer_moore_ok = 1;
      Ibyte *patbuf = alloca_ibytes (len * MAX_ICHAR_LEN);
      Ibyte *pat = patbuf;

#ifdef MULE
      int entirely_one_byte_p = buf->text->entirely_one_byte_p;
      int nothing_greater_than_0xff =
        buf->text->num_8_bit_fixed_chars == BUF_Z(buf) - BUF_BEG (buf);

      while (len > 0)
	{
	  Ibyte tmp_str[MAX_ICHAR_LEN];
	  Ichar c, translated, inverse;
	  Bytecount orig_bytelen, new_bytelen;

	  /* If we got here and the RE flag is set, it's because
	     we're dealing with a regexp known to be trivial, so the
	     backslash just quotes the next character.  */
	  if (RE && *base_pat == '\\')
	    {
	      len--;
	      base_pat++;
	    }
	  c = itext_ichar (base_pat);
	  translated = TRANSLATE (trt, c);
	  inverse = TRANSLATE (inverse_trt, c);

	  orig_bytelen = itext_ichar_len (base_pat);
	  new_bytelen = set_itext_ichar (tmp_str, translated);

          if (boyer_moore_ok
              /* Only do the Boyer-Moore check for characters needing
                 translation. */
              && (translated != c || inverse != c))
            {
	      Ichar starting_c = c;
	      int checked = 0;

	      do 
		{
		  c = TRANSLATE (inverse_trt, c);

                  /* If a character cannot occur in the buffer, ignore
                     it. */
                  if (c > 0x7F && entirely_one_byte_p)
                    continue;

                  if (c > 0xFF && nothing_greater_than_0xff)
                    continue;

                  checked = 1;

		  /* Track the original character in string char
		     representation (minus final byte); we will compare it
		     against each other character (again minus final byte),
		     to see if they're the same. */
		  if (char_base_len == -1)
		    char_base_len = set_itext_ichar (char_base, c);
		  else if (chars_differ_in_non_final_bytes
			   (char_base, char_base_len, c))
		    {
		      /* If two different rows, or two different charsets,
			 appear, needing non-ASCII translation, then we
			 cannot use boyer_moore search.  See the comment at
			 the head of boyer_moore(). */
		      boyer_moore_ok = 0;
		      break;
                    }

		  if (ichar_len (c) > 2)
		    {
		      /* Case-equivalence plus repeated octets throws off
			 the construction of the stride table; avoid this.

		         It should be possible to correct boyer_moore to
		         behave correctly even in this case--it doesn't have
		         problems with repeated octets when case conversion
		         is not involved--but this is not a critical
		         issue. */
		      Ibyte encoded[MAX_ICHAR_LEN];
		      Bytecount clen = set_itext_ichar (encoded, c);
		      int a, b;
		      for (a = 0; a < clen && boyer_moore_ok; ++a)
			{
			  for (b = a + 1; b < clen && boyer_moore_ok; ++b)
			    {
			      if (encoded[a] == encoded[b])
				{
				  boyer_moore_ok = 0;
				}
			    }
			}

		      if (0 == boyer_moore_ok)
			{
			  break;
			}
		    }
			  
                } while (c != starting_c);

              if (!checked)
                {
#ifdef DEBUG_XEMACS
                  if (debug_searches)
                    {
                      Lisp_Symbol *sym = XSYMBOL (Qsearch_algorithm_used);
                      sym->value = Qnil;
                    }
#endif
                  /* The "continue" clauses were used above, for every
                     translation of the character. As such, this character
                     is not to be found in the buffer and neither is the
                     string as a whole. Return immediately; also avoid
                     triggering the assertion a few lines down. */
                  return n > 0 ? -n : n;
                }


              if (boyer_moore_ok && char_base_len != -1)
		{
		  if (chars_differ_in_non_final_bytes
		      (char_base, char_base_len, translated))
		    {
		      /* In the rare event that the CANON entry for this
			 character is not in the desired set, choose one
			 that is, from the equivalence set. It doesn't much
			 matter which. */
		      Ichar starting_ch = translated;
		      do
			{
			  translated = TRANSLATE (inverse_trt, translated);
			  if (!chars_differ_in_non_final_bytes
			      (char_base, char_base_len, translated))
			    break;

			} while (starting_ch != translated);

		      assert (starting_ch != translated);

		      new_bytelen = set_itext_ichar (tmp_str, translated);
		    }
		}
	    }

	  memcpy (pat, tmp_str, new_bytelen);
	  pat += new_bytelen;
	  base_pat += orig_bytelen;
	  len -= orig_bytelen;
	}

      if (char_base_len == -1)
        {
          char_base_len = 1; /* Default to ASCII. */
        }

#else /* not MULE */
      while (--len >= 0)
	{
	  /* If we got here and the RE flag is set, it's because
	     we're dealing with a regexp known to be trivial, so the
	     backslash just quotes the next character.  */
	  if (RE && *base_pat == '\\')
	    {
	      len--;
	      base_pat++;
	    }
	  *pat++ = TRANSLATE (trt, *base_pat++);
	}
#endif /* MULE */
      len = pat - patbuf;
      pat = base_pat = patbuf;

#ifdef DEBUG_XEMACS
      if (debug_searches)
        {
          Lisp_Symbol *sym = XSYMBOL (Qsearch_algorithm_used);
          sym->value = boyer_moore_ok ? Qboyer_moore : Qsimple_search;
        }
#endif

      if (boyer_moore_ok)
	return boyer_moore (buf, base_pat, len, pos, lim, n,
			    trt, inverse_trt, char_base, char_base_len);
      else
	return simple_search (buf, base_pat, len, pos, lim, n, trt);
    }
}

/* Do a simple string search N times for the string PAT, whose length is
   LEN/LEN_BYTE, from buffer position POS until LIM.  TRT is the
   translation table.

   Return the character position where the match is found.
   Otherwise, if M matches remained to be found, return -M.

   This kind of search works regardless of what is in PAT and
   regardless of what is in TRT.  It is used in cases where
   boyer_moore cannot work.  */

static Bytebpos
simple_search (struct buffer *buf, Ibyte *base_pat, Bytecount len,
	       Bytebpos pos, Bytebpos lim, EMACS_INT n, Lisp_Object trt)
{
  int forward = n > 0;
  Bytecount buf_len = 0; /* Shut up compiler. */

  if (lim > pos)
    while (n > 0)
      {
	while (1)
	  {
	    Bytecount this_len = len;
	    Bytebpos this_pos = pos;
	    Ibyte *p = base_pat;
	    if (pos >= lim)
	      goto stop;

	    while (this_len > 0)
	      {
		Ichar pat_ch, buf_ch;
		Bytecount pat_len;

		pat_ch = itext_ichar (p);
		buf_ch = BYTE_BUF_FETCH_CHAR (buf, this_pos);

		buf_ch = TRANSLATE (trt, buf_ch);

		if (buf_ch != pat_ch)
		  break;

		pat_len = itext_ichar_len (p);
		p += pat_len;
		this_len -= pat_len;
		INC_BYTEBPOS (buf, this_pos);
	      }
	    if (this_len == 0)
	      {
		buf_len = this_pos - pos;
		pos = this_pos;
		break;
	      }
	    INC_BYTEBPOS (buf, pos);
	  }
	n--;
      }
  else
    {
      /* If lim < len, then there are too few buffer positions to hold the
	 pattern between the beginning of the buffer and lim.  Adjust to
	 ensure pattern fits.  If we don't do this, we can assert in the
	 DEC_BYTEBPOS below. */
      if (lim < len)
	lim = len;
      while (n < 0)
	{
	  while (1)
	    {
	      Bytecount this_len = len;
	      Bytebpos this_pos = pos;
	      Ibyte *p;
	      if (pos <= lim)
		goto stop;
	      p = base_pat + len;

	      while (this_len > 0)
		{
		  Ichar pat_ch, buf_ch;

		  DEC_IBYTEPTR (p);
		  DEC_BYTEBPOS (buf, this_pos);
		  pat_ch = itext_ichar (p);
		  buf_ch = BYTE_BUF_FETCH_CHAR (buf, this_pos);

		  buf_ch = TRANSLATE (trt, buf_ch);

		  if (buf_ch != pat_ch)
		    break;

		  this_len -= itext_ichar_len (p);
		}
	      if (this_len == 0)
		{
		  buf_len = pos - this_pos;
		  pos = this_pos;
		  break;
		}
	      DEC_BYTEBPOS (buf, pos);
	    }
	  n++;
	}
    }
 stop:
  if (n == 0)
    {
      structure_checking_assert (search_regs.num_regs > 0);
      search_regs.num_regs = 1;

      if (forward)
	{
          search_regs.start[0] = pos - buf_len;
          search_regs.end[0] = pos;
	}
      else
	{
          search_regs.start[0] = pos;
          search_regs.end[0] = pos + buf_len;
	}

      set_lisp_search_registers (wrap_buffer (buf), &search_regs);

      return pos;
    }
  else if (n > 0)
    return -n;
  else
    return n;
}

/* Do Boyer-Moore search N times for the string PAT,
   whose length is LEN/LEN_BYTE,
   from buffer position POS/POS_BYTE until LIM/LIM_BYTE.
   DIRECTION says which direction we search in.
   TRT and INVERSE_TRT are translation tables.

   This kind of search works if all the characters in PAT that have
   (non-ASCII) translation are the same aside from the last byte.  This
   makes it possible to translate just the last byte of a character, and do
   so after just a simple test of the context.

   If that criterion is not satisfied, do not call this function.  You will
   get an assertion failure. */
	    
static Bytebpos
boyer_moore (struct buffer *buf, Ibyte *base_pat, Bytecount len,
	     Bytebpos pos, Bytebpos lim, EMACS_INT n, Lisp_Object trt,
	     Lisp_Object inverse_trt, Ibyte *USED_IF_MULE (char_base),
	     int USED_IF_MULE (char_base_len))
{
  /* #### Someone really really really needs to comment the workings
     of this junk somewhat better.

     BTW "BM" stands for Boyer-Moore, which is one of the standard
     string-searching algorithms.  It's the best string-searching
     algorithm out there, provided that:

     a) You're not fazed by algorithm complexity. (Rabin-Karp, which
     uses hashing, is much much easier to code but not as fast.)
     b) You can freely move backwards in the string that you're
     searching through.

     As the comment below tries to explain (but garbles in typical
     programmer-ese), the idea is that you don't have to do a
     string match at every successive position in the text.  For
     example, let's say the pattern is "a very long string".  We
     compare the last character in the string (`g') with the
     corresponding character in the text.  If it mismatches, and
     it is, say, `z', then we can skip forward by the entire
     length of the pattern because `z' does not occur anywhere
     in the pattern.  If the mismatching character does occur
     in the pattern, we can usually still skip forward by more
     than one: e.g. if it is `l', then we can skip forward
     by the length of the substring "ong string" -- i.e. the
     largest end section of the pattern that does not contain
     the mismatched character.  So what we do is compute, for
     each possible character, the distance we can skip forward
     (the "stride") and use it in the string matching.  This
     is what the BM_tab holds. */
  REGISTER EMACS_INT *BM_tab;
  EMACS_INT *BM_tab_base;
  REGISTER Bytecount dirlen;
  EMACS_INT infinity;
  Bytebpos limit;
  Bytecount stride_for_teases = 0;
  REGISTER EMACS_INT i, j;
  Ibyte *pat, *pat_end;
  REGISTER Ibyte *cursor, *p_limit, *ptr2;
  Ibyte simple_translate[256];
  REGISTER int direction = ((n > 0) ? 1 : -1);
#ifdef MULE
  Ibyte translate_prev[MAX_ICHAR_LEN];
  Bytecount translate_prev_len;
  /* These need to be rethought in the event that the internal format
     changes, or in the event that num_8_bit_fixed_chars disappears
     (entirely_one_byte_p can be trivially worked out by checking is the
     byte count equal to the char count.)  */
  int buffer_entirely_one_byte_p = buf->text->entirely_one_byte_p;
  int buffer_nothing_greater_than_0xff =
    buf->text->num_8_bit_fixed_chars == BUF_Z(buf) - BUF_BEG (buf);
#endif
#ifdef C_ALLOCA
  EMACS_INT BM_tab_space[256];
  BM_tab = &BM_tab_space[0];
#else
  BM_tab = alloca_array (EMACS_INT, 256);
#endif
  
  /* The general approach is that we are going to maintain that we
     know the first (closest to the present position, in whatever
     direction we're searching) character that could possibly be
     the last (furthest from present position) character of a
     valid match.  We advance the state of our knowledge by
     looking at that character and seeing whether it indeed
     matches the last character of the pattern.  If it does, we
     take a closer look.  If it does not, we move our pointer (to
     putative last characters) as far as is logically possible.
     This amount of movement, which I call a stride, will be the
     length of the pattern if the actual character appears nowhere
     in the pattern, otherwise it will be the distance from the
     last occurrence of that character to the end of the pattern.
     As a coding trick, an enormous stride is coded into the table
     for characters that match the last character.  This allows
     use of only a single test, a test for having gone past the
     end of the permissible match region, to test for both
     possible matches (when the stride goes past the end
     immediately) and failure to match (where you get nudged past
     the end one stride at a time).

     Here we make a "mickey mouse" BM table.  The stride of the
     search is determined only by the last character of the
     putative match.  If that character does not match, we will
     stride the proper distance to propose a match that
     superimposes it on the last instance of a character that
     matches it (per trt), or misses it entirely if there is
     none. */

  dirlen = len * direction;
  infinity = dirlen - (lim + pos + len + len) * direction;
  /* Record position after the end of the pattern.  */
  pat_end = base_pat + len;
  if (direction < 0)
    base_pat = pat_end - 1;
  BM_tab_base = BM_tab;
  BM_tab += 256;
  j = dirlen;		/* to get it in a register */
  /* A character that does not appear in the pattern induces a
     stride equal to the pattern length. */
  while (BM_tab_base != BM_tab)
    {
      *--BM_tab = j;
      *--BM_tab = j;
      *--BM_tab = j;
      *--BM_tab = j;
    }
  /* We use this for translation, instead of TRT itself.  We
     fill this in to handle the bytes that actually occur
     in the pattern.  Others don't matter anyway!  */
  xzero (simple_translate);
  for (i = 0; i < 256; i++)
    simple_translate[i] = (Ibyte) i;
  i = 0;

  while (i != infinity)
    {
      Ibyte *ptr = base_pat + i;
      i += direction;
      if (i == dirlen)
	i = infinity;
      if (!NILP (trt))
	{
#ifdef MULE
	  Ichar ch = -1;
	  Ibyte byte;
	  int this_translated = 1;

	  /* Is *PTR the last byte of a character?  */
	  if (pat_end - ptr == 1 || ibyte_first_byte_p (ptr[1]))
	    {
	      Ibyte *charstart = ptr;
	      Ichar untranslated;

	      while (!ibyte_first_byte_p (*charstart))
		charstart--;
	      untranslated = itext_ichar (charstart);

              ch = TRANSLATE (trt, untranslated);
	      /* We set everything to zero.  Since we use translate_prev
		 only for storing parts of multi-byte characters, there
		 won't be any zero's in them. */
	      xzero (translate_prev);
	      translate_prev_len = 0;
	      ptr2 = ptr;
	      while (!ibyte_first_byte_p (*ptr2))
		translate_prev[translate_prev_len++] = *--ptr2;

              if (ch != untranslated) /* Was translation done? */
		{
		  if (chars_differ_in_non_final_bytes
		      (char_base, char_base_len, ch))
		    {
		      /* In the very rare event that the CANON entry for this
			 character is not in the desired set, choose one that
			 is, from the equivalence set. It doesn't much matter
			 which, since we're building our own cheesy equivalence
			 table instead of using that belonging to the case
			 table directly.

			 We can get here if search_buffer has worked out that
			 the buffer is entirely single width. */
		      Ichar starting_ch = ch;
		      int count = 0;
		      do
			{
			  ch = TRANSLATE (inverse_trt, ch);
			  if (chars_differ_in_non_final_bytes
			      (char_base, char_base_len, ch))
			    break;
			  ++count;
			} while (starting_ch != ch);

		      /* If starting_ch is equal to ch (and count is not
			 one, which means no translation is necessary), the
			 case table is corrupt. (Any mapping in the canon
			 table should be reflected in the equivalence
			 table, and we know from the canon table that
			 untranslated maps to starting_ch and that
			 untranslated when converted to Itext has the
			 correct value for all but the final byte.) */
		      assert (1 == count || starting_ch != ch);
		    }
		}
	      {
		Ibyte tmp[MAX_ICHAR_LEN];
		Bytecount chlen;

		chlen = set_itext_ichar (tmp, ch);
		byte = tmp[chlen - 1];
	      }
	    }
	  else
	    {
	      byte = *ptr;
	      this_translated = 0;
	      ch = -1;
	    }

	  /* BYTE = last byte of character CH when represented as text */
	  j = byte;
	      
	  if (i == infinity)
	    stride_for_teases = BM_tab[j];
	  BM_tab[j] = dirlen - i;
	  /* A translation table is accompanied by its inverse -- see
	     comment in casetab.c. */
	  if (this_translated)
	    {
	      Ichar starting_ch = ch;
	      EMACS_INT starting_j = j;

	      text_checking_assert (valid_ichar_p (ch));
	      do
		{
		  ch = TRANSLATE (inverse_trt, ch);

                  if (ch > 0x7F && buffer_entirely_one_byte_p)
                    continue;

                  if (ch > 0xFF && buffer_nothing_greater_than_0xff)
                    continue;


		  /* Retrieve last byte of character CH when represented as
		     text */
		  {
		    Ibyte tmp[MAX_ICHAR_LEN];
		    Bytecount chlen;

		    chlen = set_itext_ichar (tmp, ch);
		    j = tmp[chlen - 1];
		  }
	      
                  /* For all the characters that map into CH, set up
                     simple_translate to map the last byte into
                     STARTING_J.  */
                  simple_translate[j] = (Ibyte) starting_j;
                  BM_tab[j] = dirlen - i;

		}
	      while (ch != starting_ch);
	    }
#else /* not MULE */
	  EMACS_INT k;
	  j = *ptr;
	  k = (j = TRANSLATE (trt, j));
	  if (i == infinity)
	    stride_for_teases = BM_tab[j];
	  BM_tab[j] = dirlen - i;
	  /* A translation table is accompanied by its inverse --
	     see comment in casetab.c. */
	  while ((j = TRANSLATE (inverse_trt, j)) != k)
	    {
	      simple_translate[j] = (Ibyte) k;
	      BM_tab[j] = dirlen - i;
	    }
#endif /* (not) MULE */
	}
      else
	{
	  j = *ptr;

	  if (i == infinity)
	    stride_for_teases = BM_tab[j];
	  BM_tab[j] = dirlen - i;
	}
      /* stride_for_teases tells how much to stride if we get a
	 match on the far character but are subsequently
	 disappointed, by recording what the stride would have been
	 for that character if the last character had been
	 different. */
    }
  infinity = dirlen - infinity;
  pos += dirlen - ((direction > 0) ? direction : 0);
  /* loop invariant - pos points at where last char (first char if
     reverse) of pattern would align in a possible match.  */
  while (n != 0)
    {
      Bytebpos tail_end;
      Ibyte *tail_end_ptr;
      /* It's been reported that some (broken) compiler thinks
	 that Boolean expressions in an arithmetic context are
	 unsigned.  Using an explicit ?1:0 prevents this.  */
      if ((lim - pos - ((direction > 0) ? 1 : 0)) * direction < 0)
	return n * (0 - direction);
      /* First we do the part we can by pointers (maybe
	 nothing) */
      QUIT;
      pat = base_pat;
      limit = pos - dirlen + direction;
      /* XEmacs change: definitions of CEILING_OF and FLOOR_OF
	 have changed.  See buffer.h. */
      limit = ((direction > 0)
	       ? BYTE_BUF_CEILING_OF (buf, limit) - 1
	       : BYTE_BUF_FLOOR_OF (buf, limit + 1));
      /* LIMIT is now the last (not beyond-last!) value POS can
	 take on without hitting edge of buffer or the gap.  */
      limit = ((direction > 0)
	       ? min (lim - 1, min (limit, pos + 20000))
	       : max (lim, max (limit, pos - 20000)));
      tail_end = BYTE_BUF_CEILING_OF (buf, pos);
      tail_end_ptr = BYTE_BUF_BYTE_ADDRESS (buf, tail_end);

#ifndef MULE
      USED (tail_end_ptr);
#endif

      if ((limit - pos) * direction > 20)
	{
	  /* We have to be careful because the code can generate addresses
	     that don't point to the beginning of characters. */
	  p_limit = BYTE_BUF_BYTE_ADDRESS_NO_VERIFY (buf, limit);
	  ptr2 = (cursor = BYTE_BUF_BYTE_ADDRESS_NO_VERIFY (buf, pos));
	  /* In this loop, pos + cursor - ptr2 is the surrogate
	     for pos */
	  while (1)	/* use one cursor setting as long as i can */
	    {
	      if (direction > 0) /* worth duplicating */
		{
		  /* Use signed comparison if appropriate to make
		     cursor+infinity sure to be > p_limit.
		     Assuming that the buffer lies in a range of
		     addresses that are all "positive" (as ints)
		     or all "negative", either kind of comparison
		     will work as long as we don't step by
		     infinity.  So pick the kind that works when
		     we do step by infinity.  */
		  if ((EMACS_INT) (p_limit + infinity) >
		      (EMACS_INT) p_limit)
		    while ((EMACS_INT) cursor <=
			   (EMACS_INT) p_limit)
		      cursor += BM_tab[*cursor];
		  else
		    while ((EMACS_UINT) cursor <=
			   (EMACS_UINT) p_limit)
		      cursor += BM_tab[*cursor];
		}
	      else
		{
		  if ((EMACS_INT) (p_limit + infinity) <
		      (EMACS_INT) p_limit)
		    while ((EMACS_INT) cursor >=
			   (EMACS_INT) p_limit)
		      cursor += BM_tab[*cursor];
		  else
		    while ((EMACS_UINT) cursor >=
			   (EMACS_UINT) p_limit)
		      cursor += BM_tab[*cursor];
		}
	      /* If you are here, cursor is beyond the end of the
		 searched region.  This can happen if you match on
		 the far character of the pattern, because the
		 "stride" of that character is infinity, a number
		 able to throw you well beyond the end of the
		 search.  It can also happen if you fail to match
		 within the permitted region and would otherwise
		 try a character beyond that region */
	      if ((cursor - p_limit) * direction <= len)
		break;	/* a small overrun is genuine */
	      cursor -= infinity; /* large overrun = hit */
	      i = dirlen - direction;
	      if (!NILP (trt))
		{
		  while ((i -= direction) + direction != 0)
		    {
#ifdef MULE
		      int dotrans;
		      Ichar ch;
		      cursor -= direction;
		      /* Translate only the last byte of a character.  */
		      dotrans = (cursor == tail_end_ptr
				 || ibyte_first_byte_p (cursor[1]));
		      if (dotrans)
			{
			  int k = 0;
			  Ibyte *curs2 = cursor;
			  while (1)
			    {
			      if (ibyte_first_byte_p (*curs2))
				break;
			      if (translate_prev[k++] != *--curs2)
				{
				  dotrans = 0;
				  break;
				}
			    }
			}

		      if (dotrans)
			ch = simple_translate[*cursor];
		      else
			ch = *cursor;
		      if (pat[i] != ch)
			break;
#else
		      if (pat[i] != TRANSLATE (trt, *(cursor -= direction)))
			break;
#endif
		    }
		}
	      else
		{
		  while ((i -= direction) + direction != 0)
		    if (pat[i] != *(cursor -= direction))
		      break;
		}
	      cursor += dirlen - i - direction;	/* fix cursor */
	      if (i + direction == 0)
		{
                  Bytebpos bytstart;

		  cursor -= direction;
                  bytstart = (pos + cursor - ptr2 + ((direction > 0)
                                                     ? 1 - len : 0));

                  structure_checking_assert (search_regs.num_regs > 0);

                  search_regs.start[0] = bytstart;
                  search_regs.end[0] = bytstart + len;
                  search_regs.num_regs = 1;

                  set_lisp_search_registers (wrap_buffer (buf), &search_regs);

		  if ((n -= direction) != 0)
		    cursor += dirlen; /* to resume search */
		  else if (direction > 0)
                    {
                      return search_regs.end[0];
                    }
                  else
                    {
                      return search_regs.start[0];
                    }
		}
	      else
		cursor += stride_for_teases; /* <sigh> we lose -  */
	    }
	  pos += cursor - ptr2;
	}
      else
	/* Now we'll pick up a clump that has to be done the hard
	   way because it covers a discontinuity */
	{
	  /* XEmacs change: definitions of CEILING_OF and FLOOR_OF
	     have changed.  See buffer.h. */
	  limit = ((direction > 0)
		   ? BYTE_BUF_CEILING_OF (buf, pos - dirlen + 1) - 1
		   : BYTE_BUF_FLOOR_OF (buf, pos - dirlen));
	  limit = ((direction > 0)
		   ? min (limit + len, lim - 1)
		   : max (limit - len, lim));
	  /* LIMIT is now the last value POS can have
	     and still be valid for a possible match.  */
	  while (1)
	    {
	      /* This loop can be coded for space rather than
		 speed because it will usually run only once.
		 (the reach is at most len + 21, and typically
		 does not exceed len) */
	      while ((limit - pos) * direction >= 0)
		/* *not* BYTE_BUF_FETCH_CHAR.  We are working here
		   with bytes, not characters. */
		pos += BM_tab[*BYTE_BUF_BYTE_ADDRESS_NO_VERIFY (buf, pos)];
	      /* now run the same tests to distinguish going off
		 the end, a match or a phony match. */
	      if ((pos - limit) * direction <= len)
		break;	/* ran off the end */
	      /* Found what might be a match.
		 Set POS back to last (first if reverse) char pos.  */
	      pos -= infinity;
	      i = dirlen - direction;
	      while ((i -= direction) + direction != 0)
		{
#ifdef MULE
		  Ichar ch;
		  Ibyte *ptr;
		  int dotrans;

		  pos -= direction;
		  ptr = BYTE_BUF_BYTE_ADDRESS_NO_VERIFY (buf, pos);
		  dotrans = (ptr == tail_end_ptr
			     || ibyte_first_byte_p (ptr[1]));
		  if (dotrans)
		    {
		      int k = 0;
		      Ibyte *ptrq = ptr;
		      while (1)
			{
			  if (ibyte_first_byte_p (*ptrq))
			    break;
			  if (translate_prev[k++] != *--ptrq)
			    {
			      dotrans = 0;
			      break;
			    }
			}
		    }
		  if (dotrans)
		    ch = simple_translate[*ptr];
		  else
		    ch = *ptr;
		  if (pat[i] != ch)
		    break;
		      
#else
		  pos -= direction;
		  if (pat[i] !=
		      TRANSLATE (trt,
				 *BYTE_BUF_BYTE_ADDRESS_NO_VERIFY (buf, pos)))
		    break;
#endif
		}
	      /* Above loop has moved POS part or all the way back
		 to the first char pos (last char pos if reverse).
		 Set it once again at the last (first if reverse)
		 char.  */
	      pos += dirlen - i- direction;
	      if (i + direction == 0)
		{
                  Bytebpos bytstart;
		  pos -= direction;

                  bytstart = (pos + ((direction > 0) ? 1 - len : 0));
                  structure_checking_assert (search_regs.num_regs > 0);

                  search_regs.start[0] = bytstart;
                  search_regs.end[0] = bytstart + len;
                  search_regs.num_regs = 1;

                  set_lisp_search_registers (wrap_buffer (buf), &search_regs);

		  if ((n -= direction) != 0)
		    pos += dirlen; /* to resume search */
		  else if (direction > 0)
                    {
                      return search_regs.end[0];
                    }
                  else
                    {
                      return search_regs.start[0];
                    }
		}
	      else
		pos += stride_for_teases;
	    }
	}
      /* We have done one clump.  Can we continue? */
      if ((lim - pos) * direction < 0)
	return (0 - n) * direction;
    }
  return pos;
}

/* Given a string of words separated by word delimiters,
   compute a regexp that matches those exact words
   separated by arbitrary punctuation.  */

static Lisp_Object
wordify (Lisp_Object buffer, Lisp_Object string)
{
  Charcount i, len;
  EMACS_INT punct_count = 0, word_count = 0;
  struct buffer *buf = decode_buffer (buffer, 0);
  Lisp_Object syntax_table = BUFFER_MIRROR_SYNTAX_TABLE (buf);

  CHECK_STRING (string);
  len = string_char_length (string);

  for (i = 0; i < len; i++)
    if (!WORD_SYNTAX_P (syntax_table, string_ichar (string, i)))
      {
	punct_count++;
	if (i > 0 && WORD_SYNTAX_P (syntax_table,
				    string_ichar (string, i - 1)))
          word_count++;
      }
  if (WORD_SYNTAX_P (syntax_table, string_ichar (string, len - 1)))
    word_count++;
  if (!word_count) return build_ascstring ("");

  {
    /* The following value is an upper bound on the amount of storage we
       need.  In non-Mule, it is exact. */
    Ibyte *storage =
      alloca_ibytes (XSTRING_LENGTH (string) - punct_count +
                          5 * (word_count - 1) + 4);
    Ibyte *o = storage;

    *o++ = '\\';
    *o++ = 'b';

    for (i = 0; i < len; i++)
      {
	Ichar ch = string_ichar (string, i);

	if (WORD_SYNTAX_P (syntax_table, ch))
	  o += set_itext_ichar (o, ch);
	else if (i > 0
		 && WORD_SYNTAX_P (syntax_table,
				   string_ichar (string, i - 1))
		 && --word_count)
	  {
	    *o++ = '\\';
	    *o++ = 'W';
	    *o++ = '\\';
	    *o++ = 'W';
	    *o++ = '*';
	  }
      }

    *o++ = '\\';
    *o++ = 'b';

    return make_string (storage, o - storage);
  }
}

DEFUN ("search-backward", Fsearch_backward, 1, 5, "sSearch backward: ", /*
Search backward from point for STRING.
Set point to the beginning of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend before that position.
The value nil is equivalent to (point-min).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.
*/
       (string, limit, noerror, count, buffer))
{
  return search_command (string, limit, noerror, count, buffer, -1, 0, 0);
}

DEFUN ("search-forward", Fsearch_forward, 1, 5, "sSearch: ", /*
Search forward from point for STRING.
Set point to the end of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend after that position.  The
value nil is equivalent to (point-max).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.
*/
       (string, limit, noerror, count, buffer))
{
  return search_command (string, limit, noerror, count, buffer, 1, 0, 0);
}

DEFUN ("word-search-backward", Fword_search_backward, 1, 5,
       "sWord search backward: ", /*
Search backward from point for STRING, ignoring differences in punctuation.
Set point to the beginning of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend before that position.
The value nil is equivalent to (point-min).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.
*/
       (string, limit, noerror, count, buffer))
{
  return search_command (wordify (buffer, string), limit, noerror, count,
			 buffer, -1, 1, 0);
}

DEFUN ("word-search-forward", Fword_search_forward, 1, 5, "sWord search: ", /*
Search forward from point for STRING, ignoring differences in punctuation.
Set point to the end of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend after that position.  The
value nil is equivalent to (point-max).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.
*/
       (string, limit, noerror, count, buffer))
{
  return search_command (wordify (buffer, string), limit, noerror, count,
			 buffer, 1, 1, 0);
}

DEFUN ("re-search-backward", Fre_search_backward, 1, 5,
       "sRE search backward: ", /*
Search backward from point for match for regular expression REGEXP.
Set point to the beginning of the match, and return point.
The match found is the one starting last in the buffer
and yet ending before the origin of the search.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend before that position.
The value nil is equivalent to (point-min).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails, the
match data from the previous successful match are preserved. Wrap your code in
the macro `save-match-data' if you prefer to always preserve the match data
for other code, an approach which will reduce the amount of bugs users see.
*/
       (regexp, limit, noerror, count, buffer))
{
  return search_command (regexp, limit, noerror, count, buffer, -1, 1, 0);
}

DEFUN ("re-search-forward", Fre_search_forward, 1, 5, "sRE search: ", /*
Search forward from point for regular expression REGEXP.
Set point to the end of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend after that position.  The
value nil is equivalent to (point-max).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails,
the match data from the previous successful match are preserved.  If you have
no need for the match data, call `looking-at-p' instead, which always
preserves the match data.  See also `save-match-data'.

See also the function `replace-match'.
*/
       (regexp, limit, noerror, count, buffer))
{
  return search_command (regexp, limit, noerror, count, buffer, 1, 1, 0);
}

DEFUN ("posix-search-backward", Fposix_search_backward, 1, 5,
       "sPosix search backward: ", /*
Search backward from point for match for regular expression REGEXP.
Find the longest match in accord with Posix regular expression rules.
Set point to the beginning of the match, and return point.
The match found is the one starting last in the buffer
and yet ending before the origin of the search.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend before that position.
The value nil is equivalent to (point-min).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails,
the match data from the previous successful match are preserved.  If you have
no need for the match data, call `looking-at-p' instead, which always
preserves the match data.  See also `save-match-data'.
*/
       (regexp, limit, noerror, count, buffer))
{
  return search_command (regexp, limit, noerror, count, buffer, -1, 1, 1);
}

DEFUN ("posix-search-forward", Fposix_search_forward, 1, 5, "sPosix search: ", /*
Search forward from point for regular expression REGEXP.
Find the longest match in accord with Posix regular expression rules.
Set point to the end of the occurrence found, and return point.

Optional second argument LIMIT bounds the search; it is a buffer
position.  The match found must not extend after that position.  The
value nil is equivalent to (point-max).

Optional third argument NOERROR, if t, means just return nil (no
error) if the search fails.  If neither nil nor t, set point to LIMIT
and return nil.

Optional fourth argument COUNT is a repeat count--search for
successive occurrences.

Optional fifth argument BUFFER specifies the buffer to search in and
defaults to the current buffer.

When the match is successful, this function modifies the match data that
`match-string', `replace-match' and friends access.  If the match fails,
the match data from the previous successful match are preserved.  If you have
no need for the match data, call `looking-at-p' instead, which always
preserves the match data.  See also `save-match-data'.
*/
       (regexp, limit, noerror, count, buffer))
{
  return search_command (regexp, limit, noerror, count, buffer, 1, 1, 1);
}


DEFUN ("replace-match", Freplace_match, 1, 5, 0, /*
Replace text matched by last search with REPLACEMENT.
Leaves point at end of replacement text.
Optional boolean FIXEDCASE inhibits matching case of REPLACEMENT to source.
Optional boolean LITERAL inhibits interpretation of escape sequences.
Optional STRING provides the source text to replace.
Optional STRBUFFER may be a buffer, providing match context, or an integer
 specifying the subexpression to replace.

If FIXEDCASE is non-nil, do not alter case of replacement text.
Otherwise maybe capitalize the whole text, or maybe just word initials,
based on the replaced text.
If the replaced text has only capital letters and has at least one
multiletter word, convert REPLACEMENT to all caps.
If the replaced text has at least one word starting with a capital letter,
then capitalize each word in REPLACEMENT.

If LITERAL is non-nil, insert REPLACEMENT literally.
Otherwise treat `\\' as special:
  `\\&' in REPLACEMENT means substitute original matched text.
  `\\N' means substitute what matched the Nth `\\(...\\)'.
       If Nth parens didn't match, substitute nothing.
  `\\\\' means insert one `\\'.
  `\\u' means upcase the next character.
  `\\l' means downcase the next character.
  `\\U' means begin upcasing all following characters.
  `\\L' means begin downcasing all following characters.
  `\\E' means terminate the effect of any `\\U' or `\\L'.
  Case changes made with `\\u', `\\l', `\\U', and `\\L' override
  all other case changes that may be made in the replaced text.

If non-nil, STRING is the source string, and a new string with the specified
replacements is created and returned.  Otherwise the current buffer is the
source text.

If non-nil, STRBUFFER may be an integer, interpreted as the index of the
subexpression to replace in the source text, or a buffer to provide the
syntax table and case table.  If nil, then the \"subexpression\" is 0, i.e.,
the whole match, and the current buffer provides the syntax and case tables.
If STRING is nil, STRBUFFER must be nil or an integer.

Specifying a subexpression is only useful after a regular expression match,
since a fixed string search has no non-trivial subexpressions.

It is not possible to specify both a buffer and a subexpression.  If that is
desired, the idiom `(with-current-buffer BUFFER (replace-match ... INTEGER))'
may be appropriate.

If STRING is nil but the last thing matched (or searched) was a string, or
STRING is a string but the last thing matched was a buffer, an
`invalid-argument' error will be signaled.  XEmacs checks and warns if the
last thing searched is not the source string, but it does not currently error.
It is unusual to have it useful to to use a different string as source, and so
this warning generally reflects a bug.

If no match (including searches) has been successful or the requested
subexpression was not matched, an `args-out-of-range' error will be
signaled.  If the match data have been cleared, or if no match has ever been
conducted in this instance of XEmacs, an `invalid-operation' error will be
signaled.
*/
       (replacement, fixedcase, literal, string, strbuffer))
{
  /* This function can GC */
  enum { nochange, all_caps, cap_initial } case_action;
  Bytebpos pos, last;
  Boolint some_multiletter_word;
  Boolint some_lowercase;
  Boolint some_uppercase;
  Boolint some_nonuppercase_initial;
  Ichar c, prevc;
  Lisp_Object match_context_buffer, buffer;
  struct buffer *match_context_buf, *buf = NULL;
  Lisp_Object syntax_table, registers_vector, staging = Qnil;
  /* A better default than Qnil, because write_lisp_string () will interpret
     Qnil as Vstandard_output and silently print rather than erroring. */
  Lisp_Object accum = Qunbound; 
  int mc_count;
  int case_escapes_seen = 0;
  Elemcount sub = 0;
  Charbpos substartchar, subendchar;
  Charcount inslen;
  struct gcpro gcpro1, gcpro2;
  
  CHECK_STRING (replacement);

  /* Because GNU decided to be incompatible here, we support the following
     baroque and bogus API for the STRING and STRBUFFER arguments:
          types            interpretations
     STRING   STRBUFFER   STRING   STRBUFFER
     nil      nil         none     0 = index of subexpression to replace
     nil      integer     none     index of subexpression to replace
     nil      other       ***** error *****
     string   nil         source   buffer saved at match time (as a property
                                   of match zero) provides syntax table.
                                   subexpression = 0 (whole match)
     string   buffer      source   buffer provides syntax table
                                   subexpression = 0 (whole match)
     string   integer     source   current buffer provides syntax table
                                   subexpression = STRBUFFER
     string   other       ***** error *****
  */

  /* Do STRBUFFER first; if STRING is nil, we'll overwrite BUF and BUFFER. */
  if (NILP (strbuffer) || BUFFERP (strbuffer))
    {
      match_context_buf = decode_buffer (strbuffer, 0);
    }
  else if (!NILP (strbuffer))
    {
      CHECK_NATNUM (strbuffer);

      if (BIGNUMP (strbuffer)
          || ((sub = XFIXNUM (strbuffer)),
              sub >= XFIXNUM (XCAR (Vsearch_registers)))
          || NILP (XVECTOR_DATA (XCDR (Vsearch_registers))[sub])
          || (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[sub])
              && extent_detached_p
              (XEXTENT (XVECTOR_DATA (XCDR (Vsearch_registers))[sub]))))
        {
          invalid_argument ("match data register not set", strbuffer);
        }

      match_context_buf = current_buffer;
    }
  else
    invalid_argument ("STRBUFFER must be nil, a buffer, or an integer",
		      strbuffer);

  match_context_buffer = wrap_buffer (match_context_buf);

  case_action = nochange;	/* We tried an initialization */
				/* but some C compilers blew it */

  if (EQ (Qzero, XCAR (Vsearch_registers)))
    {
      signal_error (Qinvalid_operation,
                    "replace-match called with no match data available",
                    Qunbound);
    }

  if (!NILP (string))
    {
      CHECK_STRING (string);
      if (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[0]))
	{
	  if (!STRINGP (extent_object
			(XEXTENT (XVECTOR_DATA
				  (XCDR (Vsearch_registers))[0]))))
	    {
	      invalid_argument ("last thing matched was not a string",
				extent_object
				(XEXTENT (XVECTOR_DATA
					  (XCDR (Vsearch_registers)) [0])));
	    }

	  if (!EQ (match_context_buffer,
		   Fextent_property (XVECTOR_DATA
				     (XCDR (Vsearch_registers))[0],
				     Qcontext,
				     match_context_buffer)))
	    {
	      if (BUFFERP (strbuffer))
		{
		  search_warn_or_error
		    (Qerror,
		     "Likely bug: #'replace-match called with match context "
                     "buffer (arg STRBUFFER) %S, match data have a distinct "
                     "match context buffer %S", strbuffer,
                     Fextent_property (XVECTOR_DATA (XCDR
                                                     (Vsearch_registers))[0],
                                       Qcontext,
                                       match_context_buffer));
		  /* For better compatibility, do the wrong thing. */
		}
	      else
		{
		  match_context_buffer
		    = Fextent_property (XVECTOR_DATA (XCDR
						      (Vsearch_registers))[0],
					Qcontext, match_context_buffer);
		  if (BUFFERP (match_context_buffer)
		      && BUFFER_LIVE_P (XBUFFER (match_context_buffer)))
		    {
		      match_context_buf = XBUFFER (match_context_buffer);
		    }
		  else
		    {
		      match_context_buffer = wrap_buffer (current_buffer);
		      match_context_buf = current_buffer;
		    }
		}
	    }
	}
      canonicalize_lisp_search_registers_for_replace (string);
    }
  else
    {
      if (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[0])
          && !BUFFERP (extent_object
                       (XEXTENT (XVECTOR_DATA
                                 (XCDR (Vsearch_registers))[0]))))
        {
          invalid_argument ("last thing matched was not a buffer",
                            extent_object (XEXTENT (XVECTOR_DATA
                                                    (XCDR (Vsearch_registers))
                                                    [0])));
        }
      buffer = extent_object (XEXTENT (XVECTOR_DATA
                                       (XCDR (Vsearch_registers))[0]));
      buf = XBUFFER (buffer);
      canonicalize_lisp_search_registers_for_replace (buffer);
    }

  syntax_table = BUFFER_MIRROR_SYNTAX_TABLE (match_context_buf);

  registers_vector = XCDR (Vsearch_registers);

  if (NILP (string))
    {
      if (extent_endpoint_byte
          (XEXTENT (XVECTOR_DATA (registers_vector)[sub]), 0)
          < BYTE_BUF_BEGV (buf))
        {
          args_out_of_range
            (wrap_buffer (buf), 
             Fextent_start_position (XVECTOR_DATA (registers_vector)[sub],
                                     Qnil));
        }

      if (extent_endpoint_byte
          (XEXTENT (XVECTOR_DATA (registers_vector)[sub]), 1)
          > BYTE_BUF_ZV (buf))
        {
          args_out_of_range
            (wrap_buffer (buf), 
             Fextent_end_position (XVECTOR_DATA (registers_vector)[sub],
                                   Qnil));
        }
    }

  GCPRO2 (staging, accum);

  if (NILP (fixedcase))
    {
      /* Decide how to casify by examining the matched text. */
      last = extent_endpoint_byte (XEXTENT (XVECTOR_DATA
                                            (registers_vector)[sub]), 1);
      prevc = '\n';
      case_action = all_caps;

      /* some_multiletter_word is set nonzero if any original word
	 is more than one letter long. */
      some_multiletter_word = 0;
      some_lowercase = 0;
      some_nonuppercase_initial = 0;
      some_uppercase = 0;
      
      pos = extent_endpoint_byte (XEXTENT (XVECTOR_DATA
                                           (registers_vector)[sub]),
                                  0);
      while (pos < last)
	{
	  if (NILP (string))
            {
              c = BYTE_BUF_FETCH_CHAR (buf, pos);
              INC_BYTEBPOS (buf, pos);
            }
	  else
            {
              c = itext_ichar (string_byte_addr (string, pos));
              pos = next_string_index (string, pos);
            }

	  if (LOWERCASEP (match_context_buf, c))
	    {
	      /* Cannot be all caps if any original char is lower case */

	      some_lowercase = 1;
	      if (!WORD_SYNTAX_P (syntax_table, prevc))
		some_nonuppercase_initial = 1;
	      else
		some_multiletter_word = 1;
	    }
	  else if (!NOCASEP (match_context_buf, c))
	    {
	      some_uppercase = 1;
	      if (!WORD_SYNTAX_P (syntax_table, prevc))
		;
	      else
		some_multiletter_word = 1;
	    }
	  else
	    {
	      /* If the initial is a caseless word constituent,
		 treat that like a lowercase initial.  */
	      if (!WORD_SYNTAX_P (syntax_table, prevc))
		some_nonuppercase_initial = 1;
	    }

	  prevc = c;
	}

      /* Convert to all caps if the old text is all caps
	 and has at least one multiletter word.  */
      if (! some_lowercase && some_multiletter_word)
	case_action = all_caps;
      /* Capitalize each word, if the old text has all capitalized words.  */
      else if (!some_nonuppercase_initial && some_multiletter_word)
	case_action = cap_initial;
      else if (!some_nonuppercase_initial && some_uppercase)
	/* Should x -> yz, operating on X, give Yz or YZ?
	   We'll assume the latter.  */
	case_action = all_caps;
      else
	case_action = nochange;
    }

  /* Do regexp substitution and set up for \U, \E case transforms into
     REPLACEMENT if desired.  */
  if (NILP (literal))
    {
      Bytecount stlen = XSTRING_LENGTH (replacement);
      Bytecount strpos;
      /* XEmacs change: rewrote this loop somewhat to make it cleaner.  Also
         added \U, \E, etc. */
      Bytecount literal_start = 0;

      /* OK, the basic idea here is that we scan through the replacement
         string until we find a backslash, which represents a substring of the
         original string to be substituted.  We then append onto ACCUM the
         literal text before the backslash (LASTPOS marks the beginning of
         this) followed by the substring of the original string that needs to
         be inserted. */
      for (strpos = 0; strpos < stlen;
           strpos = next_string_index (replacement, strpos))
        {
          /* If LITERAL_END is set, we've encountered a backslash
             (the end of literal text to be inserted). */
          Bytecount literal_end = -1;
          /* If SUBSTART is set, we need to also insert the text from SUBSTART
             to SUBEND in the original string (or buffer). */
          Bytecount substart = -1;
          Bytecount subend = -1;
          Lisp_Object casing = Qnil;

          c = itext_ichar (string_byte_addr (replacement, strpos));
          if (c == '\\' && strpos < stlen - 1)
            {
              strpos = next_string_index (replacement, strpos);
              c = itext_ichar (string_byte_addr (replacement, strpos));
              switch (c)
                {
                case '&':
                  literal_end = prev_string_index (replacement, strpos);
                  substart = extent_endpoint_byte
                    (XEXTENT (XVECTOR_DATA (registers_vector)[0]), 0);
                  subend = extent_endpoint_byte
                    (XEXTENT (XVECTOR_DATA (registers_vector)[0]), 1);
                  break;

                case '\\':
                  /* So we get just one backslash. */
                  literal_end = strpos;
                  break;

                case '1': case '2': case '3': case '4': case '5':
                case '6': case '7': case '8': case '9':
                  {
                    const Ibyte *charpos
                      = string_byte_addr (replacement, strpos);
                    Ibyte *regend = NULL;
                    Bytecount limit = min (XSTRING_LENGTH (replacement)
                                           - (charpos -
                                              XSTRING_DATA (replacement)),
                                           /* most-positive-fixnum on 32
                                              bit is ten decimal digits,
                                              nine will keep us in fixnum
                                              territory. */
                                           9);
                    Lisp_Object regno;
                    Fixnum regnoing;

                    /* Parse the longest backreference we can, but don't
                       produce a bignum, that can't correspond to a
                       backreference and would needlessly complicate code
                       further down.  */
                    regno = parse_integer (charpos, &regend, limit, 10, 1,
                                           /* Don't accept non-ASCII
                                              decimal digits. See the
                                              reasoning in regex.c. */
                                           Vdigit_fixnum_ascii);

                    if (FIXNUMP (regno) &&
                        ((regnoing = XREALFIXNUM (regno), regnoing > -1)))
                      {
                        Elemcount num_regs
                          = XFIXNUM (XCAR (Vsearch_registers));
                        /* Progressively divide down the backreference until
                           we find one that corresponds to an existing
                           register. */
                        while (regnoing > 10 && regnoing >= num_regs)
                          {
                            DEC_IBYTEPTR (regend);
                            regnoing /= 10;
                          }

                        if (regnoing < num_regs
                            && EXTENTP (XVECTOR_DATA (registers_vector)
                                        [regnoing])
                            && !extent_detached_p
                            (XEXTENT (XVECTOR_DATA (registers_vector)
                                      [regnoing])))
                          {
                            literal_end
                              = prev_string_index (replacement, strpos);
                            strpos
                              = prev_string_index (replacement,
                                                   regend -
                                                   XSTRING_DATA
                                                   (replacement));
                            substart = extent_endpoint_byte
                              (XEXTENT (XVECTOR_DATA (registers_vector)
                                        [regnoing]), 0);
                            subend = extent_endpoint_byte
                              (XEXTENT (XVECTOR_DATA (registers_vector)
                                        [regnoing]), 1);
                          }
                      }
                    break;
                  }
                case 'U': case 'u': case 'L': case 'l': case 'E':
                  {
                    /* Keep track of all case changes requested, but don't
                       make them now.  Do them later so we override everything
                       else. */
                    literal_end
                      = prev_string_index (replacement, strpos);

                    /* Don't yet attach the extent to ACCUM, its offset won't
                       be correct. */
                    casing = Fmake_extent (Qnil, Qnil, Qnil);
                    Fset_extent_property (casing, Vcase_flag_symbol,
                                          make_char (c));
                    Fset_extent_property (casing, Qduplicable, Qt);
                    case_escapes_seen++;
                  }
                }
            }

          if (literal_end >= 0)
            {
              if (UNBOUNDP (accum))
                {
                  accum = make_resizing_buffer_output_stream ();
                }

              if (literal_end != literal_start)
                {
                  write_lisp_string (accum, replacement, literal_start,
                                     literal_end - literal_start);
                }

              if (!NILP (casing))
                {
                  set_extent_endpoints (XEXTENT (casing),
                                        stream_extent_position (accum),
                                        stream_extent_position (accum),
                                        accum);
                  casing = Qnil;
                }

              if (substart >= 0 && subend != substart)
                {
                  if (NILP (string))
                    {
                      staging = make_string_from_buffer (buf, substart,
                                                         subend - substart);
                      write_lisp_string (accum, staging, 0,
                                         XSTRING_LENGTH (staging));
                      staging = Qnil;
                    }
                  else
                    {
                      write_lisp_string (accum, string, substart,
                                         subend - substart);
                    }
                }

              literal_start = next_string_index (replacement, strpos);
            }
        }

      if (!UNBOUNDP (accum))
        {
          if (literal_start < XSTRING_LENGTH (replacement))
            {
              write_lisp_string (accum, replacement, literal_start,
                                 XSTRING_LENGTH (replacement)
                                 - literal_start);
            }

          replacement = Fget_output_stream_string (accum);
        }
    }

  if (case_action == all_caps)
    replacement = Fupcase (replacement, match_context_buffer);
  else if (case_action == cap_initial)
    replacement = Fupcase_initials (replacement, match_context_buffer);

  /* Now finally, we need to process the \U's, \E's, etc. */
  if (case_escapes_seen)
    {
      int i = 0;
      Ichar cur_action = 'E';
      Bytecount stlen = XSTRING_LENGTH (replacement);
      Bytecount strpos;
      Ibyte *byte_staging = alloca_ibytes (stlen * MAX_ICHAR_LEN);
      Ibyte *byte_staging_ptr = byte_staging;
      Boolint modifiedp = 0, force_new_string = 0;
      Lisp_Object escape_extent = Qnil, escape_flag = Qnil;

      for (strpos = 0; strpos < stlen;
           strpos = next_string_index (replacement, strpos))
        {
          Ichar curchar
            = itext_ichar (string_byte_addr (replacement, strpos));
          Ichar newchar = -1;

          if (i < case_escapes_seen)
            {
              escape_extent = extent_at (strpos, replacement,
                                         Vcase_flag_symbol, 0,
                                         EXTENT_AT_AT, 0);
              if (EXTENTP (escape_extent))
                {
                  escape_flag = Fextent_property (escape_extent,
                                                  Vcase_flag_symbol, Qnil);
                  Fdetach_extent (escape_extent);
                }
              else
                {
                  escape_flag = Qnil;
                }
            }

          if (CHARP (escape_flag))
            {
              Ichar new_action = XCHAR (escape_flag);
              i++;
              if (new_action == 'u')
                newchar = UPCASE (match_context_buf, curchar);
              else if (new_action == 'l')
                newchar = DOWNCASE (match_context_buf, curchar);
              else
                cur_action = new_action;

              Fdelete_extent (escape_extent);
              if (i == case_escapes_seen && purify_flag)
                {
		  XWEAK_LIST_LIST (Vstrings_to_nuke_extents)
		    = Fcons (replacement,
			     XWEAK_LIST_LIST (Vstrings_to_nuke_extents));
                }
            }
          if (newchar == -1)
            {
              if (cur_action == 'U')
                newchar = UPCASE (match_context_buf, curchar);
              else if (cur_action == 'L')
                newchar = DOWNCASE (match_context_buf, curchar);
              else
                newchar = curchar;
            }
          if (newchar != curchar)
            {
              if (ichar_len (newchar) != ichar_len (curchar))
                {
                  /* Need to do this explicitly now, can't check the byte
                     length at the end and compare, one character getting
                     longer may have been evened out by another character
                     getting shorter, and that would make the extent info
                     inaccurate. */
                  force_new_string = 1;
                }

              modifiedp = 1;
            }
          
          byte_staging_ptr += set_itext_ichar (byte_staging_ptr, newchar);
        }
      
      if (modifiedp)
        {
          if (!force_new_string)
            {
              memcpy (XSTRING_DATA (replacement), byte_staging,
                      byte_staging_ptr - byte_staging);
              init_string_ascii_end (replacement);
              bump_string_modiff (replacement);
              sledgehammer_check_ascii_end (replacement);
            }
          else
            {
              Lisp_Object repl
                = make_string (byte_staging, byte_staging_ptr - byte_staging);
              stretch_string_extents (repl, replacement, 0, 0,
                                      XSTRING_LENGTH (replacement),
                                      XSTRING_LENGTH (repl));
              replacement = repl;
            }
        }
    }

  /* Do replacement in a string.  */
  if (!NILP (string))
    {
      if (UNBOUNDP (accum))
        {
          accum = make_resizing_buffer_output_stream ();
        }

      write_lisp_string (accum, string, 0,
                         extent_endpoint_byte
                         (XEXTENT (XVECTOR_DATA (registers_vector) [sub]),
                          0));
      write_lisp_string (accum, replacement, 0,
                         XSTRING_LENGTH (replacement));
      write_lisp_string (accum, string, 
                         extent_endpoint_byte
                         (XEXTENT (XVECTOR_DATA (registers_vector) [sub]),
                          1),
                         XSTRING_LENGTH (string) - 
                         extent_endpoint_byte
                         (XEXTENT (XVECTOR_DATA (registers_vector) [sub]),
                          1));
      UNGCPRO;
      return resizing_buffer_to_lisp_string (XLSTREAM (accum));
    }

  substartchar
    = XFIXNUM (Fextent_start_position (XVECTOR_DATA (registers_vector)[sub],
                                       Qnil));
  subendchar
    = XFIXNUM (Fextent_end_position (XVECTOR_DATA (registers_vector)[sub],
                                     Qnil));

  mc_count = begin_multiple_change (buf, substartchar, subendchar);

  /* We insert the replacement text before the old text, and then delete the
     original text.  This means that markers at the beginning or end of the
     original will float to the corresponding position in the replacement.  */
  BUF_SET_PT (buf, substartchar);
  Finsert (1, &replacement);

  inslen = BUF_PT (buf) - substartchar;
  buffer_delete_range (buf, substartchar + inslen,
                       subendchar + inslen, 0);
  end_multiple_change (buf, mc_count);

  UNGCPRO;
  return Qnil;
}

static Lisp_Object
match_limit (Lisp_Object num, Boolint beginningp)
{
  Elemcount n = 0;

  CHECK_NATNUM (num);
  if (BIGNUMP (num) || ((n = XFIXNUM (num)),
                        n >= XFIXNUM (XCAR (Vsearch_registers))))
    {
      return Qnil;
    }

  if (EXTENTP (XVECTOR_DATA (XCDR (Vsearch_registers))[n]))
    {
      EXTENT ext = XEXTENT (XVECTOR_DATA (XCDR (Vsearch_registers))[n]);
      Lisp_Object extent_object;

      extent_object = extent_object (ext);

      if (BUFFERP (extent_object) &&
          !EQ (extent_object, wrap_buffer (current_buffer)))
        {
          search_warn_or_error
            (Qerror,
             beginningp ? "Likely bug: (match-beginning %d) called with "
             "current buffer %S, match data reflect buffer %S"
             : "Likely bug: (match-end %d) called with current buffer %S, "
             "match data reflect buffer %S",
             num, wrap_buffer (current_buffer), extent_object);
        }


      if (extent_detached_p (ext))
        {
          return Qnil;
        }

      return make_fixnum (extent_endpoint_char (ext, !beginningp));
    }

  if (CONSP (XVECTOR_DATA (XCDR (Vsearch_registers))[n]))
    {
      return beginningp ? XCAR (XVECTOR_DATA (XCDR (Vsearch_registers))[n])
        : XCDR (XVECTOR_DATA (XCDR (Vsearch_registers))[n]);
    }

  return Qnil;
}

DEFUN ("match-beginning", Fmatch_beginning, 1, 1, 0, /*
Return position of start of text matched by last regexp search.
NUM, specifies which parenthesized expression in the last regexp.
 Value is nil if NUMth pair didn't match, or there were less than NUM pairs.
Zero means the entire text matched by the whole regexp or whole string.
*/
       (num))
{
  return match_limit (num, 1);
}

DEFUN ("match-end", Fmatch_end, 1, 1, 0, /*
Return position of end of text matched by last regexp search.
NUM specifies which parenthesized expression in the last regexp.
 Value is nil if NUMth pair didn't match, or there were less than NUM pairs.
Zero means the entire text matched by the whole regexp or whole string.
*/
       (num))
{
  return match_limit (num, 0);
}

DEFUN ("match-string", Fmatch_string, 1, 2, 0, /*
Return string of text matched by last search.

NUM specifies which parenthesized expression in the last regexp.
Value is nil if NUMth pair didn't match, or there were fewer than NUM pairs.
Zero means the entire text matched by the whole regexp or whole string.

STRING should be given if the last search was by `string-match' on STRING.

In this implementation, `match-string' can be significantly less costly than
`match-beginning' or `match-end' in large buffers, since it has no need to
convert the byte position of the underlying buffer text to the character
position exposed to Lisp, an O(N) operation where N is the buffer text
position. Whatever the implementation, it usually gives more natural code.

This optimization will be hampered if the programmer attempts to manipulate
the match data by hand, using `store-match-data' or `match-data'.  In general
the only operation on the match data that is likely to be reliable and
performant across implementations is `save-match-data', which see.
*/
       (num, string))
{
  Lisp_Object obj;
  Fixnum n = -1;

  if (!FIXNUMP (num) || ((n = XREALFIXNUM (num)), n < 0))
    {
      CHECK_NATNUM (num);
      return Qnil;
    }

  if (n >= XFIXNUM (XCAR (Vsearch_registers)))
    {
      return Qnil;
    }

  obj = XVECTOR_DATA (XCDR (Vsearch_registers))[n];

  if (EXTENTP (obj))
    {
      Lisp_Object extent_object;

      if (extent_detached_p (XEXTENT (obj)))
        {
          return Qnil;
        }

      extent_object = extent_object (XEXTENT (obj));

      if (BUFFERP (extent_object))
        {
          if (EQ (extent_object, wrap_buffer (current_buffer)))
            {
              return make_string_from_buffer
                (XBUFFER (extent_object),
                 extent_endpoint_byte (XEXTENT (obj), 0),
                 extent_endpoint_byte (XEXTENT (obj), 1) -
                 extent_endpoint_byte (XEXTENT (obj), 0));
            }
          else
            {
              search_warn_or_error
                (Qerror,
                 "Likely bug: (match-string %d) called with current buffer "
                 "%S match data reflect buffer %S", num,
                 wrap_buffer (current_buffer), extent_object);
              /* Give the wrong result, and the byte->char hit, for
                 compatibility's sake. */
              return Fbuffer_substring (Fextent_start_position (obj, Qnil),
                                        Fextent_end_position (obj, Qnil),
                                        wrap_buffer (current_buffer));
            }
        }
      else if (STRINGP (extent_object))
        {
          if (EQ (extent_object, string))
            {
              return make_string (XSTRING_DATA (extent_object) + 
                                  extent_endpoint_byte (XEXTENT (obj), 0),
                                  extent_endpoint_byte (XEXTENT (obj), 1) -
                                  extent_endpoint_byte (XEXTENT (obj), 0));
            }
          else if (NILP (string))
            {
              search_warn_or_error
                (Qinfo,
                 "(match-string %d ...) called with nil STRING, match data "
                 "reflect string `%s', this code will not work on GNU or "
                 "old XEmacs", num, extent_object);
              return make_string (XSTRING_DATA (extent_object) + 
                                  extent_endpoint_byte (XEXTENT (obj), 0),
                                  extent_endpoint_byte (XEXTENT (obj), 1) -
                                  extent_endpoint_byte (XEXTENT (obj), 0));
            }
          else
            {
	      if (!internal_equal (string, extent_object, 0))
		{
		  search_warn_or_error
		    (Qerror,
                     "Likely bug: (match-string %d STRING) called with "
                     "STRING %S, match data reflect distinct string %S",
                     num, string, extent_object);
		}
              return Fsubseq (string,
                              Fextent_start_position (obj, Qnil),
                              Fextent_end_position (obj, Qnil));
            }
        }
      else if (LSTREAMP (extent_object))
        {
          Lisp_Object result
            = make_string (resizing_buffer_stream_ptr (XLSTREAM (extent_object))
                           + extent_endpoint_byte (XEXTENT (obj), 0),
                           extent_endpoint_byte (XEXTENT (obj), 1) -
                           extent_endpoint_byte (XEXTENT (obj), 0));
          copy_string_extents (result, extent_object, 0,
                               extent_endpoint_byte (XEXTENT (obj), 0),
                               XSTRING_LENGTH (result));
          return result;
        }
      else
        {
          assert (0);
        }
    }

  if (!NILP (string) && CONSP (obj))
    {
      return Fsubseq (string, XCAR (obj), XCDR (obj));
    }

  if (CONSP (obj))
    {
      return Fbuffer_substring (XCAR (obj), XCDR (obj),
                                wrap_buffer (current_buffer));
    }

  return Qnil;
}

DEFUN ("match-data-canonical", Fmatch_data_canonical, 0, 0, 0, /*
Return the canonical internal form of the match data.

This is a list of extents, with each extent end position corresponding to the
boundaries of the corresponding subexpression, and the extent's object
corresponding to the object (string or buffer) that was matched. An entry can
also be nil, to indicate the corresponding subexpression did not match.

The returned list will be nil if there is no saved match data available.

If `store-match-data' has been called with a LIST that comprises exclusively
fixnums, there is no way for XEmacs to work out the corresponding object until
`match-string' or `replace-match' is called, but it still needs to store the
match data internally for later access as needed. In that case, this function
returns those stored fixnums as elements comprising a cons of integers.

This function returns freshly allocated conses and extents each time, you need
to call `store-match-data' with the result list in order to affect the saved
match data as known to XEmacs.  Modification by side effect will not work.
See also `match-data' and `store-match-data'.
*/
       ())
{
  Elemcount num_regs = XFIXNUM (XCAR (Vsearch_registers));
  Lisp_Object result = Qnil, reg_vector = XCDR (Vsearch_registers), elt;
  Lisp_Object object = Qunbound;
  Boolint nonnil_seen = 0;

  while (num_regs > 0)
    {
      elt = XVECTOR_DATA (reg_vector)[--num_regs];
      if (EXTENTP (elt))
        {
          Lisp_Object elt1;

          if (extent_detached_p (XEXTENT (elt)))
            {
              elt1 = Qnil;
            }
          else
            {
              nonnil_seen = 1;
              if (UNBOUNDP (object))
                {
                  object = extent_object (XEXTENT (elt));
                }

              elt1 = Fcopy_extent (elt, object);
              set_extent_endpoints (XEXTENT (elt1),
                                    extent_endpoint_byte (XEXTENT (elt), 0),
                                    extent_endpoint_byte (XEXTENT (elt), 1),
                                    object);
            }

          if (nonnil_seen)
            {
              result = Fcons (elt1, result);
            }
        }
      else
        {
          structure_checking_assert (LISTP (elt));
          if (CONSP (elt))
            {
              structure_checking_assert (FIXNUMP (XCAR (elt)));
              structure_checking_assert (FIXNUMP (XCDR (elt)));
              elt = Fcons (XCAR (elt), (XCDR (elt)));
              nonnil_seen = 1;
            }

          if (nonnil_seen)
            {
              result = Fcons (elt, result);
            }
        }
    }

  return result;
}

#define PUTF_WITH_REUSE(plist, elt1, elt2) do                 \
    {                                                         \
      Lisp_Object pwr_elt1 = (elt1), pwr_elt2 = (elt2);       \
      if (CONSP (reuse))                                      \
        {                                                     \
          Lisp_Object old_reuse_cdr = XCDR (reuse);           \
                                                              \
          XSETCAR (reuse, pwr_elt2);                          \
          XSETCDR (reuse, plist);                             \
          plist = reuse;                                      \
                                                              \
          reuse = old_reuse_cdr;                              \
                                                              \
          if (CONSP (reuse))                                  \
            {                                                 \
              old_reuse_cdr = XCDR (reuse);                   \
              XSETCAR (reuse, pwr_elt1);                      \
              XSETCDR (reuse, plist);                         \
              plist = reuse;                                  \
              reuse = old_reuse_cdr;                          \
            }                                                 \
          else                                                \
            {                                                 \
              plist = Fcons (pwr_elt1, plist);                \
            }                                                 \
        }                                                     \
      else                                                    \
        {                                                     \
          plist = Fcons (pwr_elt1, Fcons (pwr_elt2, plist));  \
        }                                                     \
    } while (0)

DEFUN ("match-data", Fmatch_data, 0, 2, 0, /*
Return a list containing info on what the last regexp search matched.

Element 2N is `(match-beginning N)'; element 2N + 1 is `(match-end N)'.
All the elements are markers or nil (nil if the Nth pair didn't match)
if the last match was on a buffer; integers or nil if a string was matched.
Use `store-match-data' to reinstate the data in this list.

If INTEGERS (the optional first argument) is non-nil, always use fixnums
\(rather than markers) to represent buffer positions.

If REUSE is a list, reuse it as part of the value.  If REUSE is long enough to
hold all the values, and if INTEGERS is non-nil, no new memory is allocated.

Match offsets are internally stored as extents, and so passing INTEGERS to
this function forces calculation of these offset's character position, an O(N)
operation on the underlying byte position. So any gain in memory use will be
outweighed by poor and worsening performance as buffers get larger, involving
poorly-localized operations that interact poorly with processor caching.

If INTEGERS is non-nil, the returned markers' byte positions are directly set
from the underlying extents' byte end points, avoiding this performance trap.

In general the only operation on the match data that is likely to be reliable
and performant across implementations is `save-match-data', which see.

See also `match-data-canonical', which returns extents, allowing knowledge of
which string was matched to carry over with `save-match-data', and avoiding
the above performance trap for strings as well as integers.
*/
       (integers, reuse))
{
  Elemcount num_regs = XFIXNUM (XCAR (Vsearch_registers));
  Lisp_Object result = Qnil, reg_vector = XCDR (Vsearch_registers), elt;
  Lisp_Object object = Qunbound;
  Boolint nonnil_seen = 0;

  while (num_regs > 0)
    {
      elt = XVECTOR_DATA (reg_vector)[--num_regs];
      if (EXTENTP (elt))
        {
          Lisp_Object elt1, elt2;

          if (extent_detached_p (XEXTENT (elt)))
            {
              elt1 = elt2 = Qnil;
            }
          else
            {
              nonnil_seen = 1;
              if (UNBOUNDP (object))
                {
                  object = extent_object (XEXTENT (elt));
                }

              structure_checking_assert (EQ (object,
                                             extent_object (XEXTENT (elt))));
              
              if (BUFFERP (object) && NILP (integers))
                {
                  elt1 = Fextent_start_position (elt, Qt);
                  elt2 = Fextent_end_position (elt, Qt);
                }
              else
                {
                  elt1 = Fextent_start_position (elt, Qnil);
                  elt2 = Fextent_end_position (elt, Qnil);
                }
            }

          if (nonnil_seen)
            {
              PUTF_WITH_REUSE (result, elt1, elt2);
            }
        }
      else if (NILP (elt))
        {
          if (nonnil_seen)
            {
              PUTF_WITH_REUSE (result, Qnil, Qnil);
            }
        }
      else if (CONSP (elt))
        {
          structure_checking_assert (FIXNUMP (XCAR (elt)));
          structure_checking_assert (FIXNUMP (XCDR (elt)));

          PUTF_WITH_REUSE (result, XCAR (elt), XCDR (elt));

          nonnil_seen = 1;
        }
      else
        {
          structure_checking_assert (0);
        }
    }
  return result;
}

#undef PUTF_WITH_REUSE

static void
store_match_data_fixnums (Lisp_Object list,
                          Lisp_Object *staging,
                          Elemcount *num_regs)
{
  Elemcount ii = 0, expected_regs = (*num_regs) / 2;

  LIST_LOOP_3 (elt, list, tail)
    {
      if (FIXNUMP (elt) && CONSP (XCDR (tail))
          && FIXNUMP (XCAR (XCDR (tail))))
        {
	  Lisp_Object elt0 = elt;
	  Lisp_Object elt1 = XCAR (XCDR (tail));
	  Lisp_Object swap = elt0;

	  if (XREALFIXNUM (elt0) > XREALFIXNUM (elt1))
	    {
              search_warn_or_error
                (Qwarning,
                 "store-match-data: start fixnum %d greater than end fixnum "
                 "%d, possible corruption of saved match data", elt0, elt1);
	      elt0 = elt1;
	      elt1 = swap;
	    }

          if (XREALFIXNUM (elt0) < 0)
            {
	      /* The start of a match being negative has always been a way
		 to indicate no match. 

		 The code above to force whichever value is less to come
		 first means that this is an incompatibility with the old
		 code and with GNU, which are happy to store negative end
		 positions but transform negative start positions to
		 Qnil. */
              staging[ii] = Qnil;
            }
          else
            {
	      staging[ii] = Fcons (elt0, elt1);
            }
          tail = XCDR (tail);
        }
      else if (NILP (elt) && CONSP (XCDR (tail))
               && NILP (XCAR (XCDR (tail))))
        {
          staging[ii] = Qnil;
          tail = XCDR (tail);
        }
      else if (ii < expected_regs)
        {
          staging[ii] = Qnil;
          tail = XCDR (tail);
        }
      ii++;
    }
  *num_regs = ii;
}

static void
store_match_data_markers (Lisp_Object list,
                          Lisp_Object *staging,
                          Elemcount *num_regs)
{
  Lisp_Object obj = Qunbound;
  Elemcount ii = 0, expected_regs = (*num_regs) / 2;

  /* Validate the entries in the list, copying the values into the staging
     array. No need for EXTERNAL_LIST_LOOP_2, we've checked type and
     circularity in our caller. */
  LIST_LOOP_3 (elt, list, tail)
    {
      if (MARKERP (elt) && CONSP (XCDR (tail)) && MARKERP (XCAR (XCDR (tail))))
        {
          if (!UNBOUNDP (obj) && 
              (!EQ (Fmarker_buffer (elt), obj) ||
               !EQ (Fmarker_buffer (XCAR (XCDR (tail))), obj)))
            {
              sferror_2 ("Distinct search objects in match data",
                         obj, Fmarker_buffer (elt));
            }
          obj = Fmarker_buffer (elt);

          if (!EQ (obj, Fmarker_buffer (XCAR (XCDR (tail)))))
            {
              sferror_2 ("Distinct search objects in match data",
                         obj, Fmarker_buffer (XCAR (XCDR (tail))));
            }

	  /* Don't create the extent just yet. */
	  if (marker_byte_position (elt)
	      <= marker_byte_position ((XCAR (XCDR (tail)))))
	    {
	      staging[ii] = Fcons (elt, XCAR (XCDR (tail)));
	    }
	  else
	    {
              search_warn_or_error
                (Qwarning, "store-match-data: start marker %S greater than "
                 "end marker %S, possible corruption of saved match data",
                 elt, XCAR (XCDR (tail)));
	      staging[ii] = Fcons (XCAR (XCDR (tail)), elt);
	    }

          tail = XCDR (tail);
        }
      else if (NILP (elt) && CONSP (XCDR (tail)) && NILP (XCAR (XCDR (tail))))
        {
          staging[ii] = Qnil;
          tail = XCDR (tail);
        }
      else if (ii < expected_regs)
        {
          staging[ii] = Qnil;
          tail = XCDR (tail);
        }
      ii++;
    }
  *num_regs = ii;
}

DEFUN ("store-match-data", Fstore_match_data, 1, 1, 0, /*
Set internal data on last search match from elements of LIST.

LIST should have been created by calling `match-data' previously, or be nil,
to clear the internal match data.

This function also accepts the output of `match-data-canonical', something not
true of GNU Emacs, nor of older XEmacs.  See also the macro `save-match-data'.
*/
       (list))
{
  Elemcount num_regs, ii = 0;
  Lisp_Object registers_vector = XCDR (Vsearch_registers), obj = Qunbound;
  Lisp_Object *staging = NULL;

  GET_EXTERNAL_LIST_LENGTH (list, num_regs);

  staging = alloca_array (Lisp_Object, num_regs);

  {
    /* Validate the entries in the list, copying the values into the staging
       array. No need for EXTERNAL_LIST_LOOP_2, we've checked type and
       circularity above. */
    LIST_LOOP_3 (elt, list, tail) 
      {
        if (EXTENTP (elt))
          {
            Lisp_Object extent_obj = extent_object (XEXTENT (elt));

            if (extent_detached_p (XEXTENT (elt)))
              {
                staging[ii] = Qnil;
              }
            else if (NILP (extent_obj))
              {
                sferror ("Extent doesn't have an associated object", elt);
              }
            else if (!UNBOUNDP (obj) && !EQ (extent_obj, obj))
              {
                sferror_2 ("Distinct search objects in match data",
                           obj, extent_obj);
              }
            else
              {
                if (UNBOUNDP (obj))
                  {
                    obj = extent_obj;
                  }

                staging[ii] = elt;
              }
          }
        else if (NILP (elt))
          {
            staging[ii] = elt;
          }
        else if (MARKERP (elt) && CONSP (XCDR (tail))
                 && MARKERP (XCAR (XCDR (tail))))
          {
            if (ii == 0)
              {
                /* Representing the match data as markers is sufficiently
                   sensible behaviour that I want to encourage it, implement
                   this in C. */
                store_match_data_markers (list, staging, &num_regs);
                break;
              }
            dead_wrong_type_argument (Qextentp, elt);
          }
        else if (FIXNUMP (elt))
          {
            if (ii == 0)
              {
                /* And I wanted to do this in Lisp, but GCPROing with stack
                   allocation is a chore. */
                store_match_data_fixnums (list, staging, &num_regs);
                break;
              }
            dead_wrong_type_argument (Qextentp, elt);
          }
        else if (CONSP (elt) && FIXNUMP (XCAR (elt)) && FIXNUMP (XCDR (elt)))
          {
            if (XREALFIXNUM (XCAR (elt)) < 0)
              {
                staging[ii] = Qnil;
              }
            else
              {
                staging[ii] = elt;
              }
          }
        else
          {
            dead_wrong_type_argument (Qextentp, elt);
          }
        ii++;
      }
  }

  clear_lisp_search_registers ();

  if (purify_flag && num_regs == 1 && EXTENTP (staging[0]) && 
      /* The Qsearch property is a conspiracy between loadup.el and this
	 file: */
      EQ (Qdiscard, Fextent_property (staging[0], Qsearch, Qnil)))
    {
      LIST_LOOP_2 (elt, XWEAK_LIST_LIST (Vstrings_to_nuke_extents))
	{
	  uninit_object_extents (elt);
	}
      num_regs = 0;
      Vstrings_to_nuke_extents = Qnil;
    }
  
  for (ii = 0; ii < num_regs; ii++)
    {
      if (EXTENTP (staging[ii]))
        {
          if (!EXTENTP (XVECTOR_DATA (registers_vector)[ii]))
            {
              XVECTOR_DATA (registers_vector)[ii]
                = Fcopy_extent (staging[ii], Qnil);
	      set_extent_start_open_p (XEXTENT (XVECTOR_DATA
						(registers_vector)[ii]),
				       1);
            }
          set_extent_endpoints (XEXTENT (XVECTOR_DATA
                                         (registers_vector)[ii]),
                                extent_endpoint_byte (XEXTENT (staging
                                                               [ii]),
                                                      0),
                                extent_endpoint_byte (XEXTENT (staging
                                                               [ii]),
                                                      1),
                                extent_object (XEXTENT (staging[ii])));
          if (STRINGP (extent_object (XEXTENT (staging[ii]))))
            {
              Lisp_Object context =
                extent_plist_get (XEXTENT (staging[ii]), Qcontext);
              if (!EQ (context,
                       extent_plist_get (XEXTENT
                                         (XVECTOR_DATA (registers_vector)
                                          [ii]), Qcontext)))
                {
                  Fset_extent_property (XVECTOR_DATA (registers_vector) [ii],
                                        Qcontext, context);
                }
            }
        }
      else if (CONSP (staging[ii]) && MARKERP (XCAR (staging[ii])))
        {
          if (!EXTENTP (XVECTOR_DATA (registers_vector)[ii]))
            {
              XVECTOR_DATA (registers_vector)[ii]
                = Fmake_extent (Qnil, Qnil, Qnil);
	      set_extent_start_open_p (XEXTENT (XVECTOR_DATA
						(registers_vector)[ii]),
				       1);
            }
          set_extent_endpoints (XEXTENT (XVECTOR_DATA
                                         (registers_vector)[ii]),
                                marker_byte_position (XCAR
                                                      (staging[ii])),
                                marker_byte_position (XCDR
                                                      (staging[ii])),
                                Fmarker_buffer (XCAR (staging[ii])));
        }
      else
        {
          if (EXTENTP (XVECTOR_DATA (registers_vector)[ii]))
            {
              set_extent_endpoints (XEXTENT (XVECTOR_DATA
                                             (XCDR (Vsearch_registers))
                                             [ii]),
                                    0, 0, 
                                    XSYMBOL_NAME (Qsearch));
              Fdetach_extent (XVECTOR_DATA (registers_vector)[ii]);
            }
          
          if (CONSP (staging[ii]))
            {
              XVECTOR_DATA (registers_vector)[ii]
                = Fcons (XCAR (staging[ii]), XCDR (staging[ii]));
            }
          else
            {
              XVECTOR_DATA (registers_vector)[ii] = Qnil;
            }
        }
    }

  XSETCAR (Vsearch_registers, make_fixnum (num_regs));

  return Qnil;
}

/* Quote a string to inactivate reg-expr chars */

DEFUN ("regexp-quote", Fregexp_quote, 1, 1, 0, /*
Return a regexp string which matches exactly STRING and nothing else.
*/
       (string))
{
  REGISTER Ibyte *in, *out, *end;
  REGISTER Ibyte *temp;

  CHECK_STRING (string);

  temp = alloca_ibytes (XSTRING_LENGTH (string) * 2);

  /* Now copy the data into the new string, inserting escapes. */

  in = XSTRING_DATA (string);
  end = in + XSTRING_LENGTH (string);
  out = temp;

  while (in < end)
    {
      Ichar c = itext_ichar (in);

      if (c == '[' || c == ']'
	  || c == '*' || c == '.' || c == '\\'
	  || c == '?' || c == '+'
	  || c == '^' || c == '$')
	*out++ = '\\';
      out += set_itext_ichar (out, c);
      INC_IBYTEPTR (in);
    }

  return make_string (temp, out - temp);
}

DEFUN ("set-word-regexp", Fset_word_regexp, 1, 1, 0, /*
Set the regexp to be used to match a word in regular-expression searching.
#### Not yet implemented.  Currently does nothing.
#### Do not use this yet.  Its calling interface is likely to change.
*/
       (UNUSED (regexp)))
{
  return Qnil;
}


#ifdef DEBUG_XEMACS

static int
debug_regexps_changed (Lisp_Object UNUSED (sym), Lisp_Object *val,
		       Lisp_Object UNUSED (in_object),
		       int UNUSED (flags))
{
  int newval = 0;

  EXTERNAL_LIST_LOOP_2 (elt, *val)
    {
      CHECK_SYMBOL (elt);
      if (EQ (elt, Qcompilation))
	newval |= RE_DEBUG_COMPILATION;
      else if (EQ (elt, Qfailure_point))
	newval |= RE_DEBUG_FAILURE_POINT;
      else if (EQ (elt, Qmatching))
	newval |= RE_DEBUG_MATCHING;
      else
	invalid_argument
	  ("Expected `compilation', `failure-point' or `matching'", elt);
    }
  debug_regexps = newval;
  return 0;
}

#endif /* DEBUG_XEMACS */


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_search (void)
{

  DEFERROR_STANDARD (Qsearch_failed, Qinvalid_operation);
  DEFERROR_STANDARD (Qinvalid_regexp, Qsyntax_error);
  Fput (Qinvalid_regexp, Qerror_lacks_explanatory_string, Qt);

  DEFSUBR (Flooking_at);
  DEFSUBR (Fposix_looking_at);
  DEFSUBR (Flooking_at_p);
  DEFSUBR (Fstring_match);
  DEFSUBR (Fposix_string_match);
  DEFSUBR (Fstring_match_p);
  DEFSUBR (Fskip_chars_forward);
  DEFSUBR (Fskip_chars_backward);
  DEFSUBR (Fskip_syntax_forward);
  DEFSUBR (Fskip_syntax_backward);
  DEFSUBR (Fsearch_forward);
  DEFSUBR (Fsearch_backward);
  DEFSUBR (Fword_search_forward);
  DEFSUBR (Fword_search_backward);
  DEFSUBR (Fre_search_forward);
  DEFSUBR (Fre_search_backward);
  DEFSUBR (Fposix_search_forward);
  DEFSUBR (Fposix_search_backward);
  DEFSUBR (Freplace_match);
  DEFSUBR (Fmatch_beginning);
  DEFSUBR (Fmatch_end);
  DEFSUBR (Fmatch_string);
  DEFSUBR (Fmatch_data_canonical);
  DEFSUBR (Fmatch_data);
  DEFSUBR (Fstore_match_data);
  DEFSUBR (Fregexp_quote);
  DEFSUBR (Fset_word_regexp);
}

void
reinit_vars_of_search (void)
{
  int i;

  for (i = 0; i < REGEXP_CACHE_SIZE; ++i)
    {
      searchbufs[i].buf.allocated = 100;
      searchbufs[i].buf.buffer = (unsigned char *) xmalloc (100);
      searchbufs[i].buf.fastmap = searchbufs[i].fastmap;
      searchbufs[i].regexp = Qnil;
      staticpro_nodump (&searchbufs[i].regexp);
      searchbufs[i].next = (i == REGEXP_CACHE_SIZE-1 ? 0 : &searchbufs[i+1]);
    }
  searchbuf_head = &searchbufs[0];

  search_regs.start = xnew_array (regoff_t, RE_NREGS);
  search_regs.end   = xnew_array (regoff_t, RE_NREGS);
  search_regs.num_regs = RE_NREGS;
}

void
vars_of_search (void)
{
  DEFVAR_LISP ("forward-word-regexp", &Vforward_word_regexp /*
*Regular expression to be used in `forward-word'.
#### Not yet implemented.
*/ );
  Vforward_word_regexp = Qnil;

  DEFVAR_LISP ("backward-word-regexp", &Vbackward_word_regexp /*
*Regular expression to be used in `backward-word'.
#### Not yet implemented.
*/ );
  Vbackward_word_regexp = Qnil;

  DEFVAR_INT ("warn-about-possibly-incompatible-back-references",
	      &warn_about_possibly_incompatible_back_references /*
If true, issue warnings when new-semantics back references occur.
This is to catch places where old code might inadvertently have changed
semantics.  This will occur in old code only where more than nine groups
occur and a back reference to one of them is directly followed by a digit.
*/ );
  warn_about_possibly_incompatible_back_references = 1;

  Vskip_chars_range_table = Fmake_range_table (Qstart_closed_end_closed);
  staticpro (&Vskip_chars_range_table);

  Vsearch_registers
    = Fcons (Qzero, Fmake_vector (make_fixnum (RE_NREGS), Qnil));
  staticpro (&Vsearch_registers);

  Vcase_flag_symbol = Fmake_symbol (build_ascstring ("case-flag-symbol"));
  staticpro (&Vcase_flag_symbol);

  DEFVAR_BOOL ("search-error-on-bad-match-data",
               &search_error_on_bad_match_data /*
If non-nil, error when encountering match data suggestive of a bug.

This includes `replace-match' receiving a STRING or a current buffer that do
not reflect those specified when the current match data were generated, when
`match-beginning', `match-end' or `match-string' are called with a current
buffer that is distinct from that the saved match data, or when
`store-match-data' is passed elements that are not appropriately ordered,.

For reasons of compatibility XEmacs normally warns in these situations, and
continues. This variable prompts it to error instead, which can make it more
practical to debug any problems; see `debug-on-error'.
*/ );
  search_error_on_bad_match_data = 0;

  Vstrings_to_nuke_extents = make_weak_list (WEAK_LIST_SIMPLE);
  staticpro (&Vstrings_to_nuke_extents);

#ifdef DEBUG_XEMACS 
  DEFSYMBOL (Qsearch_algorithm_used);
  DEFSYMBOL (Qboyer_moore);
  DEFSYMBOL (Qsimple_search);

  DEFSYMBOL (Qcompilation);
  DEFSYMBOL (Qfailure_point);
  DEFSYMBOL (Qmatching);

  DEFVAR_INT ("debug-searches", &debug_searches /*
If non-zero, bind `search-algorithm-used' to `boyer-moore' or `simple-search',
depending on the algorithm used for each search.  Used for testing.
*/ );
  debug_searches = 0;

  DEFVAR_LISP_MAGIC ("debug-regexps", &Vdebug_regexps, /*
List of areas to display debug info about during regexp operation.
The following areas are recognized:

`compilation'    Display the result of compiling a regexp.
`failure-point'	 Display info about failure points reached.
`matching'	 Display info about the process of matching a regex against
                 text.
*/ debug_regexps_changed);
  Vdebug_regexps = Qnil;
  debug_regexps = 0;
#endif /* DEBUG_XEMACS */
}
