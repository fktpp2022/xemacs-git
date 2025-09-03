/* Why the hell is XEmacs so fucking slow?
   Copyright (C) 1996, 2002, 2003, 2004, 2010 Ben Wing.
   Copyright (C) 1998 Free Software Foundation, Inc.

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

#include <config.h>
#include "lisp.h"

#include "backtrace.h"
#include "bytecode.h"
#include "elhash.h"
#include "gc.h"
#include "profile.h"

#include "syssignal.h"
#include "systime.h"

#ifndef HAVE_SETITIMER
#error Sorry charlie.  We need a scalpel and all we have is a lawnmower.
#endif

#ifdef WIN32_ANY
int mswindows_is_blocking;
#endif

/* Written by Ben Wing. */

/*

Documented in

  (Info-goto-node "(internals)Profiling")
*/

Lisp_Object Vbig_profile_table;
Lisp_Object Vtotal_timing_profile_table;
Lisp_Object Vcall_count_profile_table;
Lisp_Object Vtotal_gc_usage_profile_table;
Lisp_Object Vgc_usage_profile_table;

extern EMACS_UINT total_consing;
static volatile EMACS_UINT total_ticks;

Fixnum default_profiling_interval;

int profiling_active;

static Lisp_Object QSprocessing_events_at_top_level;
static Lisp_Object QSunknown, QSprofile_overhead;

#ifdef DEBUG_XEMACS
/* For temporary profiling */
Lisp_Object QSin_temp_spot_1;
Lisp_Object QSin_temp_spot_2;
Lisp_Object QSin_temp_spot_3;
Lisp_Object QSin_temp_spot_4;
Lisp_Object QSin_temp_spot_5;
#endif /* DEBUG_XEMACS */

static Lisp_Object Qtiming, Qtotal_timing, Qcall_count;
static Lisp_Object Qgc_usage, Qtotal_gc_usage;

/* We use profiling_lock to prevent the signal handler from writing to
   the table while another routine is operating on it.  We also set
   profiling_lock in case the timeout between signal calls is short
   enough to catch us while we're already in there. */
static volatile int profiling_lock;

/* Whether we're in the process of doing *any* profiling-related stuff.
   Used to indicate amount of time spent profiling. */
static int in_profiling;

static void
create_profile_tables (void)
{
  if (NILP (Vbig_profile_table))
    Vbig_profile_table =
      make_lisp_hash_table (2000, HASH_TABLE_NON_WEAK, Qeq);
  if (NILP (Vtotal_timing_profile_table))
    Vtotal_timing_profile_table =
      make_lisp_hash_table (1000, HASH_TABLE_NON_WEAK, Qeq);
  if (NILP (Vcall_count_profile_table))
    Vcall_count_profile_table =
      make_lisp_hash_table (1000, HASH_TABLE_NON_WEAK, Qeq);
  if (NILP (Vgc_usage_profile_table))
    Vgc_usage_profile_table =
      make_lisp_hash_table (1000, HASH_TABLE_NON_WEAK, Qeq);
  if (NILP (Vtotal_gc_usage_profile_table))
    Vtotal_gc_usage_profile_table =
      make_lisp_hash_table (1000, HASH_TABLE_NON_WEAK, Qeq);
}

static Lisp_Object
current_profile_function (void)
{
  Lisp_Object fun;
  struct backtrace *bt = backtrace_list;

  /* 2 because we set in_profiling when we entered the current routine. */
  if (in_profiling >= 2)
    return QSprofile_overhead;

  /* Find a function actually being called.  Potentially (?) there could be
     a number of non-calling funs -- calling foo autoloads, which tries to
     call bar, but requires evalling its args first, which calls baz, ...
     If profiling was not enabled when the function was called, just treat
     the function as actually called, because the info about whether we've
     finished the preamble will not have been recorded. */
  for (; bt && !bt->function_being_called; bt = bt->next)
    ;

  if (bt)
    {
      fun = *bt->function;

      if (!SYMBOLP (fun)
	  && !COMPILED_FUNCTIONP (fun)
	  && !SUBRP (fun)
	  && !CONSP (fun)
	  && !STRINGP (fun))
	fun = QSunknown;
    }
  else
    fun = QSprocessing_events_at_top_level;
  return fun;
}

void
profile_record_consing (EMACS_INT size)
{
  in_profiling++;
  inchash_eq (current_profile_function (), Vgc_usage_profile_table, size);
  in_profiling--;
}

void
profile_record_unconsing (EMACS_INT size)
{
  /* If we don't want to record values less than 0, change this; but then
     the totals won't be accurate. */
  profile_record_consing (-size);
}

inline static void
profile_sow_backtrace (struct backtrace *bt)
{
  bt->current_total_timing_val =
    XFIXNUM (Fgethash (*bt->function, Vtotal_timing_profile_table, Qzero));
  bt->current_total_gc_usage_val =
    XFIXNUM (Fgethash (*bt->function, Vtotal_gc_usage_profile_table, Qzero));
  bt->function_being_called = 1;
  /* Need to think carefully about the exact order of operations here
    so that we don't end up with totals being less than function-only
    values; */
  bt->total_consing_at_start = total_consing;
  /* Order of operation is tricky here because we want the total function
     time to be as close as possible to (and absolutely not less than) the
     function-only time.  From the sigprof-handler's perspective, the
     function is "entered" the moment we finish executing the
     in_profiling-- statement below, and ends the moment we finish
     executing the in_profiling++ statement in
     profile_record_just_called().  By recording the tick value as close as
     possible to the "in-function" window but not in it, we satisfy the
     conditions just mentioned. */
  bt->total_ticks_at_start = total_ticks;

  /* Make sure an entry for this function exists in the big profile table,
     growing it as needed. */
  profiling_lock = 1;
  inchash_eq (*bt->function, Vbig_profile_table, 0);
  profiling_lock = 0;
}

void
profile_record_about_to_call (struct backtrace *bt)
{
  in_profiling++;
  profiling_lock = 1;
  /* Force an entry for this function to exist in Vbig_profile_table,
     enlarging it now if needed. */
  inchash_eq (*bt->function, Vbig_profile_table, 0);
  profiling_lock = 0;
  inchash_eq (*bt->function, Vcall_count_profile_table, 1);
  /* This may be set if the function was in its preamble at the time that
     `start-profiling' was called.  If so, we shouldn't reset the values
     because we may get inconsistent results, since we have already started
     recording ticks and consing for the function. */
  if (!bt->function_being_called)
    profile_sow_backtrace (bt);
  in_profiling--;
}

inline static void
profile_reap_backtrace (struct backtrace *bt)
{
  EMACS_UINT ticks;
  /* The following statement *MUST* come directly after the preceding one!
     See the comment above. */
  ticks = total_ticks;
  /* We need to reset the "in-function" flag here.  Otherwise the sigprof
     handler will record more ticks for the function while the post-amble
     is executing, and its value will be > our total value. */
  bt->function_being_called = 0;
  Fputhash (*bt->function,
	    /* This works even when the total_ticks value has overwrapped.
	       Same for total_consing below. */
	    make_fixnum ((EMACS_INT) (ticks - bt->total_ticks_at_start)
		      + bt->current_total_timing_val),
	    Vtotal_timing_profile_table);
  Fputhash (*bt->function,
	    make_fixnum ((EMACS_INT)
		      (total_consing - bt->total_consing_at_start)
		       + bt->current_total_gc_usage_val),
	    Vtotal_gc_usage_profile_table);
}

void
profile_record_just_called (struct backtrace *bt)
{
  in_profiling++;
  profile_reap_backtrace (bt);
  in_profiling--;
}

/* Called when unwinding the catch stack after a throw or signal, to
   note that we are exiting the function. */
void
profile_record_unwind (struct backtrace *bt)
{
  /* We may have thrown while still in a function's preamble. */
  if (bt->function_being_called)
    profile_record_just_called (bt);
}

static SIGTYPE
sigprof_handler (int UNUSED (signo))
{
#ifdef WIN32_ANY
  /* Windows unfortunately does not have any such thing as setitimer
     (ITIMER_PROF, ...), which runs in process time.  Everything is real
     time.  So to get slightly more reasonable results, ignore completely
     the times when we're blocking.  Same applies, of course, to Cygwin. */
  if (mswindows_is_blocking)
    return;
#endif

  in_profiling++;
  total_ticks++;

  /* Don't do anything if we are shutting down, or are doing a maphash
     or clrhash on the table. */
  if (!profiling_lock && !preparing_for_armageddon)
    {
      Lisp_Object fun = current_profile_function ();
      htentry *ht;

      /* If something below causes an error to be signaled, we'll
	 not correctly reset this flag.  But we'll be in worse shape
	 than that anyways, since we'll longjmp back to the last
	 condition case. */
      profiling_lock = 1;
      ht = inchash_eq (fun, Vbig_profile_table, 1);
      /* realloc() is not re-entrant, if we needed to enlarge the table,
	 crash. */
      assert (ht != NULL);
      profiling_lock = 0;
    }
  in_profiling--;
}

DEFUN ("start-profiling", Fstart_profiling, 0, 1, "", /*
Start profiling, with profile queries every MICROSECS.
If MICROSECS is nil or omitted, the value of `default-profiling-interval'
is used.

Information on function timings and call counts is currently recorded.
You can retrieve the recorded profiling info using `get-profiling-info',
or the higher-level function `profile-results'.

Starting and stopping profiling does not clear the currently recorded
info.  Thus you can start and stop as many times as you want and everything
will be properly accumulated. (To clear, use `clear-profiling-info'.)
*/
       (microsecs))
{
  /* This function can GC */
  int msecs;
  struct itimerval foo;
  int depth;

  if (profiling_active)
    return Qnil;
  depth = internal_bind_int (&in_profiling, 1 + in_profiling);

  create_profile_tables ();

  if (NILP (microsecs))
    msecs = default_profiling_interval;
  else
    {
#ifdef HAVE_BIGNUM
      check_integer_range (microsecs, make_fixnum (1000), make_integer (INT_MAX));
      msecs =
        BIGNUMP (microsecs) ? bignum_to_int (XBIGNUM_DATA (microsecs)) :
                                             XFIXNUM (microsecs);
#else
      check_integer_range (microsecs, make_fixnum (1000),
                           make_fixnum (MOST_POSITIVE_FIXNUM));
      msecs = XFIXNUM (microsecs);
#endif
    }
  if (msecs <= 0)
    msecs = 1000;

  set_timeout_signal (SIGPROF, sigprof_handler);
  {
    struct backtrace *bt = backtrace_list;

    /* When we begin profiling, pretend like we just entered all the
       functions currently on the stack.  When we stop profiling, do the
       opposite.  This ensures consistent values being recorded for both
       function-only and total in such cases. */
    for (; bt; bt = bt->next)
      profile_sow_backtrace (bt);
  }
  profiling_active = 1;
  profiling_lock = 0;
  foo.it_value.tv_sec = 0;
  foo.it_value.tv_usec = msecs;
  EMACS_NORMALIZE_TIME (foo.it_value);
  foo.it_interval = foo.it_value;
  qxe_setitimer (ITIMER_PROF, &foo, 0);
  unbind_to (depth);
  return Qnil;
}

DEFUN ("stop-profiling", Fstop_profiling, 0, 0, "", /*
Stop profiling.
*/
       ())
{
  /* This function does not GC */
  struct itimerval foo;

  if (!profiling_active)
    return Qnil;
  in_profiling++;
  foo.it_value.tv_sec = 0;
  foo.it_value.tv_usec = 0;
  foo.it_interval = foo.it_value;
  qxe_setitimer (ITIMER_PROF, &foo, 0);
  profiling_active = 0;
  {
    struct backtrace *bt = backtrace_list;

    for (; bt; bt = bt->next)
      profile_reap_backtrace (bt);
  }
  set_timeout_signal (SIGPROF, fatal_error_signal);
  in_profiling--;
  return Qnil;
}

DEFUN ("clear-profiling-info", Fclear_profiling_info, 0, 0, "", /*
Clear out the recorded profiling info.
This clears both the internal timing information and the call counts in
`call-count-profile-table'.
*/
       ())
{
  in_profiling++;
  /* This function does not GC */
  if (!NILP (Vbig_profile_table))
    {
      profiling_lock = 1;
      Fclrhash (Vbig_profile_table);
      profiling_lock = 0;
    }
  if (!NILP (Vtotal_timing_profile_table))
    Fclrhash (Vtotal_timing_profile_table);
  if (!NILP (Vcall_count_profile_table))
    Fclrhash (Vcall_count_profile_table);
  if (!NILP (Vgc_usage_profile_table))
    Fclrhash (Vgc_usage_profile_table);
  if (!NILP (Vtotal_gc_usage_profile_table))
    Fclrhash (Vtotal_gc_usage_profile_table);
  in_profiling--;
  
  return Qnil;
}

struct get_profiling_info_closure
{
  Lisp_Object timing;
};

static int
get_profiling_info_timing_maphash (Lisp_Object key,
				   Lisp_Object val,
				   void *void_closure)
{
  /* This function does not GC */
  struct get_profiling_info_closure *closure
    = (struct get_profiling_info_closure *) void_closure;

  if (!EQ (val, Qzero))
    {
      Fputhash (key, val, closure->timing);
    }

  return 0;
}

static Lisp_Object
copy_hash_table_or_blank (Lisp_Object table)
{
  return !NILP (table) ? Fcopy_hash_table (table) :
    make_lisp_hash_table (100, HASH_TABLE_NON_WEAK,
			  Qeq);
}

DEFUN ("get-profiling-info", Fget_profiling_info, 0, 0, 0, /*
Return the currently recorded profiling info.
The format is a plist of symbols describing type of info recorded and
an associated type-specific entry.  Currently, the following info types
are recorded

`timing'
  A hash table of function descriptions (funcallable objects or strings
  describing internal processing operations -- redisplay, garbage
  collection, etc.), along with associated tick counts (the frequency of
  ticks is controlled by `default-profiling-interval' or the argument to
  `start-profiling').

`total-timing'
  A hash table of function descriptions and associated timing count for
  the function and all descendants.

`call-count'
  A hash table of function descriptions and associated call counts.

`gc-usage'
  A hash table of function descriptions and associated amount of consing.

`total-gc-usage'
  A hash table of function descriptions and associated amount of consing
  in the function and all descendants.
*/
       ())
{
  /* This function does not GC */
  struct get_profiling_info_closure closure;
  Lisp_Object retv;
  int depth = internal_bind_int (&in_profiling, 1 + in_profiling);
  Lisp_Object overhead;

  closure.timing =
    make_lisp_hash_table (100, HASH_TABLE_NON_WEAK, Qequal);

  if (!NILP (Vbig_profile_table))
    {
      int count = internal_bind_int ((int *) &profiling_lock, 1);
      elisp_maphash_unsafe (get_profiling_info_timing_maphash, 
			    Vbig_profile_table, &closure);

      /* OK, OK ...  the total-timing table is not going to have an entry
	 for profile overhead, and it looks strange for it to come out 0,
	 so make sure it looks reasonable. */
      overhead = Fgethash (QSprofile_overhead, Vbig_profile_table, Qunbound);
      if (UNBOUNDP (overhead))
	{
	  overhead = Qzero;
	}

      Fputhash (QSprofile_overhead, overhead, Vtotal_timing_profile_table);
      unbind_to (count);
    }

  retv = listu (Qtiming, closure.timing,
                Qtotal_timing,
                copy_hash_table_or_blank (Vtotal_timing_profile_table),
                Qcall_count,
                copy_hash_table_or_blank (Vcall_count_profile_table),
                Qgc_usage,
                copy_hash_table_or_blank (Vgc_usage_profile_table),
                Qtotal_gc_usage,
                copy_hash_table_or_blank (Vtotal_gc_usage_profile_table),
                Qunbound);
  unbind_to (depth);
  return retv;
}

static int
set_profiling_info_timing_maphash (Lisp_Object key,
				   Lisp_Object val,
				   void *UNUSED (void_closure))
{
  /* This function does not GC */
  if (!FIXNUMP (val))
    invalid_argument_2
      ("Function timing count is not an integer in given entry",
       key, val);

  Fputhash (key, val, Vbig_profile_table);

  return 0;
}

DEFUN ("set-profiling-info", Fset_profiling_info, 1, 1, 0, /*
Set the currently recorded profiling info.
INFO should be in the same format returned by `get-profiling-info',
as described there.
*/
       (info))
{
  int depth;
  /* This function does not GC */
  Fclear_profiling_info ();

  depth = internal_bind_int (&in_profiling, 1 + in_profiling);
  {
    EXTERNAL_PROPERTY_LIST_LOOP_3 (key, value, info)
      {
	if (EQ (key, Qtiming))
	  {
	    CHECK_HASH_TABLE (value);
	    if (NILP (Vbig_profile_table))
	      {
		Vbig_profile_table
		  = make_lisp_hash_table (2000, HASH_TABLE_NON_WEAK, Qeq);
	      }
	    profiling_lock = 1;
	    elisp_maphash_unsafe (set_profiling_info_timing_maphash, value,
				  NULL);
	    profiling_lock = 0;
	  }
	else if (EQ (key, Qcall_count))
	  Vcall_count_profile_table = Fcopy_hash_table (value);
	else if (EQ (key, Qtotal_timing))
	  Vtotal_timing_profile_table = Fcopy_hash_table (value);
	else if (EQ (key, Qgc_usage))
	  Vgc_usage_profile_table = Fcopy_hash_table (value);
	else if (EQ (key, Qtotal_gc_usage))
	  Vtotal_gc_usage_profile_table = Fcopy_hash_table (value);
	else
	  invalid_constant ("Unrecognized profiling-info keyword", key);
      }
  }

  unbind_to (depth);
  return Qnil;
}

DEFUN ("profiling-active-p", Fprofiling_active_p, 0, 0, 0, /*
Return non-nil if profiling information is currently being recorded.
*/
       ())
{
  return profiling_active ? Qt : Qnil;
}

void
syms_of_profile (void)
{
  DEFSUBR (Fstart_profiling);
  DEFSUBR (Fstop_profiling);
  DEFSUBR (Fget_profiling_info);
  DEFSUBR (Fset_profiling_info);
  DEFSUBR (Fclear_profiling_info);
  DEFSUBR (Fprofiling_active_p);
}

void
vars_of_profile (void)
{
  DEFVAR_INT ("default-profiling-interval", &default_profiling_interval /*
Default CPU time in microseconds between profiling sampling.
Used when the argument to `start-profiling' is nil or omitted.
Under Unix, the time in question is CPU time (when the program is executing
or the kernel is executing on behalf of the program) and not real time.
Under MS Windows and Cygwin, the time is real time, but time spent blocking
while waiting for an event is ignored, to get more accurate results.
Note that there is usually a machine-dependent limit on how small this
value can be.
*/ );
  default_profiling_interval = 1000;

  staticpro (&Vbig_profile_table);
  Vbig_profile_table = Qnil;

  staticpro (&Vcall_count_profile_table);
  Vcall_count_profile_table = Qnil;

  staticpro (&Vgc_usage_profile_table);
  Vgc_usage_profile_table = Qnil;

  staticpro (&Vtotal_gc_usage_profile_table);
  Vtotal_gc_usage_profile_table = Qnil;

  staticpro (&Vtotal_timing_profile_table);
  Vtotal_timing_profile_table = Qnil;

  profiling_lock = 0;

#ifdef DEBUG_XEMACS
  QSin_temp_spot_1 = build_defer_string ("(in temp spot 1)");
  staticpro (&QSin_temp_spot_1);

  QSin_temp_spot_2 = build_defer_string ("(in temp spot 2)");
  staticpro (&QSin_temp_spot_2);

  QSin_temp_spot_3 = build_defer_string ("(in temp spot 3)");
  staticpro (&QSin_temp_spot_3);

  QSin_temp_spot_4 = build_defer_string ("(in temp spot 4)");
  staticpro (&QSin_temp_spot_4);

  QSin_temp_spot_5 = build_defer_string ("(in temp spot 5)");
  staticpro (&QSin_temp_spot_5);
#endif /* DEBUG_XEMACS */

  QSunknown = build_defer_string ("(unknown)");
  staticpro (&QSunknown);
  QSprocessing_events_at_top_level =
    build_defer_string ("(processing events at top level)");
  staticpro (&QSprocessing_events_at_top_level);
  QSprofile_overhead = build_defer_string ("(profile overhead)");
  staticpro (&QSprofile_overhead);

  DEFSYMBOL (Qtiming);
  DEFSYMBOL (Qtotal_timing);
  DEFSYMBOL (Qcall_count);
  DEFSYMBOL (Qgc_usage);
  DEFSYMBOL (Qtotal_gc_usage);
}
