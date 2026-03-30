/* The console object.
   Copyright (C) 1992, 1993, 1994 Free Software Foundation, Inc.
   Copyright (C) 1996, 2002, 2010 Ben Wing.

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

/* Written by Ben Wing, late 1995?.
   suspend-console, set-input-mode, and related stuff largely based on
   existing code.
*/

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "console-impl.h"
#include "device-impl.h"
#include "events.h"
#include "frame-impl.h"
#include "redisplay.h"
#include "sysdep.h"
#include "window.h"

#include "console-stream-impl.h"
#ifdef HAVE_TTY
#include "console-tty-impl.h"
#endif

Lisp_Object Vconsole_list, Vselected_console;

Lisp_Object Vcreate_console_hook, Vdelete_console_hook;

Lisp_Object Vfunction_key_map_parent;

Lisp_Object Qconsolep, Qconsole_live_p;
Lisp_Object Qcreate_console_hook;
Lisp_Object Qdelete_console_hook;

Lisp_Object Qsuspend_hook;
Lisp_Object Qsuspend_resume_hook;

/* This structure holds the default values of the console-local
   variables defined with DEFVAR_CONSOLE_LOCAL, that have special
   slots in each console.  The default value occupies the same slot
   in this structure as an individual console's value occupies in
   that console.  Setting the default value also goes through the
   list of consoles and stores into each console that does not say
   it has a local value.  */
Lisp_Object Vconsole_defaults;

/* This structure marks which slots in a console have corresponding
   default values in console_defaults.
   Each such slot has a nonzero value in this structure.
   The value has only one nonzero bit.

   When a console has its own local value for a slot,
   the bit for that slot (found in the same slot in this structure)
   is turned on in the console's local_var_flags slot.

   If a slot in this structure is 0, then there is a DEFVAR_CONSOLE_LOCAL
   for the slot, but there is no default value for it; the corresponding
   slot in console_defaults is not used except to initialize newly-created
   consoles.

   If a slot is -1, then there is a DEFVAR_CONSOLE_LOCAL for it
   as well as a default value which is used to initialize newly-created
   consoles and as a reset-value when local-vars are killed.

   If a slot is -2, there is no DEFVAR_CONSOLE_LOCAL for it.
   (The slot is always local, but there's no lisp variable for it.)
   The default value is only used to initialize newly-creation consoles.

   If a slot is -3, then there is no DEFVAR_CONSOLE_LOCAL for it but
   there is a default which is used to initialize newly-creation
   consoles and as a reset-value when local-vars are killed. */
struct console console_local_flags;

/* This structure holds the names of symbols whose values may be
   console-local.  It is indexed and accessed in the same way as the above. */
static Lisp_Object Vconsole_local_symbols;

DEFINE_CONSOLE_TYPE (dead);

const_console_methods_pointer_dynarr *the_console_methods_dynarr;


static const struct memory_description console_data_description_1 []= {
#ifdef HAVE_TTY
  { XD_BLOCK_PTR, tty_console, 1, { &tty_console_data_description} },
#endif
  { XD_BLOCK_PTR, stream_console, 1, { &stream_console_data_description} },
  { XD_END }
};

static const struct sized_memory_description console_data_description = {
  sizeof (void *), console_data_description_1
};

static const struct memory_description console_description [] = {
  { XD_INT, offsetof (struct console, contype) },
#define MARKED_SLOT(x) { XD_LISP_OBJECT, offsetof (struct console, x) },
#include "conslots.h"
  { XD_BLOCK_PTR, offsetof (struct console, conmeths), 1,
    { &console_methods_description } },
  { XD_UNION, offsetof (struct console, console_data), 
    XD_INDIRECT (0, 0), { &console_data_description } },
  { XD_END }
};

static void
print_console (Lisp_Object obj, Lisp_Object printcharfun,
	       int UNUSED (escapeflag))
{
  struct console *con = XCONSOLE (obj);

  if (print_readably)
    printing_unreadable_lisp_object (obj, XSTRING_DATA (con->name));

  write_fmt_string_lisp (printcharfun, "#<%s-console on %S",
                         CONSOLE_TYPE (con), CONSOLE_CONNECTION (con));
  write_fmt_string (printcharfun, " 0x%x>", LISP_OBJECT_UID (obj));
}


static void
set_quit_events (struct console *con, Lisp_Object key)
{
  /* Make sure to run Fcharacter_to_event() *BEFORE* setting QUIT_CHAR,
     so that nothing is changed when invalid values trigger an error! */
  con->quit_event = Fcharacter_to_event (key, Qnil, wrap_console (con), Qnil);
  con->quit_char = key;
  con->critical_quit_event = Fcopy_event (con->quit_event, Qnil);
  upshift_event (con->critical_quit_event);
}

static struct console *
allocate_console (Lisp_Object type)
{
  Lisp_Object console = ALLOC_NORMAL_LISP_OBJECT (console);
  struct console *con = XCONSOLE (console);
  struct gcpro gcpro1;

  copy_lisp_object (console, Vconsole_defaults);

  GCPRO1 (console);

  con->conmeths = decode_console_type (type, ERROR_ME);
  con->contype = get_console_variant (type);
  con->command_builder = allocate_command_builder (console, 1);
  con->function_key_map = Fmake_sparse_keymap (Qnil);
  Fset_keymap_parents (con->function_key_map, Vfunction_key_map_parent);
  set_quit_events (con, make_char (7)); /* C-g */

  UNGCPRO;
  return con;
}

struct console *
decode_console (Lisp_Object console)
{
  if (NILP (console))
    console = Fselected_console ();
  /* quietly accept devices and frames for the console arg */
  if (DEVICEP (console) || FRAMEP (console))
    console = DEVICE_CONSOLE (decode_device (console));
  CHECK_LIVE_CONSOLE (console);
  return XCONSOLE (console);
}


const struct console_methods *
decode_console_type (Lisp_Object type, Error_Behavior errb)
{
  Elemcount ii = Dynarr_length (the_console_methods_dynarr);

  while (ii)
    {
      const struct console_methods *elt
        = Dynarr_at (the_console_methods_dynarr, --ii);

      if (EQ (type, elt->name))
        {
          return elt;
        }
    }

  maybe_invalid_constant ("Invalid console type", type, Qconsole, errb);

  return 0;
}

enum console_variant
get_console_variant (Lisp_Object type)
{
  if (EQ (type, Qtty)) 
    return tty_console;

  if (EQ (type, Qgtk)) 
    return gtk_console;

  if (EQ (type, Qx)) 
    return x_console;

  if (EQ (type, Qmswindows)) 
    return mswindows_console;

  if (EQ (type, Qmsprinter)) 
    return msprinter_console;

  if (EQ (type, Qstream)) 
    return stream_console;

  ABORT (); /* should never happen */
  return dead_console; 
}

Boolint
valid_console_type_p (Lisp_Object type)
{
  return decode_console_type (type, ERROR_ME_NOT) != 0;
}

DEFUN ("valid-console-type-p", Fvalid_console_type_p, 1, 1, 0, /*
Return t if CONSOLE-TYPE is a valid console type.
Valid types are `x', `tty', `mswindows', `msprinter', `gtk', and `stream'.
*/
       (console_type))
{
  return valid_console_type_p (console_type) ? Qt : Qnil;
}

DEFUN ("console-type-list", Fconsole_type_list, 0, 0, 0, /*
Return a list of valid console types.
*/
       ())
{
  Lisp_Object result = Qnil;
  Elemcount ii = Dynarr_length (the_console_methods_dynarr);

  while (ii)
    {
      result = Fcons (Dynarr_at (the_console_methods_dynarr, --ii)->name,
                      result);
    }

  return result;
}

DEFUN ("cdfw-console", Fcdfw_console, 1, 1, 0, /*
Given a console, device, frame, or window, return the associated console.
Return nil otherwise.
*/
       (object))
{
  return CDFW_CONSOLE (object);
}

Boolint
console_live_p (const struct console *c)
{
  return CONSOLE_LIVE_P (c);
}

Lisp_Object
console_device_list (const struct console *c)
{
  return CONSOLE_DEVICE_LIST (c);
}


DEFUN ("selected-console", Fselected_console, 0, 0, 0, /*
Return the console which is currently active.
*/
       ())
{
  return Vselected_console;
}

/* Called from selected_device_1(), called from selected_frame_1(),
   called from Fselect_window() */
void
select_console_1 (Lisp_Object console)
{
  /* perhaps this should do something more complicated */
  Vselected_console = console;

  /* #### Schedule this to be removed in 19.14 */
#ifdef HAVE_X_WINDOWS
  if (CONSOLE_X_P (XCONSOLE (console)))
    Vwindow_system = Qx;
  else
#endif
#ifdef HAVE_GTK
  if (CONSOLE_GTK_P (XCONSOLE (console)))
    Vwindow_system = Qgtk;
  else
#endif
#ifdef HAVE_MS_WINDOWS
  if (CONSOLE_MSWINDOWS_P (XCONSOLE (console)))
    Vwindow_system = Qmswindows;
  else
#endif
    Vwindow_system = Qnil;
}

DEFUN ("select-console", Fselect_console, 1, 1, 0, /*
Select the console CONSOLE.
Subsequent editing commands apply to its selected device, selected frame,
and selected window.  The selection of CONSOLE lasts until the next time
the user does something to select a different console, or until the next
time this function is called.
*/
       (console))
{
  Lisp_Object device;

  CHECK_LIVE_CONSOLE (console);

  device = CONSOLE_SELECTED_DEVICE (XCONSOLE (console));
  if (!NILP (device))
    {
      struct device *d = XDEVICE (device);
      Lisp_Object frame = DEVICE_SELECTED_FRAME (d);
      if (!NILP (frame))
	{
	  struct frame *f = XFRAME(frame);
	  Fselect_window (FRAME_SELECTED_WINDOW (f), Qnil);
	}
      else
	invalid_operation ("Can't select console with no frames", Qunbound);
    }
  else
    invalid_operation ("Can't select a console with no devices", Qunbound);
  return Qnil;
}

void
set_console_last_nonminibuf_frame (struct console *con,
				   Lisp_Object frame)
{
  con->last_nonminibuf_frame = frame;
}

DEFUN ("console-live-p", Fconsole_live_p, 1, 1, 0, /*
Return non-nil if OBJECT is a console that has not been deleted.
*/
       (object))
{
  return CONSOLEP (object) && CONSOLE_LIVE_P (XCONSOLE (object)) ? Qt : Qnil;
}

DEFUN ("console-type", Fconsole_type, 0, 1, 0, /*
Return the console type (e.g. `x' or `tty') of CONSOLE.
Value is
`tty' for a tty console (a character-only terminal),
`x' for a console that is an X display,
`mswindows' for a console that is an MS Windows connection,
`msprinter' for a console that is an MS Windows printer connection,
`gtk' for a console that is a GTK connection,
`stream' for a stream console (which acts like a stdio stream), and
`dead' for a deleted console.
*/
       (console))
{
  /* don't call decode_console() because we want to allow for dead
     consoles. */
  if (NILP (console))
    console = Fselected_console ();
  CHECK_CONSOLE (console);
  return CONSOLE_TYPE (XCONSOLE (console));
}

DEFUN ("console-name", Fconsole_name, 0, 1, 0, /*
Return the name of CONSOLE.
*/
       (console))
{
  return CONSOLE_NAME (decode_console (console));
}

DEFUN ("console-connection", Fconsole_connection, 0, 1, 0, /*
Return the connection of the specified console.
CONSOLE defaults to the selected console if omitted.
*/
       (console))
{
  return CONSOLE_CONNECTION (decode_console (console));
}

static Lisp_Object
semi_canonicalize_console_connection (const struct console_methods *meths,
				      Lisp_Object name, Error_Behavior errb)
{
  if (HAS_CONTYPE_METH_P (meths, semi_canonicalize_console_connection))
    return CONTYPE_METH (meths, semi_canonicalize_console_connection,
			 (name, errb));
  else
    return CONTYPE_METH_OR_GIVEN (meths, canonicalize_console_connection,
				  (name, errb), name);
}

static Lisp_Object
canonicalize_console_connection (const struct console_methods *meths,
				 Lisp_Object name, Error_Behavior errb)
{
  if (HAS_CONTYPE_METH_P (meths, canonicalize_console_connection))
    return CONTYPE_METH (meths, canonicalize_console_connection,
			 (name, errb));
  else
    return CONTYPE_METH_OR_GIVEN (meths, semi_canonicalize_console_connection,
				  (name, errb), name);
}

static Lisp_Object
find_console_of_type (const struct console_methods *meths, Lisp_Object canon)
{
  Lisp_Object concons;

  CONSOLE_LOOP (concons)
    {
      Lisp_Object console = XCAR (concons);

      if (EQ (CONMETH_NAME (meths), CONSOLE_TYPE (XCONSOLE (console)))
	  && internal_equal (CONSOLE_CANON_CONNECTION (XCONSOLE (console)),
			     canon, 0))
	return console;
    }

  return Qnil;
}

DEFUN ("find-console", Ffind_console, 1, 2, 0, /*
Look for an existing console attached to connection CONNECTION.
Return the console if found; otherwise, return nil.

If TYPE is specified, only return consoles of that type; otherwise,
return consoles of any type. (It is possible, although unlikely,
that two consoles of different types could have the same connection
name; in such a case, the first console found is returned.)
*/
       (connection, type))
{
  Lisp_Object canon = Qnil;
  struct gcpro gcpro1;

  GCPRO1 (canon);

  if (!NILP (type))
    {
      const struct console_methods *conmeths
        = decode_console_type (type, ERROR_ME);
      canon = canonicalize_console_connection (conmeths, connection,
					       ERROR_ME_NOT);
      if (UNBOUNDP (canon))
	RETURN_UNGCPRO (Qnil);

      RETURN_UNGCPRO (find_console_of_type (conmeths, canon));
    }
  else
    {
      int i;

      for (i = 0; i < Dynarr_length (the_console_methods_dynarr); i++)
	{
	  const struct console_methods *conmeths =
	    Dynarr_at (the_console_methods_dynarr, i);
	  canon = canonicalize_console_connection (conmeths, connection,
						   ERROR_ME_NOT);
	  if (!UNBOUNDP (canon))
	    {
	      Lisp_Object console = find_console_of_type (conmeths, canon);
	      if (!NILP (console))
		RETURN_UNGCPRO (console);
	    }
	}

      RETURN_UNGCPRO (Qnil);
    }
}

DEFUN ("get-console", Fget_console, 1, 2, 0, /*
Look for an existing console attached to connection CONNECTION.
Return the console if found; otherwise, signal an error.

If TYPE is specified, only return consoles of that type; otherwise,
return consoles of any type. (It is possible, although unlikely,
that two consoles of different types could have the same connection
name; in such a case, the first console found is returned.)
*/
       (connection, type))
{
  Lisp_Object console = Ffind_console (connection, type);
  if (NILP (console))
    {
      if (NILP (type))
	invalid_argument ("No such console", connection);
      else
	invalid_argument_2 ("No such console", type, connection);
    }
  return console;
}

Lisp_Object
create_console (Lisp_Object name, Lisp_Object type, Lisp_Object connection,
		Lisp_Object props)
{
  /* This function can GC */
  struct console *con;
  Lisp_Object console;
  struct gcpro gcpro1;

  console = Ffind_console (connection, type);
  if (!NILP (console))
    return console;

  con = allocate_console (type);
  console = wrap_console (con);

  GCPRO1 (console);

  CONSOLE_NAME (con) = name;
  CONSOLE_CONNECTION (con) =
    semi_canonicalize_console_connection (con->conmeths, connection,
					  ERROR_ME);
  CONSOLE_CANON_CONNECTION (con) =
    canonicalize_console_connection (con->conmeths, connection,
				     ERROR_ME);

  MAYBE_CONMETH (con, init_console, (con, props));

  /* Do it this way so that the console list is in order of creation */
  Vconsole_list = nconc2 (Vconsole_list, Fcons (console, Qnil));
  note_object_created (console);

  if (CONMETH_OR_GIVEN (con, initially_selected_for_input, (con), 0))
    event_stream_select_console (con);

  UNGCPRO;
  return console;
}

/* find a console other than the selected one.  Prefer non-stream
   consoles over stream consoles. */

static Lisp_Object
find_other_console (Lisp_Object console)
{
  Lisp_Object concons;

  /* look for a non-stream console */
  CONSOLE_LOOP (concons)
    {
      Lisp_Object con = XCAR (concons);
      if (!CONSOLE_STREAM_P (XCONSOLE (con))
	  && !EQ (con, console)
	  && !NILP (CONSOLE_SELECTED_DEVICE (XCONSOLE (con)))
	  && !NILP (DEVICE_SELECTED_FRAME
		    (XDEVICE (CONSOLE_SELECTED_DEVICE (XCONSOLE (con))))))
	break;
    }
  if (!NILP (concons))
    return XCAR (concons);

  /* OK, now look for a stream console */
  CONSOLE_LOOP (concons)
    {
      Lisp_Object con = XCAR (concons);
      if (!EQ (con, console)
	  && !NILP (CONSOLE_SELECTED_DEVICE (XCONSOLE (con)))
	  && !NILP (DEVICE_SELECTED_FRAME
		    (XDEVICE (CONSOLE_SELECTED_DEVICE (XCONSOLE (con))))))
	break;
    }
  if (!NILP (concons))
    return XCAR (concons);

  /* Sorry, there ain't none */
  return Qnil;
}

static int
find_nonminibuffer_frame_not_on_console_predicate (Lisp_Object frame,
						   void *closure)
{
  Lisp_Object console;

  console = GET_LISP_FROM_VOID (closure);
  if (FRAME_MINIBUF_ONLY_P (XFRAME (frame)))
    return 0;
  if (EQ (console, FRAME_CONSOLE (XFRAME (frame))))
    return 0;
  return 1;
}

static Lisp_Object
find_nonminibuffer_frame_not_on_console (Lisp_Object console)
{
  return find_some_frame (find_nonminibuffer_frame_not_on_console_predicate,
			  STORE_LISP_IN_VOID (console));
}

static void
nuke_all_console_slots (struct console *con, Lisp_Object zap)
{
  zero_nonsized_lisp_object (wrap_console (con));
  structure_checking_assert (con->contype == dead_console);
  /* The above sets CON->CONTYPE to dead_console, which is what we want. The
     following needs to be done explicitly, so we can print Vconsole_defaults,
     Vconsole_local_symbols usefully from the debugger: */
  con->conmeths = dead_console_methods;

#define MARKED_SLOT(x)	con->x = zap;
#include "conslots.h"
}

/* Delete console CON.

   If FORCE is non-zero, allow deletion of the only frame.

   If CALLED_FROM_KILL_EMACS is non-zero, then, if
   deleting the last console, just delete it,
   instead of calling `save-buffers-kill-emacs'.

   If FROM_IO_ERROR is non-zero, then the console is gone due
   to an I/O error.  This affects what happens if we exit
   (we do an emergency exit instead of `save-buffers-kill-emacs'.)
*/

void
delete_console_internal (struct console *con, Boolint force,
			 Boolint called_from_kill_emacs,
			 Boolint from_io_error)
{
  /* This function can GC */
  Lisp_Object console;
  struct gcpro gcpro1;

  /* OK to delete an already-deleted console. */
  if (!CONSOLE_LIVE_P (con))
    return;

  console = wrap_console (con);

  if (!force)
    check_allowed_operation (OPERATION_DELETE_OBJECT, console, Qnil);

  GCPRO1 (console);

  if (!called_from_kill_emacs)
    {
      int down_we_go = 0;

      if ((XFIXNUM (Flength (Vconsole_list)) == 1)
	  /* if we just created the console, it might not be listed,
	     or something ... */
	  && !NILP (memq_no_quit (console, Vconsole_list)))
	down_we_go = 1;
      /* If there aren't any nonminibuffer frames that would
	 be left, then exit. */
      else if (NILP (find_nonminibuffer_frame_not_on_console (console)))
	down_we_go = 1;

      if (down_we_go)
	{
	  if (!force)
	    invalid_operation ("Attempt to delete the only frame", Qunbound);
	  else if (from_io_error)
	    {
	      /* Mayday mayday!  We're going down! */
	      stderr_out ("  Autosaving and exiting...\n");
	      Vwindow_system = Qnil; /* let it lie! */
	      preparing_for_armageddon = 1;
	      Fkill_emacs (make_fixnum (70));
	    }
	  else
	    {
	      call0 (Qsave_buffers_kill_emacs);
	      UNGCPRO;
	      /* If we get here, the user said they didn't want
		 to exit, so don't. */
	      return;
	    }
	}
    }

  /* Breathe a sigh of relief.  We're still alive. */

  {
    Lisp_Object frmcons, devcons;

    /* First delete all frames without their own minibuffers,
       to avoid errors coming from attempting to delete a frame
       that is a surrogate for another frame.

       We don't set "called_from_delete_console" because we want the
       device to go ahead and get deleted if we delete the last frame
       on a device.  We won't run into trouble here because for any
       frame without a minibuffer, there has to be another one on
       the same console with a minibuffer, and we're not deleting that,
       so delete_console_internal() won't get recursively called.

       WRONG!  With surrogate minibuffers this isn't true.  Frames
       with only a minibuffer are not enough to prevent
       delete_frame_internal from triggering a device deletion. */
    CONSOLE_FRAME_LOOP_NO_BREAK (frmcons, devcons, con)
      {
	struct frame *f = XFRAME (XCAR (frmcons));
	/* delete_frame_internal() might do anything such as run hooks,
	   so be defensive. */
	if (FRAME_LIVE_P (f) && !FRAME_HAS_MINIBUF_P (f))
	  delete_frame_internal (f, 1, 1, from_io_error);

	if (!CONSOLE_LIVE_P (con)) /* make sure the delete-*-hook didn't
				      go ahead and delete anything */
	  {
	    UNGCPRO;
	    return;
	  }
      }

    CONSOLE_DEVICE_LOOP (devcons, con)
      {
	struct device *d = XDEVICE (XCAR (devcons));
	/* delete_device_internal() might do anything such as run hooks,
	   so be defensive. */
	if (DEVICE_LIVE_P (d))
	  delete_device_internal (d, 1, 1, from_io_error);
	if (!CONSOLE_LIVE_P (con)) /* make sure the delete-*-hook didn't
				      go ahead and delete anything */
	  {
	    UNGCPRO;
	    return;
	  }
      }
  }

  CONSOLE_SELECTED_DEVICE (con) = Qnil;

  /* try to select another console */

  if (EQ (console, Fselected_console ()))
    {
      Lisp_Object other_dev = find_other_console (console);
      if (!NILP (other_dev))
	Fselect_console (other_dev);
      else
	{
	  /* necessary? */
	  Vselected_console = Qnil;
	  Vwindow_system = Qnil;
	}
    }

  if (con->input_enabled)
    event_stream_unselect_console (con);

  MAYBE_CONMETH (con, delete_console, (con));

  Vconsole_list = delq_no_quit (console, Vconsole_list);

  RESET_CHANGED_SET_FLAGS;

  /* Nobody should be accessing anything in this object any more, and
     making all Lisp_Objects Qnil allows for better GC'ing in case a
     pointer to the dead console continues to hang around.  Zero all
     other structs in case someone tries to access something through
     them. */
  nuke_all_console_slots (con, Qnil);
  note_object_deleted (console);

  UNGCPRO;
}

void
io_error_delete_console (Lisp_Object console)
{
  delete_console_internal (XCONSOLE (console), 1, 0, 1);
}

DEFUN ("delete-console", Fdelete_console, 1, 2, 0, /*
Delete CONSOLE, permanently eliminating it from use.
Normally, you cannot delete the last non-minibuffer-only frame (you must
use `save-buffers-kill-emacs' or `kill-emacs').  However, if optional
second argument FORCE is non-nil, you can delete the last frame. (This
will automatically call `save-buffers-kill-emacs'.)
*/
       (console, force))
{
  CHECK_CONSOLE (console);
  delete_console_internal (XCONSOLE (console), !NILP (force), 0, 0);
  return Qnil;
}

DEFUN ("console-list", Fconsole_list, 0, 0, 0, /*
Return a list of all consoles.
*/
       ())
{
  return Fcopy_list (Vconsole_list);
}

DEFUN ("console-device-list", Fconsole_device_list, 0, 1, 0, /*
Return a list of all devices on CONSOLE.
If CONSOLE is nil, the selected console is used.
*/
       (console))
{
  return Fcopy_list (CONSOLE_DEVICE_LIST (decode_console (console)));
}

DEFUN ("console-enable-input", Fconsole_enable_input, 1, 1, 0, /*
Enable input on console CONSOLE.
*/
       (console))
{
  struct console *con = decode_console (console);
  if (!con->input_enabled)
    event_stream_select_console (con);
  return Qnil;
}

DEFUN ("console-disable-input", Fconsole_disable_input, 1, 1, 0, /*
Disable input on console CONSOLE.
*/
       (console))
{
  struct console *con = decode_console (console);
  if (con->input_enabled)
    event_stream_unselect_console (con);
  return Qnil;
}

DEFUN ("console-on-window-system-p", Fconsole_on_window_system_p, 0, 1, 0, /*
Return t if CONSOLE is on a window system.
If CONSOLE is nil, the selected console is used.
This generally means that there is support for the mouse, the menubar,
the toolbar, glyphs, etc.
*/
       (console))
{
  Lisp_Object type = CONSOLE_TYPE (decode_console (console));

  return !EQ (type, Qtty) && !EQ (type, Qstream) ? Qt : Qnil;
}



/**********************************************************************/
/*               Miscellaneous low-level functions                    */
/**********************************************************************/

static Lisp_Object
unwind_init_sys_modes (Lisp_Object console)
{
  reinit_initial_console ();

  if (!no_redraw_on_reenter &&
      CONSOLEP (console) &&
      CONSOLE_LIVE_P (XCONSOLE (console)))
    {
      struct frame *f =
	XFRAME (DEVICE_SELECTED_FRAME
		(XDEVICE (CONSOLE_SELECTED_DEVICE (XCONSOLE (console)))));
      MARK_FRAME_CHANGED (f);
    }
  return Qnil;
}

DEFUN ("suspend-emacs", Fsuspend_emacs, 0, 1, "", /*
Stop Emacs and return to superior process.  You can resume later.
On systems that don't have job control, run a subshell instead.

If optional arg STUFFSTRING is non-nil, its characters are stuffed
to be read as terminal input by Emacs's superior shell.

Before suspending, run the normal hook `suspend-hook'.
After resumption run the normal hook `suspend-resume-hook'.

Some operating systems cannot stop the Emacs process and resume it later.
On such systems, Emacs will start a subshell and wait for it to exit.
*/
       (stuffstring))
{
  int speccount = specpdl_depth ();
  struct gcpro gcpro1;

  if (!NILP (stuffstring))
    CHECK_STRING (stuffstring);
  GCPRO1 (stuffstring);

  /* There used to be a check that the initial console is TTY.
     This is bogus.  Even checking to see whether any console
     is a controlling terminal is not correct -- maybe
     the user used the -t option or something.  If we want to
     suspend, then we suspend.  Period. */

  /* Call value of suspend-hook. */
  run_hook (Qsuspend_hook);

  reset_initial_console ();
  /* sys_suspend can get an error if it tries to fork a subshell
     and the system resources aren't available for that.  */
  record_unwind_protect (unwind_init_sys_modes, Vcontrolling_terminal);
  stuff_buffered_input (stuffstring);
  sys_suspend ();
  /* the console is un-reset inside of the unwind-protect. */
  unbind_to (speccount);

#ifdef SIGWINCH
  /* It is possible that a size change occurred while we were
     suspended.  Assume one did just to be safe.  It won't hurt
     anything if one didn't. */
  asynch_device_change_pending++;
#endif

  /* Call value of suspend-resume-hook
     if it is bound and value is non-nil.  */
  run_hook (Qsuspend_resume_hook);

  UNGCPRO;
  return Qnil;
}

/* If STUFFSTRING is a string, stuff its contents as pending terminal input.
   Then in any case stuff anything Emacs has read ahead and not used.  */

void
stuff_buffered_input (
#if defined(BSD) && defined(HAVE_TTY)
		      Lisp_Object stuffstring
#else
		      Lisp_Object UNUSED (stuffstring)
#endif
		      )
{
/* stuff_char works only in BSD, versions 4.2 and up.  */
#if defined(BSD) && defined(HAVE_TTY)
  if (!CONSOLEP (Vcontrolling_terminal) ||
      !CONSOLE_LIVE_P (XCONSOLE (Vcontrolling_terminal)))
    return;

  if (STRINGP (stuffstring))
    {
      Bytecount count;
      Extbyte *p;

      LISP_STRING_TO_SIZED_EXTERNAL (stuffstring, p, count, Qkeyboard);
      while (count-- > 0)
	stuff_char (XCONSOLE (Vcontrolling_terminal), *p++);
      stuff_char (XCONSOLE (Vcontrolling_terminal), '\n');
    }
  /* Anything we have read ahead, put back for the shell to read.  */
# if 0 /* oh, who cares about this silliness */
  while (kbd_fetch_ptr != kbd_store_ptr)
    {
      if (kbd_fetch_ptr == kbd_buffer + KBD_BUFFER_SIZE)
	kbd_fetch_ptr = kbd_buffer;
      stuff_char (XCONSOLE (Vcontrolling_terminal), *kbd_fetch_ptr++);
    }
# endif
#endif /* BSD && HAVE_TTY */
}

DEFUN ("suspend-console", Fsuspend_console, 0, 1, "", /*
Suspend a console.  For tty consoles, it sends a signal to suspend
the process in charge of the tty, and removes the devices and
frames of that console from the display.

If optional arg CONSOLE is non-nil, it is the console to be suspended.
Otherwise it is assumed to be the selected console.

Some operating systems cannot stop processes and resume them later.
On such systems, who knows what will happen.
*/
       (USED_IF_TTY (console)))
{
#ifdef HAVE_TTY
  struct console *con = decode_console (console);

  if (CONSOLE_TTY_P (con))
    {
      /*
       * hide all the unhidden frames so the display code won't update
       * them while the console is suspended.
       */
      Lisp_Object device = CONSOLE_SELECTED_DEVICE (con);
      if (!NILP (device))
	{
	  struct device *d = XDEVICE (device);
	  Lisp_Object frame_list = DEVICE_FRAME_LIST (d);
	  while (CONSP (frame_list))
	    {
	      struct frame *f = XFRAME (XCAR (frame_list));
	      if (FRAME_REPAINT_P (f))
		FRAME_VISIBLE_P (f) = -1;
	      frame_list = XCDR (frame_list);
	    }
	}
      reset_one_console (con);
      event_stream_unselect_console (con);
      sys_suspend_process (XFIXNUM (Fconsole_tty_controlling_process (console)));
    }
#endif /* HAVE_TTY */

  return Qnil;
}

DEFUN ("resume-console", Fresume_console, 1, 1, "", /*
Re-initialize a previously suspended console.
For tty consoles, do stuff to the tty to make it sane again.
*/
       (USED_IF_TTY (console)))
{
#ifdef HAVE_TTY
  struct console *con = decode_console (console);

  if (CONSOLE_TTY_P (con))
    {
      /* raise the selected frame */
      Lisp_Object device = CONSOLE_SELECTED_DEVICE (con);
      if (!NILP (device))
	{
	  struct device *d = XDEVICE (device);
	  Lisp_Object frame = DEVICE_SELECTED_FRAME (d);
	  if (!NILP (frame))
	    {
	      /* force the frame to be cleared */
	      SET_FRAME_CLEAR (XFRAME (frame));
	      Fraise_frame (frame);
	    }
	}
      init_one_console (con);
      event_stream_select_console (con);
#ifdef SIGWINCH
      /* The same as in Fsuspend_emacs: it is possible that a size
	 change occurred while we were suspended.  Assume one did just
	 to be safe.  It won't hurt anything if one didn't. */
      asynch_device_change_pending++;
#endif
    }
#endif /* HAVE_TTY */

  return Qnil;
}

DEFUN ("set-input-mode", Fset_input_mode, 3, 5, 0, /*
Set mode of reading keyboard input.
First arg (formerly INTERRUPT-INPUT) is ignored, for backward compatibility.
Second arg FLOW non-nil means use ^S/^Q flow control for output to terminal
 (no effect except in CBREAK mode).
Third arg META t means accept 8-bit input (for a Meta key).
 META nil means ignore the top bit, on the assumption it is parity.
 Otherwise, accept 8-bit input and don't use the top bit for Meta.
First three arguments only apply to TTY consoles.
Optional fourth arg QUIT if non-nil specifies character to use for quitting.
Optional fifth arg CONSOLE specifies console to make changes to; nil means
 the selected console.
See also `current-input-mode'.
*/
       (UNUSED (ignored), USED_IF_TTY (flow), meta, quit, console))
{
  struct console *con = decode_console (console);
  unsigned int meta_key = (!CONSOLE_TTY_P (con) ? 1 :
			     EQ (meta, Qnil)      ? 0 :
			     EQ (meta, Qt)        ? 1 :
			     2);

  if (!NILP (quit))
    {
      if (CHAR_OR_CHAR_INTP (quit) && !meta_key)
	set_quit_events (con, make_char (XCHAR_OR_CHAR_INT (quit) & 0177));
      else
	set_quit_events (con, quit);
    }

#ifdef HAVE_TTY
  if (CONSOLE_TTY_P (con))
    {
      reset_one_console (con);
      TTY_FLAGS (con).flow_control = !NILP (flow);
      TTY_FLAGS (con).meta_key = meta_key & 0x2;
      init_one_console (con);
      MARK_FRAME_CHANGED (XFRAME (CONSOLE_SELECTED_FRAME (con)));
    }
#endif

  return Qnil;
}

DEFUN ("current-input-mode", Fcurrent_input_mode, 0, 1, 0, /*
Return information about the way Emacs currently reads keyboard input.
Optional arg CONSOLE specifies console to return information about; nil means
 the selected console.
The value is a list of the form (nil FLOW META QUIT), where
  FLOW is non-nil if Emacs uses ^S/^Q flow control for output to the
    terminal; this does not apply if Emacs uses interrupt-driven input.
  META is t if accepting 8-bit input with 8th bit as Meta flag.
    META nil means ignoring the top bit, on the assumption it is parity.
    META is neither t nor nil if accepting 8-bit input and using
    all 8 bits as the character code.
  QUIT is the character Emacs currently uses to quit.
FLOW, and META are only meaningful for TTY consoles.
The elements of this list correspond to the arguments of
`set-input-mode'.
*/
       (console))
{
  struct console *con = decode_console (console);
  Lisp_Object flow, meta;

#ifdef HAVE_TTY
  flow = CONSOLE_TTY_P (con) && TTY_FLAGS (con).flow_control ? Qt : Qnil;
  meta = (!CONSOLE_TTY_P (con) ? Qt :
	  TTY_FLAGS (con).meta_key == 1 ? Qt :
	  TTY_FLAGS (con).meta_key == 2 ? Qzero :
	  Qnil);
#else
  flow = Qnil;
  meta = Qt;
#endif

  return list4 (Qnil, flow, meta, CONSOLE_QUIT_CHAR (con));
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_console (void)
{
  DEFINE_NODUMP_LISP_OBJECT ("console", console, print_console, 0, 0, 0, 
                             console_description,
                             struct console);

  DEFSUBR (Fvalid_console_type_p);
  DEFSUBR (Fconsole_type_list);
  DEFSUBR (Fcdfw_console);
  DEFSUBR (Fselected_console);
  DEFSUBR (Fselect_console);
  DEFSUBR (Fconsole_live_p);
  DEFSUBR (Fconsole_type);
  DEFSUBR (Fconsole_name);
  DEFSUBR (Fconsole_connection);
  DEFSUBR (Ffind_console);
  DEFSUBR (Fget_console);
  DEFSUBR (Fdelete_console);
  DEFSUBR (Fconsole_list);
  DEFSUBR (Fconsole_device_list);
  DEFSUBR (Fconsole_enable_input);
  DEFSUBR (Fconsole_disable_input);
  DEFSUBR (Fconsole_on_window_system_p);
  DEFSUBR (Fsuspend_console);
  DEFSUBR (Fresume_console);

  DEFSUBR (Fsuspend_emacs);
  DEFSUBR (Fset_input_mode);
  DEFSUBR (Fcurrent_input_mode);

  DEFSYMBOL (Qconsolep);
  DEFSYMBOL (Qconsole_live_p);

  DEFSYMBOL (Qcreate_console_hook);
  DEFSYMBOL (Qdelete_console_hook);

  DEFSYMBOL (Qsuspend_hook);
  DEFSYMBOL (Qsuspend_resume_hook);
}

static const struct memory_description console_methods_pointer_description_1[] = {
  { XD_BLOCK_PTR, 0, 1, { &console_methods_description } },
  { XD_END }
};

static const struct sized_memory_description console_methods_pointer_description = {
  sizeof (struct console_methods *),
  console_methods_pointer_description_1
};

static const struct memory_description cmpd_description_1[] = {
  XD_DYNARR_DESC (const_console_methods_pointer_dynarr,
                  &console_methods_pointer_description),
  { XD_END }
};

const struct sized_memory_description cmpd_description = {
  sizeof (const_console_methods_pointer_dynarr),
  cmpd_description_1,
};

static const struct memory_description console_methods_description_1[] = {
  { XD_LISP_OBJECT, offsetof (struct console_methods, name) },

  /* console methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
                                   init_console_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
                                   initially_selected_for_input_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   delete_console_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   semi_canonicalize_device_connection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   semi_canonicalize_console_connection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   canonicalize_console_connection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   canonicalize_device_connection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   device_to_console_connection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   perhaps_init_unseen_key_defaults_method) },

  /* device methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   init_device_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   finish_init_device_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   delete_device_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   asynch_device_change_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   device_system_metrics_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   own_selection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   disown_selection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   get_foreign_selection_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   selection_exists_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   available_selection_types_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   register_selection_data_type_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   selection_data_type_name_method) },
  { XD_BLOCK_DATA_PTR, offsetof (struct console_methods,
				 device_specific_frame_props), 1,
    { &lisp_object_description } },

  /* frame methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   init_frame_1_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   init_frame_2_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   init_frame_3_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   after_init_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   delete_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   focus_on_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   raise_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   lower_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   enable_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   disable_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   get_mouse_position_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_mouse_position_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   make_frame_visible_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   make_frame_invisible_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   iconify_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_property_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   internal_frame_property_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_properties_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_frame_properties_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_frame_size_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_frame_position_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_visible_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_totally_visible_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_iconified_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_title_from_ibyte_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_icon_name_from_ibyte_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_frame_pointer_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_frame_icon_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   popup_menu_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   get_frame_parent_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   update_frame_external_traits_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_size_fixed_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   eject_page_method) },

  /* redisplay methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   left_margin_width_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   right_margin_width_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   text_width_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_display_block_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   divider_height_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   eol_cursor_width_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_vertical_divider_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   clear_to_window_end_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   clear_region_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   clear_frame_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   window_output_begin_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_output_begin_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   window_output_end_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_output_end_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   flash_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   ring_bell_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   frame_redraw_cursor_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   set_final_cursor_coords_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   bevel_area_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_pixmap_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_string_method) },

  /* color methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   initialize_color_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   print_color_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   finalize_color_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   color_instance_equal_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   color_instance_hash_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   color_instance_rgb_components_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   valid_color_name_p_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   color_list_method) },

  /* font methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   initialize_font_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   print_font_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   finalize_font_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   font_instance_truename_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   font_instance_properties_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   font_list_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   find_charset_font_method) }, 
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   font_spec_matches_charset_method) },

  /* image methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   print_image_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   finalize_image_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   unmap_subwindow_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   map_subwindow_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   resize_subwindow_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   redisplay_subwindow_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   redisplay_widget_method) },
  /* Maybe this should be a specifier. Unfortunately specifiers don't
     allow us to represent things at the toolkit level, which is what
     is required here. */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   widget_border_width_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   widget_spacing_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   image_instance_equal_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   image_instance_hash_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   init_image_instance_from_eimage_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   locate_pixmap_file_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   colorize_image_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   widget_query_string_geometry_method) },

  { XD_LISP_OBJECT, offsetof (struct console_methods, image_conversion_list) },

#ifdef HAVE_TOOLBARS
  /* toolbar methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_frame_toolbars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   clear_frame_toolbars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   initialize_frame_toolbars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   free_frame_toolbars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   output_toolbar_button_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   redraw_frame_toolbars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   redraw_exposed_toolbars_method) },
#endif

#ifdef HAVE_SCROLLBARS
  /* scrollbar methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   inhibit_scrollbar_slider_size_change_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   free_scrollbar_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   release_scrollbar_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   create_scrollbar_instance_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   update_scrollbar_instance_values_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   update_scrollbar_instance_status_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   scrollbar_pointer_changed_in_window_method) },
#ifdef MEMORY_USAGE_STATS
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   compute_scrollbar_instance_usage_method) },
#endif
  /* Paint the window's deadbox, a rectangle between window
     borders and two short edges of both scrollbars. */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   redisplay_deadbox_method) },
#endif /* HAVE_SCROLLBARS */

#ifdef HAVE_MENUBARS
  /* menubar methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   update_frame_menubars_method) },
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   free_frame_menubars_method) },
#endif

#ifdef HAVE_DIALOGS
  /* dialog methods */
  { XD_FUNCTION_POINTER, offsetof (struct console_methods,
				   make_dialog_box_internal_method) },
#endif
  { XD_END }
};

const struct sized_memory_description console_methods_description = {
  sizeof (struct console_methods),
  console_methods_description_1
};

void
initialize_console_type (struct console_methods **dest,
                         Lisp_Object symbol)
{
  struct console_methods *result
    = xnew_and_zero (struct console_methods);

  result->name = symbol;
  result->image_conversion_list = Qnil;
  staticpro (&(result->image_conversion_list));
  Dynarr_add (the_console_methods_dynarr, result);

  /* Pick up duplicate definitions of the same console type. */
  structure_checking_assert (*dest == NULL);
  *dest = result;
  dump_add_root_block_ptr (dest, &console_methods_description);
}

void
console_type_create (void)
{
  the_console_methods_dynarr
    = Dynarr_new2 (const_console_methods_pointer_dynarr, 
                   const struct console_methods *);
  dump_add_root_block_ptr (&the_console_methods_dynarr, &cmpd_description);

  /* Initialize the dead console type */
  INITIALIZE_CONSOLE_TYPE (dead);

  /* Now reset the console methods dynarr, because `dead' is not really a
     valid console type. INITIALIZE_CONSOLE_TYPE() does a
     dump_add_root_block_ptr() so the dead console is still reachable for
     dump. */
  Dynarr_delete (the_console_methods_dynarr, 0);
}

void
vars_of_console (void)
{
  DEFVAR_LISP ("create-console-hook", &Vcreate_console_hook /*
Function or functions to call when a console is created.
One argument, the newly-created console.
This is called after the first frame has been created, but before
  calling the `create-device-hook' or `create-frame-hook'.
Note that in general the console will not be selected.
*/ );
  Vcreate_console_hook = Qnil;

  DEFVAR_LISP ("delete-console-hook", &Vdelete_console_hook /*
Function or functions to call when a console is deleted.
One argument, the to-be-deleted console.
*/ );
  Vdelete_console_hook = Qnil;

  DEFVAR_LISP ("function-key-map-parent", &Vfunction_key_map_parent /*
Parent keymap for `function-key-map'.

This keymap is appropriate for bindings that are not console-specific, but
yet should take advantage of the substitution made by `read-key-sequence'
for bindings in `function-key-map'.
*/ );
  Vfunction_key_map_parent = Fmake_sparse_keymap (Qnil);

#ifdef HAVE_WINDOW_SYSTEM
  Fprovide (intern ("window-system"));
#endif

  Vconsole_list = Qnil;
  staticpro_dump_nil (&Vconsole_list);

  Vselected_console = Qnil;
  staticpro_dump_nil (&Vselected_console);

  Vconsole_defaults = Qnil;
  staticpro (&Vconsole_defaults);

  Vconsole_local_symbols = Qnil;
  staticpro (&Vconsole_local_symbols);
}

/* The docstrings for DEFVAR_* are recorded externally by make-docfile.el. */
#define DEFVAR_CONSOLE_LOCAL_1(lname, field_name, forward_type, magicfun)   \
do {									    \
  DEFVAR_SYMVAL_FWD_OBJECT (lname, &(console_local_flags.field_name),	    \
			   forward_type, magicfun);			    \
  {									    \
    Bytecount offset = ((Rawbyte *) XSYMBOL_VALUE_FORWARD_OBJECT_FORWARD    \
			(XSYMBOL_VALUE (intern (lname)))		    \
			- (Rawbyte *) &console_local_flags);		    \
    *((Lisp_Object *)(offset +						    \
		      (Rawbyte *) XCONSOLE (Vconsole_local_symbols)))       \
	    = intern (lname);						    \
  }									    \
} while (0)

#define DEFVAR_CONSOLE_LOCAL_MAGIC(lname, field_name, magicfun)		\
	DEFVAR_CONSOLE_LOCAL_1 (lname, field_name,			\
				SYMVAL_SELECTED_CONSOLE_FORWARD, magicfun)
#define DEFVAR_CONSOLE_LOCAL(lname, field_name)				\
	DEFVAR_CONSOLE_LOCAL_MAGIC (lname, field_name, 0)
#define DEFVAR_CONST_CONSOLE_LOCAL_MAGIC(lname, field_name, magicfun)	\
	DEFVAR_CONSOLE_LOCAL_1 (lname, field_name,			\
				SYMVAL_CONST_SELECTED_CONSOLE_FORWARD, magicfun)
#define DEFVAR_CONST_CONSOLE_LOCAL(lname, field_name)			\
	DEFVAR_CONST_CONSOLE_LOCAL_MAGIC (lname, field_name, 0)

#define DEFVAR_CONSOLE_DEFAULTS_MAGIC(lname, field_name, magicfun)	    \
	DEFVAR_SYMVAL_FWD_OBJECT(lname, &(console_local_flags.field_name),  \
				 SYMVAL_DEFAULT_CONSOLE_FORWARD,	    \
				 magicfun)
#define DEFVAR_CONSOLE_DEFAULTS(lname, field_name)			\
	DEFVAR_CONSOLE_DEFAULTS_MAGIC (lname, field_name, 0)

void
complex_vars_of_console (void)
{
  static const struct sized_memory_description console_description_rbp
    = { sizeof (struct console), console_description };

  /* Make sure all markable slots in console_defaults
     are initialized reasonably, so KKCC won't choke. */
  Vconsole_defaults = ALLOC_NORMAL_LISP_OBJECT (console);
  nuke_all_console_slots (XCONSOLE (Vconsole_defaults), Qnil);
  /* Dump this object, but avoid error that it is undumpable by using
     dump_add_root_block_ptr() rather than dump_add_root_lisp_object(). Don't
     do this at home, the nanny-state Kamala-voting dumper restricting what we
     can do is doing it for a reason.  */
  dump_add_root_block_ptr (&Vconsole_defaults, &console_description_rbp);

  Vconsole_local_symbols = ALLOC_NORMAL_LISP_OBJECT (console);
  nuke_all_console_slots (XCONSOLE (Vconsole_local_symbols), Qnil);
  /* Dump this object in the same way. Same warnings apply. */
  dump_add_root_block_ptr (&Vconsole_local_symbols, &console_description_rbp);

  /* Set up the non-nil default values of various console slots.
     Must do these before making the first console.
     */

  /* ... Nothing here for the moment.
   #### Console-local variables should probably be eliminated.*/

  {
    /*  0 means var is always local.  Default used only at creation.
     * -1 means var is always local.  Default used only at reset and
     *    creation.
     * -2 means there's no lisp variable corresponding to this slot
     *    and the default is only used at creation.
     * -3 means no Lisp variable.  Default used only at reset and creation.
     * >0 is mask.  Var is local if ((console->local_var_flags & mask) != 0)
     *              Otherwise default is used.
     *
     * #### We don't currently ever reset console variables, so there
     * is no current distinction between 0 and -1, and between -2 and -3.
     */
    Lisp_Object always_local_resettable = make_fixnum (-1);

#if 0 /* not used */
    Lisp_Object always_local_no_default = make_fixnum (0);
    Lisp_Object resettable = make_fixnum (-3);
#endif

    /* Assign the local-flags to the slots that have default values.
       The local flag is a bit that is used in the console
       to say that it has its own local value for the slot.
       The local flag bits are in the local_var_flags slot of the
       console.  */

    set_lheader_implementation ((struct lrecord_header *)
				&console_local_flags,
                                LRECORD_IMPLEMENTATION (console));
    nuke_all_console_slots (&console_local_flags, make_fixnum (-2));

    console_local_flags.defining_kbd_macro = always_local_resettable;
    console_local_flags.last_kbd_macro = always_local_resettable;
    console_local_flags.prefix_arg = always_local_resettable;
    console_local_flags.default_minibuffer_frame = always_local_resettable;
    console_local_flags.overriding_terminal_local_map =
      always_local_resettable;
#ifdef HAVE_TTY
    console_local_flags.tty_erase_char = always_local_resettable;
#endif

    console_local_flags.function_key_map = Qone;

    /* #### Warning, 0x4000000 (that's six zeroes) is the largest number
       currently allowable due to the XFIXNUM() handling of this value.
       With some rearrangement you can get 4 more bits. */

    /* This needs to be in the data segment because the VALUE slot of
       symbol_value_forward_object is a data segment pointer, don't use
       ALLOC_NORMAL_LISP_OBJECT() as we do for Vconsole_local_symbols,
       Vconsole_defaults.  */
    dump_add_root_block (&console_local_flags, sizeof (console_local_flags), 
			 console_description);
  }

  DEFVAR_CONSOLE_DEFAULTS ("default-function-key-map", function_key_map /*
Default value of `function-key-map' for consoles that don't override it.
This is the same as (default-value 'function-key-map).
*/ );

  DEFVAR_CONSOLE_LOCAL ("function-key-map", function_key_map /*
Keymap mapping ASCII function key sequences onto their preferred forms.
This allows Emacs to recognize function keys sent from ASCII
terminals at any point in a key sequence.

The `read-key-sequence' function replaces any subsequence bound by
`function-key-map' with its binding.  More precisely, when the active
keymaps have no binding for the current key sequence but
`function-key-map' binds a suffix of the sequence to a vector or string,
`read-key-sequence' replaces the matching suffix with its binding, and
continues with the new sequence.  See `key-binding'.

The events that come from bindings in `function-key-map' are not
themselves looked up in `function-key-map'.

For example, suppose `function-key-map' binds `ESC O P' to [f1].
Typing `ESC O P' to `read-key-sequence' would return
\[#<keypress-event f1>].  Typing `C-x ESC O P' would return
\[#<keypress-event control-X> #<keypress-event f1>].  If [f1]
were a prefix key, typing `ESC O P x' would return
\[#<keypress-event f1> #<keypress-event x>].

The parent keymap of `function-key-map' when created is
`function-key-map-parent', which is not a console-local variable.  Bindings
appropriate for `function-key-map' but which are likely to be relevant to
every created console should be created in `function-key-map-parent'.
*/ );

#ifdef HAVE_TTY
  /* #### Should this somehow go to TTY data?  How do we make it
     accessible from Lisp, then?  */
  DEFVAR_CONSOLE_LOCAL ("tty-erase-char", tty_erase_char /*
The ERASE character as set by the user with stty.
When this value cannot be determined or would be meaningless (on non-TTY
consoles, for example), it is set to nil.
*/ );
#endif

  /* While this should be const it can't be because some things
     (i.e. edebug) do manipulate it. */
  DEFVAR_CONSOLE_LOCAL ("defining-kbd-macro", defining_kbd_macro /*
Non-nil while a keyboard macro is being defined.  Don't set this!
*/ );

  DEFVAR_CONSOLE_LOCAL ("last-kbd-macro", last_kbd_macro /*
Last keyboard macro defined, as a vector of events; nil if none defined.
*/ );

  DEFVAR_CONSOLE_LOCAL ("prefix-arg", prefix_arg /*
The value of the prefix argument for the next editing command.
It may be a number, or the symbol `-' for just a minus sign as arg,
or a list whose car is a number for just one or more C-U's
or nil if no argument has been specified.

You cannot examine this variable to find the argument for this command
since it has been set to nil by the time you can look.
Instead, you should use the variable `current-prefix-arg', although
normally commands can get this prefix argument with (interactive "P").
*/ );

  DEFVAR_CONSOLE_LOCAL ("default-minibuffer-frame",
			default_minibuffer_frame /*
Minibufferless frames use this frame's minibuffer.

Emacs cannot create minibufferless frames unless this is set to an
appropriate surrogate.

XEmacs consults this variable only when creating minibufferless
frames; once the frame is created, it sticks with its assigned
minibuffer, no matter what this variable is set to.  This means that
this variable doesn't necessarily say anything meaningful about the
current set of frames, or where the minibuffer is currently being
displayed.
*/ );

  DEFVAR_CONSOLE_LOCAL ("overriding-terminal-local-map",
			overriding_terminal_local_map /*
Keymap that overrides all other local keymaps, for the selected console only.
If this variable is non-nil, it is used as a keymap instead of the
buffer's local map, and the minor mode keymaps and text property keymaps.
*/ );

  /* Check for DEFVAR_CONSOLE_LOCAL without initializing the corresponding
     slot of console_local_flags and vice-versa.  Must be done after all
     DEFVAR_CONSOLE_LOCAL() calls. */
#define MARKED_SLOT(slot)						\
  assert ((XFIXNUM (console_local_flags.slot) != -2 &&			\
           XFIXNUM (console_local_flags.slot) != -3)			\
	  == !(NILP (XCONSOLE (Vconsole_local_symbols)->slot)));
#include "conslots.h"
}
