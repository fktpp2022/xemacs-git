/* Garbage collector for XEmacs.
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

/* 
   XEmacs Garbage Collector

   The XEmacs garbage collector is a simple mark and sweep collector, its
   implementation is mainly spread out over gc.c and alloc.c.

   Until October 2021 this file contained Marcus Crestani's implementation
   of an incremental garbage collector. See the internals manual for
   documentation of this and documentation of the decision to remove it:

   (Info-goto-node "(internals)Discussion -- Incremental Collector")
   
   The mark algorthim uses an explicit stack to keep track of the current
   progress of traversal and uses memory layout descriptions (that are also
   used by the portable dumper) instead of the previous mark_* functions.  This
   work is called the "KKCC" mark algorithm from the initials of its
   implementors, Marcus Crestani and Markus Kaltenbach. It is significantly
   faster than the old mark algorithm, since it has much more locality of code
   reference. */

#include <config.h>
#include "lisp.h"

#include "backtrace.h"
#include "buffer.h"
#include "bytecode.h"
#include "chartab.h"
#include "console-stream.h"
#include "device.h"
#include "elhash.h"
#include "events.h"
#include "extents-impl.h"
#include "file-coding.h"
#include "frame-impl.h"
#include "gc.h"
#include "glyphs.h"
#include "opaque.h"
#include "lrecord.h"
#include "lstream.h"
#include "process.h"
#include "profile.h"
#include "redisplay.h"
#include "specifier.h"
#include "sysfile.h"
#include "sysdep.h"
#include "window.h"


/* Number of bytes of consing since gc before a full gc should happen. */
#define GC_CONS_THRESHOLD                 32000000
                                          
/* Number of bytes of consing done since the last GC. */
EMACS_INT consing_since_gc;

/* Number of bytes of consing done since startup. */
EMACS_UINT total_consing;

/* Number of bytes of current allocated heap objects. */
EMACS_INT total_gc_usage;

/* If the above is set. */
int total_gc_usage_set;

/* Number of bytes of consing since gc before another gc should be done. */
EMACS_INT gc_cons_threshold;

/* Nonzero during gc */
int gc_in_progress;

/* Percentage of consing of total data size before another GC. */
EMACS_INT gc_cons_percentage;


/************************************************************************/
/*		Recompute need to garbage collect			*/
/************************************************************************/

int need_to_garbage_collect;

#ifdef ERROR_CHECK_GC
int always_gc = 0;    		/* Debugging hack; equivalent to
				   (setq gc-cons-thresold -1) */
#else
#define always_gc 0
#endif

/* True if it's time to garbage collect now. */
void
recompute_need_to_garbage_collect (void)
{
  if (always_gc)
    need_to_garbage_collect = 1;
  else
    need_to_garbage_collect = 
      (consing_since_gc > gc_cons_threshold
       &&
#if 0 /* #### implement this better */
       ((double)consing_since_gc) / total_data_usage()) >=
      ((double)gc_cons_percentage / 100)
#else
       (!total_gc_usage_set ||
	((double)consing_since_gc / total_gc_usage) >=
	((double)gc_cons_percentage / 100))
#endif
       );
  recompute_funcall_allocation_flag ();
}



/************************************************************************/
/*			      Mark Phase       				*/
/************************************************************************/

static const struct memory_description int_description_1[] = {
  { XD_END }
};

const struct sized_memory_description int_description = {
  sizeof (int),
  int_description_1
};

static const struct memory_description unsigned_char_description_1[] = {
  { XD_END }
};

const struct sized_memory_description unsigned_char_description = {
  sizeof (unsigned char),
  unsigned_char_description_1
};

static const struct memory_description lisp_object_description_1[] = {
  { XD_LISP_OBJECT, 0 },
  { XD_END }
};

const struct sized_memory_description lisp_object_description = {
  sizeof (Lisp_Object),
  lisp_object_description_1
};

static const struct memory_description Lisp_Object_pair_description_1[] = {
  { XD_LISP_OBJECT, offsetof (Lisp_Object_pair, key) },
  { XD_LISP_OBJECT, offsetof (Lisp_Object_pair, value) },
  { XD_END }
};

const struct sized_memory_description Lisp_Object_pair_description = {
  sizeof (Lisp_Object_pair),
  Lisp_Object_pair_description_1
};

/* This function extracts the value of a count variable described somewhere 
   else in the description. It is converted corresponding to the type */ 
EMACS_INT
lispdesc_indirect_count_1 (EMACS_INT code,
			   const struct memory_description *idesc,
			   const void *idata)
{
  EMACS_INT count;
  const void *irdata;

  int line = XD_INDIRECT_VAL (code);
  int delta = XD_INDIRECT_DELTA (code);

  irdata = ((char *) idata) +
    lispdesc_indirect_count (idesc[line].offset, idesc, idata);
  switch (idesc[line].type)
    {
    case XD_BYTECOUNT:
      count = * (Bytecount *) irdata;
      break;
    case XD_ELEMCOUNT:
      count = * (Elemcount *) irdata;
      break;
    case XD_HASHCODE:
      count = * (Hashcode *) irdata;
      break;
    case XD_INT:
      count = * (int *) irdata;
      break;
    case XD_LONG:
      count = * (long *) irdata;
      break;
    default:
      stderr_out ("Unsupported count type : %d (line = %d, code = %zd)\n",
		  idesc[line].type, line, code);
#ifdef DEBUG_XEMACS
      if (gc_in_progress)
	kkcc_detailed_backtrace ();
#endif
      if (in_pdump)
	pdump_backtrace ();
      count = 0; /* warning suppression */
      ABORT ();
    }
  count += delta;
  return count;
}

/* SDESC is a "description map" (basically, a list of offsets used for
   successive indirections) and OBJ is the first object to indirect off of.
   Return the description ultimately found. */

const struct sized_memory_description *
lispdesc_indirect_description_1 (const void *obj,
				 const struct sized_memory_description *sdesc)
{
  int pos;

  for (pos = 0; sdesc[pos].size >= 0; pos++)
    obj = * (const void **) ((const char *) obj + sdesc[pos].size);

  return (const struct sized_memory_description *) obj;
}

/* Compute the size of the data at RDATA, described by a single entry
   DESC1 in a description array.  OBJ and DESC are used for
   XD_INDIRECT references. */

static Bytecount
lispdesc_one_description_line_size (void *rdata,
				    const struct memory_description *desc1,
				    const void *obj,
				    const struct memory_description *desc)
{
 union_switcheroo:
  switch (desc1->type)
    {
    case XD_LISP_OBJECT_ARRAY:
      {
	EMACS_INT val = lispdesc_indirect_count (desc1->data1, desc, obj);
	return (val * sizeof (Lisp_Object));
      }
    case XD_LISP_OBJECT:
    case XD_LO_LINK:
      return sizeof (Lisp_Object);
    case XD_OPAQUE_PTR:
      return sizeof (void *);
    case XD_BLOCK_PTR:
      {
	EMACS_INT val = lispdesc_indirect_count (desc1->data1, desc, obj);
	return val * sizeof (void *);
      }
    case XD_BLOCK_ARRAY:
      {
	EMACS_INT val = lispdesc_indirect_count (desc1->data1, desc, obj);
	    
	return (val *
		lispdesc_block_size
		(rdata,
		 lispdesc_indirect_description (obj, desc1->data2.descr)));
      }
    case XD_OPAQUE_DATA_PTR:
      return sizeof (void *);
    case XD_UNION_DYNAMIC_SIZE:
      {
	/* If an explicit size was given in the first-level structure
	   description, use it; else compute size based on current union
	   constant. */
	const struct sized_memory_description *sdesc =
	  lispdesc_indirect_description (obj, desc1->data2.descr);
	if (sdesc->size)
	  return sdesc->size;
	else
	  {
	    desc1 = lispdesc_process_xd_union (desc1, desc, obj);
	    if (desc1)
	      goto union_switcheroo;
	    break;
	  }
      }
    case XD_UNION:
      {
	/* If an explicit size was given in the first-level structure
	   description, use it; else compute size based on maximum of all
	   possible structures. */
	const struct sized_memory_description *sdesc =
	  lispdesc_indirect_description (obj, desc1->data2.descr);
	if (sdesc->size)
	  return sdesc->size;
	else
	  {
	    int count;
	    Bytecount max_size = -1, size;

	    desc1 = sdesc->description;

	    for (count = 0; desc1[count].type != XD_END; count++)
	      {
		size = lispdesc_one_description_line_size (rdata,
							   &desc1[count],
							   obj, desc);
		if (size > max_size)
		  max_size = size;
	      }
	    return max_size;
	  }
      }
    case XD_ASCII_STRING:
      return sizeof (void *);
    case XD_DOC_STRING:
      return sizeof (void *);
    case XD_INT_RESET:
      return sizeof (int);
    case XD_BYTECOUNT:
      return sizeof (Bytecount);
    case XD_ELEMCOUNT:
      return sizeof (Elemcount);
    case XD_HASHCODE:
      return sizeof (Hashcode);
    case XD_INT:
      return sizeof (int);
    case XD_LONG:
      return sizeof (long);
    default:
      stderr_out ("Unsupported dump type : %d\n", desc1->type);
      ABORT ();
    }

  return 0;
}


/* Return the size of the memory block (NOT necessarily a structure!) 
   described by SDESC and pointed to by OBJ.  If SDESC records an
   explicit size (i.e. non-zero), it is simply returned; otherwise,
   the size is calculated by the maximum offset and the size of the
   object at that offset, rounded up to the maximum alignment.  In
   this case, we may need the object, for example when retrieving an
   "indirect count" of an inlined array (the count is not constant,
   but is specified by one of the elements of the memory block). (It
   is generally not a problem if we return an overly large size -- we
   will simply end up reserving more space than necessary; but if the
   size is too small we could be in serious trouble, in particular
   with nested inlined structures, where there may be alignment
   padding in the middle of a block. #### In fact there is an (at
   least theoretical) problem with an overly large size -- we may
   trigger a protection fault when reading from invalid memory.  We
   need to handle this -- perhaps in a stupid but dependable way,
   i.e. by trapping SIGSEGV and SIGBUS.) */

Bytecount
lispdesc_block_size_1 (const void *obj, Bytecount size,
		       const struct memory_description *desc)
{
  EMACS_INT max_offset = -1;
  int max_offset_pos = -1;
  int pos;

  if (size)
    return size;

  for (pos = 0; desc[pos].type != XD_END; pos++)
    {
      EMACS_INT offset = lispdesc_indirect_count (desc[pos].offset, desc, obj);
      if (offset == max_offset)
	{
#if 0
	  /* This can legitimately happen with gap arrays -- if there are
	     no elements in the array, and the gap size is 0, then both
	     parts of the array will be of size 0 and in the same place. */
	  stderr_out ("Two relocatable elements at same offset?\n");
	  ABORT ();
#endif
	}
      else if (offset > max_offset)
	{
	  max_offset = offset;
	  max_offset_pos = pos;
	}
    }

  if (max_offset_pos < 0)
    return 0;

  {
    Bytecount size_at_max;
    size_at_max =
      lispdesc_one_description_line_size ((char *) obj + max_offset,
					  &desc[max_offset_pos], obj, desc);

    /* We have no way of knowing the required alignment for this structure,
       so just make it maximally aligned. */
    return MAX_ALIGN_SIZE (max_offset + size_at_max);
  }
}

#define GC_CHECK_NOT_FREE(lheader)					\
      gc_checking_assert (! LRECORD_FREE_P (lheader));			\
      gc_checking_assert (LHEADER_IMPLEMENTATION (lheader)->frob_block_p || \
			  ! (lheader)->free)

/* The following functions implement the new mark algorithm. 
   They mark objects according to their descriptions.  They 
   are modeled on the corresponding pdumper procedures. */

#if 0
# define KKCC_STACK_AS_QUEUE 1
#endif

#ifdef DEBUG_XEMACS
/* The backtrace for the KKCC mark functions. */
#define KKCC_INIT_BT_STACK_SIZE 4096

typedef struct
{
  void *obj;
  const struct memory_description *desc;
  int pos;
  int is_lisp;
} kkcc_bt_stack_entry;

static kkcc_bt_stack_entry *kkcc_bt;
static int kkcc_bt_stack_size;
static int kkcc_bt_depth = 0;

static void
kkcc_bt_init (void)
{
  kkcc_bt_depth = 0;
  kkcc_bt_stack_size = KKCC_INIT_BT_STACK_SIZE;
  kkcc_bt = (kkcc_bt_stack_entry *)
    xmalloc_and_zero (kkcc_bt_stack_size * sizeof (kkcc_bt_stack_entry));
  if (!kkcc_bt)
    {
      stderr_out ("KKCC backtrace stack init failed for size %d\n",
		  kkcc_bt_stack_size);
      ABORT ();
    }
}

/* Workhorse backtrace function.  Not static because may potentially be
   called from a debugger. */

void kkcc_backtrace_1 (int size, int detailed);
void
kkcc_backtrace_1 (int size, int detailed)
{
  int i;
  stderr_out ("KKCC mark stack backtrace :\n");
  for (i = kkcc_bt_depth - 1; i >= kkcc_bt_depth - size && i >= 0; i--)
    {
      Lisp_Object obj = wrap_pointer_1 (kkcc_bt[i].obj);
      stderr_out (" [%d] ", i);
      if (!kkcc_bt[i].is_lisp)
	stderr_out ("non Lisp Object");
      else if (!LRECORDP (obj))
	stderr_out ("Lisp Object, non-record");
      else if (XRECORD_LHEADER (obj)->type >= lrecord_type_last_built_in_type
	       || (!XRECORD_LHEADER_IMPLEMENTATION (obj)))
	stderr_out ("WARNING! Bad Lisp Object type %d",
		    XRECORD_LHEADER (obj)->type);
      else
	stderr_out ("%s", XRECORD_LHEADER_IMPLEMENTATION (obj)->name);
      if (detailed && kkcc_bt[i].is_lisp)
	{
	  stderr_out (" ");
	  debug_print (obj);
	}
      if (detailed)
	{
	  stderr_out (" ");
	  debug_print (obj);
	}
      stderr_out (" (addr: %p, desc: %p, ",
		  (void *) kkcc_bt[i].obj,
		  (void *) kkcc_bt[i].desc);
      if (kkcc_bt[i].pos >= 0)
	stderr_out ("pos: %d)\n", kkcc_bt[i].pos);
      else
	if (kkcc_bt[i].pos == -1)
	  stderr_out ("root set)\n");
	else if (kkcc_bt[i].pos == -2)
	  stderr_out ("dirty object)\n");
    }
}

/* Various front ends onto kkcc_backtrace_1(), meant to be called from
   a debugger.

   The variants are:

   normal vs _full(): Normal displays up to the topmost 100 items on the
   stack, whereas full displays all items (even if there are thousands)

   _detailed_() vs _short_(): Detailed here means print out the actual
   Lisp objects on the stack using debug_print() in addition to their type,
   whereas short means only show the type
*/

void
kkcc_detailed_backtrace (void)
{
  kkcc_backtrace_1 (100, 1);
}

void kkcc_short_backtrace (void);
void
kkcc_short_backtrace (void)
{
  kkcc_backtrace_1 (100, 0);
}

void kkcc_detailed_backtrace_full (void);
void
kkcc_detailed_backtrace_full (void)
{
  kkcc_backtrace_1 (kkcc_bt_depth, 1);
}

void kkcc_short_backtrace_full (void);
void
kkcc_short_backtrace_full (void)
{
  kkcc_backtrace_1 (kkcc_bt_depth, 0);
}

/* Short versions for ease in calling from a debugger */

void kbt (void);
void
kbt (void)
{
  kkcc_detailed_backtrace ();
}

void kbts (void);
void
kbts (void)
{
  kkcc_short_backtrace ();
}

void kbtf (void);
void
kbtf (void)
{
  kkcc_detailed_backtrace_full ();
}

void kbtsf (void);
void
kbtsf (void)
{
  kkcc_short_backtrace_full ();
}

static void
kkcc_bt_stack_realloc (void)
{
  kkcc_bt_stack_size *= 2;
  kkcc_bt = (kkcc_bt_stack_entry *)
    xrealloc (kkcc_bt, kkcc_bt_stack_size * sizeof (kkcc_bt_stack_entry));
  if (!kkcc_bt)
    {
      stderr_out ("KKCC backtrace stack realloc failed for size %d\n", 
		  kkcc_bt_stack_size);
      ABORT ();
    }
}

static void
kkcc_bt_free (void)
{
  xfree_1 (kkcc_bt);
  kkcc_bt = 0;
  kkcc_bt_stack_size = 0;
}

static void
kkcc_bt_push (void *obj, const struct memory_description *desc,
	      int is_lisp DECLARE_KKCC_DEBUG_ARGS)
{
  kkcc_bt_depth = level;
  kkcc_bt[kkcc_bt_depth].obj = obj;
  kkcc_bt[kkcc_bt_depth].desc = desc;
  kkcc_bt[kkcc_bt_depth].pos = pos;
  kkcc_bt[kkcc_bt_depth].is_lisp = is_lisp;
  kkcc_bt_depth++;
  if (kkcc_bt_depth >= kkcc_bt_stack_size)
    kkcc_bt_stack_realloc ();
}

#else /* not DEBUG_XEMACS */
#define kkcc_bt_init()
#define kkcc_bt_push(obj, desc)
#endif /* not DEBUG_XEMACS */

/* Object memory descriptions are in the lrecord_implementation structure.
   But copying them to a parallel array is much more cache-friendly. */
const struct memory_description *lrecord_memory_descriptions[countof (lrecord_implementations_table)];

/* the initial stack size in kkcc_gc_stack_entries */
#define KKCC_INIT_GC_STACK_SIZE 16384

typedef struct
{
  void *data;
  const struct memory_description *desc;
#ifdef DEBUG_XEMACS
  int level;
  int pos;
  int is_lisp;
#endif
} kkcc_gc_stack_entry;


static kkcc_gc_stack_entry *kkcc_gc_stack_ptr;
static int kkcc_gc_stack_front;
static int kkcc_gc_stack_rear;
static int kkcc_gc_stack_size;

#define KKCC_INC(i) ((i + 1) % kkcc_gc_stack_size)
#define KKCC_INC2(i) ((i + 2) % kkcc_gc_stack_size)

#define KKCC_GC_STACK_FULL (KKCC_INC2 (kkcc_gc_stack_rear) == kkcc_gc_stack_front)
#define KKCC_GC_STACK_EMPTY (KKCC_INC (kkcc_gc_stack_rear) == kkcc_gc_stack_front)

static void
kkcc_gc_stack_init (void)
{
  kkcc_gc_stack_size = KKCC_INIT_GC_STACK_SIZE;
  kkcc_gc_stack_ptr = (kkcc_gc_stack_entry *)
    xmalloc_and_zero (kkcc_gc_stack_size * sizeof (kkcc_gc_stack_entry));
  if (!kkcc_gc_stack_ptr) 
    {
      stderr_out ("stack init failed for size %d\n", kkcc_gc_stack_size);
      ABORT ();
    }
  kkcc_gc_stack_front = 0;
  kkcc_gc_stack_rear = kkcc_gc_stack_size - 1;
}

static void
kkcc_gc_stack_free (void)
{
  xfree_1 (kkcc_gc_stack_ptr);
  kkcc_gc_stack_ptr = 0;
  kkcc_gc_stack_front = 0;
  kkcc_gc_stack_rear = 0;
  kkcc_gc_stack_size = 0;
}

static void
kkcc_gc_stack_realloc (void)
{
  kkcc_gc_stack_entry *old_ptr = kkcc_gc_stack_ptr;
  int old_size = kkcc_gc_stack_size;
  kkcc_gc_stack_size *= 2;
  kkcc_gc_stack_ptr = (kkcc_gc_stack_entry *)
    xmalloc_and_zero (kkcc_gc_stack_size * sizeof (kkcc_gc_stack_entry));
  if (!kkcc_gc_stack_ptr)
    {
      stderr_out ("stack realloc failed for size %d\n", kkcc_gc_stack_size);
      ABORT ();
    }
  if (kkcc_gc_stack_rear >= kkcc_gc_stack_front)
    {
      int number_elements = kkcc_gc_stack_rear - kkcc_gc_stack_front + 1;
      memcpy (kkcc_gc_stack_ptr, &old_ptr[kkcc_gc_stack_front], 
	      number_elements * sizeof (kkcc_gc_stack_entry));
      kkcc_gc_stack_front = 0;
      kkcc_gc_stack_rear = number_elements - 1;
    }
  else
    {
      int number_elements = old_size - kkcc_gc_stack_front;
      memcpy (kkcc_gc_stack_ptr, &old_ptr[kkcc_gc_stack_front],
	      number_elements * sizeof (kkcc_gc_stack_entry));
      memcpy (&kkcc_gc_stack_ptr[number_elements], &old_ptr[0],
	      (kkcc_gc_stack_rear + 1) * sizeof (kkcc_gc_stack_entry));
      kkcc_gc_stack_front = 0;
      kkcc_gc_stack_rear = kkcc_gc_stack_rear + number_elements;
    }
  xfree_1 (old_ptr);
}

static void
kkcc_gc_stack_push (void *data, const struct memory_description *desc
		    DECLARE_KKCC_DEBUG_ARGS)
{
  if (KKCC_GC_STACK_FULL)
      kkcc_gc_stack_realloc();
  kkcc_gc_stack_rear = KKCC_INC (kkcc_gc_stack_rear);
  kkcc_gc_stack_ptr[kkcc_gc_stack_rear].data = data;
  kkcc_gc_stack_ptr[kkcc_gc_stack_rear].desc = desc;
#ifdef DEBUG_XEMACS
  kkcc_gc_stack_ptr[kkcc_gc_stack_rear].level = level;
  kkcc_gc_stack_ptr[kkcc_gc_stack_rear].pos = pos;
#endif
}

#ifdef DEBUG_XEMACS

static inline void
kkcc_gc_stack_push_0 (void *data, const struct memory_description *desc,
		      int is_lisp DECLARE_KKCC_DEBUG_ARGS)
{
  kkcc_gc_stack_push (data, desc KKCC_DEBUG_ARGS);
  kkcc_gc_stack_ptr[kkcc_gc_stack_rear].is_lisp = is_lisp;
}

static inline void
kkcc_gc_stack_push_lisp (void *data, const struct memory_description *desc
			 DECLARE_KKCC_DEBUG_ARGS)
{
  kkcc_gc_stack_push_0 (data, desc, 1 KKCC_DEBUG_ARGS);
}

static inline void
kkcc_gc_stack_push_nonlisp (void *data, const struct memory_description *desc
			    DECLARE_KKCC_DEBUG_ARGS)
{
  kkcc_gc_stack_push_0 (data, desc, 0 KKCC_DEBUG_ARGS);
}

#else /* not DEBUG_XEMACS */

static inline void
kkcc_gc_stack_push_lisp (void *data, const struct memory_description *desc)
{
  kkcc_gc_stack_push (data, desc);
}

static inline void
kkcc_gc_stack_push_nonlisp (void *data, const struct memory_description *desc)
{
  kkcc_gc_stack_push (data, desc);
}

#endif /* (not) DEBUG_XEMACS */

static kkcc_gc_stack_entry *
kkcc_gc_stack_pop (void)
{
  if (KKCC_GC_STACK_EMPTY)
    return 0;
#ifndef KKCC_STACK_AS_QUEUE
  /* stack behaviour */
  return &kkcc_gc_stack_ptr[kkcc_gc_stack_rear--];
#else
  /* queue behaviour */
  {
    int old_front = kkcc_gc_stack_front;
    kkcc_gc_stack_front = KKCC_INC (kkcc_gc_stack_front);
    return &kkcc_gc_stack_ptr[old_front];
  }
#endif
}

void
kkcc_gc_stack_push_lisp_object (Lisp_Object obj DECLARE_KKCC_DEBUG_ARGS)
{
  if (XTYPE (obj) == Lisp_Type_Record)
    {
      struct lrecord_header *lheader = XRECORD_LHEADER (obj);
      const struct memory_description *desc;
      GC_CHECK_LHEADER_INVARIANTS (lheader);
      desc = RECORD_DESCRIPTION (lheader);
      if (! MARKED_RECORD_HEADER_P (lheader)) 
	{
	  MARK_RECORD_HEADER (lheader);
	  kkcc_gc_stack_push_lisp ((void *) lheader, desc KKCC_DEBUG_ARGS);
	}
    }
}


#ifdef ERROR_CHECK_GC
#define KKCC_DO_CHECK_FREE(obj, allow_free)			\
do								\
{								\
  if (!allow_free && XTYPE (obj) == Lisp_Type_Record)		\
    {								\
      struct lrecord_header *lheader = XRECORD_LHEADER (obj);	\
      GC_CHECK_NOT_FREE (lheader);				\
    }								\
} while (0)
#else
#define KKCC_DO_CHECK_FREE(obj, allow_free) (USED (allow_free))
#endif

static inline void
mark_object_maybe_checking_free (Lisp_Object obj, int allow_free
				 DECLARE_KKCC_DEBUG_ARGS)
{
  KKCC_DO_CHECK_FREE (obj, allow_free);
  kkcc_gc_stack_push_lisp_object (obj KKCC_DEBUG_ARGS);
}

/* This function loops all elements of a struct pointer and calls 
   mark_with_description with each element. */
static void
mark_struct_contents (const void *data,
		      const struct sized_memory_description *sdesc,
		      int count DECLARE_KKCC_DEBUG_ARGS)
{
  int i;
  Bytecount elsize;
  elsize = lispdesc_block_size (data, sdesc);

  for (i = 0; i < count; i++)
    {
      kkcc_gc_stack_push_nonlisp (((char *) data) + elsize * i,
				  sdesc->description
				  KKCC_DEBUG_ARGS);
    }
}


/* This function implements the KKCC mark algorithm.
   Instead of calling mark_object, all the alive Lisp_Objects are pushed
   on the kkcc_gc_stack. This function processes all elements on the stack
   according to their descriptions. */
static void
kkcc_marking (void)
{
  kkcc_gc_stack_entry *stack_entry = 0;
  void *data = 0;
  const struct memory_description *desc = 0;
  int pos;
#ifdef DEBUG_XEMACS
  int level = 0;
#endif
  
  while ((stack_entry = kkcc_gc_stack_pop ()) != 0)
    {
      data = stack_entry->data;
      desc = stack_entry->desc;
#ifdef DEBUG_XEMACS
      level = stack_entry->level + 1;
      kkcc_bt_push (data, desc, stack_entry->is_lisp, stack_entry->level,
		    stack_entry->pos);
#else
      kkcc_bt_push (data, desc);
#endif


      if (!data) continue;

      gc_checking_assert (data);
      gc_checking_assert (desc);

      for (pos = 0; desc[pos].type != XD_END; pos++)
	{
	  const struct memory_description *desc1 = &desc[pos];
	  const void *rdata =
	    (const char *) data + lispdesc_indirect_count (desc1->offset,
							   desc, data);
	union_switcheroo:
	  
	  /* If the flag says don't mark, then don't mark. */
	  if ((desc1->flags) & XD_FLAG_NO_KKCC)
	    continue;

	  switch (desc1->type)
	    {
	    case XD_BYTECOUNT:
	    case XD_ELEMCOUNT:
	    case XD_HASHCODE:
	    case XD_INT:
	    case XD_LONG:
	    case XD_INT_RESET:
	    case XD_LO_LINK:
	    case XD_OPAQUE_PTR:
	    case XD_OPAQUE_DATA_PTR:
	    case XD_ASCII_STRING:
	    case XD_DOC_STRING:
	      break;
	    case XD_LISP_OBJECT: 
	      {
		const Lisp_Object *stored_obj = (const Lisp_Object *) rdata;

		/* Because of the way that tagged objects work (pointers and
		   Lisp_Objects have the same representation), XD_LISP_OBJECT
		   can be used for untagged pointers.  They might be NULL,
		   though. */
		if (EQ (*stored_obj, Qnull_pointer))
		  break;
		mark_object_maybe_checking_free
		  (*stored_obj, (desc1->flags) & XD_FLAG_FREE_LISP_OBJECT
		   KKCC_DEBUG_ARGS);
		break;
	      }
	    case XD_LISP_OBJECT_ARRAY:
	      {
		int i;
		EMACS_INT count =
		  lispdesc_indirect_count (desc1->data1, desc, data);
	
		for (i = 0; i < count; i++)
		  {
		    const Lisp_Object *stored_obj =
		      (const Lisp_Object *) rdata + i;

		    if (EQ (*stored_obj, Qnull_pointer))
		      break;
		    mark_object_maybe_checking_free
		      (*stored_obj, (desc1->flags) & XD_FLAG_FREE_LISP_OBJECT
		       KKCC_DEBUG_ARGS);
		  }
		break;
	      }
	    case XD_BLOCK_PTR:
	      {
		EMACS_INT count = lispdesc_indirect_count (desc1->data1, desc,
							   data);
		const struct sized_memory_description *sdesc =
		  lispdesc_indirect_description (data, desc1->data2.descr);
		const char *dobj = * (const char **) rdata;
		if (dobj)
		  mark_struct_contents (dobj, sdesc, count KKCC_DEBUG_ARGS);
		break;
	      }
	    case XD_BLOCK_ARRAY:
	      {
		EMACS_INT count = lispdesc_indirect_count (desc1->data1, desc,
							   data);
		const struct sized_memory_description *sdesc =
		  lispdesc_indirect_description (data, desc1->data2.descr);
		      
		mark_struct_contents (rdata, sdesc, count KKCC_DEBUG_ARGS);
		break;
	      }
	    case XD_UNION:
	    case XD_UNION_DYNAMIC_SIZE:
	      desc1 = lispdesc_process_xd_union (desc1, desc, data);
	      if (desc1)
		goto union_switcheroo;
	      break;
		    
	    default:
	      stderr_out ("Unsupported description type : %d\n", desc1->type);
	      kkcc_detailed_backtrace ();
	      ABORT ();
	    }
	}

    }
}

/* I hate duplicating all this crap! */
int
marked_p (Lisp_Object obj)
{
  /* Checks we used to perform. */
  /* if (EQ (obj, Qnull_pointer)) return 1; */
  /* if (!POINTER_TYPE_P (XGCTYPE (obj))) return 1; */
  /* if (PURIFIED (XPNTR (obj))) return 1; */

  if (XTYPE (obj) == Lisp_Type_Record)
    {
      struct lrecord_header *lheader = XRECORD_LHEADER (obj);

      GC_CHECK_LHEADER_INVARIANTS (lheader);

      return MARKED_RECORD_HEADER_P (lheader);
    }
  return 1;
}


/* Mark reference to a Lisp_Object.  If the object referred to has not been
   seen yet, recursively mark all the references contained in it. */
void
mark_object (Lisp_Object UNUSED (obj))
{
  /* this code should never be reached when configured for KKCC */
  stderr_out ("KKCC: Invalid mark_object call.\n");
  stderr_out ("Replace mark_object with kkcc_gc_stack_push_lisp_object.\n");
  ABORT ();
}


/************************************************************************/
/*			       Hooks         				*/
/************************************************************************/

/* Nonzero when calling certain hooks or doing other things where a GC
   would be bad. It prevents infinite recursive calls to gc. */
int gc_currently_forbidden;

int
begin_gc_forbidden (void)
{
  return internal_bind_int (&gc_currently_forbidden, 1);
}

void
end_gc_forbidden (int count)
{
  unbind_to (count);
}

/* Hooks. */
Lisp_Object Vpre_gc_hook, Qpre_gc_hook;
Lisp_Object Vpost_gc_hook, Qpost_gc_hook;

/* Maybe we want to use this when doing a "panic" gc after memory_full()? */
static int gc_hooks_inhibited;

struct post_gc_action
{
  void (*fun) (void *);
  void *arg;
};

typedef struct post_gc_action post_gc_action;

typedef struct
{
  Dynarr_declare (post_gc_action);
} post_gc_action_dynarr;

static post_gc_action_dynarr *post_gc_actions;

/* Register an action to be called at the end of GC.
   gc_in_progress is 0 when this is called.
   This is used when it is discovered that an action needs to be taken,
   but it's during GC, so it's not safe. (e.g. in a finalize method.)

   As a general rule, do not use Lisp objects here.
   And NEVER signal an error.
*/

void
register_post_gc_action (void (*fun) (void *), void *arg)
{
  post_gc_action action;

  if (!post_gc_actions)
    post_gc_actions = Dynarr_new (post_gc_action);

  action.fun = fun;
  action.arg = arg;

  Dynarr_add (post_gc_actions, action);
}

static void
run_post_gc_actions (void)
{
  int i;

  if (post_gc_actions)
    {
      for (i = 0; i < Dynarr_length (post_gc_actions); i++)
	{
	  post_gc_action action = Dynarr_at (post_gc_actions, i);
	  (action.fun) (action.arg);
	}

      Dynarr_reset (post_gc_actions);
    }
}



/************************************************************************/
/*			    Garbage Collection				*/
/************************************************************************/

/* For profiling. */
static Lisp_Object QSin_garbage_collection;

/* Nonzero means display messages at beginning and end of GC.  */
int garbage_collection_messages;

/* "Garbage collecting" */
Lisp_Object Vgc_message;
Lisp_Object Vgc_pointer_glyph;
static const Ascbyte gc_default_message[] = "Garbage collecting";
Lisp_Object Qgarbage_collecting;

/* "Locals" during GC. */
struct frame *f;
int speccount;
int cursor_changed;
Lisp_Object pre_gc_cursor;

/* PROFILE_DECLARE */
int do_backtrace;
struct backtrace backtrace;

/* Maximum amount of C stack to save when a GC happens.  */
#ifndef MAX_SAVE_STACK
#define MAX_SAVE_STACK 0 /* 16000 */
#endif

static void
show_gc_cursor_and_message (void) 
{
  /* Now show the GC cursor/message. */
  pre_gc_cursor = Qnil;
  cursor_changed = 0;

  /* We used to call selected_frame() here.

     The following functions cannot be called inside GC
     so we move to after the above tests. */
  {
    Lisp_Object frame;
    Lisp_Object device = Fselected_device (Qnil);
    if (NILP (device)) /* Could happen during startup, eg. if always_gc */
      return;
    frame = Fselected_frame (device);
    if (NILP (frame))
      invalid_state ("No frames exist on device", device);
    f = XFRAME (frame);
  }

  if (!noninteractive)
    {
      if (FRAME_WIN_P (f))
	{
	  Lisp_Object frame = wrap_frame (f);
	  Lisp_Object cursor = glyph_image_instance (Vgc_pointer_glyph,
						     FRAME_SELECTED_WINDOW (f),
						     ERROR_ME_NOT, 1);
	  pre_gc_cursor = f->pointer;
	  if (POINTER_IMAGE_INSTANCEP (cursor)
	      /* don't change if we don't know how to change back. */
	      && POINTER_IMAGE_INSTANCEP (pre_gc_cursor))
	    {
	      cursor_changed = 1;
	      Fset_frame_pointer (frame, cursor);
	    }
	}

      /* Don't print messages to the stream device. */
      if (!cursor_changed && !FRAME_STREAM_P (f))
	{
	  if (garbage_collection_messages)
	    {
	      Lisp_Object args[2], whole_msg;
	      args[0] = (STRINGP (Vgc_message) ? Vgc_message :
			 build_msg_string (gc_default_message));
	      args[1] = build_ascstring ("...");
	      whole_msg = concatenate (2, args, Qstring, 0);
	      echo_area_message (f, (Ibyte *) 0, whole_msg, 0, -1,
				 Qgarbage_collecting);
	    }
	}
    }
}

static void
remove_gc_cursor_and_message (void)
{
  /* Now remove the GC cursor/message */
  if (!noninteractive)
    {
      if (cursor_changed)
	Fset_frame_pointer (wrap_frame (f), pre_gc_cursor);
      else if (!FRAME_STREAM_P (f))
	{
	  /* Show "...done" only if the echo area would otherwise be empty. */
	  if (NILP (clear_echo_area (selected_frame (),
				     Qgarbage_collecting, 0)))
	    {
	      if (garbage_collection_messages)
		{
		  Lisp_Object args[2], whole_msg;
		  args[0] = (STRINGP (Vgc_message) ? Vgc_message :
			     build_msg_string (gc_default_message));
		  args[1] = build_msg_string ("... done");
		  whole_msg = concatenate (2, args, Qstring, 0);
		  echo_area_message (selected_frame (), (Ibyte *) 0,
				     whole_msg, 0, -1,
				     Qgarbage_collecting);
		}
	    }
	}
    }
}

static void
gc_prepare (void)
{
#if MAX_SAVE_STACK > 0
  char stack_top_variable;
  extern char *stack_bottom;
#endif


  do_backtrace = profiling_active || backtrace_with_internal_sections;

  assert (!gc_in_progress);
  assert (!in_display || gc_currently_forbidden);

  PROFILE_RECORD_ENTERING_SECTION (QSin_garbage_collection);

  need_to_signal_post_gc = 0;
  recompute_funcall_allocation_flag ();
  flush_unused_lisp_search_registers ();
  flush_cached_extent_info ();

  if (!gc_hooks_inhibited)
    run_hook_trapping_problems
      (Qgarbage_collecting, Qpre_gc_hook,
       INHIBIT_EXISTING_PERMANENT_DISPLAY_OBJECT_DELETION);

  /***** Now we actually start the garbage collection. */

  gc_in_progress = 1;
  inhibit_non_essential_conversion_operations++;

#if MAX_SAVE_STACK > 0

  /* Save a copy of the contents of the stack, for debugging.  */
  if (!purify_flag)
    {
      /* Static buffer in which we save a copy of the C stack at each GC.  */
      static char *stack_copy;
      static Bytecount stack_copy_size;

      ptrdiff_t stack_diff = &stack_top_variable - stack_bottom;
      Bytecount stack_size = (stack_diff > 0 ? stack_diff : -stack_diff);
      if (stack_size < MAX_SAVE_STACK)
	{
	  if (stack_copy_size < stack_size)
	    {
	      stack_copy = (char *) xrealloc (stack_copy, stack_size);
	      stack_copy_size = stack_size;
	    }

	  memcpy (stack_copy,
		  stack_diff > 0 ? stack_bottom : &stack_top_variable,
		  stack_size);
	}
    }
#endif /* MAX_SAVE_STACK > 0 */

  /* Do some totally ad-hoc resource clearing. */
  /* #### generalize this? */
  clear_event_resource ();
  cleanup_specifiers ();
  cleanup_buffer_undo_lists ();
}

static void
gc_mark_root_set (void)
{

  /* Mark all the special slots that serve as the roots of accessibility. */

# define mark_object(obj) kkcc_gc_stack_push_lisp_object_0 (obj)

  { /* staticpro() */
    Lisp_Object **p = Dynarr_begin (staticpros);
    Elemcount len = Dynarr_length (staticpros);
    Elemcount count;
    for (count = 0; count < len; count++, p++)
      /* Need to check if the pointer in the staticpro array is not
	 NULL. A gc can occur after variable is added to the staticpro
	 array and _before_ it is correctly initialized. In this case
	 its value is NULL, which we have to catch here. */
      if (*p)
	mark_object (**p);
  }

  { /* staticpro_nodump() */
    Lisp_Object **p = Dynarr_begin (staticpros_nodump);
    Elemcount len = Dynarr_length (staticpros_nodump);
    Elemcount count;
    for (count = 0; count < len; count++, p++)
      /* Need to check if the pointer in the staticpro array is not
	 NULL. A gc can occur after variable is added to the staticpro
	 array and _before_ it is correctly initialized. In this case
	 its value is NULL, which we have to catch here. */
      if (*p)
	mark_object (**p);
  }


  { /* GCPRO() */
    struct gcpro *tail;
    int i;
    for (tail = gcprolist; tail; tail = tail->next)
      for (i = 0; i < tail->nvars; i++)
	mark_object (tail->var[i]);
  }

  { /* specbind() */
    struct specbinding *bind;
    for (bind = specpdl; bind != specpdl_ptr; bind++)
      {
	mark_object (bind->symbol);
	mark_object (bind->old_value);
      }
  }

  {
    struct catchtag *c;
    for (c = catchlist; c; c = c->next)
      {
	mark_object (c->tag);
	mark_object (c->val);
	mark_object (c->actual_tag);
	mark_object (c->backtrace);
      }
  }

  {
    struct backtrace *backlist;
    for (backlist = backtrace_list; backlist; backlist = backlist->next)
      {
	int nargs = backlist->nargs;
	int i;

	mark_object (*backlist->function);
	if (nargs < 0 /* nargs == UNEVALLED || nargs == MANY */
	    /* might be fake (internal profiling entry) */
	    && backlist->args)
	  mark_object (backlist->args[0]);
	else
	  for (i = 0; i < nargs; i++)
	    mark_object (backlist->args[i]);
      }
  }

# undef mark_object
}

static void
gc_finish_mark (void)
{
  init_marking_ephemerons ();

  while (finish_marking_weak_hash_tables () > 0 ||
	 finish_marking_weak_lists       () > 0 ||
	 continue_marking_ephemerons     () > 0)
    {
      kkcc_marking ();
    }

  /* At this point, we know which objects need to be finalized: we
     still need to resurrect them */

  while (finish_marking_ephemerons       () > 0 ||
	 finish_marking_weak_lists       () > 0 ||
	 finish_marking_weak_hash_tables () > 0)
    {
      kkcc_marking ();
    }

  /* And prune (this needs to be called after everything else has been marked
     and before we do any sweeping). If you're considering whether you would
     like to add a prune function for a new object type you have added,
     consider implementing the mark-and-prune approach as a weak list first of
     all, and only if that doesn't work or is uneconomic of memory, consider
     adding another entry here.  */
  prune_weak_lists ();
  prune_weak_hash_tables ();
  prune_ephemerons ();
}



static void
gc_finish (void)
{
  finish_object_memory_usage_stats ();
  consing_since_gc = 0;
#ifndef DEBUG_XEMACS
  /* Allow you to set it really fucking low if you really want ... */
  if (gc_cons_threshold < 10000)
    gc_cons_threshold = 10000;
#endif
  recompute_need_to_garbage_collect ();

  inhibit_non_essential_conversion_operations--;
  gc_in_progress = 0;

  run_post_gc_actions ();

  /******* End of garbage collection ********/

  if (!breathing_space)
    {
      breathing_space = malloc (4096 - MALLOC_OVERHEAD);
    }

  need_to_signal_post_gc = 1;
  funcall_allocation_flag = 1;

  PROFILE_RECORD_EXITING_SECTION (QSin_garbage_collection);

}

void garbage_collect_1 (void)
{
  if (gc_in_progress
      || gc_currently_forbidden
      || in_display
      || preparing_for_armageddon)
    return;

  /* Very important to prevent GC during any of the following
     stuff that might run Lisp code; otherwise, we'll likely
     have infinite GC recursion. */
  speccount = begin_gc_forbidden ();

  show_gc_cursor_and_message ();

  gc_prepare ();
  kkcc_gc_stack_init();
#ifdef DEBUG_XEMACS
  kkcc_bt_init ();
#endif
  gc_mark_root_set ();
  kkcc_marking ();
  gc_finish_mark ();
  kkcc_gc_stack_free ();
#ifdef DEBUG_XEMACS
  kkcc_bt_free ();
#endif
  gc_sweep_1 ();
  gc_finish ();

  remove_gc_cursor_and_message ();

  /* now stop inhibiting GC */
  unbind_to (speccount);
}


/************************************************************************/
/*			     Initializations				*/
/************************************************************************/

/* Initialization */
static void
common_init_gc_early (void)
{
  Vgc_message = Qzero;

  gc_currently_forbidden = 0;
  gc_hooks_inhibited = 0;

  need_to_garbage_collect = always_gc;

  gc_cons_threshold = GC_CONS_THRESHOLD;
  gc_cons_percentage = 40; /* #### what is optimal? */
  total_gc_usage_set = 0;
}

void
init_gc_early (void)
{
}

void
reinit_gc_early (void)
{
  common_init_gc_early ();
}

void
init_gc_once_early (void)
{
  common_init_gc_early ();
}

void
syms_of_gc (void)
{
  DEFSYMBOL (Qpre_gc_hook);
  DEFSYMBOL (Qpost_gc_hook);
}

void
vars_of_gc (void)
{
  staticpro_nodump (&pre_gc_cursor);

  QSin_garbage_collection = build_defer_string ("(in garbage collection)");
  staticpro (&QSin_garbage_collection);

  DEFVAR_INT ("gc-cons-threshold", &gc_cons_threshold /*
*Number of bytes of consing between full garbage collections.
\"Consing\" is a misnomer in that this actually counts allocation
of all different kinds of objects, not just conses.
Garbage collection can happen automatically once this many bytes have been
allocated since the last garbage collection.  All data types count.

Garbage collection happens automatically when `eval' or `funcall' are
called.  (Note that `funcall' is called implicitly as part of evaluation.)
By binding this temporarily to a large number, you can effectively
prevent garbage collection during a part of the program.

Normally, you cannot set this value less than 10,000 (if you do, it is
automatically reset during the next garbage collection).  However, if
XEmacs was compiled with DEBUG_XEMACS, this does not happen, allowing
you to set this value very low to track down problems with insufficient
GCPRO'ing.  If you set this to a negative number, garbage collection will
happen at *EVERY* call to `eval' or `funcall'.  This is an extremely
effective way to check GCPRO problems, but be warned that your XEmacs
will be unusable!  You almost certainly won't have the patience to wait
long enough to be able to set it back.
 
See also `consing-since-gc' and `gc-cons-percentage'.
*/ );

  DEFVAR_INT ("gc-cons-percentage", &gc_cons_percentage /*
*Percentage of memory allocated between garbage collections.

Garbage collection will happen if this percentage of the total amount of
memory used for data (see `lisp-object-memory-usage') has been allocated
since the last garbage collection.  However, it will not happen if less
than `gc-cons-threshold' bytes have been allocated -- this sets an absolute
minimum in case very little data has been allocated or the percentage is
set very low.  Set this to 0 to have garbage collection always happen after
`gc-cons-threshold' bytes have been allocated, regardless of current memory
usage.

See also `consing-since-gc' and `gc-cons-threshold'.
*/ );


  DEFVAR_BOOL ("purify-flag", &purify_flag /*
Non-nil means loading Lisp code in order to dump an executable.
This means that certain objects should be allocated in readonly space.
*/ );

  DEFVAR_BOOL ("garbage-collection-messages", &garbage_collection_messages /*
*Non-nil means display messages at start and end of garbage collection.
*/ );
  garbage_collection_messages = 0;

  DEFVAR_LISP ("pre-gc-hook", &Vpre_gc_hook /*
Function or functions to be run just before each garbage collection.
Interrupts, garbage collection, and errors are inhibited while this hook
runs, so be extremely careful in what you add here.  In particular, avoid
consing, and do not interact with the user.
*/ );
  Vpre_gc_hook = Qnil;

  DEFVAR_LISP ("post-gc-hook", &Vpost_gc_hook /*
Function or functions to be run just after each garbage collection.
Interrupts, garbage collection, and errors are inhibited while this hook
runs.  Each hook is called with one argument which is an alist with
finalization data.
*/ );
  Vpost_gc_hook = Qnil;

  DEFVAR_LISP ("gc-message", &Vgc_message /*
String to print to indicate that a garbage collection is in progress.
This is printed in the echo area.  If the selected frame is on a
window system and `gc-pointer-glyph' specifies a value (i.e. a pointer
image instance) in the domain of the selected frame, the mouse pointer
will change instead of this message being printed.
*/ );
  Vgc_message = build_defer_string (gc_default_message);

  DEFVAR_LISP ("gc-pointer-glyph", &Vgc_pointer_glyph /*
Pointer glyph used to indicate that a garbage collection is in progress.
If the selected window is on a window system and this glyph specifies a
value (i.e. a pointer image instance) in the domain of the selected
window, the pointer will be changed as specified during garbage collection.
Otherwise, a message will be printed in the echo area, as controlled
by `gc-message'.
*/ );

}

void
complex_vars_of_gc (void)
{
  Vgc_pointer_glyph = Fmake_glyph_internal (Qpointer);
}
