/* emodules.c - Support routines for dynamic module loading
   (C) Copyright 1998, 1999 J. Kean Johnston. All rights reserved.
   (C) Copyright 2010 Ben Wing.
   (C) Copyright 2025 Free Software Foundation.

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

/* This file has been Mule-ized, Ben Wing, 1-26-10. */

#include "emodules.h"
#include "backtrace.h"
#include "sysdll.h"
#ifdef HAVE_LTDL
#include <ltdl.h>
#endif

static Lisp_Object Vmodule_load_path, Vmodule_extensions;

#ifdef HAVE_SHLIB

/* Weak list of known file names, so we don't have to duplicate them when
   dealing with multiple symbols with the same file name. */
static Lisp_Object Vknown_module_file_names;

/* Module infrastructure version number */
Lisp_Object Vmodule_version;

/* Do we do our work quietly? */
Boolint load_modules_quietly;

Lisp_Object Qdll_error, Qmodule, Qmodule_string_coding_system;
static Lisp_Object Vloaded_modules, Vmodule_tag;

int emodules_depth;

/* Handle to the module currently being loaded, before a module object is
   created for it. */
static dll_handle dlhandle;

struct Lisp_Module
{
  NORMAL_LISP_OBJECT_HEADER header;
  /* Full path to the the shared object loaded. */
  Lisp_Object soname;
  /* The name of the module */
  Lisp_Object modname;
  /* The module version string */
  Lisp_Object modver;
  /* How the module announces itself */
  Lisp_Object modtitle;
  /* Module cleanup function to run before unloading. */
  void (*unload) (void);
  /* Details of Lisp objects that need adjustment after unload.

     In particular subrs where the SUBR_FN needs to be redirected to give an
     error rather than crash on attempting to call it;
     symbol_value_forward_objects where VALUE needs to be set NULL so that GC
     doesn't attempt to dereference a pointer into the data segment of
     unloaded modules; and fixnums reflecting new lisp objects types created
     (new entries in lrecord_implementations_table). This is a key-assoc weak
     list. */
  Lisp_Object lisp_cleanup_info;
  /* Dynamic lib handle for this module. */
  dll_handle dlhandle;
  /* Either 1, indicating the module was successfully loaded, or a number
     greater than 1 (indicating the module was loaded as part of a chain, and
     may yet be unloaded if loading that chain fails). */
  int used;
};

DECLARE_LISP_OBJECT (module, struct Lisp_Module);
#define XMODULE(x) XRECORD (x, module, struct Lisp_Module)
#define wrap_module(p) wrap_record (p, module)
#define MODULEP(x) RECORDP (x, module)
#define CHECK_MODULE(x) CHECK_RECORD (x, module)
#define CONCHECK_MODULE(x) CONCHECK_RECORD (x, module)

static const struct memory_description module_description[] = {
  { XD_LISP_OBJECT, offsetof (struct Lisp_Module, soname) },
  { XD_LISP_OBJECT, offsetof (struct Lisp_Module, modname) },
  { XD_LISP_OBJECT, offsetof (struct Lisp_Module, modver) },
  { XD_LISP_OBJECT, offsetof (struct Lisp_Module, modtitle) },
  { XD_FUNCTION_POINTER, offsetof (struct Lisp_Module, unload) },
  { XD_LISP_OBJECT, offsetof (struct Lisp_Module, lisp_cleanup_info) },
  { XD_END }
};

static Lisp_Object
make_module (Lisp_Object soname, Lisp_Object modname,
              Lisp_Object modver, Lisp_Object modtitle,
              void (*unload) (void), int used, dll_handle dlhandle)
{
  Lisp_Object result = ALLOC_NORMAL_LISP_OBJECT (module);
  struct Lisp_Module *eresult = XMODULE (result);

  eresult->soname = soname;
  eresult->modname = modname;
  eresult->modver = modver;
  eresult->modtitle = modtitle;
  eresult->lisp_cleanup_info = make_weak_list (WEAK_LIST_KEY_ASSOC);
  eresult->unload = unload;
  eresult->used = used;
  eresult->dlhandle = dlhandle;

  return result;
}

static Lisp_Object
find_module (Lisp_Object soname, Lisp_Object name, Lisp_Object ver)
{
  Boolint ignore_case_p = !NILP (call1 (Qfile_system_ignore_case_p, soname));

  LIST_LOOP_2 (elt, Vloaded_modules)
    {
      if (internal_equal (XMODULE (elt)->soname, soname, ignore_case_p))
        {
          if (!NILP (name) && !internal_equal (XMODULE (elt)->modname,
                                               name, 0))
            {
              continue;
            }
          if (!NILP (ver) && !internal_equal (XMODULE (elt)->modver,
                                              ver, 0))
            {
              continue;
            }

          return elt; /* Found a match */
        }
    }
  return Qnil;
}

/* Store details about Lisp-visible entities that will need to be unitialized
   on #'unload-module. This is called from within LOADHIST_ATTACH() at the
   point the entry is added to Vcurrent_load_list, which pre-empts Lisp
   corrupting it before we get a chance to save this info. */
void
add_module_loadhist_elt (Lisp_Object elt)
{
  Lisp_Object emodule = XCAR (Vloaded_modules);
  Lisp_Object weak_list = XMODULE (emodule)->lisp_cleanup_info;

  if (CONSP (elt) && EQ (XCAR (elt), Qdefun))
    {
      if (SYMBOLP (XCDR (elt)) && SUBRP (XSYMBOL_FUNCTION (XCDR (elt))))
	{
	  /* Appropriate for subrs to be completely weak elements (the car and
	     the cdr of the entries are identical), if the associated symbol
	     has a #'fmakunbound done there's nothing that needs to be kept
	     around to prevent XEmacs crashing.  */
	  XWEAK_LIST_LIST (weak_list)
	    = Fcons (Fcons (XSYMBOL_FUNCTION (XCDR (elt)),
			    XSYMBOL_FUNCTION (XCDR (elt))),
		     XWEAK_LIST_LIST (weak_list));
	}
      else if (SYMBOLP (XCDR (elt)) && CONSP (XSYMBOL_FUNCTION (XCDR (elt)))
               && EQ (Qmacro, XCAR (XSYMBOL_FUNCTION (XCDR (elt))))
               && SUBRP (XCDR (XSYMBOL_FUNCTION (XCDR (elt)))))
        {
	  XWEAK_LIST_LIST (weak_list)
	    = Fcons (Fcons (XCDR (XSYMBOL_FUNCTION (XCDR (elt))),
			    XCDR (XSYMBOL_FUNCTION (XCDR (elt)))),
		     XWEAK_LIST_LIST (weak_list));
        }
    }
  else if (CONSP (elt) && EQ (XCAR (elt), Qobject))
    {
      structure_checking_assert (FIXNUMP (XCDR (elt)));

      /* For object types created (new entries in
	 lrecord_implementations_table), the weak list entry should not be
	 deleted until the module is unloaded. Ensure it is reachable by using
	 Qobject as the car. */
      XWEAK_LIST_LIST (weak_list) = Fcons (elt, XWEAK_LIST_LIST (weak_list));

      /* And remove this entry from Vcurrent_load_list, avoid confusing
	 loadhist.el. */
      structure_checking_assert (EQ (elt, Fcar (Vcurrent_load_list)));
      Vcurrent_load_list = XCDR (Vcurrent_load_list);
    }
  else if (CONSP (elt) && EQ (XCAR (elt), Qsymbol))
    {
      structure_checking_assert (FIXNUMP (XCDR (elt)));

      /* Similar reasoning for defsymbol_no_dump(). */
      XWEAK_LIST_LIST (weak_list) = Fcons (elt, XWEAK_LIST_LIST (weak_list));
      structure_checking_assert (EQ (elt, Fcar (Vcurrent_load_list)));
      Vcurrent_load_list = XCDR (Vcurrent_load_list);
    }
  else if (SYMBOLP (elt))
    {
      /* This is an entry corresponding to DEFVAR_LISP(), DEFVAR_BOOL(). */
      Lisp_Object val = XSYMBOL_VALUE (elt);
      if (SYMBOL_VALUE_FORWARD_OBJECTP (val))
	{
	  /* The weak list element needs to be reachable for as long as this
	     module is reachable, set EMODULE as the car of the element.
	     Leave the entry in Vcurrent_load_list. */
	  XWEAK_LIST_LIST (weak_list)
	    = Fcons (Fcons (emodule, XSYMBOL_VALUE (elt)),
		     XWEAK_LIST_LIST (weak_list));
	}
      else if (SYMBOL_VALUE_FORWARD_FIXNUMP (val) ||
	       SYMBOL_VALUE_FORWARD_BOOLINTP (val))
	{
	  /* Entry can be fully weak; our post-unload processing is to ensure
	     that a reachable SYMBOL_VALUE_FORWARD_{FIXNUM,BOOLINT} doesn't
	     attempt to read or write unloaded data segment entries. */
	  XWEAK_LIST_LIST (weak_list)
	    = Fcons (Fcons (emodule, XSYMBOL_VALUE (elt)),
		     XWEAK_LIST_LIST (weak_list));
	}
    }
}

static Fixnum deadbeef_constant = 0xDEADBEEF;
static Boolint deadbeef_boolint = 0;

static Lisp_Object
unloaded_subr_error (void)
{
  signal_error (Qinvalid_function,
                "Attempt to call function from an unloaded module",
                *(backtrace_list->function));
  RETURN_NOT_REACHED (Qnil);
}

static void
attempt_module_delete (Lisp_Object mod)
{
  if (XMODULE (mod)->unload != NULL)
    {
      XMODULE (mod)->unload();
    }

  if (dll_close (XMODULE (mod)->dlhandle) == 0)
    {
      ALIST_LOOP_3 (elt_car, elt_cdr,
                    XWEAK_LIST_LIST (XMODULE (mod)->lisp_cleanup_info))
        {
          if (SUBRP (elt_cdr))
            {
              XSUBR (elt_cdr)->subr_fn = &unloaded_subr_error;
              assert (EQ (elt_car, elt_cdr));
            }
          else if (SYMBOL_VALUE_FORWARD_OBJECTP (elt_cdr))
            {
              XSYMBOL_VALUE_FORWARD_OBJECT_FORWARD (elt_cdr) = &Qnil;
              XSYMBOL_VALUE_FORWARD_OBJECT_MAGICFUN (elt_cdr) = NULL;
              XSYMBOL_VALUE_MAGIC_TYPE (elt_cdr)
                = SYMVAL_CONST_OBJECT_FORWARD;
              assert (EQ (elt_car, mod));
            }
          else if (SYMBOL_VALUE_FORWARD_FIXNUMP (elt_cdr))
            {
              XSYMBOL_VALUE_FORWARD_FIXNUM_FORWARD (elt_cdr)
                = &deadbeef_constant;
              XSYMBOL_VALUE_FORWARD_FIXNUM_MAGICFUN (elt_cdr) = NULL;
              XSYMBOL_VALUE_MAGIC_TYPE (elt_cdr)
                = SYMVAL_CONST_FIXNUM_FORWARD;
              CLEAR_C_READONLY_RECORD_HEADER
                (&(XSYMBOL_VALUE_FORWARD_FIXNUM (elt_cdr)->magic.header));
              assert (EQ (elt_car, elt_cdr));
            }
          else if (SYMBOL_VALUE_FORWARD_BOOLINTP (elt_cdr))
            {
              XSYMBOL_VALUE_FORWARD_BOOLINT_FORWARD (elt_cdr)
                = &deadbeef_boolint;
              XSYMBOL_VALUE_FORWARD_BOOLINT_MAGICFUN (elt_cdr) = NULL;
              XSYMBOL_VALUE_MAGIC_TYPE (elt_cdr)
                = SYMVAL_CONST_BOOLEAN_FORWARD;
              CLEAR_C_READONLY_RECORD_HEADER
                (&(XSYMBOL_VALUE_FORWARD_BOOLINT (elt_cdr)->magic.header));
              assert (EQ (elt_car, elt_cdr));
            }
          else if (EQ (Qobject, elt_car))
            {
              undef_lisp_object (XFIXNUM (elt_cdr));
            }
          else if (EQ (Qsymbol, elt_car))
            {
              unstaticpro_nodump ((Lisp_Object *)
                                  GET_VOID_FROM_LISP (elt_cdr));
            }
          else
            {
              assert (0);
            }
        }
      Vloaded_modules = delq_no_quit (mod, Vloaded_modules);
    }
  else
    {
      XMODULE (mod)->used = 1; /* We couldn't delete it - it stays */
    }
}

static Lisp_Object
module_load_unwind (Lisp_Object upto)
{
  Lisp_Object tail, nexttail;

  /* Close off the current handle if it is open. */
  if (dlhandle != 0)
    {
      dll_close (dlhandle);
      dlhandle = 0;
    }

  /* Here we need to go through and dlclose() those modules that were loaded
     as part of this load chain, in order of most recently to least recently
     loaded. The elements of Vloaded_modules are removed from Vloaded_modules
     if dll_close () succeeds, within attempt_module_delete(); otherwise they
     stay. */
  LIST_LOOP_DELETING (tail, nexttail, Vloaded_modules)
    {
      if (EQ (tail, upto))
        {
          break;
        }

      if (XMODULE (XCAR (tail))->used > 1)
        {
          attempt_module_delete (XCAR (tail));
        }
    }

  emodules_depth = 0;

  return Qnil;
}

static Lisp_Object
module_load_unwind_coding (Lisp_Object old_alias)
{
  Fdefine_coding_system_alias (Qmodule_string_coding_system, old_alias);
  return Qnil;
}

/* Do the actual grunt-work of loading in a module. We first try and dlopen()
   the module. If that fails, we have an error and we bail out immediately. If
   the dlopen() succeeds, we need to check for the existence of certain
   special symbols.

   All modules will have complete access to the variables and functions
   defined within XEmacs itself.  It is up to the module to declare any
   variables or functions it uses, however.  Modules will also have access to
   other functions and variables in other loaded modules, unless they are
   defined as STATIC.

   We need to be very careful with how we load modules. If we encounter an
   error along the way, we need to back out completely to the point at which
   the user started. Since we can be called recursively, we need to take care
   with marking modules as loaded. When we first start loading modules, we set
   the counter to zero. As we enter the function each time, we increment the
   counter, and before we leave we decrement it. When we get back down to 0,
   we know we are at the end of the chain and we can mark all the modules in
   the list as loaded.

   When we signal an error, we need to be sure to unwind all modules loaded
   thus far (but only for this module chain). It is assumed that if any
   modules in a chain fail, then they all do. This is logical, considering
   that the only time we recurse is when we have dependent modules. So in the
   error handler we take great care to close off the module chain before we
   call "error" and let the Fmodule_load unwind_protect() function handle the
   cleaning up. */
void
emodules_load (const Ibyte *module, const Ibyte *modname,
	       const Ibyte *modver)
{
  Lisp_Object old_load_list = Qnil, mname = Qnil, mver = Qnil, mtitle;
  Lisp_Object foundname, filename, emodule, coding;
  const Extbyte **f;
  const long *ellcc_rev;
  Ibyte *symname;
  Bytecount symname_len;
  void (*modload)(void) = 0;
  void (*modsyms)(void) = 0;
  void (*modvars)(void) = 0;
  void (*moddocs)(void) = 0;
  void (*modunld)(void) = 0;
  struct gcpro gcpro1, gcpro2, gcpro3, gcpro4, gcpro5;
  int speccount = specpdl_depth();

  foundname = Qnil;

  emodules_depth++;
  dlhandle = 0;

  if (module == NULL || module[0] == '\0')
    invalid_argument ("Empty module name", Qunbound);

  filename = build_istring (module);
  GCPRO5 (filename, foundname, old_load_list, mname, mver);
  if (locate_file (Vmodule_load_path, filename, Vmodule_extensions,
		   &foundname, 0) < 0)
    {
      signal_error (Qfile_error, "Cannot open dynamic module", filename);
    }

  dlhandle = dll_open (foundname);
  if (dlhandle == NULL)
    {
      signal_error_2 (Qdll_error, "Opening dynamic module", foundname,
                      dll_error ());
    }

  symname_len = XSTRING_LENGTH (foundname) + sizeof ("modules_of_")
    + sizeof ("emodule_version");
  symname = alloca_ibytes (symname_len);
  emacs_snprintf (symname, symname_len, "emodule_coding");

  f = (const Extbyte **) dll_variable (dlhandle, symname);
  if (f == NULL || *f == NULL || **f == '\0')
    {
    missing_symbol:
      signal_error (Qdll_error, "Invalid dynamic module: Missing symbol",
                    build_istring (symname));
#define MISSING_SYMBOL() goto missing_symbol
    }

  CHECK_ASCTEXT (*f);
  coding
    = find_coding_system_for_text_file (intern ((const CIbyte *) *f), 0);
  record_unwind_protect (module_load_unwind_coding,
                         Ffind_coding_system (Qmodule_string_coding_system));
  Fdefine_coding_system_alias (Qmodule_string_coding_system,
                               Fget_coding_system (coding));

  emacs_snprintf (symname, symname_len, "emodule_name");
  f = (const Extbyte **) dll_variable (dlhandle, symname);
  if (f == NULL || *f == NULL || **f == '\0')
    {
      MISSING_SYMBOL();
    }

  mname = build_extstring (*f, Qmodule_string_coding_system);
  if (modname && modname[0]
      && qxestrcmp (modname, XSTRING_DATA (mname)))
    {
      signal_error (Qdll_error, "Module name mismatch", mname);
    }

  if (XSTRING_LENGTH (mname) > XSTRING_LENGTH (foundname))
    {
      symname_len += XSTRING_LENGTH (mname) - XSTRING_LENGTH (foundname);
      symname = alloca_ibytes (symname_len);
    }

  emacs_snprintf (symname, symname_len, "emodule_compiler");
  ellcc_rev = (const long *) dll_variable (dlhandle, symname);
  if (ellcc_rev == NULL || *ellcc_rev <= 0L)
    {
      MISSING_SYMBOL();
    }

  if (*ellcc_rev > EMODULES_REVISION)
    {
      signal_ferror (Qdll_error,
                     "Invalid dynamic module: Unsupported version `%ld(%ld)'",
                     *ellcc_rev, EMODULES_REVISION);
    }

  emacs_snprintf (symname, symname_len, "emodule_version");
  f = (const Extbyte **) dll_variable (dlhandle, symname);
  if (f == NULL || *f == NULL)
    {
      MISSING_SYMBOL();
    }

  mver = build_extstring (*f, Qmodule_string_coding_system);
  if (modver && modver[0] && qxestrcmp (modver, XSTRING_DATA (mver)))
    {
      signal_error (Qdll_error, "Module version mismatch", mver);
    }

  emacs_snprintf (symname, symname_len, "emodule_title");
  f = (const Extbyte **) dll_variable (dlhandle, symname);
  if (f == NULL || *f == NULL)
    {
      MISSING_SYMBOL();
    }

  mtitle = build_extstring (*f, Qmodule_string_coding_system);

  emacs_snprintf (symname, symname_len, "modules_of_%s",
                  XSTRING_DATA (mname));
  modload = (void (*)(void)) dll_function (dlhandle, symname);
  /* modload is optional. If the module doesn't require other modules it can
     be left out. */

  emacs_snprintf (symname, symname_len, "syms_of_%s",
                  XSTRING_DATA (mname));
  modsyms = (void (*)(void)) dll_function (dlhandle, symname);
  if (modsyms == NULL)
    {
      MISSING_SYMBOL();
    }

  emacs_snprintf (symname, symname_len, "vars_of_%s", XSTRING_DATA (mname));
  modvars = (void (*)(void)) dll_function (dlhandle, symname);
  if (modvars == NULL)
    {
      MISSING_SYMBOL();
    }

  emacs_snprintf (symname, symname_len, "docs_of_%s", XSTRING_DATA (mname));
  moddocs = (void (*)(void)) dll_function (dlhandle, symname);
  if (moddocs == NULL)
    {
      MISSING_SYMBOL();
    }

  /* Now look for the optional unload function. */
  emacs_snprintf (symname, symname_len, "unload_%s", XSTRING_DATA (mname));
  modunld = (void (*)(void)) dll_function (dlhandle, symname);

  /* Check if this module (with the specified file name, module name and
     module version) is already loaded. If so, there is nothing further to
     do. */
  emodule = find_module (foundname, mname, mver);
  if (!NILP (emodule))
  {
    emodules_depth--;
    dll_close (dlhandle);
    dlhandle = 0;  /* Zero this out before module_load_unwind runs */
    return;
  }

  if (!load_modules_quietly)
    message ("Loading %s v%s (%s)", XSTRING_DATA (mname),
             XSTRING_DATA (mver), XSTRING_DATA (mtitle));

  /* We have passed the basic initialization, and can now add this
     module to the list of modules. */
  emodule = make_module (foundname, mname, mver, mtitle, modunld,
                         emodules_depth + 1, dlhandle);
  Vloaded_modules = Fcons (emodule, Vloaded_modules);
  dlhandle = 0;

  old_load_list = Vcurrent_load_list;
  Vcurrent_load_list = Qnil;
  LOADHIST_ATTACH (call1 (Qfile_name_sans_extension,
                          Ffile_name_nondirectory (foundname)));
  LOADHIST_ATTACH (Vmodule_tag);

  /* Now we need to call the module init function and perform the various
     startup tasks. */
  if (modload != 0)
    (*modload) ();

  /* Now we can get the module to initialize its symbols, and then its
     variables, and lastly the documentation strings. */
  (*modsyms) ();
  (*modvars) ();
  (*moddocs) ();

  if (!load_modules_quietly)
    message ("Loaded module %s v%s (%s)", XSTRING_DATA (mname),
             XSTRING_DATA (mver), XSTRING_DATA (mtitle));

  Vload_history = Fcons (Fnreverse (Vcurrent_load_list), Vload_history);
  Vcurrent_load_list = old_load_list;
  UNGCPRO;

  emodules_depth--;
  if (emodules_depth == 0)
    {
      /* We have reached the end of the load chain. We now go through the
         list of loaded modules and mark all the valid modules as just
         that. */
      LIST_LOOP_2 (elt, Vloaded_modules)
        {
          if (XMODULE (elt)->used > 1)
            {
              XMODULE (elt)->used = 1;
            }
        }
    }

  unbind_to (speccount);
  return;
}

static void
emodules_doc (const Extbyte *symname,
              const Extbyte *doc,
              const Extbyte *file_name,
              Boolint subrp)
{
  Lisp_Object sym;
  Ibyte *symname_internal;
  Bytecount len;

  TO_INTERNAL_FORMAT (C_STRING, symname, ALLOCA, (symname_internal, len),
                      Qmodule_string_coding_system);
  
  sym = oblookup (Vobarray, symname_internal, len);

  if (SYMBOLP (sym))
    {
      Ibyte *file_name_internal;
      Lisp_Object lisp_file_name = Qnil;

      TO_INTERNAL_FORMAT (C_STRING, file_name, ALLOCA, (file_name_internal,
                                                        len),
                          Qmodule_string_coding_system);

      {
        LIST_LOOP_2 (elt, XWEAK_LIST_LIST (Vknown_module_file_names))
          {
            if (qxememcmp (XSTRING_DATA (elt), file_name_internal, len)
                == 0)
              {
                lisp_file_name = elt;
                break;
              }
          }

        if (NILP (lisp_file_name))
          {
            lisp_file_name = make_string (file_name_internal, len);
            XWEAK_LIST_LIST (Vknown_module_file_names)
              = Fcons (lisp_file_name,
                       XWEAK_LIST_LIST (Vknown_module_file_names));
          }
      }

      if (subrp) 
        {
          if (SUBRP (XSYMBOL (sym)->function))
            {
              Lisp_Subr *subr = XSUBR (XSYMBOL (sym)->function);
              subr->doc = build_extstring (doc, Qmodule_string_coding_system);
              Fput (subr->doc, Qsymbol_file, lisp_file_name);
            }
        }
      else
        {
	  Lisp_Object lispdoc = build_extstring (doc,
                                                 Qmodule_string_coding_system);
	  Fput (sym, Qvariable_documentation, lispdoc);
	  Fput (lispdoc, Qsymbol_file, lisp_file_name);
        }
    }
}

void
emodules_doc_subr (const Extbyte *symname,
                   const Extbyte *doc,
                   const Extbyte *file_name)
{
  emodules_doc (symname, doc, file_name, 1);
}

void
emodules_doc_sym (const Extbyte *symname,
                  const Extbyte *doc,
                  const Extbyte *file_name)
{
  emodules_doc (symname, doc, file_name, 0);
}

DEFUN ("load-module", Fload_module, 1, 3, "FLoad dynamic module: ", /*
Load a C Emacs extension module from FILE.

The optional NAME and VERSION are used to identify specific modules.

Do not use this function in your programs.  Use `require' instead.

This function is similar in intent to `load' except that it loads in
pre-compiled C or C++ code, using dynamic shared objects.  If NAME is
specified, then the module is only loaded if its internal name matches the
NAME specified.  If VERSION is specified, then the module is only loaded if it
matches that VERSION.  This function will check to make sure that the same
module is not loaded twice.  Modules are searched for in the same way as Lisp
files, except that the valid file extensions are `.so', `.dll', `.ell', or
`.dylib', and the path examined is specified by the `module-load-path'
variable rather than `load-path'.

Certain functions in the shared module must be completely resolved in order for
this function to be successful.  These are, currently `syms_of_NAME',
`vars_of_NAME', and `docs_of_NAME'.  Other functions are supplied when the
module is built by `ellcc' and they are also sanity-checked.

Any modules which the specified FILE depends on will be automatically loaded.
You can determine which modules have been loaded as dynamic shared objects by
examining the return value of the function `list-modules'.

It is possible to unload modules using `unload-feature'.  There is also a
lower-level `unload-module' used by `unload-feature'; calling this directly on
a module without calling `unload-feature' will lead to Lisp errors on
attempting to call functions no longer available or to set variables no longer
available.  Any Lisp objects of a type made available by the module will
remain in existence but it will not be possible to do anything useful with
them.

Messages informing you of the progress of the load are displayed unless
the variable `load-modules-quietly' is non-NIL.
*/
       (file, name, version))
{
  int speccount = specpdl_depth();
  const Ibyte *mname = (const Ibyte *) "", *mver = mname;

  CHECK_STRING (file);

  if (!NILP (name))
    {
      CHECK_STRING (name);
      mname = XSTRING_DATA (name);
    }

  if (!NILP (version))
    {
      CHECK_STRING (version);
      mver = XSTRING_DATA (version);
    }

  dlhandle = 0;
  record_unwind_protect (module_load_unwind, Vloaded_modules);
  emodules_load (XSTRING_DATA (file), mname, mver);
  unbind_to (speccount);

  return Qt;
}

DEFUN ("unload-module", Funload_module, 1, 3, 0, /*
Unload a module previously loaded with load-module.

Do not use this function in your programs.  Use `unload-feature' instead.

As with load-module, this function requires at least the module FILE, and
optionally the module NAME and VERSION to unload.  It may not be possible for
the module to be unloaded from memory, as other loaded modules may have
dependencies on it.
*/
       (file, name, version))
{
  Lisp_Object foundname = Qnil, module = Qnil;
  struct gcpro gcpro1;

  CHECK_STRING (file);

  if (!NILP (name))
    {
      CHECK_STRING (name);
    }

  if (!NILP (version))
    {
      CHECK_STRING (version);
    }

  GCPRO1 (foundname);
  if (locate_file (Vmodule_load_path, file, Vmodule_extensions, &foundname, 0)
      < 0)
    {
      RETURN_UNGCPRO (Qt);
    }

  module = find_module (foundname, name, version);
  if (!NILP (module))
    {
      attempt_module_delete (module);
    }
  
  RETURN_UNGCPRO (Qt);
}

DEFUN ("list-modules", Flist_modules, 0, 0, "", /*
Produce a list of loaded dynamic modules.

This function will return a list of all the loaded dynamic modules.
Each element in the list is a list in the form (SONAME NAME VER DESC),
where SONAME is the name of the shared object that was loaded, NAME
is the internal module name, VER is the version of the module, and DESC
is how the module describes itself.

This function returns a list, so you will need to assign the return value
to a variable and then examine the variable with `describe-variable'.
For example:

  (setq mylist (list-modules))
  (describe-variable 'mylist)


NOTE: It is possible for the same module to be loaded more than once,
at different versions.  However, you should never see the same module,
with the same name and version, loaded more than once.  If you do, this
is a bug, and you are encouraged to report it.
*/
       ())
{
  Lisp_Object mlist = Qnil;

  LIST_LOOP_2 (elt, Vloaded_modules)
    {
      mlist = Fcons (list4 (XMODULE (elt)->soname,
                            XMODULE (elt)->modname,
                            XMODULE (elt)->modver,
                            XMODULE (elt)->modtitle),
                     mlist);
    }

  return mlist;
}

void
syms_of_module (void)
{
  DEFINE_NODUMP_INTERNAL_LISP_OBJECT ("module", module, module_description,
                                      struct Lisp_Module);

  DEFERROR_STANDARD (Qdll_error, Qerror);
  DEFSYMBOL (Qmodule);
  DEFSYMBOL (Qmodule_string_coding_system);

  DEFSUBR (Fload_module);
  DEFSUBR (Flist_modules);
  DEFSUBR (Funload_module);

  Vmodule_tag = Fcons (Qmodule, Qnil);
  staticpro (&Vmodule_tag);
}

#endif /* HAVE_SHLIB */

void
vars_of_module (void)
{
#ifdef HAVE_SHLIB
  Fprovide (intern ("modules"));

#ifdef HAVE_LTDL
  lt_dlinit ();
  lt_dlmalloc = (lt_ptr (*) (size_t)) xmalloc;
  lt_dlrealloc = (lt_ptr (*) (lt_ptr, size_t)) xrealloc;
  lt_dlfree = (void (*) (lt_ptr)) xfree_1;
#endif

  DEFVAR_LISP ("module-version", &Vmodule_version /*
Emacs dynamic loading mechanism version, as a string.

This string is in the form XX.YY.ppp, where XX is the major version
number, YY is the minor version number, and ppp is the patch level.
This variable can be used to distinguish between different versions of
the dynamic loading technology used in Emacs, if required.  It is not
a given that this value will be the same as the Emacs version number.
*/ );
  Vmodule_version = build_cistring (EMODULES_VERSION);

  DEFVAR_BOOL ("load-modules-quietly", &load_modules_quietly /*
*Set to t if module loading is to be silent.

Normally, when loading dynamic modules, Emacs will inform you of its
progress, and will display the module name and version if the module
is loaded correctly.  Setting this variable to `t' will suppress these
messages.  This would normally only be done if `load-module' was being
called by a Lisp function.
*/);
  load_modules_quietly = 0;

#endif /* HAVE_SHLIB */

  DEFVAR_LISP ("module-load-path", &Vmodule_load_path /*
*List of directories to search for dynamic modules to load.
Each element is a string (directory name) or nil (try default directory).

Note that elements of this list *may not* begin with "~", so you must
call `expand-file-name' on them before adding them to this list.

Initialized based on EMACSMODULEPATH environment variable, if any, otherwise
to default specified the file `paths.h' when XEmacs was built.  If there
were no paths specified in `paths.h', then XEmacs chooses a default
value for this variable by looking around in the file-system near the
directory in which the XEmacs executable resides.

Due to the nature of dynamic modules, the path names should almost always
refer to architecture-dependent directories.  It is unwise to attempt to
store dynamic modules in a heterogenous environment.  Some environments
are similar enough to each other that XEmacs will be unable to determine
the correctness of a dynamic module, which can have unpredictable results
when a dynamic module is loaded.
*/);
  Vmodule_load_path = Qnil;

  DEFVAR_LISP ("module-extensions", &Vmodule_extensions /*
*List of filename extensions to use when searching for dynamic modules.
*/);
  Vmodule_extensions = list5 (build_ascstring (".ell"),
			      build_ascstring (".so"),
			      build_ascstring (".dll"),
			      build_ascstring (".dylib"),
			      build_ascstring (""));

  Vknown_module_file_names = make_weak_list (WEAK_LIST_SIMPLE);
  staticpro (&Vknown_module_file_names);

  Vloaded_modules = Qnil;
  staticpro (&Vloaded_modules);
}

/* emodules.c ends here */

