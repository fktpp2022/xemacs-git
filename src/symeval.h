/* Definitions of symbol-value forwarding for XEmacs Lisp interpreter.
   Copyright (C) 1985, 1986, 1987, 1992, 1993 Free Software Foundation, Inc.
   Copyright (C) 2000, 2001, 2002, 2010 Ben Wing.

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

/* Fsymbol_value checks whether XSYMBOL (sym)->value is one of these,
 *  and does weird magic stuff if so */

#ifndef INCLUDED_symeval_h_
#define INCLUDED_symeval_h_

BEGIN_C_DECLS

enum symbol_value_type
{
  /* The following tags use the 'symbol_value_forward_fixnum' structure
     and are strictly for variables DEFVARed on the C level. */
  SYMVAL_FIXNUM_FORWARD,	/* Forward C "Fixnum", really "EMACS_INT" */
  SYMVAL_CONST_FIXNUM_FORWARD,	/* Same, but can't be set */

  /* The following tags use the 'symbol_value_forward_boolint' structure, also
     only for variables DEFVARed in C. */
  SYMVAL_BOOLEAN_FORWARD,	/* Forward C boolean ("Boolint") */
  SYMVAL_CONST_BOOLEAN_FORWARD,	/* Same, but can't be set */

  /* The following tags use the 'symbol_value_forward_object' structure, also
     for variables DEFVARed in C. */
  SYMVAL_OBJECT_FORWARD,	/* Forward C Lisp_Object */
  SYMVAL_CONST_OBJECT_FORWARD,	/* Same, but can't be set */
  SYMVAL_CONST_SPECIFIER_FORWARD, /* Same, can't be set, but gives a
                                     different message when attempting to
				     set that says "use set-specifier" */
  SYMVAL_DEFAULT_BUFFER_FORWARD, /* Forward Lisp_Object into Vbuffer_defaults */
  SYMVAL_CURRENT_BUFFER_FORWARD, /* Forward Lisp_Object into current_buffer */
  SYMVAL_CONST_CURRENT_BUFFER_FORWARD, /* Forward Lisp_Object into
					  current_buffer, can't be set */
  SYMVAL_DEFAULT_CONSOLE_FORWARD, /* Forward Lisp_Object into
				     Vconsole_defaults */
  SYMVAL_SELECTED_CONSOLE_FORWARD, /* Forward Lisp_Object into
				      Vselected_console */
  SYMVAL_CONST_SELECTED_CONSOLE_FORWARD, /* Forward Lisp_Object into
					    Vselected_console,
					    can't be set */

  /* The following tags use the 'symbol_value_buffer_local' structure and can
     be created from Lisp.  */
  SYMVAL_BUFFER_LOCAL,		/* make-variable-buffer-local */
  SYMVAL_SOME_BUFFER_LOCAL,	/* make-local-variable */

  /* The following tag uses the 'symbol_value_lisp_magic' structure, ditto. */
  SYMVAL_LISP_MAGIC,		/* Forward to lisp callbacks */

  /* The following tag uses the 'symbol_value_varalias' structure, ditto. */
  SYMVAL_VARALIAS		/* defvaralias */
};

struct symbol_value_magic
{
  NORMAL_LISP_OBJECT_HEADER header; /* This is a struct old_lcrecord_header,
				       with a NEXT pointer after the struct
				       lrecord_header. This is abused in the
				       implementation of
				       symbol_value_forward_boolint and
				       symbol_value_forward_fixnum, which are
				       c_readonly and never GCed, so there is
				       no need for a next pointer. */
  enum symbol_value_type type;
};
#define SYMBOL_VALUE_MAGIC_P(x)						\
(LRECORDP (x) &&							\
 XRECORD_LHEADER (x)->type <= lrecord_type_max_symbol_value_magic)

#ifdef ERROR_CHECK_TYPES
DECLARE_INLINE_HEADER (
struct symbol_value_magic *
error_check_symbol_value_magic (Lisp_Object obj, const Ascbyte *file,
				int line)
)
{
  assert_at_line (SYMBOL_VALUE_MAGIC_P (obj), file, line);
  return (struct symbol_value_magic *) XPNTR (obj);
}

#define XSYMBOL_VALUE_MAGIC(x) \
  error_check_symbol_value_magic (x, __FILE__, __LINE__)

DECLARE_INLINE_HEADER (
Lisp_Object
wrap_symbol_value_magic_1 (const void *ptr, const Ascbyte *file, int line)
)
{
  Lisp_Object obj = wrap_pointer_1 (ptr);

  assert_at_line (SYMBOL_VALUE_MAGIC_P (obj), file, line);
  return obj;
}

#define wrap_symbol_value_magic(x) \
  wrap_symbol_value_magic_1(x, __FILE__, __LINE__)
#else
#define XSYMBOL_VALUE_MAGIC(x) ((struct symbol_value_magic *) XPNTR (obj))
#define wrap_symbol_value_magic(p) wrap_pointer_1 (p)
#endif /* ERROR_CHECK_TYPES */

#define XSYMBOL_VALUE_MAGIC_TYPE(v) (XSYMBOL_VALUE_MAGIC (v)->type)

void print_symbol_value_magic (Lisp_Object, Lisp_Object, int);

/********** The various different symbol-value-magic types ***********/

/* 1a. symbol-value-forward-object */

/* This type of symbol-value-magic is used for variables declared
   DEFVAR_LISP, DEFVAR_CONST_LISP, DEFVAR_SPECIFIER.

   Note that some of these types of variables can be made buffer-local.
   Then, the symbol's value field contains a symbol-value-buffer-local,
   whose CURRENT-VALUE field then contains a symbol-value-forward. */

struct symbol_value_forward_object
{
  struct symbol_value_magic magic;
  Lisp_Object *value;

  /* `magicfun' is a function controlling the magic behavior of this
      forward variable.

     SYM is the symbol being operated on (read, set, etc.);

     VAL is either the value to set or the value to be returned.

     IN_OBJECT is the buffer or console that the value is read in
       or set in.  A value of Qnil means that the current buffer
       and possibly other buffers are being set. (This value will
       never be passed for built-in buffer-local or console-local
       variables such as `truncate-lines'.) (Currently, a value of
       Qnil is always passed for DEFVAR_INT, DEFVAR_LISP, and
       DEFVAR_BOOL variables; the code isn't smart enough to figure
       out what buffers besides the current buffer are being
       affected.  Because the magic function is called
       before the value is changed, it's not that easy
       to determine which buffers are getting changed.
       #### If this information is important, let me know
       and I will look into providing it.) (Remember also
       that the only console-local variables currently existing
       are built-in ones, because others can't be created.)

     FLAGS gives more information about the operation being performed.

     The return value indicates what the magic function actually did.

     Currently FLAGS and the return value are not used.  This
     function is only called when the value of a forward variable
     is about to be changed.  Note that this can occur explicitly
     through a call to `set', `setq', `set-default', or `setq-default',
     or implicitly by the current buffer being changed.  */
  int (*magicfun) (Lisp_Object sym, Lisp_Object *val, Lisp_Object in_object,
		   int flags);
};
DECLARE_LISP_OBJECT (symbol_value_forward_object,
		     struct symbol_value_forward_object);
#define SYMBOL_VALUE_FORWARD_OBJECTP(x) RECORDP (x, symbol_value_forward_object)
#define XSYMBOL_VALUE_FORWARD_OBJECT(x) \
	XRECORD (x, symbol_value_forward_object,\
		 struct symbol_value_forward_object)
#define symbol_value_forward_object_forward(m) ((m)->value)
#define symbol_value_forward_object_magicfun(m) ((m)->magicfun)
#define XSYMBOL_VALUE_FORWARD_OBJECT_FORWARD(x) \
  symbol_value_forward_object_forward (XSYMBOL_VALUE_FORWARD_OBJECT (x))
#define XSYMBOL_VALUE_FORWARD_OBJECT_MAGICFUN(m) \
  symbol_value_forward_object_magicfun (XSYMBOL_VALUE_FORWARD_OBJECT (x))

/* 1b. symbol-value-forward-fixnum.

   This type of symbol-value-magic is used for variables declared DEFVAR_INT,
   DEFVAR_CONST_INT and DEFVAR_INT_MAGIC. Commentary above as for
   symbol_value_forward_object also applies.

   In contrast to symbol_value_forward_object, GC does not have to mark through
   symbol_value_forward_fixnum. This means we can mark these objects as
   c_readonly and lisp_readonly, and use what would otherwise be the NEXT
   pointer of the NORMAL_LISP_OBJECT_HEADER as the pointer to the C variable of
   interest. */

/* Underlying C type used to implement DEFVAR_INT */
typedef EMACS_INT Fixnum;

struct symbol_value_forward_fixnum_magic
{
  FROB_BLOCK_LISP_OBJECT_HEADER header;
  Fixnum *value;
  enum symbol_value_type type;
};

struct symbol_value_forward_fixnum
{
  struct symbol_value_forward_fixnum_magic magic;
  int (*magicfun) (Lisp_Object sym, Lisp_Object *val, Lisp_Object in_object,
		   int flags);
};
DECLARE_LISP_OBJECT (symbol_value_forward_fixnum,
		     struct symbol_value_forward_fixnum);
#define SYMBOL_VALUE_FORWARD_FIXNUMP(x) RECORDP (x, symbol_value_forward_fixnum)
#define XSYMBOL_VALUE_FORWARD_FIXNUM(x) \
	XRECORD (x, symbol_value_forward_fixnum,\
		 struct symbol_value_forward_fixnum)
#define symbol_value_forward_fixnum_forward(m) ((m)->magic.value)
#define symbol_value_forward_fixnum_magicfun(m) ((m)->magicfun)
#define XSYMBOL_VALUE_FORWARD_FIXNUM_FORWARD(x) \
  symbol_value_forward_fixnum_forward (XSYMBOL_VALUE_FORWARD_FIXNUM (x))
#define XSYMBOL_VALUE_FORWARD_FIXNUM_MAGICFUN(x) \
  symbol_value_forward_fixnum_magicfun (XSYMBOL_VALUE_FORWARD_FIXNUM (x))

/* 1c. symbol-value-forward-boolint.

   This type of symbol-value-magic is used for variables declared DEFVAR_BOOL,
   DEFVAR_CONST_BOOL and DEFVAR_BOOL_MAGIC. Commentary above as for
   symbol_value_forward_fixnum also applies. */

struct symbol_value_forward_boolint_magic
{
  FROB_BLOCK_LISP_OBJECT_HEADER header;
  Boolint *value;
  enum symbol_value_type type;
};

struct symbol_value_forward_boolint
{
  struct symbol_value_forward_boolint_magic magic;
  /* `magicfun' is a function controlling the magic behavior of this
     forward variable. Details as for symbol_value_forward_object. */
  int (*magicfun) (Lisp_Object sym, Lisp_Object *val, Lisp_Object in_object,
		   int flags);
};
DECLARE_LISP_OBJECT (symbol_value_forward_boolint,
		     struct symbol_value_forward_boolint);
#define SYMBOL_VALUE_FORWARD_BOOLINTP(x) RECORDP (x, symbol_value_forward_boolint)
#define XSYMBOL_VALUE_FORWARD_BOOLINT(x) \
	XRECORD (x, symbol_value_forward_boolint,\
		 struct symbol_value_forward_boolint)
#define symbol_value_forward_boolint_forward(m) ((m)->magic.value)
#define symbol_value_forward_boolint_magicfun(m) ((m)->magicfun)
#define XSYMBOL_VALUE_FORWARD_BOOLINT_FORWARD(x) \
  symbol_value_forward_boolint_forward (XSYMBOL_VALUE_FORWARD_BOOLINT (x))
#define XSYMBOL_VALUE_FORWARD_BOOLINT_MAGICFUN(x) \
  symbol_value_forward_boolint_magicfun(XSYMBOL_VALUE_FORWARD_BOOLINT (x))

/* 2. symbol-value-buffer-local */

struct symbol_value_buffer_local
{
  struct symbol_value_magic magic; /* The NEXT pointer of the
				      old_lcrecord_header is used here, by
				      ALLOC_NORMAL_LISP_OBJECT(). */
  /* Used in a symbol value cell when the symbol's value is per-buffer.

     The type of the symbol-value-magic will be either
     SYMVAL_BUFFER_LOCAL (i.e. `make-variable-buffer-local' was called)
     or SYMVAL_SOME_BUFFER_LOCAL (i.e. `make-local-variable' was called).
     The only difference between the two is that when setting the
     former kind of variable, an implicit `make-local-variable' is
     called.

     A buffer-local variable logically has

     -- a default value
     -- local values in some buffers

     The primary place where the local values are stored is in each
     buffer's local_var_alist slot.

     In the simplest implementation, all that this structure needs to
     keep track of is the default value; to retrieve the value in
     a buffer, look in that buffer's local_var_alist, and use the
     default value if there is no local value.  To implement
     `make-local-variable' in a buffer, look in the buffer's
     local_var_alist, and if no element exists for this symbol,
     add one, copying the value from the default value.  When setting
     the value in a buffer, look in the buffer's local_var_alist, and set
     the value in that list if an element exists for this symbol;
     otherwise, set the default. (Remember that SYMVAL_BUFFER_LOCAL
     variables implicitly call `make-local-variable' first, so when
     setting a value, there will always be an entry in the buffer's
     local_var_alist to set.)

     However, this operation is potentially slow.  To speed it up,
     we cache the value in one buffer in this structure.

     NOTE: This is *not* a write-through cache.  I.e. when setting
     the value in the buffer that is cached, we *only* change the
     cache and don't write the value through to either the buffer's
     local_var_alist or the default value.  Therefore, when retrieving
     a value in a buffer, you must *always* look in the cache to see if
     it refers to that buffer.

     The cache consists of

     -- a buffer, or nil if the cache has not been set up
     -- the value in that buffer
     -- the element (a cons) from the buffer's local_var_alist, or
        nil if there is no local value in the buffer

    These slots are called CURRENT-BUFFER, CURRENT-VALUE, and
    CURRENT-ALIST-ELEMENT, respectively.

    If we want to examine or set the value in BUFFER and CURRENT-BUFFER
    equals BUFFER, we just examine or set CURRENT-VALUE.  Otherwise,
    we store CURRENT-VALUE value into CURRENT-ALIST-ELEMENT (or maybe
    into DEFAULT-VALUE), then find the appropriate alist element for
    BUFFER and set up CURRENT-ALIST-ELEMENT.  Then we set CURRENT-VALUE
    out of that element (or maybe out of DEFAULT-VALUE), and store
    BUFFER into CURRENT-BUFFER.

    If we are setting the variable and the current buffer does not have
    an alist entry for this variable, an alist entry is created.

    Note that CURRENT-BUFFER's local_var_alist value for this variable
    might be out-of-date (the correct value is stored in CURRENT-VALUE).
    Similarly, if CURRENT-BUFFER sees the default value, then
    DEFAULT-VALUE might be out-of-date.

    Note that CURRENT-VALUE (but not DEFAULT-VALUE) can be a
    forwarding pointer.  Each time it is examined or set,
    forwarding must be done.
   */
  Lisp_Object default_value;
  Lisp_Object current_value;
  Lisp_Object current_buffer;
  Lisp_Object current_alist_element;
};
DECLARE_LISP_OBJECT (symbol_value_buffer_local, struct symbol_value_buffer_local);
#define XSYMBOL_VALUE_BUFFER_LOCAL(x) \
	XRECORD (x, symbol_value_buffer_local, struct symbol_value_buffer_local)
#define SYMBOL_VALUE_BUFFER_LOCAL_P(x) RECORDP (x, symbol_value_buffer_local)

/* 3. symbol-value-lisp-magic */

enum lisp_magic_handler
{
  MAGIC_HANDLER_GET_VALUE,
  MAGIC_HANDLER_SET_VALUE,
  MAGIC_HANDLER_BOUND_PREDICATE,
  MAGIC_HANDLER_MAKE_UNBOUND,
  MAGIC_HANDLER_LOCAL_PREDICATE,
  MAGIC_HANDLER_MAKE_LOCAL,
  MAGIC_HANDLER_MAX
};

struct symbol_value_lisp_magic
{
  struct symbol_value_magic magic;
  Lisp_Object handler[MAGIC_HANDLER_MAX];
  Lisp_Object harg[MAGIC_HANDLER_MAX];
  Lisp_Object shadowed;
};
DECLARE_LISP_OBJECT (symbol_value_lisp_magic, struct symbol_value_lisp_magic);
#define XSYMBOL_VALUE_LISP_MAGIC(x) \
	XRECORD (x, symbol_value_lisp_magic, struct symbol_value_lisp_magic)
#define SYMBOL_VALUE_LISP_MAGIC_P(x) RECORDP (x, symbol_value_lisp_magic)

/* 4. symbol-value-varalias */

struct symbol_value_varalias
{
  struct symbol_value_magic magic;
  Lisp_Object aliasee;
  Lisp_Object shadowed;
};
DECLARE_LISP_OBJECT (symbol_value_varalias,	struct symbol_value_varalias);
#define XSYMBOL_VALUE_VARALIAS(x) \
	XRECORD (x, symbol_value_varalias, struct symbol_value_varalias)
#define SYMBOL_VALUE_VARALIAS_P(x) RECORDP (x, symbol_value_varalias)
#define symbol_value_varalias_aliasee(m) ((m)->aliasee)
#define symbol_value_varalias_shadowed(m) ((m)->shadowed)

/* To define a Lisp primitive function using a C function `Fname', do this:
   DEFUN ("name, Fname, ...); // at top level in foo.c
   DEFSUBR (Fname);           // in syms_of_foo();
*/
/* To define a Lisp primitive function using a C function `Fname', do this:
   DEFUN ("name, Fname, ...); // at top level in foo.c
   DEFSUBR (Fname);           // in syms_of_foo();
*/
MODULE_API void defsubr (Lisp_Subr *);
#define DEFSUBR(Fname) defsubr (&S##Fname)

/* To define a Lisp primitive macro using a C function `Fname', do this:
   DEFUN ("name, Fname, ...); // at top level in foo.c
   DEFSUBR_MACRO (Fname);     // in syms_of_foo();
*/
MODULE_API void defsubr_macro (Lisp_Subr *);
#define DEFSUBR_MACRO(Fname) defsubr_macro (&S##Fname)

MODULE_API void defsymbol_massage_name (Lisp_Object *location,
					const Ascbyte *name);
MODULE_API void defsymbol_massage_name_nodump (Lisp_Object *location,
					       const Ascbyte *name);
MODULE_API void defsymbol_massage_multiword_predicate (Lisp_Object *location,
						       const Ascbyte *name);
MODULE_API void
defsymbol_massage_multiword_predicate_nodump (Lisp_Object *location,
					      const Ascbyte *name);
MODULE_API void defsymbol (Lisp_Object *location, const Ascbyte *name);
MODULE_API void defsymbol_nodump (Lisp_Object *location, const Ascbyte *name);

/* Defining symbols:

   (1) A standard symbol is defined with DEFSYMBOL.  That means that
       the symbol's print name can be derived from the symbol's variable
       name by removing the initial Q and replacing underscores with hyphens.
   (2) A keyword symbol is defined with DEFKEYWORD.  That means that
       the symbol's print name can be derived from the symbol's variable
       name by removing the initial Q and replacing underscores with hyphens,
       except that the initial underscore, which comes directly after the Q,
       is replaced by a colon.
   (3) DEFSYMBOL_MULTIWORD_PREDICATE is used for the predicates that are
       associated with a particular type of Lisp Object.  Because of the
       limitations of C macros, they're always given a predicate symbol
       whose C name simply appends `p' to the type name, modulo hyphen/
       underscore conversion.  Properly, however, the Lisp name should have
       `-p' if there is more than one word in the type name.
       DEFSYMBOL_MULTIWORD_PREDICATE is for these weird symbols -- the
       C name as supplied to the macro should end with a `p' with no
       underscore before it, and the macro will insert a hyphen there in
       the Lisp name.
   (4) In case you have some weird symbol where the equivalence between
       the C and Lisp names is more complicated (e.g. the Lisp symbol has
       non-alphabetic, non-numeric characters in it), you can just call
       defsymbol() (the lowercase version) directly.
*/

#define DEFSYMBOL(name) defsymbol_massage_name (&name, #name)
#define DEFSYMBOL_NO_DUMP(name) defsymbol_massage_name_nodump (&name, #name)
#define DEFSYMBOL_MULTIWORD_PREDICATE(name) \
  defsymbol_massage_multiword_predicate (&name, #name)
#define DEFSYMBOL_MULTIWORD_PREDICATE_NO_DUMP(name) \
  defsymbol_massage_multiword_predicate_nodump (&name, #name)

MODULE_API void defkeyword (Lisp_Object *location, const Ascbyte *name);
MODULE_API void defkeyword_massage_name (Lisp_Object *location,
					 const Ascbyte *name);
#define DEFKEYWORD(name) defkeyword_massage_name (&name, #name)

MODULE_API void deferror (Lisp_Object *symbol, const Ascbyte *name,
			  const Ascbyte *message, Lisp_Object inherits_from);
MODULE_API void deferror_massage_name (Lisp_Object *symbol, const Ascbyte *name,
				       const Ascbyte *message,
				       Lisp_Object inherits_from);
MODULE_API void deferror_massage_name_and_message (Lisp_Object *symbol,
						   const Ascbyte *name,
						   Lisp_Object inherits_from);
#define DEFERROR(name, message, inherits_from) \
  deferror_massage_name (&name, #name, message, inherits_from)
/* In this case, the error message is the same as the name, modulo some
   prettifying */
#define DEFERROR_STANDARD(name, inherits_from) \
  deferror_massage_name_and_message (&name, #name, inherits_from)

MODULE_API void defvar_magic (const Ascbyte *symbol_name, Lisp_Object magic);

#define ASSERT_OK_FOR_MAGIC(forward_type) do {                          \
    enum symbol_value_type aofmft = (forward_type);                     \
    structure_checking_assert                                           \
      (aofmft == SYMVAL_FIXNUM_FORWARD           ||                     \
       aofmft == SYMVAL_BOOLEAN_FORWARD          ||                     \
       aofmft == SYMVAL_OBJECT_FORWARD           ||                     \
       aofmft == SYMVAL_CURRENT_BUFFER_FORWARD   ||                     \
       aofmft == SYMVAL_SELECTED_CONSOLE_FORWARD ||                     \
       aofmft == SYMVAL_BUFFER_LOCAL             ||                     \
       aofmft == SYMVAL_SOME_BUFFER_LOCAL);                             \
  } while (0)

#define DEFVAR_SYMVAL_FWD_NON_LISP(lname, c_location, forward_type, magic_fun, subtype) \
do									\
{									\
  struct symbol_value_forward_##subtype *symbol_value_forward =		\
    XRECORD (ALLOC_C_READONLY_LISP_OBJECT				\
	     (symbol_value_forward_##subtype),				\
	     symbol_value_forward_##subtype,				\
	     struct symbol_value_forward_##subtype);			\
									\
  symbol_value_forward->magic.type = forward_type;			\
  symbol_value_forward->magic.value = c_location;			\
  symbol_value_forward->magicfun = magic_fun;				\
  if (symbol_value_forward->magicfun)					\
    {									\
      ASSERT_OK_FOR_MAGIC (symbol_value_forward->magic.type);		\
    }									\
      									\
  defvar_magic (lname, wrap_symbol_value_magic (symbol_value_forward));	\
} while (0)

#define DEFVAR_SYMVAL_FWD_FIXNUM(lname, c_location, forward_type, magicfun) \
DEFVAR_SYMVAL_FWD_NON_LISP(lname, c_location, forward_type, magicfun, fixnum)

#define DEFVAR_SYMVAL_FWD_BOOL(lname, c_location, forward_type, magicfun) \
DEFVAR_SYMVAL_FWD_NON_LISP(lname, c_location, forward_type, magicfun, boolint)

#define DEFVAR_SYMVAL_FWD_OBJECT(lname, c_location, forward_type, magic_fun) \
do									\
{									\
  struct symbol_value_forward_object *symbol_value_forward =		\
    XSYMBOL_VALUE_FORWARD_OBJECT (ALLOC_NORMAL_LISP_OBJECT		\
				  (symbol_value_forward_object));	\
  /* Don't set this as readonly, otherwise GC of *VALUE */		\
  /* doesn't happen. */							\
  symbol_value_forward->magic.type = forward_type;			\
  symbol_value_forward->value = c_location;				\
  symbol_value_forward->magicfun = magic_fun;				\
  if (symbol_value_forward->magicfun)					\
    {									\
      ASSERT_OK_FOR_MAGIC (symbol_value_forward->magic.type);		\
    }									\
									\
  defvar_magic (lname, wrap_symbol_value_magic (symbol_value_forward)); \
} while (0)

#define DEFVAR_LISP(lname, c_location) \
	DEFVAR_SYMVAL_FWD_OBJECT (lname, c_location, SYMVAL_OBJECT_FORWARD, 0)
#define DEFVAR_CONST_LISP(lname, c_location) \
	DEFVAR_SYMVAL_FWD_OBJECT (lname, c_location, SYMVAL_CONST_OBJECT_FORWARD, 0)
#define DEFVAR_SPECIFIER(lname, c_location) \
	DEFVAR_SYMVAL_FWD_OBJECT (lname, c_location, SYMVAL_CONST_SPECIFIER_FORWARD, 0)
#define DEFVAR_LISP_MAGIC(lname, c_location, magicfun) \
	DEFVAR_SYMVAL_FWD_OBJECT (lname, c_location, SYMVAL_OBJECT_FORWARD, magicfun)

#define DEFVAR_INT(lname, c_location) \
	DEFVAR_SYMVAL_FWD_FIXNUM (lname, c_location, SYMVAL_FIXNUM_FORWARD, 0)
#define DEFVAR_CONST_INT(lname, c_location) \
	DEFVAR_SYMVAL_FWD_FIXNUM (lname, c_location, SYMVAL_CONST_FIXNUM_FORWARD, 0)
#define DEFVAR_INT_MAGIC(lname, c_location, magicfun) \
	DEFVAR_SYMVAL_FWD_FIXNUM (lname, c_location, SYMVAL_FIXNUM_FORWARD, magicfun)

#define DEFVAR_BOOL(lname, c_location) \
	DEFVAR_SYMVAL_FWD_BOOL (lname, c_location, SYMVAL_BOOLEAN_FORWARD, 0)
#define DEFVAR_CONST_BOOL(lname, c_location) \
	DEFVAR_SYMVAL_FWD_BOOL (lname, c_location, SYMVAL_CONST_BOOLEAN_FORWARD, 0)
#define DEFVAR_BOOL_MAGIC(lname, c_location, magicfun) \
	DEFVAR_SYMVAL_FWD_BOOL (lname, c_location, SYMVAL_BOOLEAN_FORWARD, magicfun)

void flush_all_buffer_local_cache (void);

struct multiple_value {
  NORMAL_LISP_OBJECT_HEADER header;
  Elemcount count;
  Elemcount allocated_count; 
  Elemcount first_desired;
  Lisp_Object contents[1];
};
typedef struct multiple_value multiple_value;

DECLARE_LISP_OBJECT (multiple_value, multiple_value);
#define MULTIPLE_VALUEP(x) RECORDP (x, multiple_value)

#define XMULTIPLE_VALUE(x) XRECORD (x, multiple_value, multiple_value)
#define wrap_multiple_value(p) wrap_record (p, multiple_value)

#define CHECK_MULTIPLE_VALUE(x) CHECK_RECORD (x, multiple_value)
#define CONCHECK_MULTIPLE_VALUE(x) CONCHECK_RECORD (x, multiple_value)

#define multiple_value_count(x) ((x)->count)
#define multiple_value_allocated_count(x) ((x)->allocated_count)
#define multiple_value_first_desired(x) ((x)->first_desired)
#define multiple_value_contents(x) ((x)->contents)

#define XMULTIPLE_VALUE_COUNT(x) multiple_value_count (XMULTIPLE_VALUE (x))
#define XMULTIPLE_VALUE_ALLOCATED_COUNT(x) \
  multiple_value_allocated_count (XMULTIPLE_VALUE (x))
#define XMULTIPLE_VALUE_FIRST_DESIRED(x) \
  multiple_value_first_desired (XMULTIPLE_VALUE(x))
#define XMULTIPLE_VALUE_CONTENTS(x) multiple_value_contents (XMULTIPLE_VALUE(x))

Lisp_Object multiple_value_call (int nargs, Lisp_Object *args);
Lisp_Object multiple_value_list_internal (int nargs, Lisp_Object *args);

/* It's slightly ugly to expose this here, but it does cut down the amount
   of work the bytecode interpreter has to do substantially. */
extern int multiple_value_current_limit;

/* Bind the multiple value limits that #'values and #'values-list pay
   attention to. Used by bytecode and interpreted code. */
int bind_multiple_value_limits (int first, int upper);

Lisp_Object multiple_value_aref (Lisp_Object, Elemcount);
void multiple_value_aset (Lisp_Object, Elemcount, Lisp_Object);

Lisp_Object values2 (Lisp_Object first, Lisp_Object second);

DECLARE_INLINE_HEADER (
Lisp_Object 
ignore_multiple_values (Lisp_Object obj)
)
{
  return MULTIPLE_VALUEP (obj) ? multiple_value_aref (obj, 0) : obj;
}

#ifdef ERROR_CHECK_MULTIPLE_VALUES

DECLARE_INLINE_HEADER (
Lisp_Object
ignore_multiple_values_1 (Lisp_Object obj)
)
{
  if (1 == multiple_value_current_limit)
    {
      assert (!MULTIPLE_VALUEP (obj));
      return obj;
    }

  return ignore_multiple_values (obj);
}

#define IGNORE_MULTIPLE_VALUES(X) ignore_multiple_values_1 (X)

#else 
#define IGNORE_MULTIPLE_VALUES(X) (multiple_value_current_limit == 1 ? (X) \
                                   : ignore_multiple_values (X))
#endif

END_C_DECLS

#endif /* INCLUDED_symeval_h_ */
