/* Generic object functions -- interface.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995, 1996, 2002, 2010 Ben Wing.

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

#ifndef INCLUDED_fontcolor_h_
#define INCLUDED_fontcolor_h_

DECLARE_DOESNT_RETURN (finalose (void *ptr));

typedef unsigned long Rgbref;
extern const Rgbref invalid_rgb;

#define __INTERNAL_RGB_PACK(r, g, b) ((((r) << 16) | ((g) << 8) | (b)) & 0xffffff)
#define RGB_CONSTANT(r, g, b) __INTERNAL_RGB_PACK(r, g, b)

DECLARE_INLINE_HEADER (
Rgbref
rgb_pack (unsigned char r, unsigned char g, unsigned char b)
)
{
  return __INTERNAL_RGB_PACK (r, g, b);
}

DECLARE_INLINE_HEADER (
Boolint
rgb_valid_p (Rgbref rgb)
)
{
  return rgb <= RGB_CONSTANT(255, 255, 255);
}

DECLARE_INLINE_HEADER (
unsigned char
rgb_red (Rgbref rgb)
)
{
  assert (rgb_valid_p (rgb));
  return (rgb >> 16) & 0xff;
}

DECLARE_INLINE_HEADER (
unsigned char
rgb_green (Rgbref rgb)
)
{
  assert (rgb_valid_p (rgb));
  return (rgb >> 8) & 0xff;
}

DECLARE_INLINE_HEADER (
unsigned char
rgb_blue (Rgbref rgb)
)
{
  assert (rgb_valid_p (rgb));
  return rgb & 0xff;
}

Rgbref shared_X_string_to_color (const Ibyte *name);
Lisp_Object shared_X_color_to_string (Rgbref color);
Lisp_Object shared_X_color_list (void);

/****************************************************************************
 *                           Color Instance Object                          *
 ****************************************************************************/

DECLARE_LISP_OBJECT (color_instance, Lisp_Color_Instance);
#define XCOLOR_INSTANCE(x) XRECORD (x, color_instance, Lisp_Color_Instance)
#define wrap_color_instance(p) wrap_record (p, color_instance)
#define COLOR_INSTANCEP(x) RECORDP (x, color_instance)
#define CHECK_COLOR_INSTANCE(x) CHECK_RECORD (x, color_instance)
#define CONCHECK_COLOR_INSTANCE(x) CONCHECK_RECORD (x, color_instance)

EXFUN (Fmake_color_instance, 3);

extern Lisp_Object Vthe_null_color_instance;

void set_color_attached_to (Lisp_Object obj, Lisp_Object face,
			    Lisp_Object property);

/****************************************************************************
 *                            Font Instance Object                          *
 ****************************************************************************/

void initialize_charset_font_caches (struct device *d);
void invalidate_charset_font_caches (Lisp_Object charset);

DECLARE_LISP_OBJECT (font_instance, Lisp_Font_Instance);
#define XFONT_INSTANCE(x) XRECORD (x, font_instance, Lisp_Font_Instance)
#define wrap_font_instance(p) wrap_record (p, font_instance)
#define FONT_INSTANCEP(x) RECORDP (x, font_instance)
#define CHECK_FONT_INSTANCE(x) CHECK_RECORD (x, font_instance)
#define CONCHECK_FONT_INSTANCE(x) CONCHECK_RECORD (x, font_instance)

EXFUN (Fmake_font_instance, 4);
EXFUN (Ffont_instance_name, 1);
EXFUN (Ffont_instance_p, 1);
EXFUN (Ffont_instance_truename, 1);
EXFUN (Ffont_instance_charset, 1);

extern Lisp_Object Vthe_null_font_instance;

void set_font_attached_to (Lisp_Object obj, Lisp_Object face,
			   Lisp_Object property);

/*****************************************************************************
 *                       Face Boolean Specifier Object                       *
 *****************************************************************************/

void set_face_boolean_attached_to (Lisp_Object obj, Lisp_Object face,
				   Lisp_Object property);

/*****************************************************************************
 *              Face Background Placement Specifier Object                   *
 *****************************************************************************/

void set_face_background_placement_attached_to (Lisp_Object obj,
						Lisp_Object face);


#endif /* INCLUDED_fontcolor_h_ */
