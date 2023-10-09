/* TTY-specific Lisp objects.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995 Ben Wing

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

/* Synched up with:  Not in FSF. */

#ifndef INCLUDED_fontcolor_tty_impl_h_
#define INCLUDED_fontcolor_tty_impl_h_

#include "fontcolor-impl.h"
#include "fontcolor-tty.h"

struct tty_color_instance_data
{
  Lisp_Object symbol; /* so we don't have to constantly call Fintern() */
  Lisp_Object escape_fore; /* string: ANSI escape sequence */
  Lisp_Object escape_back; /* string: ANSI escape sequence */
  Boolint is_rgb_known;
  unsigned char red; /* 0-255 */
  unsigned char green; /* 0-255 */
  unsigned char blue; /* 0-255 */
};


#define TTY_COLOR_INSTANCE_DATA(c) 				\
  ((struct tty_color_instance_data *) (c)->data)

#define COLOR_INSTANCE_TTY_SYMBOL(c) (TTY_COLOR_INSTANCE_DATA (c)->symbol)
#define COLOR_INSTANCE_TTY_ESCAPE_FORE(c) (TTY_COLOR_INSTANCE_DATA (c)->escape_fore)
#define COLOR_INSTANCE_TTY_ESCAPE_BACK(c) (TTY_COLOR_INSTANCE_DATA (c)->escape_back)
#define COLOR_INSTANCE_TTY_IS_RGB_KNOWN(c) (TTY_COLOR_INSTANCE_DATA (c)->is_rgb_known)
#define COLOR_INSTANCE_TTY_RED(c) (TTY_COLOR_INSTANCE_DATA (c)->red)
#define COLOR_INSTANCE_TTY_GREEN(c) (TTY_COLOR_INSTANCE_DATA (c)->green)
#define COLOR_INSTANCE_TTY_BLUE(c) (TTY_COLOR_INSTANCE_DATA (c)->blue)

struct tty_font_instance_data
{
  Lisp_Object charset;
};


#define TTY_FONT_INSTANCE_DATA(c) 				\
  ((struct tty_font_instance_data *) (c)->data)

#define FONT_INSTANCE_TTY_CHARSET(c) (TTY_FONT_INSTANCE_DATA (c)->charset)

#endif /* INCLUDED_fontcolor_tty_impl_h_ */
