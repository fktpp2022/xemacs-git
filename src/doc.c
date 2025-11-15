/* Record indices of function doc strings stored in a file.
   Copyright (C) 1985, 1986, 1992, 1993, 1994, 1995
   Free Software Foundation, Inc.
   Copyright (C) 2001, 2002, 2004 Ben Wing.

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

/* Synched up with: FSF 19.30. */

/* This file has been Mule-ized. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "bytecode.h"
#include "file-coding.h"
#include "insdel.h"
#include "keymap.h"
#include "lstream.h"
#include "sysfile.h"
#include "extents.h"
#include "elhash.h"

Lisp_Object Vinternal_doc_file_name;
Lisp_Object Qdefvar, Qfunction_documentation;

/* Work out what source file a function or variable came from, taking the
   information from the documentation file. */
static Lisp_Object
extract_object_file_name (int fd, OFF_T doc_pos, Boolint standard_doc_file)
{
  Ibyte buf[DOC_MAX_FILENAME_LENGTH+1];
  Ibyte *buffer = buf;
  Bytecount buffer_size = sizeof (buf) - 1, space_left;
  Ibyte *from, *to;
  REGISTER Ibyte *p = buffer;
  Lisp_Object return_me;
  Lisp_Object fdstream = Qnil, instream = Qnil;
  struct gcpro gcpro1, gcpro2;
  EMACS_INT position, seenS = 0;

  GCPRO2 (fdstream, instream);

  position = doc_pos > buffer_size  ? 
    doc_pos - buffer_size : 0; 

  if (0 > lseek (fd, position, 0))
    {
      return_me = list2 (build_msg_string
			 ("Position out of range in doc string file"),
                         make_fixnum (position));
      goto done;
    }

  fdstream = make_filedesc_input_stream (fd, 0, -1, 0, NULL);
  Lstream_set_buffering (XLSTREAM (fdstream), LSTREAM_UNBUFFERED, 0);
  instream =
    make_coding_input_stream
      (XLSTREAM (fdstream), standard_doc_file ? Qescape_quoted : Qbinary,
       CODING_DECODE, 0);
  Lstream_set_buffering (XLSTREAM (instream), LSTREAM_UNBUFFERED, 0);

  space_left = buffer_size - (p - buffer);
  while (space_left > 0)
    {
      Bytecount nread;

      nread = Lstream_read (XLSTREAM (instream), p, space_left);
      if (nread < 0)
	{
	  return_me = list1 (build_msg_string
			     ("Read error on documentation file"));
	  goto done;
	}

      p[nread] = 0;

      if (!nread)
	break;

      p += nread;
      space_left = buffer_size - (p - buffer);
    }

  /* First, search backward for the "\037S" that marks the beginning of the
     file name, then search forward from that to the newline or to the end
     of the buffer. */
  from = p; 

  while (from > buf)
    {
      --from;
      if (seenS)
	{
	  if ('\037' == *from) 
	    {
	      /* Got a file name; adjust `from' to point to it, break out of
		 the loop.  */
	      from += 2;
	      break; 
	    }
	}
      /* Is *from 'S' ? */
      seenS = ('S' == *from);
    }

  if (buf == from)
    {
      /* We've scanned back to the beginning of the buffer without hitting the
	 file name. Either the DOC file is corrupt, or it was produced by a
	 version of make-docfile that doesn't store the file name with the
	 symbol name and docstring, which shouldn't happen, given the tight
	 integration of make-docfile.el with the build process. */ 
      return_me = list1 (build_msg_string
			 ("Object file name not stored in doc file"));
      goto done;
    }

  to = from;
  /* Search for the end of the file name. */
  while (++to < p)
    {
      if ('\n' == *to || '\037' == *to)
	{
	  break;
	}
    }

  /* Don't require the file name to end in a newline. */
  return_me = make_string (from, to - from);

 done:
  if (!NILP (instream))
    {
      Lstream_delete (XLSTREAM (instream));
      Lstream_delete (XLSTREAM (fdstream));
    }

  UNGCPRO;
  return return_me;
}

Lisp_Object
unparesseuxify_doc_string (int fd, OFF_T position, Ibyte *name_nonreloc,
                           Lisp_Object name_reloc, Boolint standard_doc_file)
{
  Ibyte buf[512 * 32 + 1];
  Ibyte *buffer = buf;
  int buffer_size = sizeof (buf) - 1;
  Ibyte *from, *to;
  REGISTER Ibyte *p = buffer;
  Lisp_Object return_me;
  Lisp_Object fdstream = Qnil, instream = Qnil;
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (fdstream, instream);

  if (0 > lseek (fd, position, 0))
    {
      if (name_nonreloc)
	name_reloc = build_istring (name_nonreloc);
      return_me = list3 (build_msg_string
			 ("Position out of range in doc string file"),
			  name_reloc, make_fixnum (position));
      goto done;
    }

  fdstream = make_filedesc_input_stream (fd, 0, -1, 0, NULL);
  Lstream_set_buffering (XLSTREAM (fdstream), LSTREAM_UNBUFFERED, 0);
  instream =
    make_coding_input_stream
      /* Major trouble if we are too clever when reading byte-code
	 instructions!

	 #### We should have a way of handling escape-quoted elc files
	 (i.e. files with non-ASCII/Latin-1 chars in them).  Currently this
	 is "solved" in bytecomp.el by never inserting lazy references in
	 such files. */
      (XLSTREAM (fdstream), standard_doc_file ? Qescape_quoted : Qbinary,
       CODING_DECODE, 0);
  Lstream_set_buffering (XLSTREAM (instream), LSTREAM_UNBUFFERED, 0);
  
  /* Read the doc string into a buffer.
     Use the fixed buffer BUF if it is big enough; otherwise allocate one.
     We store the buffer in use in BUFFER and its size in BUFFER_SIZE.  */

  while (1)
    {
      int space_left = buffer_size - (p - buffer);
      int nread;

      /* Switch to a bigger buffer if we need one.  */
      if (space_left == 0)
	{
          Ibyte *old_buffer = buffer;
	  buffer_size *= 2;

	  if (buffer == buf)
	    {
	      buffer = xnew_ibytes (buffer_size + 1);
	      memcpy (buffer, old_buffer, p - old_buffer);
	    }
	  else
            XREALLOC_ARRAY (buffer, Ibyte, buffer_size + 1);
          p += buffer - old_buffer;
	  space_left = buffer_size - (p - buffer);
	}

      /* Don't read too much at one go.  */
      if (space_left > 1024 * 8)
	space_left = 1024 * 8;
      nread = Lstream_read (XLSTREAM (instream), p, space_left);
      if (nread < 0)
	{
	  return_me = list1 (build_msg_string
			     ("Read error on documentation file"));
	  goto done;
	}
      p[nread] = 0;
      if (!nread)
	break;
      {
	Ibyte *p1
	  /* End of doc string marker */
	  = (Ibyte *) memchr ((void *) p, '\037', nread); 
	if (p1)
	  {
	    *p1 = 0;
	    p = p1;
	    break;
	  }
      }
      p += nread;
    }

  /* Scan the text and remove quoting with ^A (char code 1).
     ^A^A becomes ^A, ^A0 becomes a null char, and ^A_ becomes a ^_.  */
  from = to = buffer;
  while (from < p)
    {
      if (*from != 1 /*^A*/)
	*to++ = *from++;
      else
	{
	  int c = *(++from);

	  from++;
          switch (c)
            {
            case 1:   *to++ =  c;     break;
            case '0': *to++ = '\0';   break;
            case '_': *to++ = '\037'; break;
            default:
              return_me = list2 (build_msg_string
	("Invalid data in documentation file -- ^A followed by weird code"),
                                 make_fixnum (c));
              goto done;
            }
	}
    }

  return_me = make_string (buffer, to - buffer);

 done:
  if (!NILP (instream))
    {
      Lstream_delete (XLSTREAM (instream));
      Lstream_delete (XLSTREAM (fdstream));
    }
  UNGCPRO;
  if (buffer != buf) /* We must have allocated buffer above */
    xfree (buffer);
  return return_me;
}

/* Given FILEPOS, either a cons of (FILENAME . POSITION) or just a POSITION,
   return a file descriptor reflecting either FILENAME (if specified) or
   Vinternal_doc_file_name. Error if the doc file name cannot be determined,
   or if it cannot be opened. In *POSITION_OUT, return the decoded POSITION as
   a positive OFF_T. In *standard_doc_filep_out, return whether
   Vinternal_doc_file_name was opened.  */
static int
open_doc_file (Lisp_Object filepos, OFF_T *position_out,
               Boolint *standard_doc_filep_out)
{
  Lisp_Object file, absolute_p, file_id = Qnil;
  Ibyte *file_nonreloc = NULL;
  Bytecount file_nonreloc_len;
  int fd;

  if (CONSP (filepos) && STRINGP (XCAR (filepos)))
    {
      file = XCAR (filepos);
      *standard_doc_filep_out = 0;
      *position_out = lisp_to_OFF_T (XCDR (filepos));
    }
  else
    {
      file = Vinternal_doc_file_name;
      *standard_doc_filep_out = 1;
      *position_out = lisp_to_OFF_T (filepos);
    }

  if (*position_out < 0)
    *position_out = -*position_out;

  if (!STRINGP (file))
    {
      signal_error (Qinternal_error, "No known doc file for position",
                    filepos);
      return -1;
    }

  file_nonreloc_len = sizeof ("../lib-src/-abcdef12") + XSTRING_LENGTH (file);

  if (STRINGP (Vdoc_directory))
    {
      file_nonreloc_len += XSTRING_LENGTH (Vdoc_directory);
    }

  file_nonreloc = alloca_ibytes (file_nonreloc_len);

  /* Put the file name in FILE_NONRELOC as a C string.  If it is relative,
     combine it with Vdoc_directory.  */
  absolute_p = Ffile_name_absolute_p (file);
  if (NILP (absolute_p))
    {
      /* XEmacs: Move this check here.  OK if called during loadup to load
	 byte code instructions. */
      if (!STRINGP (Vdoc_directory))
        {
          signal_error (Qinternal_error, "`doc-directory' not known",
                        filepos);
          return -1;
        }

      if (XSTRING_LENGTH (file) > (Bytecount) (sizeof ("abcdef12")))
        {
          Ibyte *minusp = qxestrrchr (XSTRING_DATA (file), '-');
          if (minusp)
            {
              Ibyte *minus_end;
              INC_IBYTEPTR (minusp);

              file_id = parse_integer (minusp, &minus_end,
                                       XSTRING_LENGTH (file) -
                                       (minusp - XSTRING_DATA (file)),
                                       16, JUNK_ALLOWED, Vdigit_fixnum_ascii);
              /* Check if there is already a dump ID in the doc file name. */
              if (FIXNUMP (file_id) && XFIXNUM (file_id) == dump_id)
                {
                  emacs_snprintf (file_nonreloc, file_nonreloc_len, "%s%s",
                                  XSTRING_DATA (Vdoc_directory),
                                  XSTRING_DATA (file));
                }
              else
                {
                  file_id = Qnil;
                }
            }
        }

      if (NILP (file_id))
        {
          /* If we're looking in Vdoc_directory, prefer a DOC file with our
             dump ID at the end to one without. */
          emacs_snprintf (file_nonreloc, file_nonreloc_len,
                          "%s%s-%08x",
                          XSTRING_DATA (Vdoc_directory),
                          XSTRING_DATA (file), dump_id);
        }
    }
  else
    {
      memcpy (file_nonreloc, XSTRING_DATA (file), XSTRING_LENGTH (file) + 1);
    }

  fd = qxe_open (file_nonreloc, O_RDONLY | OPEN_BINARY, 0);
  if (fd < 0)
    {
      if (purify_flag)
	{
	  /* Preparing to dump; DOC file is probably not installed.  So check
	     in ../lib-src. */
          emacs_snprintf (file_nonreloc, file_nonreloc_len,
                          "../lib-src/%s", XSTRING_DATA (file));

	  fd = qxe_open (file_nonreloc, O_RDONLY | OPEN_BINARY, 0);
	}
      else if (NILP (file_id))
        {
          /* Dumped and possibly installed; check DOC without the dump ID on
             the end. Will never happen with an installed XEmacs with current
             code, but will happen with in-place. */
          emacs_snprintf (file_nonreloc, file_nonreloc_len, "%s%s",
                          XSTRING_DATA (Vdoc_directory), XSTRING_DATA (file));

	  fd = qxe_open (file_nonreloc, O_RDONLY | OPEN_BINARY, 0);
        }

      if (fd < 0)
	report_file_error ("Cannot open doc string file",
			   build_istring (file_nonreloc));
    }
  else if (*standard_doc_filep_out && NILP (absolute_p) && NILP (file_id))
    {
      /* Successfully opened the file by adding the dump ID at the end. Update
         Vinternal_doc_file_name, which was initially set at dump time and
         doesn't reflect the installed DOC file name. */
      Vinternal_doc_file_name
        = Ffile_name_nondirectory (build_istring (file_nonreloc));
    }

  return fd;
}

/* Extract a doc string from a file.  FILEPOS says where to get it.
   If it is an integer, use that position in the standard DOC file.
   If it is (FILE . INTEGER), use FILE as the file name
   and INTEGER as the position in that file.
   But if INTEGER is negative, make it positive.
   (A negative integer is used for user variables, so we can distinguish
   them without actually fetching the doc string.)  */

Lisp_Object
get_doc_string (Lisp_Object filepos)
{
  REGISTER int fd;
  OFF_T position;
  Boolint standard_doc_file = 0;
  Lisp_Object tem;

  fd = open_doc_file (filepos, &position, &standard_doc_file);

  tem = unparesseuxify_doc_string (fd, position, NULL,
                                   standard_doc_file ? Vinternal_doc_file_name
                                   : Fcar (filepos), standard_doc_file);
  retry_close (fd);

  if (!STRINGP (tem))
    signal_error_1 (Qinvalid_byte_code, tem);

  return tem;
}

static Lisp_Object
get_object_file_name (Lisp_Object filepos)
{
  REGISTER int fd;
  OFF_T position;
  Boolint standard_doc_file = 0;
  Lisp_Object tem;

  fd = open_doc_file (filepos, &position, &standard_doc_file);

  tem = extract_object_file_name (fd, position, standard_doc_file);
  retry_close (fd);

  if (!STRINGP (tem))
    signal_error_1 (Qinvalid_byte_code, tem);

  return tem;
}


DEFUN ("built-in-symbol-file", Fbuilt_in_symbol_file, 1, 2, 0, /*
Return the C source file built-in symbol SYM comes from. 
Don't use this.  Use the more general `symbol-file' (q.v.) instead. 

If TYPE is nil or omitted, any kind of definition is acceptable. 
If TYPE is `defun', then function, subr, special operator or macro definitions
are acceptable.
If TYPE is `defvar', then variable definitions are acceptable.
*/
       (symbol, type))
{
  /* This function can GC */
  Lisp_Object fun;
  Lisp_Object filename = Qnil;

  CHECK_SYMBOL (symbol);

  if (!UNBOUNDP (XSYMBOL_FUNCTION (symbol))
      && (NILP (type) || EQ (type, Qdefun)))
    {
      fun = Findirect_function (symbol);

      if (CONSP (fun) && EQ (Qmacro, XCAR (fun)))
        {
          fun = XCDR (fun);
        }

      if (SUBRP (fun))
	{
          if (NILP (XSUBR (fun)->doc) || EQ (XSUBR (fun)->doc, Qnull_pointer))
            {
              return Qnil;
            }
          else if (FIXNUMP (XSUBR (fun)->doc))
            {
	      return get_object_file_name (XSUBR (fun)->doc);
            }
          else if (STRINGP (XSUBR (fun)->doc))
            {
              return Fget (XSUBR (fun)->doc, Qsymbol_file, Qnil);
            }
          else
            {
              structure_checking_assert (0);
              return Qnil;
            }
	}

      if (COMPILED_FUNCTIONP (fun))
	{
	  Lisp_Object tem;
	  Lisp_Compiled_Function *f = XCOMPILED_FUNCTION (fun);

	  if (! (f->flags.documentationp))
	    return Qnil;
	  tem = compiled_function_documentation (f);
	  if (FIXNUMP (tem))
	    {
              return get_object_file_name (tem);
            }
	}
    }

  if (!UNBOUNDP (XSYMBOL_VALUE (symbol)) && (NILP (type) || EQ (type, Qdefvar)))
    {
      Lisp_Object doc_offset = Fget (symbol, Qvariable_documentation, Qnil);

      if (!NILP (doc_offset))
	{
	  if (FIXNUMP (doc_offset))
	    {
	      filename = get_object_file_name (doc_offset);
	    }
          else if (STRINGP (doc_offset))
            {
              filename = Fget (doc_offset, Qsymbol_file, Qnil);
            }
	  return filename;
	}
    }

  return Qnil;
}

DEFUN ("documentation-property", Fdocumentation_property, 2, 3, 0, /*
Return the documentation string that is SYMBOL's PROP property.
This is like `get', but it can refer to strings stored in the
`doc-directory/DOC' file; and if the value is a string, it is passed
through `substitute-command-keys'.  A non-nil third argument avoids this
translation.
*/
       (symbol, prop, raw))
{
  /* This function can GC */
  Lisp_Object doc = Qnil;
#ifdef I18N3
  REGISTER Lisp_Object domain;
#endif
  struct gcpro gcpro1;

  GCPRO1 (doc);

  doc = Fget (symbol, prop, Qnil);
  if (FIXNUMP (doc))
    doc = get_doc_string (XFIXNUM (doc) > 0 ? doc : make_fixnum (- XFIXNUM (doc)));
  else if (CONSP (doc))
    doc = get_doc_string (doc);
#ifdef I18N3
  if (!NILP (doc))
    {
      domain = Fget (symbol, Qvariable_domain, Qnil);
      if (NILP (domain))
	doc = Fgettext (doc);
      else
	doc = Fdgettext (domain, doc);
    }
#endif
  if (NILP (raw) && STRINGP (doc))
    doc = Fsubstitute_command_keys (doc);
  UNGCPRO;
  return doc;
}

DEFUN ("documentation", Fdocumentation, 1, 2, 0, /*
Return the documentation string of FUNCTION.
Unless a non-nil second argument RAW is given, the
string is passed through `substitute-command-keys'.
*/
       (function, raw))
{
  /* This function can GC */
  Lisp_Object fun;
  Lisp_Object doc;

  if (SYMBOLP (function))
    {
      Lisp_Object tem = Fget (function, Qfunction_documentation, Qnil);
      if (!NILP (tem))
	return Fdocumentation_property (function, Qfunction_documentation,
					raw);
    }

  fun = Findirect_function (function);

  if (SUBRP (fun))
    {
      if (NILP (XSUBR (fun)->doc))
	return Qnil;

      if (FIXNUMP (XSUBR (fun)->doc))
        doc = get_doc_string (XSUBR (fun)->doc);
      else
	doc = XSUBR (fun)->doc;
    }
  else if (COMPILED_FUNCTIONP (fun))
    {
      Lisp_Object tem;
      Lisp_Compiled_Function *f = XCOMPILED_FUNCTION (fun);
      if (! (f->flags.documentationp))
        return Qnil;
      tem = compiled_function_documentation (f);
      if (STRINGP (tem))
	doc = tem;
      else if (NATNUMP (tem) || CONSP (tem))
        {
          doc = get_doc_string (tem);
          /* We may have zero length strings in the docfile for file
             information. */
          if (STRINGP (doc) && 0 == XSTRING_LENGTH (doc))
            {
              return Qnil;
            }
        }
      else
        return Qnil;
    }
  else if (KEYMAPP (fun))
    return build_msg_string ("Prefix command (definition is a keymap of subcommands).");
  else if (STRINGP (fun) || VECTORP (fun))
    return build_msg_string ("Keyboard macro.");
  else if (CONSP (fun))
    {
      Lisp_Object funcar = Fcar (fun);

      if (!SYMBOLP (funcar))
	return Fsignal (Qinvalid_function, list1 (fun));
      else if (EQ (funcar, Qlambda)
             || EQ (funcar, Qautoload))
	{
	  Lisp_Object tem, tem1;
	  tem1 = Fcdr (Fcdr (fun));
	  tem = Fcar (tem1);
	  if (STRINGP (tem))
	    doc = tem;
	  /* Handle a doc reference--but these never come last
	     in the function body, so reject them if they are last.  */
	  else if ((NATNUMP (tem)
                    || (CONSP (tem)
                        && STRINGP (XCAR (tem)) && FIXNUMP (XCDR (tem))))
                   && ! NILP (XCDR (tem1)))
	    doc = get_doc_string (tem);
	  else
	    return Qnil;
	}
      else if (EQ (funcar, Qmacro))
	return Fdocumentation (Fcdr (fun), raw);
      else
	goto oops;
    }
  else
    {
    oops:
      return Fsignal (Qinvalid_function, list1 (fun));
    }

  if (NILP (raw))
    {
      struct gcpro gcpro1;
#ifdef I18N3
      Lisp_Object domain = Qnil;
      if (COMPILED_FUNCTIONP (fun))
	domain = compiled_function_domain (XCOMPILED_FUNCTION (fun));
      if (NILP (domain))
	doc = Fgettext (doc);
      else
	doc = Fdgettext (domain, doc);
#endif

      GCPRO1 (doc);
      doc = Fsubstitute_command_keys (doc);
      UNGCPRO;
    }
  return doc;
}

DEFUN ("substitute-command-keys", Fsubstitute_command_keys, 1, 1, 0, /*
Substitute key descriptions for command names in STRING.
Return a new string which is STRING with substrings of the form \\=\\[COMMAND]
replaced by either:  a keystroke sequence that will invoke COMMAND,
or "M-x COMMAND" if COMMAND is not on any keys.
Substrings of the form \\=\\{MAPVAR} are replaced by summaries
\(made by `describe-bindings') of the value of MAPVAR, taken as a keymap.
Substrings of the form \\=\\<MAPVAR> specify to use the value of MAPVAR
as the keymap for future \\=\\[COMMAND] substrings.
\\=\\= quotes the following character and is discarded;
thus, \\=\\=\\=\\= puts \\=\\= into the output, and \\=\\=\\=\\[ puts \\=\\[ into the output.
*/
       (string))
{
  /* This function can GC */
  const Ibyte *strp, *backslashp, *strdata;
  Boolint changed = 0;
  Ibyte *symname;
  Bytecount strlength = MOST_POSITIVE_FIXNUM, stretch_begin;
  Bytecount idx = 0, symlen, partlen;
  Lisp_Object tem = Qnil, keymap = Qnil, name = Qnil, stream = Qnil;
  struct gcpro gcpro1, gcpro2, gcpro3, gcpro4, gcpro5;

  if (NILP (string))
    return Qnil;

  CHECK_STRING (string);
  GCPRO5 (string, tem, keymap, name, stream);

  /* There is the possibility that the string is not destined for a
     translating stream, and it could be argued that we should do the
     same thing here as in Fformat(), but there are very few times
     when this will be the case and many calls to this function
     would have to have `gettext' calls added. (I18N3) */
  string = LISP_GETTEXT (string);

  /* KEYMAP is either nil (which means search all the active keymaps)
     or a specified local map (which means search just that and the
     global map).  If non-nil, it might come from Voverriding_local_map,
     or from a \\<mapname> construct in STRING itself..  */
#if 0 /* FSFmacs */
  /* This is really weird and garbagey.  If keymap is nil and there's
     an overriding-local-map, `where-is-internal' will correctly note
     this, so there's no reason to do it here.  Maybe FSFmacs
     `where-is-internal' is broken. */
  /*
  keymap = current_kboard->Voverriding_terminal_local_map;
  if (NILP (keymap))
    keymap = Voverriding_local_map;
  */
#endif

  /* XEmacs; revise this extensively to preserve extent information coming
     from the source string in the output of describe_map_tree (); use our
     resizing buffer rather than a full-on Lisp buffer to preserve
     non-duplicable string extents and to avoid entertainment with default
     values for hooks.

     See doprnt.c for commentary on `text-quoting-style' and #'format-message
     also relevant to #'substitute-command-keys. */
  while (idx < strlength)
    {
      /* Re-fetch the string data after Lisp has been called. */
      strdata = XSTRING_DATA (string);
      strlength = XSTRING_LENGTH (string);
      strp = strdata + idx;
      backslashp = (const Ibyte *) memchr (strp, '\\', 
					   max (strlength - idx, 0));
      partlen = backslashp ? backslashp - strp : strlength - idx;

      if (changed)
        {
          write_lisp_string (stream, string, idx, partlen);
        }

      if (!backslashp)
        {
          break;
        }
      
      idx = backslashp - strdata + ichar_len ('\\');
      strdata = XSTRING_DATA (string);
      strlength = XSTRING_LENGTH (string);
      strp = strdata + idx;

      if (idx + ichar_len ('[') < strlength && itext_ichar_eql (strp, '['))
        {
          if (!changed)
            {
              changed = 1;
              stream = make_resizing_buffer_output_stream ();
              Lstream_write_with_extents (XLSTREAM (stream),
                                          string, 0, idx - ichar_len ('\\'));
            }
          
          idx += ichar_len ('[');
          strp = strdata + idx;
          strp = (Ibyte *) memchr (strp, ']', max (strlength - idx, 0));
 
          if (!strp)
            {
              strp = strdata + strlength;
            }
 
          symlen = (strp - strdata) - idx;
          symname = alloca_ibytes (symlen + 1);
          memcpy (symname, strdata + idx, symlen);
          symname[symlen] = '\0';

          tem = intern ((const CIbyte *) symname);
          tem = Fwhere_is_internal (tem, keymap, Qt, Qnil, Qnil);

          stretch_begin = stream_extent_position (stream);
          
          if (NILP (tem))
            {
              write_ascstring (stream, "M-x ");
              write_string_1 (stream, symname, symlen);
            }
          else
            {
              /* Don't do the assignment within the write_lisp_string()
                 arglist, TEM is GCPROd while the argument may not be. */
              tem = Fkey_description (tem);
              write_lisp_string (stream, tem, 0, XSTRING_LENGTH (tem));
            }

          stretch_string_extents (stream, string, stretch_begin,
                                  idx - ichar_len ('\\') - ichar_len ('['),
                                  symlen + ichar_len ('\\') + ichar_len ('['),
                                  stream_extent_position (stream)
                                  - stretch_begin);
          idx = idx + symlen;
          if (idx < strlength)
            {
              idx += ichar_len (']');
            }
        }
      else if (idx + ichar_len ('{') < strlength
               && itext_ichar_eql (strp, '{'))
        {
          if (!changed)
            {
              changed = 1;
              stream = make_resizing_buffer_output_stream ();
              Lstream_write_with_extents (XLSTREAM (stream),
                                          string, 0, idx - ichar_len ('\\'));
            }
          
          idx += ichar_len ('{');
          strdata = XSTRING_DATA (string);
          strp = strdata + idx;
          strp = (Ibyte *) memchr (strp, '}', strlength - idx);
 
          if (!strp)
            {
              strp = strdata + strlength;
            }
 
          symlen = (strp - strdata) - idx;
          symname = alloca_ibytes (symlen + 1);
          memcpy (symname, strdata + idx, symlen);
          symname[symlen] = '\0';

          name = intern ((const CIbyte *) symname);
          tem = Fboundp (name);
          if (!NILP (tem))
            {
              tem = Fsymbol_value (name);
              if (!NILP (tem))
                tem = get_keymap (tem, 0, 1);
            }

          stretch_begin = stream_extent_position (stream);

          if (NILP (tem))
            {
              write_ascstring (stream, "(uses keymap \"");
              write_lisp_string (stream, XSYMBOL_NAME (name), 0, 
                                 XSTRING_LENGTH (XSYMBOL_NAME (name)));
              write_ascstring (stream,
                               "\", which is not currently defined) ");
            }
          else
            {
              describe_map_tree (tem, 1, Qnil, Qnil, 0, stream);
            }

          stretch_string_extents (stream, string, stretch_begin,
                                  idx - ichar_len ('\\') - ichar_len ('{'),
                                  symlen + ichar_len ('\\') + ichar_len ('{'),
                                  stream_extent_position (stream)
                                  - stretch_begin);
          idx = idx + symlen;
          if (idx < strlength)
            {
              idx += ichar_len ('}');
            }
        }
      else if (idx + ichar_len ('=') < strlength
	       && itext_ichar_eql (strp, '='))
        {
          Bytecount blen;

          if (!changed)
            {
              changed = 1;
              stream = make_resizing_buffer_output_stream ();
              Lstream_write_with_extents (XLSTREAM (stream),
                                          string, 0,
                                          idx - ichar_len ('\\'));
            }

          idx += ichar_len ('=');
          /* \= quotes the next character; thus, to put in \[ without its
             special meaning, use \=\[.  */
          blen = itext_ichar_len (strdata + idx);
          write_lisp_string (stream, string, idx, blen);
          idx += blen;
        }
      else if (idx + ichar_len ('<') < strlength
               && itext_ichar_eql (strp, '<'))
        {
          if (!changed)
            {
              changed = 1;
              stream = make_resizing_buffer_output_stream ();
              Lstream_write_with_extents (XLSTREAM (stream),
                                          string, 0,
                                          idx - ichar_len ('\\'));
            }

          idx += ichar_len ('<');
          strdata = XSTRING_DATA (string);
          strp = strdata + idx;
          strp = (Ibyte *) memchr (strp, '>', strlength - idx);
 
          if (!strp)
            {
              strp = strdata + strlength;
            }
 
          symlen = (strp - strdata) - idx;
          symname = alloca_ibytes (symlen + 1);
          memcpy (symname, strdata + idx, symlen);
          symname[symlen] = '\0';

          name = intern ((const CIbyte *) symname);
          tem = Fboundp (name);
          if (!NILP (tem))
            {
              tem = Fsymbol_value (name);
              if (!NILP (tem))
                tem = get_keymap (tem, 0, 1);
            }

          stretch_begin = stream_extent_position (stream);
          
          if (NILP (tem))
            {
              write_ascstring (stream, "(uses keymap \"");
              write_lisp_string (stream, XSYMBOL_NAME (name), 0, 
                                 XSTRING_LENGTH (XSYMBOL_NAME (name)));
              write_ascstring (stream,
                               "\", which is not currently defined) ");
            }
          else
            {
              keymap = tem;
            }
          
          stretch_string_extents (stream, string, stretch_begin,
                                  idx - ichar_len ('\\') -
                                  ichar_len ('<'),
                                  symlen + ichar_len ('\\') +
                                  ichar_len ('<'),
                                  stream_extent_position (stream)
                                  - stretch_begin);
          idx = idx + symlen;
          if (idx < strlength)
            {
              idx += ichar_len ('>');
            }
        }
      else if (changed)
        {
          Lstream_write_with_extents (XLSTREAM (stream), string,
                                      idx - ichar_len ('\\'),
                                      ichar_len ('\\'));
        }
      /* If !changed, do nothing with this backslash, just increment past
         it, which we've done above already. */
    }

  if (changed)			/* don't bother if nothing substituted */
    {
      tem = resizing_buffer_to_lisp_string (XLSTREAM (stream));
      Lstream_delete (XLSTREAM (stream));
    }
  else
    {
      tem = string;
    }

  UNGCPRO;
  return tem;
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_doc (void)
{
  DEFSUBR (Fbuilt_in_symbol_file);
  DEFSUBR (Fdocumentation);
  DEFSUBR (Fdocumentation_property);
  DEFSUBR (Fsubstitute_command_keys);

  DEFSYMBOL (Qdefvar);
  DEFSYMBOL (Qfunction_documentation);
}

void
vars_of_doc (void)
{
  DEFVAR_LISP ("internal-doc-file-name", &Vinternal_doc_file_name /*
Name of file containing documentation strings of built-in symbols.
*/ );
  Vinternal_doc_file_name = Qnil;
}
