/* New incremental garbage collector for XEmacs.
   Copyright (C) 2005 Marcus Crestani.
   Copyright (C) 2010 Ben Wing.

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

/* Synched up with: Not in FSF. */

#ifndef INCLUDED_gc_h_
#define INCLUDED_gc_h_

BEGIN_C_DECLS




/************************************************************************/
/*		             Global Variables     			*/
/************************************************************************/
/* Number of bytes of consing done since the last GC. */
extern EMACS_INT consing_since_gc;

/* Number of bytes of consing done since startup. */
extern EMACS_UINT total_consing;

/* Number of bytes of current allocated heap objects. */
extern EMACS_INT total_gc_usage;

/* If the above is set. */
extern int total_gc_usage_set;

/* Number of bytes of consing since gc before another gc should be done. */
extern EMACS_INT gc_cons_threshold;

/* Percentage of consing of total data size before another GC. */
extern EMACS_INT gc_cons_percentage;



/************************************************************************/
/*		               Prototypes         			*/
/************************************************************************/

#ifndef MALLOC_OVERHEAD
#ifdef HAVE_GLIBC
#define MALLOC_OVERHEAD 0
#elif defined (rcheck)
#define MALLOC_OVERHEAD 20
#else
#define MALLOC_OVERHEAD 8
#endif
#endif /* MALLOC_OVERHEAD */

#ifdef ERROR_CHECK_GC
#define GC_CHECK_LHEADER_INVARIANTS(lheader) do {		\
  struct lrecord_header * GCLI_lh = (lheader);			\
  assert (GCLI_lh != 0);					\
  assert (GCLI_lh->type < (unsigned int) lrecord_type_count);	\
} while (0)
#else
#define GC_CHECK_LHEADER_INVARIANTS(lheader)
#endif

void recompute_need_to_garbage_collect (void);

#ifdef DEBUG_XEMACS
#define KKCC_DEBUG_ARGS , level, pos
#define DECLARE_KKCC_DEBUG_ARGS , int level, int pos
#else
#define KKCC_DEBUG_ARGS
#define DECLARE_KKCC_DEBUG_ARGS
#endif


/* KKCC mark algorithm. */
void kkcc_gc_stack_push_lisp_object (Lisp_Object obj DECLARE_KKCC_DEBUG_ARGS);
void kkcc_gc_stack_repush_dirty_object (Lisp_Object obj
					DECLARE_KKCC_DEBUG_ARGS);

#ifdef DEBUG_XEMACS
#define kkcc_gc_stack_push_lisp_object_0(obj) \
  kkcc_gc_stack_push_lisp_object (obj, 0, -1)
void kkcc_backtrace_1 (int size, int detailed);
void kkcc_short_backtrace (void);
void kkcc_detailed_backtrace (void);
void kkcc_short_backtrace_full (void);
void kkcc_detailed_backtrace_full (void);
#else
#define kkcc_gc_stack_push_lisp_object_0(obj) \
  kkcc_gc_stack_push_lisp_object (obj)
#define kkcc_detailed_backtrace()
#endif


/* Initializers */
void init_gc_early (void);
void reinit_gc_early (void);
void init_gc_once_early (void);

void syms_of_gc (void);
void vars_of_gc (void);
void complex_vars_of_gc (void);

/* Needed prototypes due to the garbage collector code move from
   alloc.c to gc.c. */
void gc_sweep_1 (void);

extern void *breathing_space;


END_C_DECLS

#endif /* INCLUDED_gc_h_ */
