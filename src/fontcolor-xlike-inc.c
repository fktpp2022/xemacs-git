/* Common code between X and GTK -- fonts and colors.
   Copyright (C) 1991-5, 1997 Free Software Foundation, Inc.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 1996, 2001, 2002, 2003, 2010 Ben Wing.

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

/* Before including this file, you need to define either THIS_IS_X or
   THIS_IS_GTK. */

/* See comment at top of console-xlike-inc.h for an explanation of
   how this file works. */

/* Pango is ready for prime-time now, as far as I understand it. The GTK
   people should be using that. Oh well. (Aidan Kehoe, Sat Nov 4 12:41:12
   CET 2006) */

#include "console-xlike-inc.h"

#ifdef DEBUG_XEMACS
# define DEBUG_FONTS1(format, arg)		\
do {						\
  if (debug_x_fonts)				\
    debug_out (format, arg);			\
} while (0)

# define DEBUG_FONTS2(format, arg1, arg2)	\
do {						\
  if (debug_x_fonts)				\
    debug_out (format, arg1, arg2);		\
} while (0)

# define DEBUG_FONTS3(format, arg1, arg2, arg3)	\
do {						\
  if (debug_x_fonts)				\
    debug_out (format, arg1, arg2, arg3);	\
} while (0)

# define DEBUG_FONTS4(format, arg1, arg2, arg3, arg4)	\
do {							\
  if (debug_x_fonts)					\
    debug_out (format, arg1, arg2, arg3, arg4);		\
} while (0)

# define DEBUG_FONTS_LISP1(format, arg)		\
do {						\
  if (debug_x_fonts)				\
    debug_out_lisp (format, arg);		\
} while (0)

# define DEBUG_FONTS_LISP2(format, arg1, arg2)	\
do {						\
  if (debug_x_fonts)				\
    debug_out_lisp (format, arg1, arg2);	\
} while (0)

# define DEBUG_FONTS_LISP3(format, arg1, arg2, arg3)	\
do {							\
  if (debug_x_fonts)					\
    debug_out_lisp (format, arg1, arg2, arg3);	\
} while (0)

# define DEBUG_FONTS_LISP4(format, arg1, arg2, arg3, arg4)	\
do {								\
  if (debug_x_fonts)						\
    debug_out_lisp (format, arg1, arg2, arg3, arg4);		\
} while (0)
#else /* not DEBUG_XEMACS */
# define DEBUG_FONTS1(format, arg)
# define DEBUG_FONTS2(format, arg1, arg2)
# define DEBUG_FONTS3(format, arg1, arg2, arg3)
# define DEBUG_FONTS4(format, arg1, arg2, arg3, arg4)
# define DEBUG_FONTS_LISP1(format, arg)
# define DEBUG_FONTS_LISP2(format, arg1, arg2)
# define DEBUG_FONTS_LISP3(format, arg1, arg2, arg3)
# define DEBUG_FONTS_LISP4(format, arg1, arg2, arg3, arg4)
#endif


/* For some code it's reasonable to have only one copy and conditionalize
   at run-time.  For other code it isn't. */

static int 
count_hyphens (const Ibyte *str, Bytecount length, Ibyte **last_hyphen)
{
  int hyphen_count = 0; 
  const Ibyte *hyphening = str;
  const Ibyte *new_hyphening;

  for (hyphen_count = 0; 
       NULL != (new_hyphening = (Ibyte *) memchr ((const void *)hyphening, '-', length));
       hyphen_count++)
    {
      ++new_hyphening;
      length -= new_hyphening - hyphening; 
      hyphening = new_hyphening;
    }

  if (NULL != last_hyphen) 
    {
      *last_hyphen = (Ibyte *)hyphening;
    }

  return hyphen_count;
}

static int
XFUN (font_spec_matches_charset) (struct device * USED_IF_XFT (d),
				  Lisp_Object charset,
				  const Ibyte *nonreloc, Lisp_Object reloc,
				  Bytecount offset, Bytecount length,
				  enum font_specifier_matchspec_stages stage)
{
  Lisp_Object registries = Qnil;
  Elemcount i, registries_len;
  const Ibyte *the_nonreloc;
  Bytecount the_length;

  the_nonreloc = nonreloc;
  the_length = length;

  if (!the_nonreloc)
    the_nonreloc = XSTRING_DATA (reloc);
  fixup_internal_substring (nonreloc, reloc, offset, &the_length);
  the_nonreloc += offset;

#ifdef XEMACS_USE_XFT
  if (stage == STAGE_FINAL)
    {
      if (!NILP (reloc))
	{
	  return 0;	 /* #### maybe this will compile and run ;) */
			 /* Jesus, Stephen, what the fuck? */
	}
    }
#endif

  /* Hmm, this smells bad. */
  if (NILP (charset))
    return 1;

  /* Hack! Short font names don't have the registry in them,
     so we just assume the user knows what they're doing in the
     case of ASCII.  For other charsets, you gotta give the
     long form; sorry buster.
     #### FMH: this screws fontconfig/Xft?
     STRATEGY: use fontconfig's ability to hack languages and character
     sets (lang and charset properties).
     #### Maybe we can use the fontconfig model to eliminate the difference
     between faces and fonts?  No - it looks like that would be an abuse
     (fontconfig doesn't know about colors, although Xft does).
     */
  if (EQ (charset, Vcharset_ascii) && 
      (!memchr (the_nonreloc, '*', the_length))
      && (5 > (count_hyphens (the_nonreloc, the_length, NULL))))
    {
      return 1;
    }

  if (STAGE_FINAL == stage)
    {
      registries = Qunicode_registries;
    }
  else if (STAGE_INITIAL == stage)
    {
      registries = XCHARSET_REGISTRIES (charset);
      if (NILP (registries))
	{
	  return 0;
	}
    }
  else assert (0);

  CHECK_VECTOR (registries);
  registries_len = XVECTOR_LENGTH (registries);

  for (i = 0; i < registries_len; ++i)
    {
      if (!(STRINGP (XVECTOR_DATA (registries)[i]))
     	  || (XSTRING_LENGTH (XVECTOR_DATA (registries)[i]) > the_length))
     	{
     	  continue;
     	}

       /* Check if the font spec ends in the registry specified. X11 says
     	  this comparison is case insensitive: XLFD, section 3.11:

     	  "Alphabetic case distinctions are allowed but are for human
     	  readability concerns only. Conforming X servers will perform
     	  matching on font name query or open requests independent of case." */
       if (0 == qxestrcasecmp (XSTRING_DATA (XVECTOR_DATA (registries)[i]), 
     			      the_nonreloc + (the_length - 
     					      XSTRING_LENGTH
     					      (XVECTOR_DATA (registries)[i]))))
     	 {
     	   return 1;
     	 }
    }
  return 0;
}

#ifndef HAVE_GTK

static Lisp_Object
xlistfonts_checking_charset (Lisp_Object device, const Ibyte *xlfd,
			     Lisp_Object charset, 
			     enum font_specifier_matchspec_stages stage)
{
  Extbyte **names;
  Lisp_Object result = Qnil;
  int count = 0, i;
  DECLARE_EISTRING (ei_single_result);
  Extbyte *fontext;

  DEBUG_FONTS2 ("xlistfonts_checking_charset called, XLFD %s stage %s",
		xlfd, stage == STAGE_INITIAL ? "initial" : "final");
  DEBUG_FONTS_LISP1 (" charset %s\n", charset);
  fontext = ITEXT_TO_EXTERNAL (xlfd, Qx_font_name_encoding); 
  names = XListFonts (GET_XLIKE_X_DISPLAY (XDEVICE (device)),
		      fontext, MAX_FONT_COUNT, &count);

  for (i = 0; i < count; ++i)
    {
      eireset (ei_single_result);
      eicpy_ext (ei_single_result, names[i], Qx_font_name_encoding);

      if (DEVMETH_OR_GIVEN (XDEVICE (device), font_spec_matches_charset,
			   (XDEVICE (device), charset,
			    eidata (ei_single_result), Qnil, 0,
			    -1, stage), 0))
	{
	  result = eimake_string (ei_single_result);
	  break;
	}
    }

  if (names)
    {
      XFreeFontNames (names);
    }

  DEBUG_FONTS_LISP1 ("xlistfonts_checking_charset returns %s\n", result);
  return result;
}
#endif

#ifdef XEMACS_USE_XFT
/* Build the fontconfig character set holding the Unicode codepoints of
   MULE charset CHARSET.  Codepoints without a Unicode mapping are
   skipped; a charset that maps to nothing yields an empty (hence
   vacuously contained) set.  Control characters are skipped too: fonts
   do not provide glyphs for them (fontconfig leaves them out of
   FC_CHARSET), so counting them would make every coverage test fail. */
static FcCharSet *
mule_to_fc_charset (Lisp_Object charset)
{
  int ucode, i, j;
  int l1, l2, h1, h2;
  FcCharSet *fccs;

  CHECK_CHARSET (charset);
  fccs = FcCharSetCreate ();

  get_charset_limits (charset, &l1, &l2, &h1, &h2);

  /* #### This works on the whole charset.  When the font layer starts
     passing the character instead, only that codepoint needs testing. */
  for (i = l1; i <= h1; i++)
    for (j = l2; j <= h2; j++)
      if ((ucode = charset_codepoint_to_unicode (charset, i, j,
						 CONVERR_FAIL)) >= 0
	  && ! (ucode < 0x20 || (ucode >= 0x7f && ucode < 0xa0)))
	/* #### should check for allocation failure */
	FcCharSetAddChar (fccs, (FcChar32) ucode);

  return fccs;
}

/* Under Xft the face font is the base font for every charset, and
   per-character fallback in the draw path supplies glyphs the face font
   lacks.  Font selection here is therefore about which stage of the
   specifier may claim the charset, not about rejecting a font outright:

   - STAGE_INITIAL: accept FONT only if the concrete font it resolves to
     covers the charset's repertoire.  This keeps the documented
     `(two-dimensional final)' contract working: a plain font that does
     not cover the charset falls through to the XLFD/registry matching
     below, and only then to the final tag.  Coverage, not FC_LANG, is the
     test, because a font can carry the wrong language tags and still have
     the glyphs.  A rejection here is harmless; it only advances the
     specifier past this stage.

   - STAGE_FINAL: accept FONT unconditionally, before any registry/XLFD
     work.  The tag mechanism has already chosen this font for the
     charset.  Returning something here is what keeps redisplay from
     substituting CANT_DISPLAY_CHAR (`~') for a face with no final-tag
     font; a font that lacks some glyphs is handled per character in the
     draw path instead. */
static Lisp_Object
xft_find_charset_font (Lisp_Object font, Lisp_Object charset,
		       enum font_specifier_matchspec_stages stage)
{
  FcPattern *p, *match;
  FcCharSet *wanted, *have;
  FcResult fcresult = FcResultNoMatch;

  if (stage == STAGE_FINAL)
    return font;
  assert (stage == STAGE_INITIAL);

  FcInit ();			/* No-op if already initialized.  In
				   fontconfig 2.3.2, this cannot return
				   failure, but that looks like a bug.  We
				   check for it with FcConfigGetCurrent(),
				   which *can* fail. */
  if (!FcConfigGetCurrent ())
    {
      stderr_out ("Failed fontconfig initialization\n");
      return Qnil;
    }

  /* Parse the name, do the substitutions, and match the concrete font.
     Fontconfig converts all FreeType names to UTF-8 before passing them
     back to callers---see fcfreetype.c (FcFreeTypeQuery).
     I don't believe this is documented.  */
  p = FcNameParse ((FcChar8 *) LISP_STRING_TO_EXTERNAL (font,
							Qfc_font_name_encoding));
  if (!p)
    return Qnil;
  FcConfigSubstitute (NULL, p, FcMatchPattern);
  FcDefaultSubstitute (p);
  match = FcFontMatch (NULL, p, &fcresult);
  FcPatternDestroy (p);
  if (!match)
    return Qnil;

  wanted = mule_to_fc_charset (charset);
  if (fcresult == FcResultMatch
      && wanted
      && FcPatternGetCharSet (match, FC_CHARSET, 0, &have) == FcResultMatch
      && FcCharSetIsSubset (wanted, have))
    {
      FcCharSetDestroy (wanted);
      FcPatternDestroy (match);
      return font;
    }

  if (wanted)
    FcCharSetDestroy (wanted);
  FcPatternDestroy (match);
  return Qnil;
}

#endif /* XEMACS_USE_XFT */

#if !defined (HAVE_GTK)
/* find a font spec that matches font spec FONT and also matches
   (the registry of) CHARSET. */
static Lisp_Object
XFUN (find_charset_font) (Lisp_Object device, Lisp_Object font,
			  Lisp_Object charset,
			  enum font_specifier_matchspec_stages stage)
{
  Lisp_Object result = Qnil, registries = Qnil;
  int j, hyphen_count, registries_len = 0;
  Ibyte *hyphening, *new_hyphening;
  Bytecount xlfd_length;

  DECLARE_EISTRING (ei_xlfd_without_registry);
  DECLARE_EISTRING (ei_xlfd);

#ifdef XEMACS_USE_XFT 
  result = xft_find_charset_font (font, charset, stage);
  if (!NILP (result)) 
    {
      return result;
    }
#endif 

  switch (stage) 
    {
    case STAGE_INITIAL:
      {
	if (!(NILP (XCHARSET_REGISTRIES (charset))) 
	    && VECTORP (XCHARSET_REGISTRIES (charset)))
	  {
	    registries_len = XVECTOR_LENGTH (XCHARSET_REGISTRIES (charset));
	    registries = XCHARSET_REGISTRIES (charset);
	  }
	break;
      }
    case STAGE_FINAL:
      {
	registries_len = 1;
	registries = Qunicode_registries;
	break;
      }
    default:
      {
	assert (0);
	break;
      }
    }

  eicpy_lstr (ei_xlfd, font);
  hyphening = eidata (ei_xlfd);
  xlfd_length = eilen (ei_xlfd);

  /* Count the hyphens in the string, moving new_hyphening to just after the
     last one. */
  hyphen_count = count_hyphens (hyphening, xlfd_length, &new_hyphening);

  if (0 == registries_len || (5 > hyphen_count && 
			      !(1 == xlfd_length && '*' == *hyphening)))
    {
      /* No proper XLFD specified, or we can't modify the pattern to change
	 the registry and encoding to match what we want, or we have no
	 information on the registry needed.  */
      result = xlistfonts_checking_charset (device, eidata (ei_xlfd),
					    charset, stage);
      /* No need to loop through the available registries; return
	 immediately. */
      return result;
    }
  else if (1 == xlfd_length && '*' == *hyphening) 
    {
      /* It's a single asterisk. We can add the registry directly to the
         end. */
      eicpy_ch (ei_xlfd_without_registry, '*');
    }
  else 
    {
      /* It's a fully-specified XLFD. Work out where the registry and
         encoding are, and initialise ei_xlfd_without_registry to the string
         without them. */

      /* count_hyphens has set new_hyphening to just after the last
	 hyphen. Move back to just after the hyphen before it. */

      for (new_hyphening -= 2; new_hyphening > hyphening 
	     && '-' != *new_hyphening; --new_hyphening)
	;
      ++new_hyphening;

      eicpy_ei (ei_xlfd_without_registry, ei_xlfd); 

      /* Manipulate ei_xlfd_without_registry, using the information about
	 ei_xlfd, to which it's identical. */
      eidel (ei_xlfd_without_registry, new_hyphening - hyphening, -1, 
	    eilen (ei_xlfd) - (new_hyphening - hyphening), -1);

    }

  /* Now, loop through the registries and encodings defined for this
     charset, doing an XListFonts each time with the pattern modified to
     specify the regisry and encoding. This avoids huge amounts of IPC and
     duplicated searching; now we use the searching the X server was doing
     anyway, where before the X server did its search, transferred huge
     amounts of data, and then we proceeded to do a regexp search on that
     data. */
  for (j = 0; j < registries_len && NILP (result); ++j)
    {
      eireset (ei_xlfd);
      eicpy_ei (ei_xlfd, ei_xlfd_without_registry);

      eicat_lstr (ei_xlfd, XVECTOR_DATA (registries)[j]);

      result = xlistfonts_checking_charset (device, eidata (ei_xlfd),
					    charset, stage);
    }

  /* In the event that the charset is ASCII and we haven't matched
     anything up to now, even with a pattern of "*", add "iso8859-1"
     to the charset's registry and try again. Not returning a result
     for ASCII means our frame geometry calculations are
     inconsistent, and that we may crash. */

  if (1 == xlfd_length && EQ (charset, Vcharset_ascii) && NILP (result)
      && ('*' == eigetch (ei_xlfd_without_registry, 0)))

    {
      int have_latin1 = 0;

      /* Set this to, for example, is08859-1 if you want to see the
	 error behaviour. */

#define FALLBACK_ASCII_REGISTRY "iso8859-1" 

      for (j = 0; j < registries_len; ++j)
	{
	  if (0 == qxestrcasecmp (XSTRING_DATA (XVECTOR_DATA (registries)[j]),
				 (Ibyte *) FALLBACK_ASCII_REGISTRY))
	    {
	      have_latin1 = 1;
	      break;
	    }
	}

      if (!have_latin1)
	{
	  Lisp_Object new_registries = make_vector (registries_len + 1, Qnil);

	  XVECTOR_DATA (new_registries)[0]
	    = build_ascstring (FALLBACK_ASCII_REGISTRY);

	  memcpy (XVECTOR_DATA (new_registries) + 1,
		 XVECTOR_DATA (registries),
		 sizeof XVECTOR_DATA (registries)[0] * 
		 XVECTOR_LENGTH (registries));

	  /* Calling set_charset_registries instead of overwriting the
	     value directly, to allow the charset font caches to be
	     invalidated and a change to the default face to be
	     noted.  */
	  set_charset_registries (charset, new_registries);

	  warn_when_safe (Qface, Qwarning,
			  "Your ASCII charset registries contain nothing "
			  "sensible.  Adding `" FALLBACK_ASCII_REGISTRY "'.");

	  /* And recurse. */
	  result = 
	    DEVMETH_OR_GIVEN (XDEVICE (device), find_charset_font,
			      (device, font, charset, stage),
			      result);
	}
      else
	{
	  DECLARE_EISTRING (ei_connection_name);

	  /* We preserve a copy of the connection name for the error message
	     after the device is deleted. */
	  eicpy_lstr (ei_connection_name, 
		      DEVICE_CONNECTION (XDEVICE (device)));

	  stderr_out ("Cannot find a font for ASCII, deleting device on %s\n",
		      eidata (ei_connection_name));

	  io_error_delete_device (device);

	  /* Do a normal warning in the event that we have other, non-X
	     frames available. (If we don't, io_error_delete_device will
	     have exited.) */
	  warn_when_safe 
	    (Qface, Qerror,
	     "Cannot find a font for ASCII, deleting device on %s.\n"
	     "\n"
	     "Your X server fonts appear to be inconsistent; fix them, or\n"
	     "the next frame you create on that DISPLAY will crash this\n"
	     "XEmacs.   At a minimum, provide one font with an XLFD ending\n"
	     "in `" FALLBACK_ASCII_REGISTRY "', so we can work out what size\n"
	     "a frame should be. ",
	     eidata (ei_connection_name));
	}

    }

  /* This function used to return the font spec, in the case where a font
     didn't exist on the X server but it did match the charset. We're not
     doing that any more, because none of the other platform code does, and
     the old behaviour was badly-judged in other respects, so I don't trust
     the original author to have had a good reason for it. */

  return result;
}
#endif /* HAVE_GTK */

