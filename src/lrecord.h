/* The "lrecord" structure (header of a compound lisp object).
   Copyright (C) 1993, 1994, 1995 Free Software Foundation, Inc.
   Copyright (C) 1996, 2001, 2002, 2004, 2005, 2009, 2010 Ben Wing.

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

/* This file has been Mule-ized, Ben Wing, 10-13-04. */

#ifndef INCLUDED_lrecord_h_
#define INCLUDED_lrecord_h_

/* All objects other than char and int are implemented as structures and
   passed by reference.  Such objects are called "record objects" ("record"
   is another term for "structure").  The "wrapped" value of such an object
   (i.e. when stored in a variable of type Lisp_Object) is simply the raw
   pointer coerced to an integral type the same size as the pointer.
   
   There are two kinds of record objects: normal objects (those allocated on
   their own with xmalloc()) and frob-block objects (those allocated as pieces
   of large, usually 2K, chunks of memory known as "frob blocks").

   Record objects have a header at the beginning of their structure, which
   is used internally to identify the type of the object (so that an
   object's type can be recovered from its pointer); in addition, it holds
   a few flags and a "UID", which for most objects is shown when it is
   printed, and is primarily useful for debugging purposes.  The header of
   a normal object is declared as NORMAL_LISP_OBJECT_HEADER and that of a
   frob-block object FROB_BLOCK_LISP_OBJECT_HEADER.

   FROB_BLOCK_LISP_OBJECT_HEADER boils down to a `struct lrecord_header'.
   This is a 32-bit value made up of bit fields, where 8 bits are used to
   hold the type, 2 or 3 bits are used for flags associated with the
   garbage collector, and the remaining 21 or 22 bits hold the UID.

   NORMAL_LISP_OBJECT_HEADER resolves to a `struct old_lcrecord_header' (note
   the `c'), which is a larger structure -- on 32-bit machines it occupies 2
   machine words instead of 1.  Such an object is known internally as an
   "lcrecord".  The first word of `struct old_lcrecord_header' is an embedded
   `struct lrecord_header' with the same information as for frob-block
   objects; that way, all objects can be cast to a `struct lrecord_header' to
   determine their type or other info.  The other word is a pointer, used to
   thread all lcrecords together in one big linked list.

   Normal objects (i.e. lcrecords) are allocated in individual chunks using
   the underlying allocator (i.e. xmalloc(), which is a thin wrapper around
   malloc()).  Frob-block objects are more efficient than normal objects, as
   they have a smaller header and don't have the additional memory overhead
   associated with malloc() -- instead, as mentioned above, they are carved
   out of 2K chunks of memory called "frob blocks").  However, it is slightly
   more tricky to create such objects, as they require special routines in
   alloc.c to create an object of each such type and to sweep them during
   garbage collection.  In addition, there is currently no mechanism for
   handling variable-sized frob-block objects (e.g. vectors), whereas
   variable-sized normal objects are not a problem.  Frob-block objects are
   typically used for basic objects that exist in large numbers, such as
   `cons' or `string'.

   Note that strings are an apparent exception to the statement above that
   variable-sized objects can't be handled.  Strings work as follows.  A
   string consists of two parts -- a fixed-size "string header" that is
   allocated as a standard frob-block object, and a "string-chars" structure
   that is allocated out of special 8K-sized frob blocks that have a dedicated
   garbage-collection handler that compacts the blocks during the sweep stage,
   relocating the string-chars data (but not the string headers) to eliminate
   gaps.  Strings larger than 8K are not placed in frob blocks, but instead
   are stored as individually malloc()ed blocks of memory.  Strings larger
   than 8K are called "big strings" and those smaller than 8K are called
   "small strings".

   To create a new normal Lisp object, see the toolbar-button example
   below.  To create a new frob-block Lisp object, follow the lead of
   one of the existing frob-block objects, such as extents or events.
   Note that you do not need to supply all the methods (see below);
   reasonable defaults are provided for many of them.  Alternatively, if
   you're just looking for a way of encapsulating data (which possibly
   could contain Lisp_Objects in it), you may well be able to use the
   opaque type.
*/

/*
  How to declare a Lisp object:

   NORMAL_LISP_OBJECT_HEADER:
      Header for normal objects

   FROB_BLOCK_LISP_OBJECT_HEADER:
      Header for frob-block objects

  How to allocate a Lisp object:

   - For normal objects of a fixed size, simply call
     ALLOC_NORMAL_LISP_OBJECT (type), where TYPE is the name of the type
     (e.g. toolbar_button).  Such objects can be freed manually using
     free_normal_lisp_object.

   - For normal objects whose size can vary (and hence which have a
     size_in_bytes_method rather than a static_size), call
     ALLOC_SIZED_LISP_OBJECT (size, type), where TYPE is the
     name of the type. NOTE: You cannot call free_normal_lisp_object() on such
     on object!

   - For frob-block objects, use
     ALLOC_FROB_BLOCK_LISP_OBJECT (type, lisp_type, var, lrec_ptr).
     But these objects need special handling; if you don't understand this,
     just ignore it.

   - Some lrecords, which are used totally internally, use the
     noseeum-* functions for debugging reasons.

  Other operations:

   - copy_lisp_object (dst, src)

   - zero_nonsized_lisp_object (obj), zero_sized_lisp_object (obj, size):
     BUT NOTE, it is not necessary to zero out newly allocated Lisp objects.
     This happens automatically.

   - lisp_object_size (obj): Return the size of a Lisp object. NOTE: This
     requires that the object is properly initialized.

   - lisp_object_storage_size (obj, stats): Return the storage size of a
     Lisp object, including malloc or frob-block overhead; also, if STATS
     is non-NULL, accumulate info about the size and overhead into STATS.
 */

#define ALLOC_NORMAL_LISP_OBJECT(type) \
  alloc_automanaged_lcrecord (LRECORD_IMPLEMENTATION (type))
#define ALLOC_SIZED_LISP_OBJECT(size, type) \
  old_alloc_sized_lcrecord (size, LRECORD_IMPLEMENTATION (type))

#define NORMAL_LISP_OBJECT_HEADER struct old_lcrecord_header
#define FROB_BLOCK_LISP_OBJECT_HEADER struct lrecord_header
#define LISP_OBJECT_FROB_BLOCK_P(obj) (XRECORD_LHEADER_IMPLEMENTATION(obj)->frob_block_p)
#define IF_OLD_GC(x) x

#define ALLOC_C_READONLY_LISP_OBJECT(type) \
  alloc_automanaged_c_readonly_lcrecord\
  (LRECORD_IMPLEMENTATION (type)->static_size, LRECORD_IMPLEMENTATION (type))

#define LISP_OBJECT_UID(obj) (XRECORD_LHEADER (obj)->uid)

BEGIN_C_DECLS

struct lrecord_header
{
  /* Index into lrecord_implementations_table[].  Objects that have been
     explicitly freed using e.g. free_cons() have lrecord_type_free in this
     field. */
  unsigned int type :8;

  /* If `mark' is 0 after the GC mark phase, the object will be freed
     during the GC sweep phase.  There are 2 ways that `mark' can be 1:
     - by being referenced from other objects during the GC mark phase
     - because it is permanently on, for c_readonly objects */
  unsigned int mark :1;

  /* 1 if the object resides in logically read-only space, and does not
     reference other non-c_readonly objects.
     Invariant: if (c_readonly == 1), then (mark == 1 && lisp_readonly == 1) */
  unsigned int c_readonly :1;

  /* 1 if the object is readonly from lisp */
  unsigned int lisp_readonly :1;

  /* The `free' field is currently used only for lcrecords under old-GC.
     It is a flag that indicates whether this lcrecord is on a "free list".
     Free lists are used to minimize the number of calls to malloc() when
     we're repeatedly allocating and freeing a number of the same sort of
     lcrecord.  Lcrecords on a free list always get marked in a different
     fashion, so we can use this flag as a sanity check to make sure that
     free lists only have freed lcrecords and there are no freed lcrecords
     elsewhere. */
  unsigned int free :1;

  /* The `uid' field is just for debugging/printing convenience.  Having
     this slot doesn't hurt us spacewise, since the bits are unused
     anyway. (The bits are used for strings, though.) */
  unsigned int uid :20;

};

struct lrecord_implementation;
int lrecord_type_index (const struct lrecord_implementation *implementation);
extern int lrecord_uid_counter[];

#define set_lheader_implementation(header,imp) do {			\
  struct lrecord_header* SLI_header = (header);				\
  SLI_header->type = (imp)->lrecord_type_index;				\
  SLI_header->mark = 0;							\
  SLI_header->c_readonly = 0;						\
  SLI_header->lisp_readonly = 0;					\
  SLI_header->free = 0;							\
  SLI_header->uid = lrecord_uid_counter[(imp)->lrecord_type_index]++;   \
} while (0)

struct old_lcrecord_header
{
  struct lrecord_header lheader;

  /* The `next' field is normally used to chain all lcrecords together
     so that the GC can find (and free) all of them.
     `old_alloc_sized_lcrecord' threads lcrecords together.

     The `next' field may be used for other purposes as long as some
     other mechanism is provided for letting the GC do its work. */
  struct old_lcrecord_header *next;
};

/* Used for lcrecords in an lcrecord-list. */
struct free_lcrecord_header
{
  struct old_lcrecord_header lcheader;
  Lisp_Object chain;
};

/* DON'T FORGET to update .gdbinit.in.in if you change this list. */
enum lrecord_type
{
  /* Symbol value magic types come first so that SYMBOL_VALUE_MAGIC_P
     can be written as a single '<= lrecord_type_max_symbol_value_magic'.
     In practice, if we moved them somewhere else but kept them together,
     the single extra comparison would hardly make a difference. */
  /* Don't assign any type to 0, so in case we come across zeroed memory
     it will be more obvious when printed */
  lrecord_type_symbol_value_forward_object = 1, /* struct symbol_value_forward_object */
  lrecord_type_symbol_value_forward_fixnum, /* struct symbol_value_forward_fixnum */
  lrecord_type_symbol_value_forward_boolint, /* struct symbol_value_forward_boolint */
  lrecord_type_symbol_value_varalias,    /* struct symbol_value_varalias */
  lrecord_type_symbol_value_lisp_magic,  /* struct symbol_value_lisp_magic */
  lrecord_type_symbol_value_buffer_local,/* struct symbol_value_buffer_local */
  lrecord_type_max_symbol_value_magic = lrecord_type_symbol_value_buffer_local,

  /* Keep the rest of these (up to #ifdef EVENT_DATA_AS_OBJECTS) sorted,
     to facilitate keeping .gdbinit.in.in in sync.  Also sort within
     the #ifdef EVENT_DATA_AS_OBJECTS and within the commented-out */
  lrecord_type_bigfloat,		/* Lisp_Bigfloat */
  lrecord_type_bignum,			/* Lisp_Bignum */
  lrecord_type_bit_vector,		/* Lisp_Bit_Vector */
  lrecord_type_buffer,			/* struct buffer */
  lrecord_type_case_table,		/* Lisp_Case_Table */
  lrecord_type_category_table,		/* Lisp_Category_Table */
  lrecord_type_char_subtable,		/* Lisp_Char_Subtable */
  lrecord_type_char_table,		/* Lisp_Char_Table */
  lrecord_type_charset,			/* Lisp_Charset */
  lrecord_type_coding_system,		/* Lisp_Coding_System */
  lrecord_type_color_instance,		/* Lisp_Color_Instance */
  lrecord_type_command_builder,		/* struct command_builder */
  lrecord_type_compiled_function,	/* Lisp_Compiled_Function */
  lrecord_type_cons,			/* Lisp_Cons */
  lrecord_type_console,			/* struct console */
  lrecord_type_database,		/* Lisp_Database */
  lrecord_type_detection_state,		/* struct detection_state */
  lrecord_type_device,			/* struct device */
  lrecord_type_devmode,			/* Lisp_Devmode */
  lrecord_type_emacs_ffi,		/* emacs_ffi_data */
  lrecord_type_emacs_gtk_boxed,		/* emacs_gtk_boxed_data */
  lrecord_type_emacs_gtk_object,	/* emacs_gtk_object_data */
  lrecord_type_ephemeron,		/* struct ephemeron */
  lrecord_type_event,			/* Lisp_Event */
  lrecord_type_extent,			/* struct extent */
  lrecord_type_extent_auxiliary,	/* struct extent_auxiliary */
  lrecord_type_extent_info,		/* struct extent_info */
  lrecord_type_face,			/* Lisp_Face */
  lrecord_type_fc_config,		/* struct fc_config */
  lrecord_type_fc_pattern,		/* struct fc_pattern */
  lrecord_type_float,			/* Lisp_Float */
  lrecord_type_font_instance,		/* Lisp_Font_Instance */
  lrecord_type_frame,			/* struct frame */
  lrecord_type_glyph,			/* Lisp_Glyph */
  lrecord_type_gui_item,		/* Lisp_Gui_Item */
  lrecord_type_hash_table,		/* Lisp_Hash_Table */
  lrecord_type_hash_table_test,		/* Hash_Table_Test */
  lrecord_type_image_instance,		/* Lisp_Image_Instance */
  lrecord_type_keymap,			/* Lisp_Keymap */
  lrecord_type_lcrecord_list,		/* struct lcrecord_list */
  lrecord_type_lstream,			/* struct lstream */
  lrecord_type_marker,			/* Lisp_Marker */
  lrecord_type_mswindows_dialog_id,	/* struct mswindows_dialog_id */
  lrecord_type_multiple_value,		/* multiple_value */
  lrecord_type_opaque,			/* Lisp_Opaque */
  lrecord_type_opaque_ptr,		/* Lisp_Opaque_Ptr */
  lrecord_type_precedence_array,	/* struct precedence_array */
  lrecord_type_process,			/* Lisp_Process */
  lrecord_type_range_table,		/* Lisp_Range_Table */
  lrecord_type_ratio,			/* Lisp_Ratio */
  lrecord_type_scrollbar_instance,	/* struct scrollbar_instance */
  lrecord_type_specifier,		/* Lisp_Specifier */
  lrecord_type_string,			/* Lisp_String */
  lrecord_type_subr,			/* Lisp_Subr */
  lrecord_type_symbol,			/* Lisp_Symbol */
  lrecord_type_timeout,			/* Lisp_Timeout */
  lrecord_type_toolbar_button,		/* struct toolbar_button */
  lrecord_type_tooltalk_message,	/* Lisp_Tooltalk_Message */
  lrecord_type_tooltalk_pattern,	/* Lisp_Tooltalk_Pattern */
  lrecord_type_vector,			/* Lisp_Vector */
  lrecord_type_weak_list,		/* struct weak_list */
  lrecord_type_window,			/* struct window */
  lrecord_type_window_mirror,		/* struct window_mirror */
  lrecord_type_expose_ignore,		/* struct expose_ignore */

  lrecord_type_free,			/* only used for "free" lrecords */
  lrecord_type_undefined,		/* only used for debugging */
#if !defined (HAVE_SHLIB)
# ifdef HAVE_POSTGRESQL
  lrecord_type_pgconn,
  lrecord_type_pgresult,
# endif
# ifdef HAVE_LDAP
  lrecord_type_ldap,
# endif
#endif /* !defined (HAVE_SHLIB) */
  lrecord_type_last_built_in_type,	/* must be last */
};

extern MODULE_API int lrecord_type_count;

struct lrecord_implementation
{
  /* Name of the type, as an interned symbol. */
  Lisp_Object name;

  /* `printer' converts the object to a printed representation.  `printer'
     should never be NULL (if so, you will get an assertion failure when
     trying to print such an object).  Either supply a specific printing
     method, or use the default methods internal_object_printer() (for
     internal objects that should not be visible at Lisp level) or
     external_object_printer() (for objects visible at Lisp level). */
  void (*printer) (Lisp_Object, Lisp_Object printcharfun, int escapeflag);

  /* `finalizer' is called at GC time when the object is about to be freed.
     It should perform any necessary cleanup, such as freeing malloc()ed
     memory or releasing pointers or handles to objects created in external
     libraries, such as window-system windows or file handles.  This can be
     NULL, meaning no special finalization is necessary. */
  void (*finalizer) (Lisp_Object obj);

  /* This can be NULL, meaning compare objects with EQ(). */
  int (*equal) (Lisp_Object obj1, Lisp_Object obj2, int depth,
		int foldcase);

  /* `hash' generates hash values for use with hash tables that have
     `equal' as their test function.  This can be NULL, meaning use
     the Lisp_Object itself as the hash.  But, you must still satisfy
     the constraint that if two objects are `equal', then they *must*
     hash to the same value in order for hash tables to work properly.
     This means that `hash' can be NULL only if the `equal' method is
     also NULL. */
  Hashcode (*hash) (Lisp_Object, int, Boolint);

  /* Data layout description for your object.  See long comment below. */
  const struct memory_description *description;

  /* Only one of `static_size' and `size_in_bytes_method' is non-0.  If
     `static_size' is 0, this type is not instantiable by
     ALLOC_NORMAL_LISP_OBJECT().  If both are 0 (this should never happen),
     this object cannot be instantiated; you will get an abort() if you
     try.*/
  Bytecount static_size;
  Bytecount (*size_in_bytes_method) (Lisp_Object);

  /**********************************************************************/
  /* Remaining methods are not assignable statically using
     DEFINE_*_LISP_OBJECT, but must be assigned with OBJECT_HAS_METHOD,
     OBJECT_HAS_PROPERTY or the like. */

  /* The next two methods are for objects that may be recursive;
     print_preprocess descends OBJ, adding any encountered subobjects to
     NUMBER_TABLE if it's not already there. This is used by #'print when
     print-circle or relatedly print-gensym are non-nil. */
  void (*print_preprocess) (Lisp_Object obj, Lisp_Object number_table,
			    Elemcount *seen_object_count);

  /* nsubst_structures_descend descends OBJECT, modifying it by replacing any
     sub-object that is EQ to OLD with NEW_. Used by #'nsubst when the
     :descend-structures keyword is supplied, and by the Lisp reader when
     reading objects that may be circular or that may use uninterned
     symbols. */
  void (*nsubst_structures_descend) (Lisp_Object new_, Lisp_Object old,
                                     Lisp_Object object,
                                     Lisp_Object number_table,
                                     Boolint test_not_unboundp);

  /* These functions allow any object type to have builtin property
     lists that can be manipulated from the lisp level with
     `get', `put', `remprop', and `object-plist'. */
  Lisp_Object (*getprop) (Lisp_Object obj, Lisp_Object prop);
  int (*putprop) (Lisp_Object obj, Lisp_Object prop, Lisp_Object val);
  int (*remprop) (Lisp_Object obj, Lisp_Object prop);
  Lisp_Object (*plist) (Lisp_Object obj);
  Lisp_Object (*setplist) (Lisp_Object obj, Lisp_Object newplist);

  /* `disksave' is called at dump time.  It is used for objects that
     contain pointers or handles to objects created in external libraries,
     such as window-system windows or file handles.  Such external objects
     cannot be dumped, so it is necessary to release them at dump time and
     arrange somehow or other for them to be resurrected if necessary later
     on.

     It seems that even non-dumpable objects may be around at dump time,
     and a disksave may be provided. (In fact, the only object currently
     with a disksave, lstream, is non-dumpable.)
     
     Objects rarely need to provide this method; most of the time it will
     be NULL. */
  void (*disksave) (Lisp_Object);

#ifdef MEMORY_USAGE_STATS
  /* Return memory-usage information about the object in question, stored
     into STATS.

     Two types of information are stored: storage (including overhead) for
     ancillary non-Lisp structures attached to the object, and storage
     (including overhead) for ancillary Lisp objects attached to the
     object.  The third type of memory-usage information (storage for the
     object itself) is not noted here, because it's computed automatically
     by the calling function.  Also, the computed storage for ancillary
     Lisp objects is the sum of all three source of memory associated with
     the Lisp object: the object itself, ancillary non-Lisp structures and
     ancillary Lisp objects.  Note also that the `struct usage_stats u' at
     the beginning of the STATS structure is for ancillary non-Lisp usage
     *ONLY*; do not store any memory into it related to ancillary Lisp
     objects.

     Note that it may be subjective which Lisp objects are considered
     "attached" to the object.  Some guidelines:

     -- Lisp objects which are "internal" to the main object and not
        accessible except through the main object should be included
     -- Objects linked by a weak reference should *NOT* be included
  */
  void (*memory_usage) (Lisp_Object obj, struct generic_usage_stats *stats);
#else
  /* Keep this structure the same size, for the sake of modules. */
  void (*memory_usage) (Lisp_Object obj, void *stats);
#endif

  /* The index into lrecord_implementations_table. Usually reflects an enum
     lrecord_type value but for a type created by modules can be a value
     outwith those. */
  int lrecord_type_index;

  /* A "frob-block" lrecord is any lrecord that's not an lcrecord, i.e.
     one that does not have an old_lcrecord_header at the front and which
     is (usually) allocated in frob blocks. */
  unsigned int frob_block_p :1;

  /* information for the dumper: is the object dumpable and should it 
     be dumped. */
  unsigned int dumpable :1;
};

/* All the built-in lisp object types are enumerated in `enum lrecord_type'.
   Additional ones may be defined by a module (none yet).  We leave some
   room in `lrecord_implementations_table' for such new lisp object types. */
#define MODULE_DEFINABLE_TYPE_COUNT 32

extern MODULE_API struct lrecord_implementation *
lrecord_implementations_table[lrecord_type_last_built_in_type + MODULE_DEFINABLE_TYPE_COUNT];

/* Given a built-in C type name (cons, bignum and so on; usually the entry in
   enum lrecord_type without the preceding lrecord_type_), return a pointer to
   the lrecord_implementation of the corresponding Lisp object. */
#define LRECORD_IMPLEMENTATION(type) \
  lrecord_implementations_table[lrecord_type_##type]

#define LRECORD_IMPLEMENTATION_IBYTE_NAME(implementation)       \
  (XSTRING_DATA (XSYMBOL_NAME ((implementation)->name)))

/* Given a Lisp object, return its implementation
   (struct lrecord_implementation) */
#define XRECORD_LHEADER_IMPLEMENTATION(obj) \
   LHEADER_IMPLEMENTATION (XRECORD_LHEADER (obj))
#define LHEADER_IMPLEMENTATION(lh) lrecord_implementations_table[(lh)->type]

#include "gc.h"


extern int gc_in_progress;


enum lrecord_alloc_status
{
  ALLOC_IN_USE,
  ALLOC_FREE,
  ALLOC_ON_FREE_LIST
};

void tick_lrecord_stats (const struct lrecord_header *h,
			 enum lrecord_alloc_status status);

#define LRECORD_FREE_P(ptr)					\
(((struct lrecord_header *) ptr)->type == lrecord_type_free)

#define MARK_LRECORD_AS_FREE(ptr)					\
((void) (((struct lrecord_header *) ptr)->type = lrecord_type_free))

#define MARKED_RECORD_P(obj) (XRECORD_LHEADER (obj)->mark)
#define MARKED_RECORD_HEADER_P(lheader) ((lheader)->mark)
#define MARK_RECORD_HEADER(lheader)   ((void) ((lheader)->mark = 1))
#define UNMARK_RECORD_HEADER(lheader) ((void) ((lheader)->mark = 0))

#define C_READONLY_RECORD_HEADER_P(lheader)  ((lheader)->c_readonly)
#define LISP_READONLY_RECORD_HEADER_P(lheader)  ((lheader)->lisp_readonly)
#define SET_C_READONLY_RECORD_HEADER(lheader) do {	\
  struct lrecord_header *SCRRH_lheader = (lheader);	\
  SCRRH_lheader->c_readonly = 1;			\
  SCRRH_lheader->lisp_readonly = 1;			\
  SCRRH_lheader->mark = 1;				\
} while (0)
#define SET_LISP_READONLY_RECORD_HEADER(lheader) \
  ((void) ((lheader)->lisp_readonly = 1))
#define CLEAR_C_READONLY_RECORD_HEADER(lheader) do {	\
  struct lrecord_header *CCRRH_lheader = (lheader);	\
  CCRRH_lheader->c_readonly = 0;			\
  CCRRH_lheader->lisp_readonly = 0;			\
  CCRRH_lheader->mark = 0;				\
} while (0)

#define RECORD_DESCRIPTION(lheader) lrecord_memory_descriptions[(lheader)->type]

#define RECORD_DUMPABLE(lheader) (lrecord_implementations_table[(lheader)->type])->dumpable

/* Data description stuff

   Data layout descriptions describe blocks of memory (in particular, Lisp
   objects and C data segment or C heap objects generated at dump time with or
   without pointers to Lisp objects), including their size and a list of the
   elements that need relocating, marking or other special handling. They are
   used in two places: by pdump, which dumps Lisp state (and a certain amount
   of C state) at build time, and by KKCC, the mark algorithm of the garbage
   collector.

   The two subsystems use the descriptions in different ways, and as a result
   some of the descriptions are appropriate only for one or the other, when it
   is known that only that subsystem will use the description. (This is
   particularly the case with objects that can't be dumped, because pdump
   needs more info than KKCC.) However, properly written descriptions are
   appropriate for both, and you should strive to write your descriptions that
   way, since the dumpable status of an object may change and new uses for the
   descriptions may be created.

   More specifically:

   Pdump (the portable dumper) needs to write out all reachable Lisp objects,
   and a certain amount of non-Lisp data generated at runtime (called "root
   blocks" and "root block pointers"), and later on (in another invocation of
   XEmacs) load them back into memory, relocating all pointers to the Lisp
   objects and to the root blocks to reflect their new memory addresses.

   The initial allocation of the Lisp objects at dump time is on the C heap
   (where memory allocated with malloc() comes from), and the reloaded Lisp
   objects can live in an mmap()ed area (when the dump file is external to
   XEmacs) or in the C data segment (when DUMP_IN_EXEC is defined). The
   reloaded data-segment blocks are copied from the dump file back into the
   data segment and relocated, and the reloaded Lisp objects remain where they
   are but are relocated in-place.

   Pdump, then, needs to be told about the location of all global pointers to
   Lisp objects, all the descriptions of all such objects, including their
   size and any pointers to other relocatable data (usually Lisp objects). The
   heap usually occurs in different places in different invocations --
   therefore, it is not enough simply to write out the entire heap and later
   reload it at the same location.

   A more recent wrinkle is that the C data segment and the C "text" segment
   (executable machine code) are now usually intentionally loaded at different
   addresses from invocation to invocation, and so pdump needs to be aware
   explicitly of what data (often within the root blocks) reflects a pointer
   into the C data segment, and what data (e.g. the SUBR_FN field of
   Lisp_Subr) reflects a pointer to a C function.

   (This relocation of the C data segment and the C text segment was
   introduced to reduce the severity of buffer-overflow attacks. C convention
   is that the address to return to when the current function has finished is
   stored adjacent to user data in the stack, and C has no shortage of
   functions that operate on stack data (usually buffers, C arrays) without
   bounds-checking, so it was and remains possible to overwrite that address;
   the relocation described, termed address space layout randomization, ASLR,
   means it is more difficult for attackers to predict an address that would
   be useful to them and write that to the stack as the return address.)

   As mentioned above, it is possible and indeed necessary for pdump to
   restore non-Lisp data, the "root blocks".  Many files through XEmacs ignore
   this possiblity, and have reinit_vars_of*() functions that usually push
   to post-dump what could have been done at dump time. Some of this is
   because of bugs in pdump (if a pointer to a Lisp object is to be found
   within a root block, that Lisp_Object should be protected from garbage
   collection, but that is not currently automatically done), some of it
   really isn't possible because of problems with external libraries, and some
   it may reflect confusion from the situation before unexec (the pre-pdump
   solution to this problem) was removed and this code was very heavily
   #ifdef'd and difficult to maintain.

   Descriptions are used by pdump in three places: (a) descriptions of Lisp
   objects, referenced in the DEFINE_*LRECORD_*IMPLEMENTATION*() call; (b)
   descriptions of global objects to be dumped, registered by
   dump_add_root_block(); (c) descriptions of global pointers to
   non-Lisp_Object heap objects, registered by dump_add_root_block_ptr().
   The descriptions need to tell pdump which elements of your structure are
   Lisp_Objects or structure pointers, plus the descriptions in turn of the
   non-Lisp_Object structures pointed to.  If these structures are your own
   private ones, you will have to write these recursive descriptions
   yourself; otherwise, you are reusing a structure already in existence
   elsewhere and there is probably already a description for it.

   Pdump does not care about Lisp objects that cannot be dumped (the
   dumpable flag to DEFINE_*LRECORD_*IMPLEMENTATION*() is 0).

   KKCC also uses data layout descriptions, but differently.  It cares
   about all objects, dumpable or not, but specifically only wants to know
   about Lisp_Objects in your object and in structures pointed to.  Thus,
   it doesn't care about things like pointers to structures ot other blocks
   of memory with no Lisp Objects in them, which pdump would care a lot
   about.

   Technically, then, you could write your description differently
   depending on whether your object is dumpable -- the full pdump
   description if so, the abbreviated KKCC description if not.  In fact,
   some descriptions are written this way.  This is dangerous, though,
   because another use might come along for the data descriptions, that
   doesn't care about the dumper flag and makes use of some of the stuff
   normally omitted from the "abbreviated" description -- see above.

   A memory_description is an array of values.  The first value of each
   line is a type, the second the offset in the lrecord structure.  The
   third and following elements are parameters; their presence, type and
   number is type-dependent.

   The description ends with an "XD_END" record.

   The top-level description of an lrecord or lcrecord does not need
   to describe every element, just the ones that need to be relocated,
   since the size of the lrecord is known. (The same goes for nested
   structures, whenever the structure size is given, rather than being
   defaulted by specifying 0 for the size.)

   A sized_memory_description is a memory_description plus the size of the
   block of memory.  The size field in a sized_memory_description can be
   given as zero, i.e. unspecified, meaning that the last element in the
   structure is described in the description and the size of the block can
   therefore be computed from it. (This is useful for stretchy arrays.)

   memory_descriptions are used to describe lrecords (the size of the
   lrecord is elsewhere in its description, attached to its methods, so it
   does not need to be given here) and global objects, where the size is an
   argument to the call to dump_add_root_block().
   sized_memory_descriptions are used for pointers and arrays in
   memory_descriptions and for calls to dump_add_root_block_ptr(). (####
   It is not obvious why this is so in the latter case.  Probably, calls to
   dump_add_root_block_ptr() should use plain memory_descriptions and have
   the size be an argument to the call.)

   NOTE: Anywhere that a sized_memory_description occurs inside of a plain
   memory_description, a "description map" can be substituted.  Rather than
   being an actual description, this describes how to find the description
   by looking inside of the object being described.  This is a convenient
   way to describe Lisp objects with subtypes and corresponding
   type-specific data.

   Some example descriptions :

   struct Lisp_Cons
   {
     FROB_BLOCK_LISP_OBJECT_HEADER lheader;
     Lisp_Object car_, cdr_;
   };

   static const struct memory_description cons_description[] = {
     { XD_LISP_OBJECT, offsetof (Lisp_Cons, car_) },
     { XD_LISP_OBJECT, offsetof (Lisp_Cons, cdr_) },
     { XD_END }
   };

   Which means "two lisp objects starting at the 'car_' and 'cdr_' elements"

   struct Lisp_String
   {
     struct lrecord_header lheader;
     Bytecount size_;
     Ibyte *data_;
     Lisp_Object plist;
   };
   
   static const struct memory_description string_description[] = {
     { XD_BYTECOUNT,       offsetof (Lisp_String, size_) },
     { XD_OPAQUE_DATA_PTR, offsetof (Lisp_String, data_), XD_INDIRECT(0, 1) },
     { XD_LISP_OBJECT,     offsetof (Lisp_String, plist) },
     { XD_END }
   };

   "A pointer to string data at 'data_', the size of the pointed array being
   the value of the size_ variable plus 1, and one lisp object at 'plist'"

   If your object has a pointer to an array of Lisp_Objects in it, something
   like this:

   struct Lisp_Foo
   {
     ...;
     Elemcount count;
     Lisp_Object *objects;
     ...;
   }

   You'd use XD_BLOCK_PTR, something like:

   static const struct memory_description foo_description[] = {
     ...
     { XD_ELEMCOUNT,	offsetof (Lisp_Foo, count) },
     { XD_BLOCK_PTR,	offsetof (Lisp_Foo, objects),
       XD_INDIRECT (0, 0), { &lisp_object_description } },
     ...
   };

   lisp_object_description is declared in gc.c, like this:

   static const struct memory_description lisp_object_description_1[] = {
     { XD_LISP_OBJECT, 0 },
     { XD_END }
   };

   const struct sized_memory_description lisp_object_description = {
     sizeof (Lisp_Object),
     lisp_object_description_1
   };

   Another example of XD_BLOCK_PTR:

   typedef struct htentry
   {
     Lisp_Object key;
     Lisp_Object value;
   } htentry;
   
   struct Lisp_Hash_Table
   {
     NORMAL_LISP_OBJECT_HEADER header;
     Elemcount size;
     Elemcount count;
     Elemcount rehash_count;
     double rehash_size;
     double rehash_threshold;
     Elemcount golden_ratio;
     hash_table_hash_function_t hash_function;
     hash_table_test_function_t test_function;
     htentry *hentries;
     enum hash_table_weakness weakness;
   };

   static const struct memory_description htentry_description_1[] = {
     { XD_LISP_OBJECT, offsetof (htentry, key) },
     { XD_LISP_OBJECT, offsetof (htentry, value) },
     { XD_END }
   };
   
   static const struct sized_memory_description htentry_description = {
     sizeof (htentry),
     htentry_description_1
   };
   
   const struct memory_description hash_table_description[] = {
     { XD_ELEMCOUNT,     offsetof (Lisp_Hash_Table, size) },
     { XD_BLOCK_PTR, offsetof (Lisp_Hash_Table, hentries), XD_INDIRECT (0, 1),
	 { &htentry_description } },
     { XD_END }
   };

   Note that we don't need to declare all the elements in the structure, just
   the ones that need to be relocated (Lisp_Objects and structures), those
   that need to be referenced as counts for relocated objects, or those that
   point to the C data segment.

   A description map looks like this:

   static const struct sized_memory_description specifier_extra_description_map [] = {
   { offsetof (Lisp_Specifier, methods) },
   { offsetof (struct specifier_methods, extra_description) },
   { -1 }
   };
 
   const struct memory_description specifier_description[] = {
     ...
     { XD_BLOCK_ARRAY, offset (Lisp_Specifier, data), 1,
       { specifier_extra_description_map } },
     ...
     { XD_END }
   };

   This would be appropriate for an object that looks like this:
 
   struct specifier_methods
   {
     ...
     const struct sized_memory_description *extra_description;
     ...
   };

   struct Lisp_Specifier
   {
     NORMAL_LISP_OBJECT_HEADER header;
     struct specifier_methods *methods;
   
     ...
     // type-specific extra data attached to a specifier
     max_align_t data[1];
   };

   The description map means "retrieve a pointer into the object at offset
   `offsetof (Lisp_Specifier, methods)' , then in turn retrieve a pointer
   into that object at offset `offsetof (struct specifier_methods,
   extra_description)', and that is the sized_memory_description to use." 
   There can be any number of indirections, which can be either into
   straight pointers or Lisp_Objects.  The way that description maps are
   distinguished from normal sized_memory_descriptions is that in the
   former, the memory_description pointer is NULL.

   --ben


   The existing types :


    XD_LISP_OBJECT

  A Lisp_Object.  This is also the type to use for pointers to other lrecords
  (e.g. struct frame *).

    XD_LISP_OBJECT_ARRAY

  An array of Lisp_Objects or (equivalently) pointers to lrecords.
  The parameter (i.e. third element) is the count.  This would be declared
  as Lisp_Object foo[666].  For something declared as Lisp_Object *foo,
  use XD_BLOCK_PTR, whose description parameter is a sized_memory_description
  consisting of only XD_LISP_OBJECT and XD_END.

    XD_LO_LINK

  Weak link in a linked list of objects of the same type.  This is a
  link that does NOT generate a GC reference.  Thus the pdumper will
  not automatically add the referenced object to the table of all
  objects to be dumped, and when storing and loading the dumped data
  will automatically prune unreferenced objects in the chain and link
  each referenced object to the next referenced object, even if it's
  many links away.  We also need to special handling of a similar
  nature for the root of the chain, which will be a staticpro()ed
  object.  There should not, in general, be a need for an XD_LO_LINK entry
  outside of the implementation of the Lisp weak list type.  See the
  htentry_union_description_1 structure in elhash.c for the approach to marking
  objects that are either weak or not; the relevant XD_BLOCK_PTR has an
  XD_FLAG_NO_KKCC flag set.

    XD_OPAQUE_PTR

  Pointer to undumpable data.  Must be NULL when dumping.

    XD_OPAQUE_PTR_CONVERTIBLE

  Pointer to data which is not directly dumpable but can be converted
  to a dumpable, opaque external representation.  The parameter is
  a pointer to an opaque_convert_functions struct.

    XD_OPAQUE_DATA_CONVERTIBLE

  Data which is not directly dumpable but can be converted to a
  dumpable, opaque external representation.  The parameter is a
  pointer to an opaque_convert_functions struct.

    XD_BLOCK_PTR

  Pointer to block of described memory. Parameters are number of contiguous
  blocks and sized_memory_description.

    XD_BLOCK_DATA_PTR

  Pointer to block of described memory in the C data segment (which will need
  to be relocated differently than XD_BLOCK_PTR, but is identical to previous
  with regard to garbage collection). Same parameters as XD_BLOCK_PTR.

    XD_BLOCK_ARRAY

  Array of blocks of described memory.  Parameters are number of
  structures and sized_memory_description.  This differs from XD_BLOCK_PTR
  in that the parameter is declared as struct foo[666] instead of
  struct *foo.  In other words, the block of memory holding the
  structures is within the containing structure, rather than being
  elsewhere, with a pointer in the containing structure.

  NOTE NOTE NOTE: Be sure that you understand the difference between
  XD_BLOCK_PTR and XD_BLOCK_ARRAY:
    - struct foo bar[666], i.e. 666 inline struct foos
        --> XD_BLOCK_ARRAY, argument 666, pointing to a description of
            struct foo
    - struct foo *bar, i.e. pointer to a block of 666 struct foos
        --> XD_BLOCK_PTR, argument 666, pointing to a description of
            struct foo
    - struct foo *bar[666], i.e. 666 pointers to separate blocks of struct foos
        --> XD_BLOCK_ARRAY, argument 666, pointing to a description of
	    a single pointer to struct foo; the description is a single
	    XD_BLOCK_PTR, argument 1, which in turn points to a description
	    of struct foo.

  NOTE also that an XD_BLOCK_PTR of 666 foos is equivalent to an
  XD_BLOCK_PTR of 1 bar, where the description of `bar' is an
  XD_BLOCK_ARRAY of 666 foos. 

    XD_OPAQUE_DATA_PTR

  Pointer to dumpable opaque data.  Parameter is the size of the data.
  Pointed data must be relocatable without changes.

    XD_UNION

  Union of two or more different types of data.  Parameters are a constant
  which determines which type the data is (this is usually an XD_INDIRECT,
  referring to one of the fields in the structure), and a "sizing lobby" (a
  sized_memory_description, which points to a memory_description and
  indicates its size).  The size field in the sizing lobby describes the
  size of the union field in the object, and the memory_description in it
  is referred to as a "union map" and has a special interpretation: The
  offset field is replaced by a constant, which is compared to the first
  parameter of the XD_UNION descriptor to determine if this description
  applies to the union data, and XD_INDIRECT references refer to the
  containing object and description.  Note that the description applies
  "inline" to the union data, like XD_BLOCK_ARRAY and not XD_BLOCK_PTR.
  If the union data is a pointer to different types of structures, each
  element in the memory_description should be an XD_BLOCK_PTR.  See
  unicode.c, redisplay.c and fontcolor.c for examples of XD_UNION.

    XD_UNION_DYNAMIC_SIZE

  Same as XD_UNION except that this is used for objects where the size of
  the object containing the union varies depending on the particular value
  of the union constant.  That is, an object with plain XD_UNION typically
  has the union declared as `union foo' or as `void *', where an object
  with XD_UNION_DYNAMIC_SIZE typically has the union as the last element,
  and declared as something like Rawbyte foo[1].  With plain XD_UNION, the
  object is (usually) of fixed size and always contains enough space for
  the data associated with all possible union constants, and thus the union
  constant can potentially change during the lifetime of the object.  With
  XD_UNION_DYNAMIC_SIZE, however, the union constant is fixed at the time
  of creation of the object, and the size of the object is computed
  dynamically at creation time based on the size of the data associated
  with the union constant.  Currently, the only difference between XD_UNION
  and XD_UNION_DYNAMIC_SIZE is how the size of the union data is
  calculated, when (a) the structure containing the union has no size
  given; (b) the union occurs as the last element in the structure; and (c)
  the union has no size given (in the first-level sized_memory_description
  pointed to).  In this circumstance, the size of XD_UNION comes from the
  max size of the data associated with all possible union constants,
  whereas the size of XD_UNION_DYNAMIC_SIZE comes from the size of the data
  associated with the currently specified (and unchangeable) union
  constant.

    XD_FUNCTION_POINTER

  Pointer to a C function, that will need to be relocated on systems with
  address space layout randomization (ASLR).

    XD_MEMORY_DESCRIPTION

  Pointer to a struct memory_description. These are all in the C data segment
  and need to be relocated on systems with address space layout randomization
  (ASLR).

    XD_SIZED_MEMORY_DESCRIPTION

  Pointer to a struct sized_memory_description. These are handled in the same
  way as XD_MEMORY_DESCRIPTION, but any error checking on dump and load (none
  so far) will need to be handled differently.

    XD_ASCII_STRING

  Pointer to a C string, purely ASCII.

    XD_INT_RESET

  An integer which will be reset to a given value in the dump file.

    XD_ELEMCOUNT

  Elemcount value.  Used for counts.

    XD_BYTECOUNT

  Bytecount value.  Used for counts.

    XD_HASHCODE

  Hashcode value.  Used for the results of hashing functions.

    XD_INT

  int value.  Used for counts.

    XD_LONG

  long value.  Used for counts.

    XD_BYTECOUNT

  bytecount value.  Used for counts.

    XD_END

  Special type indicating the end of the array.


  Special macros:

    XD_INDIRECT (line, delta)
  Usable where a count, size, offset or union constant is requested.  Gives
  the value of the element which is at line number 'line' in the
  description (count starts at zero) and adds delta to it, which must
  (currently) be positive.
*/

enum memory_description_type
{
  XD_LISP_OBJECT_ARRAY,
  XD_LISP_OBJECT,
  XD_LO_LINK,
  XD_OPAQUE_PTR,
  XD_OPAQUE_PTR_CONVERTIBLE,
  XD_OPAQUE_DATA_CONVERTIBLE,
  XD_OPAQUE_DATA_PTR,
  XD_BLOCK_PTR,
  XD_BLOCK_DATA_PTR,
  XD_BLOCK_ARRAY,
  XD_UNION,
  XD_UNION_DYNAMIC_SIZE,
  XD_FUNCTION_POINTER,
  XD_MEMORY_DESCRIPTION,
  XD_SIZED_MEMORY_DESCRIPTION,
  XD_ASCII_STRING,
  XD_INT_RESET,
  XD_BYTECOUNT,
  XD_ELEMCOUNT,
  XD_HASHCODE,
  XD_INT,
  XD_LONG,
  XD_END
};

enum data_description_entry_flags
{
  /* If set, KKCC does not process this entry.

  (1) One obvious use is with things that pdump saves but which do not get
  marked normally -- for example the next and prev fields in a marker.  The
  marker chain is weak, with its entries removed when they are finalized.

  (2) This can be set on structures not containing any Lisp objects, or (more
  usefully) on structures that contain Lisp objects but where the objects
  always occur in another structure as well.  For example, the extent lists
  kept by a buffer keep the extents in two lists, one sorted by the start
  of the extent and the other by the end.  There's no point in marking
  both, since each contains the same objects as the other; but when dumping
  (if we were to dump such a structure), when computing memory size, etc.,
  it's crucial to tag both sides.
  */
  XD_FLAG_NO_KKCC = 1,
  /* If set, pdump does not process this entry. */
  XD_FLAG_NO_PDUMP = 2,
  /* Indicates that this is a "default" entry in a union map. */
  XD_FLAG_UNION_DEFAULT_ENTRY = 4,
  /* Indicates that this is a free Lisp object we're marking.
     Only relevant for ERROR_CHECK_GC.  This occurs when we're marking
     lcrecord-lists, where the objects have had their type changed to
     lrecord_type_free and also have had their free bit set, but we mark
     them as normal. */
  XD_FLAG_FREE_LISP_OBJECT = 8,
#if 0
  /* Suggestions for other possible flags: */

  /* Eliminate XD_UNION_DYNAMIC_SIZE and replace it with a flag, like this. */
  XD_FLAG_UNION_DYNAMIC_SIZE = 16,
  /* Require that everyone who uses a description map has to flag it, so
     that it's easy to tell, when looking through the code, where the
     description maps are and who's using them.  This might also become
     necessary if for some reason the format of the description map is
     expanded and we need to stick a pointer in the second slot (although
     we could still ensure that the second slot in the first entry was NULL
     or <0). */
  XD_FLAG_DESCRIPTION_MAP = 32,
#endif
};

union memory_contents_description
{
  /* The first element is used by static initializers only.  We always read
     from one of the other two pointers. */
  const void *write_only;
  const struct sized_memory_description *descr;
  const struct opaque_convert_functions *funcs;
};

struct memory_description
{
  enum memory_description_type type;
  Bytecount offset;
  EMACS_INT data1;
  union memory_contents_description data2;
  /* Indicates which subsystems process this entry, plus (potentially) other
     flags that apply to this entry. */
  int flags;
};

struct sized_memory_description
{
  Bytecount size;
  const struct memory_description *description;
};


struct opaque_convert_functions
{
  /* Used by XD_OPAQUE_PTR_CONVERTIBLE and
     XD_OPAQUE_DATA_CONVERTIBLE */

  /* Converter to external representation, for those objects from
     external libraries that can't be directly dumped as opaque data
     because they contain pointers.  This is called at dump time to
     convert to an opaque, pointer-less representation.

     This function must put a pointer to the opaque result in *data
     and its size in *size. */
  void (*convert) (const void *object, void **data, Bytecount *size);

  /* Post-conversion cleanup.  Optional (null if not provided).

     When provided it will be called post-dumping to free any storage
     allocated for the conversion results. */
  void (*convert_free) (const void *object, void *data, Bytecount size);

  /* De-conversion.

     At reload time, rebuilds the object from the converted form.
     "object" is 0 for the PTR case, return is ignored in the DATA
     case. */
  void *(*deconvert) (void *object, void *data, Bytecount size);

  /* There is no current (2024) need for memory_description or
     sized_memory_descriptions for struct opaque_convert_functions, the one
     use in number.c is constructed statically at compile time. This may
     change. */
};

/* If MEMORY_USAGE_STATS is defined, initialize stats for TYPE. If it is not
   defined, do nothing. */
extern MODULE_API void init_memory_usage_stats (int type,
                                                Lisp_Object
                                                memusage_stats_list);

#define INIT_MEMORY_USAGE_STATS(type, memusage_stats_list)      \
  init_memory_usage_stats (lrecord_type_##type, memusage_stats_list)

#define XD_INDIRECT(val, delta) (-1 - (Bytecount) ((val) | ((delta) << 8)))

#define XD_IS_INDIRECT(code) ((code) < 0)
#define XD_INDIRECT_VAL(code) ((-1 - (code)) & 255)
#define XD_INDIRECT_DELTA(code) ((-1 - (code)) >> 8)

/* DEFINE_*_LISP_OBJECT is for objects with constant size. (Either
   DEFINE_DUMPABLE_LISP_OBJECT for objects that can be saved in a dumped
   executable, or DEFINE_NODUMP_LISP_OBJECT for objects that cannot be
   saved -- e.g. that contain pointers to non-persistent external objects
   such as window-system windows.)

   DEFINE_*_SIZABLE_LISP_OBJECT is for objects whose size varies.

   DEFINE_*_FROB_BLOCK_LISP_OBJECT is for objects that are allocated in
   large blocks ("frob blocks"), which are parceled up individually.  Such
   objects need special handling in alloc.c.

   DEFINE_*_INTERNAL_LISP_OBJECT is for "internal" objects that should
   never be visible on the Lisp level.  This is a shorthand for the most
   common type of internal objects, which have no equal or hash method
   (since they generally won't appear in hash tables), no finalizer and
   internal_object_printer() as their print method (which prints that the
   object is internal and shouldn't be visible externally).  For internal
   objects needing a finalizer, equal or hash method, or wanting to
   customize the print method, use the normal DEFINE_*_LISP_OBJECT
   mechanism for defining these objects.

   MAKE_LISP_OBJECT is what underlies all of these; it allocates a structure
   containing pointers to object methods and other info such as the size of
   the structure containing the object, and puts a pointer to that structure
   into lrecord_implementations_table. */

/********* The dumpable versions *********** */

#define DEFINE_DUMPABLE_LISP_OBJECT(name, c_name, printer, nuker, equal, \
                                    hash, desc, structtype)              \
  MAKE_LISP_OBJECT (name, c_name, 1 /*dumpable*/, printer, nuker, equal, \
                    hash, desc, sizeof (structtype), 0, 0)

#define DEFINE_DUMPABLE_SIZABLE_LISP_OBJECT(name, c_name, printer, nuker,\
                                            equal, hash, desc, sizer,    \
                                            structtype)                  \
  MAKE_LISP_OBJECT (name, c_name, 1 /*dumpable*/, printer, nuker, equal, \
                    hash, desc, 0, sizer, 0)

#define DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT(name, c_name, printer, \
                                               nuker, equal, hash,    \
                                               desc, structtype)      \
  MAKE_LISP_OBJECT (name, c_name, 1 /*dumpable*/, printer, nuker,     \
                    equal, hash, desc, sizeof(structtype), 0, 1)

#define DEFINE_DUMPABLE_FROB_BLOCK_SIZABLE_LISP_OBJECT(name, c_name,    \
                                                       printer, nuker,  \
                                                       equal, hash,     \
                                                       desc, sizer,     \
                                                       structtype)      \
  MAKE_LISP_OBJECT (name, c_name, 1 /*dumpable*/, printer, nuker,       \
                    equal, hash, desc, 0, sizer, 1)

#define DEFINE_DUMPABLE_INTERNAL_LISP_OBJECT(name, c_name, desc,        \
                                             structtype)                \
  DEFINE_DUMPABLE_LISP_OBJECT (name, c_name, internal_object_printer, 0,\
                               0, 0, desc, structtype)

#define DEFINE_DUMPABLE_SIZABLE_INTERNAL_LISP_OBJECT(name, c_name, desc, \
                                                     sizer, structtype)  \
  DEFINE_DUMPABLE_SIZABLE_LISP_OBJECT(name, c_name,                      \
                                      internal_object_printer, 0, 0, 0,  \
                                      desc, sizer)

/********* The non-dumpable versions *********** */

#define DEFINE_NODUMP_LISP_OBJECT(name, c_name, printer, nuker, equal,  \
                                  hash, desc, structtype)               \
  MAKE_LISP_OBJECT (name, c_name, 0 /*non-dumpable*/, printer, nuker,   \
                    equal, hash, desc, sizeof (structtype), 0, 0)

#define DEFINE_NODUMP_SIZABLE_LISP_OBJECT(name, c_name, printer, nuker, \
                                          equal, hash, desc, sizer,     \
                                          structtype)                   \
  MAKE_LISP_OBJECT (name, c_name, 0 /*non-dumpable*/, printer, nuker,   \
                    equal, hash, desc, 0, sizer, 0)

#define DEFINE_NODUMP_FROB_BLOCK_LISP_OBJECT(name, c_name, printer,     \
                                             nuker, equal, hash, desc,  \
                                             structtype)                \
  MAKE_LISP_OBJECT (name, c_name, 0 /*non-dumpable*/, printer, nuker,   \
                    equal, hash, desc, sizeof (structtype), 0, 1)

#define DEFINE_NODUMP_FROB_BLOCK_SIZABLE_LISP_OBJECT(name, c_name, printer,   \
                                                     nuker, equal, hash,      \
                                                     desc, sizer, structtype) \
  MAKE_LISP_OBJECT (name, c_name, 0 /*non-dumpable*/, printer, nuker,   \
                    equal, hash, desc, 0, sizer, 1)

#define DEFINE_NODUMP_INTERNAL_LISP_OBJECT(name, c_name, desc, structtype) \
  DEFINE_NODUMP_LISP_OBJECT (name, c_name, internal_object_printer, 0,     \
                                  0, 0, desc, structtype)

#define DEFINE_NODUMP_SIZABLE_INTERNAL_LISP_OBJECT(name, c_name, desc,  \
                                                   sizer, structtype)   \
  DEFINE_NODUMP_SIZABLE_LISP_OBJECT (name, c_name,                      \
                                     internal_object_printer, 0, 0, 0,  \
                                     desc, sizer, structtype)

/********* MAKE_LISP_OBJECT, the underlying macro *********** */

#define MAKE_LISP_OBJECT(lisp_name, c_name, dumpable, m_l_o_printer,       \
                         nuker, m_l_o_equal, m_l_o_hash, desc, size, sizer,\
                         frob_block_p)                                     \
  do {                                                                     \
    define_lisp_object (init_lrecord_type_##c_name (), lisp_name, size,    \
                        desc, dumpable, frob_block_p);                     \
    OBJECT_HAS_NAMED_METHOD (c_name, printer, m_l_o_printer);              \
    OBJECT_HAS_NAMED_METHOD (c_name, finalizer, nuker);                    \
    OBJECT_HAS_NAMED_METHOD (c_name, equal, m_l_o_equal);                  \
    OBJECT_HAS_NAMED_METHOD (c_name, hash, m_l_o_hash);                    \
    OBJECT_HAS_NAMED_METHOD (c_name, size_in_bytes_method,                 \
                             sizer);                                       \
  } while (0)

extern MODULE_API const struct memory_description *lrecord_memory_descriptions[];

extern MODULE_API void define_lisp_object (int lrecord_type,
                                           const CIbyte *name,
                                           Bytecount size,
                                           const struct memory_description *,
                                           Boolint dumpable,
                                           Boolint frob_block_p);

#ifdef HAVE_SHLIB

/* Allow undefining types in order to support module unloading. */
extern MODULE_API void undef_lisp_object (int lrecord_type);

#define UNDEF_MODULE_LISP_OBJECT(c_name) \
  undef_lisp_object (lrecord_type_##c_name)

#endif /* HAVE_SHLIB */

/*************** Macros for declaring that a Lisp object has a
                 particular method, or for calling such a method. ********/

/* Declare that object-type TYPE has method M; used in
   initialization routines */
#define OBJECT_HAS_METHOD(type, m) \
  (LRECORD_IMPLEMENTATION (type)->m = type##_##m)
/* Same but the method name come before the type */
#define OBJECT_HAS_PREMETHOD(type, m) \
  (LRECORD_IMPLEMENTATION (type)->m = m##_##type)
/* Same but the name of the method is explicitly given */
#define OBJECT_HAS_NAMED_METHOD(type, m, func) \
  (LRECORD_IMPLEMENTATION (type)->m = (func))
/* Object type has a property with the given value. */
#define OBJECT_HAS_PROPERTY(type, prop, val) \
  (LRECORD_IMPLEMENTATION (type)->prop = (val))

/* Does the given object method exist? */
#define HAS_OBJECT_METH_P(obj, m) \
  (!!(XRECORD_LHEADER_IMPLEMENTATION (obj)->m))
/* Call an object method. */
#define OBJECT_METH(obj, m, args) \
  ((XRECORD_LHEADER_IMPLEMENTATION (obj)->m) args)

/* Call an object method, if it exists. */
#define MAYBE_OBJECT_METH(obj, m, args)			\
do							\
{							\
  const struct lrecord_implementation *_mom_imp =	\
    XRECORD_LHEADER_IMPLEMENTATION (obj);		\
  if (_mom_imp->m)					\
    ((_mom_imp->m) args);				\
} while (0)

/* Call an object method, if it exists, or return GIVEN.  NOTE:
   Multiply-evaluates OBJ. */
#define OBJECT_METH_OR_GIVEN(obj, m, args, given)  \
  (HAS_OBJECT_METH_P (obj, m) ?	OBJECT_METH (obj, m, args) : (given))

#define OBJECT_PROPERTY(obj, prop) (XRECORD_LHEADER_IMPLEMENTATION (obj)->prop)

/************** Other stuff **************/

#define LRECORDP(a) (XTYPE (a) == Lisp_Type_Record)
#define XRECORD_LHEADER(a) ((struct lrecord_header *) XPNTR (a))

#define RECORD_TYPEP(x, ty) \
  (LRECORDP (x) && (XRECORD_LHEADER (x)->type == (unsigned int) (ty)))

/* Steps to create a new object:

   1. Declare the struct for your object in a header file somewhere.
   Remember that it must begin with

   NORMAL_LISP_OBJECT_HEADER header;

   2. Put the "standard junk" (DECLARE_LISP_OBJECT()/XFOO/etc.) below the
      struct definition -- see below.

   3. Add this header file to inline.c.

   4. Create the methods for your object. You don't need any. The old (almost
   universal) need for a mark method has been superseded by the memory
   description used by KKCC.

   4. Create the data layout description for your object.  See
   toolbar_button_description below; the comment above in `struct lrecord',
   describing the purpose of the descriptions; and comments elsewhere in
   this file describing the exact syntax of the description structures.

   6. Define your object with DEFINE_*_LISP_OBJECT() or some
   variant.  This needs to be in the C file's syms_of_foo() function.  At the
   minimum, you need to decide whether your object can be dumped.  Objects
   that are created as part of the loadup process and need to be persistent
   across dumping should be created dumpable.  Nondumpable objects are
   generally those associated with display, particularly those containing a
   pointer to an external library object (e.g. a window-system window).

   7. Include the header file in the .c file where you defined the object.

   8. Add a type enum for the object to enum lrecord_type, earlier in this
   file.

   --ben

  An example:

------------------------------ in toolbar.h -----------------------------

  struct toolbar_button
  {
    NORMAL_LISP_OBJECT_HEADER header;
  
    Lisp_Object next;
    Lisp_Object frame;
  
    Lisp_Object up_glyph;
    Lisp_Object down_glyph;
    Lisp_Object disabled_glyph;
  
    Lisp_Object cap_up_glyph;
    Lisp_Object cap_down_glyph;
    Lisp_Object cap_disabled_glyph;
  
    Lisp_Object callback;
    Lisp_Object enabled_p;
    Lisp_Object help_string;
  
    char enabled;
    char down;
    char pushright;
    char blank;
  
    int x, y;
    int width, height;
    int dirty;
    int vertical;
    int border_width;
  };
  
  [[ the standard junk: ]]
  
  DECLARE_LISP_OBJECT (toolbar_button, struct toolbar_button);
  #define XTOOLBAR_BUTTON(x) XRECORD (x, toolbar_button, struct toolbar_button)
  #define wrap_toolbar_button(p) wrap_record (p, toolbar_button)
  #define TOOLBAR_BUTTONP(x) RECORDP (x, toolbar_button)
  #define CHECK_TOOLBAR_BUTTON(x) CHECK_RECORD (x, toolbar_button)
  #define CONCHECK_TOOLBAR_BUTTON(x) CONCHECK_RECORD (x, toolbar_button)
  
------------------------------ in toolbar.c -----------------------------
  
  #include "toolbar.h"
  
  ...
  
  static const struct memory_description toolbar_button_description [] = {
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, next) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, frame) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, up_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, down_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, disabled_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, cap_up_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, cap_down_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, cap_disabled_glyph) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, callback) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, enabled_p) },
    { XD_LISP_OBJECT, offsetof (struct toolbar_button, help_string) },
    { XD_END }
  };
  
  static Lisp_Object
  allocate_toolbar_button (struct frame *f, int pushright)
  {
    struct toolbar_button *tb;
  
    tb = XTOOLBAR_BUTTON (ALLOC_NORMAL_LISP_OBJECT (toolbar_button));
    tb->next = Qnil;
    tb->frame = wrap_frame (f);
    tb->up_glyph = Qnil;
    tb->down_glyph = Qnil;
    tb->disabled_glyph = Qnil;
    tb->cap_up_glyph = Qnil;
    tb->cap_down_glyph = Qnil;
    tb->cap_disabled_glyph = Qnil;
    tb->callback = Qnil;
    tb->enabled_p = Qnil;
    tb->help_string = Qnil;
  
    tb->pushright = pushright;
    tb->x = tb->y = tb->width = tb->height = -1;
    tb->dirty = 1;
  
    return wrap_toolbar_button (tb);
  }
   
  ...
  
  void
  syms_of_toolbar (void)
  {
    DEFINE_NODUMP_LISP_OBJECT ("toolbar-button", toolbar_button,
                               external_object_printer, 0, 0, 0,
    			       toolbar_button_description,
    			       struct toolbar_button);
  
  
    ...;
  }
  
------------------------------ in inline.c -----------------------------
  
  #ifdef HAVE_TOOLBARS
  #include "toolbar.h"
  #endif
  
------------------------------ in lrecord.h -----------------------------
  
  enum lrecord_type
  {
    ...
    lrecord_type_toolbar_button,
    ...
  };

------------------------------ in .gdbinit.in.in -----------------------------

  ...
  else
  if $lrecord_type == lrecord_type_toolbar_button
    pstructtype toolbar_button
  ...
  ...
  ...
  end

  --ben

*/

/* Note: Object types defined in external dynamically-loaded modules (not part
   of the XEmacs main source code) should use DECLARE_MODULE_LISP_OBJECT() in
   their header files (or early in the C file if no header file), to allow
   declaration of an enumerator for the type being defined. The
   DEFINE_*_LISP_OBJECT() macros can be used in the normal way in the
   syms_of_foo() function of the module C file. (Previously there were
   separate DEFINE_*_LISP_OBJECT() macros to be used in modules.) */
#define DECLARE_LISP_OBJECT(c_name, structtype)                         \
DECLARE_INLINE_HEADER (                                                 \
int                                                                    \
init_lrecord_type_##c_name (void)                                       \
)									\
{									\
  structure_checking_assert (lrecord_type_##c_name != 0);               \
  structure_checking_assert (lrecord_type_##c_name <                    \
                             lrecord_type_last_built_in_type);          \
  return lrecord_type_##c_name;                                         \
}									\
DECLARE_LISP_OBJECT_1 (c_name, structtype, extern)

#define DECLARE_MODULE_API_LISP_OBJECT(c_name, structtype)		\
DECLARE_INLINE_HEADER (                                                 \
int                                                                    \
init_lrecord_type_##c_name (void)                                       \
)									\
{									\
  structure_checking_assert (lrecord_type_##c_name != 0);               \
  structure_checking_assert (lrecord_type_##c_name <                    \
                             lrecord_type_last_built_in_type);          \
  return lrecord_type_##c_name;                                         \
}									\
DECLARE_LISP_OBJECT_1 (c_name, structtype, extern MODULE_API)

#ifdef HAVE_SHLIB
#define DECLARE_MODULE_LISP_OBJECT(c_name, structtype)                  \
extern int lrecord_type_##c_name;                                       \
DECLARE_INLINE_HEADER (                                                 \
int                                                                     \
init_lrecord_type_##c_name (void)                                       \
)									\
{									\
  return lrecord_type_##c_name = lrecord_type_count++;                  \
}									\
DECLARE_LISP_OBJECT_1 (c_name, structtype, extern)
#else
#define DECLARE_MODULE_LISP_OBJECT DECLARE_LISP_OBJECT
#endif

#ifdef ERROR_CHECK_TYPES

# define DECLARE_LISP_OBJECT_1(c_name, structtype, visibility)		\
DECLARE_INLINE_HEADER (							\
structtype *								\
error_check_##c_name (Lisp_Object obj, const Ascbyte *file, int line)	\
)									\
{									\
  assert_at_line (RECORD_TYPEP (obj, lrecord_type_##c_name), file, line); \
  return (structtype *) XPNTR (obj);					\
}									\
visibility Lisp_Object Q##c_name##p

# define XRECORD(x, c_name, structtype) \
  error_check_##c_name (x, __FILE__, __LINE__)

DECLARE_INLINE_HEADER (
Lisp_Object
wrap_record_1 (const void *ptr, int ty, const Ascbyte *file,
	       int line)
)
{
  Lisp_Object obj = wrap_pointer_1 (ptr);

  assert_at_line (RECORD_TYPEP (obj, ty), file, line);
  return obj;
}

#define wrap_record(ptr, ty) \
  wrap_record_1 (ptr, lrecord_type_##ty, __FILE__, __LINE__)

#else /* not ERROR_CHECK_TYPES */

# define DECLARE_LISP_OBJECT_1(c_name, structtype, visibility)     \
visibility Lisp_Object Q##c_name##p

# define XRECORD(x, c_name, structtype) ((structtype *) XPNTR (x))
/* wrap_pointer_1 is so named as a suggestion not to use it unless you
   know what you're doing. */
#define wrap_record(ptr, ty) wrap_pointer_1 (ptr)

#endif /* not ERROR_CHECK_TYPES */

#define RECORDP(x, c_name) RECORD_TYPEP (x, lrecord_type_##c_name)

/* Note: we now have two different kinds of type-checking macros.
   The "old" kind has now been renamed CONCHECK_foo.  The reason for
   this is that the CONCHECK_foo macros signal a continuable error,
   allowing the user (through debug-on-error) to substitute a different
   value and return from the signal, which causes the lvalue argument
   to get changed.  Quite a lot of code would crash if that happened,
   because it did things like

   foo = XCAR (list);
   CHECK_STRING (foo);

   and later on did XSTRING (XCAR (list)), assuming that the type
   is correct (when it might be wrong, if the user substituted a
   correct value in the debugger).

   To get around this, I made all the CHECK_foo macros signal a
   non-continuable error.  Places where a continuable error is OK
   (generally only when called directly on the argument of a Lisp
   primitive) should be changed to use CONCHECK().

   FSF Emacs does not have this problem because RMS took the cheesy
   way out and disabled returning from a signal entirely. */

#define CONCHECK_RECORD(x, c_name) do {			\
 if (!RECORD_TYPEP (x, lrecord_type_##c_name))		\
   x = wrong_type_argument (Q##c_name##p, x);		\
}  while (0)
#define CONCHECK_NONRECORD(x, lisp_enum, predicate) do {\
 if (XTYPE (x) != lisp_enum)				\
   x = wrong_type_argument (predicate, x);		\
 } while (0)
#define CHECK_RECORD(x, c_name) do {			\
 if (!RECORD_TYPEP (x, lrecord_type_##c_name))		\
   dead_wrong_type_argument (Q##c_name##p, x);		\
 } while (0)
#define CHECK_NONRECORD(x, lisp_enum, predicate) do {	\
 if (XTYPE (x) != lisp_enum)				\
   dead_wrong_type_argument (predicate, x);		\
 } while (0)

/*-------------------------- lcrecord-list -----------------------------*/

struct lcrecord_list
{
  NORMAL_LISP_OBJECT_HEADER header;
  Lisp_Object free;
  Bytecount size;
  const struct lrecord_implementation *implementation;
};

DECLARE_LISP_OBJECT (lcrecord_list, struct lcrecord_list);
#define XLCRECORD_LIST(x) XRECORD (x, lcrecord_list, struct lcrecord_list)
#define wrap_lcrecord_list(p) wrap_record (p, lcrecord_list)
#define LCRECORD_LISTP(x) RECORDP (x, lcrecord_list)
/* #define CHECK_LCRECORD_LIST(x) CHECK_RECORD (x, lcrecord_list)
   Lcrecord lists should never escape to the Lisp level, so
   functions should not be doing this. */

/* Various ways of allocating lcrecords.  All bytes (except lcrecord
   header) are zeroed in returned structure.

   See above for a discussion of the difference between plain lrecords and
   lrecords.  lcrecords themselves are divided into three types: (1)
   auto-managed, (2) hand-managed, and (3) unmanaged.  "Managed" refers to
   using a special object called an lcrecord-list to keep track of freed
   lcrecords, which can freed with free_normal_lisp_object() or the like
   and later be recycled when a new lcrecord is required, rather than
   requiring new malloc().  Thus, allocation of lcrecords can be very
   cheap. (Technically, the lcrecord-list manager could divide up large
   chunks of memory and allocate out of that, mimicking what happens with
   lrecords.  At that point, however, we'd want to rethink the whole
   division between lrecords and lcrecords.)

   NOTE: There is a fundamental limitation of lcrecord-lists, which is that
   they only handle blocks of a particular, fixed size.  Thus, objects that
   can be of varying sizes need to do various tricks.  These considerations
   in particular dictate the various types of management:

   -- "Auto-managed" means that you just go ahead and allocate the lcrecord
   whenever you want, using ALLOC_NORMAL_LISP_OBJECT(), and the appropriate
   lcrecord-list manager is automatically created.  To free, you just call
   "free_normal_lisp_object()" and the appropriate lcrecord-list manager is
   automatically located and called.  The limitation here of course is that
   all your objects are of the same size. (#### Eventually we should have a
   more sophisticated system that tracks the sizes seen and creates one
   lcrecord list per size, indexed in a hash table.  Usually there are only
   a limited number of sizes, so this works well.)

   -- "Hand-managed" exists because we haven't yet written the more
   sophisticated scheme for auto-handling different-sized lcrecords, as
   described in the end of the last paragraph.  In this model, you go ahead
   and create the lcrecord-list objects yourself for the sizes you will
   need, using make_lcrecord_list().  Then, create lcrecords using
   alloc_managed_lcrecord(), passing in the lcrecord-list you created, and
   free them with free_managed_lcrecord().

   -- "Unmanaged" means you simply allocate lcrecords, period.  No
   lcrecord-lists, no way to free them.  This may be suitable when the
   lcrecords are variable-sized and (a) you're too lazy to write the code
   to hand-manage them, or (b) the objects you create are always or almost
   always Lisp-visible, and thus there's no point in freeing them (and it
   wouldn't be safe to do so).  You just create them with
   ALLOC_SIZED_LISP_OBJECT(), and that's it.

   --ben

   Here is an in-depth look at the steps required to create a allocate an
   lcrecord using the hand-managed style.  Since this is the most
   complicated, you will learn a lot about the other styles as well.  In
   addition, there is useful general information about what freeing an
   lcrecord really entails, and what are the precautions:

   1) Create an lcrecord-list object using make_lcrecord_list().  This is
      often done at initialization.  Remember to staticpro_nodump() this
      object!  The arguments to make_lcrecord_list() are the same as would be
      passed to ALLOC_SIZED_LISP_OBJECT().

   2) Instead of calling ALLOC_SIZED_LISP_OBJECT(), call
      alloc_managed_lcrecord() and pass the lcrecord-list earlier created.

   3) When done with the lcrecord, call free_managed_lcrecord().  The
      standard freeing caveats apply: ** make sure there are no pointers to
      the object anywhere! **

   4) Calling free_managed_lcrecord() is just like kissing the
      lcrecord goodbye as if it were garbage-collected.  This means:
      -- the contents of the freed lcrecord are undefined, and the
         contents of something produced by alloc_managed_lcrecord()
	 are undefined, just like for ALLOC_SIZED_LISP_OBJECT().
      -- the finalize method for the lcrecord's type will be called
         at the time that free_managed_lcrecord() is called.
 */

/* UNMANAGED MODEL: */
Lisp_Object old_alloc_lcrecord (const struct lrecord_implementation *);
Lisp_Object old_alloc_sized_lcrecord (Bytecount size,
				      const struct lrecord_implementation *);

/* HAND-MANAGED MODEL: */
Lisp_Object make_lcrecord_list (Bytecount size,
				const struct lrecord_implementation
				*implementation);
Lisp_Object alloc_managed_lcrecord (Lisp_Object lcrecord_list);
void free_managed_lcrecord (Lisp_Object lcrecord_list, Lisp_Object lcrecord);

/* AUTO-MANAGED MODEL: */
MODULE_API Lisp_Object
alloc_automanaged_sized_lcrecord (Bytecount size,
				  const struct lrecord_implementation *imp);
MODULE_API Lisp_Object
alloc_automanaged_lcrecord (const struct lrecord_implementation *imp);

MODULE_API Lisp_Object
alloc_automanaged_c_readonly_lcrecord (Bytecount,
				       const struct lrecord_implementation *);

#define old_alloc_lcrecord_type(type, imp) \
  ((type *) XPNTR (alloc_automanaged_lcrecord (sizeof (type), imp)))

void old_free_lcrecord (Lisp_Object rec);

DECLARE_INLINE_HEADER (
Bytecount
detagged_lisp_object_size (const struct lrecord_header *h)
)
{
  const struct lrecord_implementation *imp = LHEADER_IMPLEMENTATION (h);

  return (imp->size_in_bytes_method ?
	  imp->size_in_bytes_method (wrap_pointer_1 (h)) :
	  imp->static_size);
}

DECLARE_INLINE_HEADER (
Bytecount
lisp_object_size (Lisp_Object o)
)
{
  return detagged_lisp_object_size (XRECORD_LHEADER (o));
}

struct usage_stats;

MODULE_API void copy_lisp_object (Lisp_Object dst, Lisp_Object src);
MODULE_API void zero_sized_lisp_object (Lisp_Object obj, Bytecount size);
MODULE_API void zero_nonsized_lisp_object (Lisp_Object obj);
Bytecount lisp_object_storage_size (Lisp_Object obj,
				    struct usage_stats *ustats);
Bytecount lisp_object_memory_usage_full (Lisp_Object object,
					 Bytecount *storage_size,
					 Bytecount *extra_nonlisp_storage,
					 Bytecount *extra_lisp_storage,
					 struct generic_usage_stats *stats);
Bytecount lisp_object_memory_usage (Lisp_Object object);
Bytecount tree_memory_usage (Lisp_Object arg, int vectorp);
void free_normal_lisp_object (Lisp_Object obj);


/************************************************************************/
/*		                 Dumping                		*/
/************************************************************************/

/* dump_add_root_block_ptr (&var, &desc) dumps the structure pointed to by
   `var'.  This is for a single relocatable pointer located in the data
   segment (i.e. the block pointed to is in the heap).

   If the structure pointed to is not a `struct' but an array, you should
   set the size field of the sized_memory_description to 0, and use
   XD_BLOCK_ARRAY in the inner memory_description.

   NOTE that a "root struct pointer" could also be described using
   dump_add_root_block(), with SIZE == sizeof (void *), and a description
   containing a single XD_BLOCK_PTR entry, offset 0, size 1, with a
   structure description the same as the value passed to
   dump_add_root_block_ptr().  That would require an extra level of
   description, though, as compared to using dump_add_root_block_ptr(),
   and thus this function is generally more convenient.
    */
void dump_add_root_block_ptr (void *, const struct sized_memory_description *);

/* dump_add_opaque (&var, size) dumps the opaque static structure `var'.
   This is for a static block of memory (in the data segment, not the
   heap), with no relocatable pointers in it. */
#define dump_add_opaque(varaddr,size) dump_add_root_block (varaddr, size, NULL)

/* dump_add_root_block (ptr, size, desc) dumps the static structure
   located at `var' of size SIZE and described by DESC.  This is for a
   static block of memory (in the data segment, not the heap), with
   relocatable pointers in it. */
void dump_add_root_block (const void *ptraddress, Bytecount size,
			  const struct memory_description *desc);

/* Call dump_add_opaque_int (&int_var) to dump `int_var', of type `int'. */
#define dump_add_opaque_int(int_varaddr) do {	\
  int *dao_ = (int_varaddr); /* type check */	\
  dump_add_opaque (dao_, sizeof (*dao_));	\
} while (0)

/* Call dump_add_opaque_fixnum (&fixnum_var) to dump `fixnum_var', of type
   `Fixnum'. */
#define dump_add_opaque_fixnum(fixnum_varaddr) do {	\
  Fixnum *dao_ = (fixnum_varaddr); /* type check */	\
  dump_add_opaque (dao_, sizeof (*dao_));		\
} while (0)

/* Call dump_add_root_lisp_object (&var) to ensure that var is properly
   updated after pdump. */
void dump_add_root_lisp_object (Lisp_Object *);

/* Like dump_add_root_lisp_object(), but tell the dumper that VAR should be
   initialized to Qnil on pdump_load(), irrespective of its value at dump
   time.  */
void dump_add_nil_lisp_object (Lisp_Object *);

/* Call dump_add_weak_lisp_object (&var) to ensure that var is properly
   updated after pdump.  var must point to a linked list of objects out of
   which some may not be dumped */
void dump_add_weak_object_chain (Lisp_Object *);

#define DUMP_ADD_WEAK_OBJECT_CHAIN(var) do {                            \
    /* Don't add to the chain before marking it for dumping! */         \
    gc_checking_assert (EQ (var, Qnull_pointer));                       \
    var = Qnil;                                                         \
    dump_add_weak_object_chain (&var);                                  \
  } while (0)

/* Nonzero means Emacs has already been initialized.
   Used during startup to detect startup of dumped Emacs.  */
extern MODULE_API int initialized;

#include "dumper.h"
#define DUMPEDP(adr) ((((Rawbyte *) (adr)) < pdump_end) && \
                      (((Rawbyte *) (adr)) >= pdump_start))

#define OBJECT_DUMPED_P(obj) DUMPEDP (XPNTR (obj))

/***********************************************************************/
/*                           data descriptions                         */
/***********************************************************************/

extern int in_pdump;

EMACS_INT lispdesc_indirect_count_1 (EMACS_INT code,
				     const struct memory_description *idesc,
				     const void *idata);
const struct sized_memory_description *lispdesc_indirect_description_1
 (const void *obj, const struct sized_memory_description *sdesc);
Bytecount lispdesc_block_size_1 (const void *obj, Bytecount size,
			         const struct memory_description *desc);

DECLARE_INLINE_HEADER (
Bytecount lispdesc_block_size (const void *obj,
			       const struct sized_memory_description *sdesc))
{
  return lispdesc_block_size_1 (obj, sdesc->size, sdesc->description);
}

DECLARE_INLINE_HEADER (
EMACS_INT
lispdesc_indirect_count (EMACS_INT code,
			 const struct memory_description *idesc,
			 const void *idata)
)
{
  if (XD_IS_INDIRECT (code))
    code = lispdesc_indirect_count_1 (code, idesc, idata);
  return code;
}

DECLARE_INLINE_HEADER (
const struct sized_memory_description *
lispdesc_indirect_description (const void *obj,
			       const struct sized_memory_description *sdesc)
)
{
  if (sdesc->description)
    return sdesc;
  else
    return lispdesc_indirect_description_1 (obj, sdesc);
}


/* Do standard XD_UNION processing.  DESC1 is an entry in DESC, which
   describes the entire data structure.  Returns NULL (do nothing, nothing
   matched), or a new value for DESC1.  In the latter case, assign to DESC1
   in your function and goto union_switcheroo. */

DECLARE_INLINE_HEADER (
const struct memory_description *
lispdesc_process_xd_union (const struct memory_description *desc1,
			   const struct memory_description *desc,
			   const void *data)
)
{
  int count = 0;
  EMACS_INT variant = lispdesc_indirect_count (desc1->data1, desc,
					       data);
  desc1 =
    lispdesc_indirect_description (data, desc1->data2.descr)->description;
  
  for (count = 0; desc1[count].type != XD_END; count++)
    {
      if ((desc1[count].flags & XD_FLAG_UNION_DEFAULT_ENTRY) ||
	  desc1[count].offset == variant)
	{
	  return &desc1[count];
	}
    }

  return NULL;
}

END_C_DECLS

#endif /* INCLUDED_lrecord_h_ */
