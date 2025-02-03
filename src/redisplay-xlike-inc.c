/* Common code between X and GTK -- redisplay-related.
   Copyright (C) 1994, 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1994 Lucid, Inc.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 2002, 2003, 2005, 2009, 2010 Ben Wing.
   Copyright (C) 2010 Didier Verna

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

/* Author: Chuck Thompson */
/* Gtk flavor by William Perry */
/* X and GTK code merged by Ben Wing, 1-10 */

/* Lots of work done by Ben Wing for Mule */

/* Before including this file, you need to define either THIS_IS_X or
   THIS_IS_GTK.  See comments in console-xlike-inc.h. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "debug.h"
#include "device-impl.h"
#include "faces.h"
#include "file-coding.h"
#include "frame-impl.h"
#include "gutter.h"
#include "redisplay.h"
#include "sysdep.h"
#include "window.h"
#include "text.h"

#include "mule-ccl.h"
#include "charset.h"

#define NEED_GCCACHE_H
#define NEED_GLYPHS_H
#define NEED_OBJECTS_IMPL_H

#if defined (HAVE_GTK) && !defined (THIS_IS_GTK)
#define THIS_IS_GTK
#endif
#include "console-xlike-inc.h"

#include "sysproc.h" /* for select() */

#ifdef THIS_IS_X
#include "EmacsFrame.h"
#include "EmacsFrameP.h"

#include <X11/bitmaps/gray>
#endif /* THIS_IS_X */

#ifdef THIS_IS_GTK
static void cr_set_foreground (cairo_t *cr, Lisp_Object color);
static void gtk_draw_rectangle (cairo_t *cr, gint x, gint y,
                                gint width, gint height);
static void gtk_fill_rectangle (cairo_t *cr, gint x, gint y,
                                gint width, gint height);
#endif

#define EOL_CURSOR_WIDTH	5

/* Device methods */

#define XLIKE_text_width XFUN (text_width)
#define XLIKE_output_display_block XFUN (output_display_block)
#define XLIKE_divider_height XFUN (divider_height)
#define XLIKE_eol_cursor_width XFUN (eol_cursor_width)
#define XLIKE_output_vertical_divider XFUN (output_vertical_divider)
#define XLIKE_clear_region XFUN (clear_region)
#define XLIKE_clear_frame XFUN (clear_frame)
#define XLIKE_flash XFUN (flash)
#define XLIKE_ring_bell XFUN (ring_bell)
#define XLIKE_bevel_area XFUN (bevel_area)
#define XLIKE_output_string XFUN (output_string)
#define XLIKE_output_pixmap XFUN (output_pixmap)
#define XLIKE_output_xlike_pixmap XFUN (output_xlike_pixmap)
#define XLIKE_window_output_begin XFUN (window_output_begin)
#define XLIKE_window_output_end XFUN (window_output_end)

/* Miscellaneous split functions */

#define console_type_create_redisplay_XLIKE XLIKE_PASTE (console_type_create_redisplay, XLIKE_NAME)
#define XLIKE_output_blank XFUN (output_blank)
#define XLIKE_output_horizontal_line XFUN (output_horizontal_line)
#define XLIKE_output_eol_cursor XFUN (output_eol_cursor)
#define XLIKE_clear_frame_window XFUN (clear_frame_window)
#define XLIKE_clear_frame_windows XFUN (clear_frame_windows)

static int XLIKE_text_width (struct frame *f, struct face_cachel *cachel,
                             const Ibyte *str, Bytecount len);
static void XLIKE_output_vertical_divider (struct window *w, int clear);
static void XLIKE_output_blank (struct window *w, struct display_line *dl,
				struct rune *rb, int start_pixpos,
				int cursor_start, int cursor_width);
static void XLIKE_output_horizontal_line (struct window *w,
					  struct display_line *dl,
					  struct rune *rb);
static void XLIKE_output_eol_cursor (struct window *w,
				     struct display_line *dl,
				     int xpos, face_index findex);
static void XLIKE_output_xlike_pixmap (struct frame *f, Lisp_Image_Instance *p, int x,
				       int y, int xoffset, int yoffset,
				       int width, int height,
				       XLIKE_COLOR fg, XLIKE_COLOR bg);
static void XLIKE_output_pixmap (struct window *w, Lisp_Object image_instance,
				struct display_box *db, struct display_glyph_area *dga,
				face_index findex, int cursor_start, int cursor_width,
				int cursor_height, int bg_pixmap);
static void XLIKE_clear_frame_windows (Lisp_Object window);
static void XLIKE_clear_frame (struct frame *f);
static void XLIKE_bevel_area (struct window *w, face_index findex,
			      int x, int y, int width, int height,
			      int shadow_thickness, int edges,
			      enum edge_style style);
static void XLIKE_ring_bell (struct device *d, int volume, int pitch,
			       int duration);
static void XLIKE_clear_region (Lisp_Object local, struct frame *f,
				face_index findex, int x, int y,
				int width, int height, Lisp_Object fg,
				Lisp_Object bg, Lisp_Object bg_pixmap,
				Lisp_Object bg_placement);
static int XLIKE_flash (struct device *d);

#ifdef THIS_IS_X
static void XLIKE_window_output_begin (struct window *UNUSED (w));
static void XLIKE_window_output_end (struct window *w);
#endif /* THIS_IS_X */

/*****************************************************************************
 XLIKE_divider_height

 Return the height of the horizontal divider.  This is a function because
 divider_height is a device method.

 #### If we add etched horizontal divider lines this will have to get
 smarter.
 ****************************************************************************/
static int
XLIKE_divider_height (void)
{
#ifdef THIS_IS_X
  return 1;
#else /* THIS_IS_GTK */
  return 2;
#endif /* THIS_IS_GTK */
}

/*****************************************************************************
 XLIKE_eol_cursor_width

 Return the width of the end-of-line cursor.  This is a function
 because eol_cursor_width is a device method.
 ****************************************************************************/
static int
XLIKE_eol_cursor_width (void)
{
  return EOL_CURSOR_WIDTH;
}

/*****************************************************************************
 XLIKE_output_display_block

 Given a display line, a block number for that start line, output all
 runes between start and end in the specified display block.
 ****************************************************************************/
static void
XLIKE_output_display_block (struct window *w, struct display_line *dl,
			    int block, int start, int end, int start_pixpos,
			    int cursor_start, int cursor_width,
			    int cursor_height)
{
#ifndef XEMACS_USE_XFT
  struct frame *f = XFRAME (w->frame);
#endif
  Ibyte *buffer, *bufp;
  Lisp_Object window;

  struct display_block *db = Dynarr_atp (dl->display_blocks, block);
  rune_dynarr *rba = db->runes;
  struct rune *rb;

  int elt = start;
  face_index findex;
  int xpos, width = 0;
  Lisp_Object charset = Qunbound; /* Gives a better error message than Qnil. */

  window = wrap_window (w);
  rb = Dynarr_atp (rba, start);

  if (!rb)
    /* Nothing to do so don't do anything. */
    return;

  findex = rb->findex;
  xpos = rb->xpos;
  if (rb->type == RUNE_CHAR)
    /* @@#### fix me */
    charset = buffer_ichar_charset_obsolete_me_baby (WINDOW_XBUFFER (w),
						     rb->object.chr.ch);

  if (end < 0)
    end = Dynarr_length (rba);

  buffer = bufp = alloca_ibytes (end * MAX_ICHAR_LEN);

  while (elt < end)
    {
      rb = Dynarr_atp (rba, elt);

      if (rb->findex == findex && rb->type == RUNE_CHAR
	  && rb->object.chr.ch != '\n' && rb->cursor_type != CURSOR_ON
	  /* @@#### fix me */
	  && EQ (charset,
		 buffer_ichar_charset_obsolete_me_baby (WINDOW_XBUFFER (w),
							rb->object.chr.ch)))
	{
          bufp += set_itext_ichar (bufp, rb->object.chr.ch);
	  width += rb->width;
	  elt++;
	}
      else
	{
	  if (bufp - buffer)
	    {
	      XLIKE_output_string (w, dl, buffer, bufp - buffer, xpos, 0,
                                   start_pixpos, width,
				   findex, 0, cursor_start, cursor_width,
				   cursor_height);
	      xpos = rb->xpos;
	      width = 0;
              bufp = buffer;
	    }
	  width = 0;

	  if (rb->type == RUNE_CHAR)
	    {
	      findex = rb->findex;
	      xpos = rb->xpos;
	      /* @@#### fix me */
	      charset =
		buffer_ichar_charset_obsolete_me_baby (WINDOW_XBUFFER (w),
						       rb->object.chr.ch);

	      if (rb->cursor_type == CURSOR_ON)
		{
		  if (rb->object.chr.ch == '\n')
		    {
		      XLIKE_output_eol_cursor (w, dl, xpos, findex);
		    }
		  else
		    {
		      XLIKE_output_string (w, dl, buffer, 
                                           set_itext_ichar (buffer,
                                                            rb->object.chr.ch),
                                           xpos, 0, start_pixpos,
					   rb->width, findex, 1,
					   cursor_start, cursor_width,
					   cursor_height);
		    }

		  xpos += rb->width;
		  elt++;
		}
	      else if (rb->object.chr.ch == '\n')
		{
		  /* Clear in case a cursor was formerly here. */
		  redisplay_clear_region (window, findex, xpos,
					  XLIKE_DISPLAY_LINE_YPOS (dl),
					  rb->width,
					  XLIKE_DISPLAY_LINE_HEIGHT (dl));
		  elt++;
		}
	    }
	  else if (rb->type == RUNE_BLANK || rb->type == RUNE_HLINE)
	    {
	      if (rb->type == RUNE_BLANK)
		XLIKE_output_blank (w, dl, rb, start_pixpos, cursor_start,
				    cursor_width);
	      else
		{
		  /* #### Our flagging of when we need to redraw the
                     modeline shadows sucks.  Since RUNE_HLINE is only used
                     by the modeline at the moment it is a good bet
                     that if it gets redrawn then we should also
                     redraw the shadows.  This won't be true forever.
                     We borrow the shadow_thickness_changed flag for
                     now. */
		  w->shadow_thickness_changed = 1;
		  XLIKE_output_horizontal_line (w, dl, rb);
		}

	      elt++;
	      if (elt < end)
		{
		  rb = Dynarr_atp (rba, elt);

		  findex = rb->findex;
		  xpos = rb->xpos;
		}
	    }
	  else if (rb->type == RUNE_DGLYPH)
	    {
	      Lisp_Object instance;
	      struct display_box dbox;
	      struct display_glyph_area dga;

	      redisplay_calculate_display_boxes (dl, rb->xpos, rb->object.dglyph.xoffset,
						 rb->object.dglyph.yoffset, start_pixpos,
                                                 rb->width, &dbox, &dga);

	      window = wrap_window (w);
	      instance = glyph_image_instance (rb->object.dglyph.glyph,
					       window, ERROR_ME_DEBUG_WARN, 1);
	      findex = rb->findex;

	      if (IMAGE_INSTANCEP (instance))
		{
		  switch (XIMAGE_INSTANCE_TYPE (instance))
		    {
		    case IMAGE_TEXT:
#ifdef THIS_IS_GTK
		      {
			/* !!#### Examine for Mule-izing */
			/* #### This is way losing.  See the comment in
			   add_glyph_rune(). */
			Lisp_Object string =
                          XIMAGE_INSTANCE_TEXT_STRING (instance);

                        XLIKE_output_string (w, dl, XSTRING_DATA (string),
                                             XSTRING_LENGTH (string), xpos,
                                             rb->object.dglyph.xoffset,
                                             start_pixpos, -1, findex,
                                             (rb->cursor_type == CURSOR_ON),
                                             cursor_start, cursor_width,
                                             cursor_height);
		      }
		      break;
#else
		      ABORT ();
#endif /* THIS_IS_GTK */
		    case IMAGE_MONO_PIXMAP:
		    case IMAGE_COLOR_PIXMAP:
		      redisplay_output_pixmap (w, instance, &dbox, &dga,
					       findex, cursor_start,
					       cursor_width,
					       cursor_height, 0);
		      break;

		    case IMAGE_WIDGET:
		      if (EQ (XIMAGE_INSTANCE_WIDGET_TYPE (instance),
			      Qlayout))
			{
			  redisplay_output_layout (window, instance, &dbox,
						   &dga, findex,
						   cursor_start, cursor_width,
						   cursor_height);
			  break;
			}

		    case IMAGE_SUBWINDOW:
		      redisplay_output_subwindow (w, instance, &dbox, &dga,
						  findex, cursor_start,
						  cursor_width, cursor_height);
		      break;

		    case IMAGE_NOTHING:
		      /* nothing is as nothing does */
		      break;

		    case IMAGE_POINTER:
		    default:
		      ABORT ();
		    }
		  IMAGE_INSTANCE_OPTIMIZE_OUTPUT
		    (XIMAGE_INSTANCE (instance)) = 0;
		}

	      xpos += rb->width;
	      elt++;
	    }
	  else
	    ABORT ();
	}
    }

  if (bufp - buffer)
    XLIKE_output_string (w, dl, buffer, bufp - buffer, xpos, 0, start_pixpos,
                         width, findex, 0, cursor_start, cursor_width,
                         cursor_height);

  if (dl->modeline
      && !EQ (Qzero, w->modeline_shadow_thickness)
#ifndef XEMACS_USE_XFT
      /* This optimization doesn't work right with some Xft fonts, which
	 leave antialiasing turds at the boundary.  I don't know if this
	 is an Xft bug or not, but I think it is.   See x_output_string. */
      && (f->clear
	  || f->windows_structure_changed
	  || w->shadow_thickness_changed)
#endif
      )
    bevel_modeline (w, dl);
}

#ifdef THIS_IS_X
GC x_get_gc (struct frame *f, Lisp_Object font, Lisp_Object fg,
             Lisp_Object bg, Lisp_Object bg_pixmap,
             Lisp_Object bg_placement, Lisp_Object lwidth);
#endif

static void
XLIKE_output_pixmap (struct window *w, Lisp_Object image_instance,
		     struct display_box *db, struct display_glyph_area *dga,
		     face_index findex, int cursor_start, int cursor_width,
		     int cursor_height, int UNUSED (bg_pixmap))
{
  struct frame *f = XFRAME (w->frame);
  struct device *d = XDEVICE (f->device);
  Lisp_Image_Instance *p = XIMAGE_INSTANCE (image_instance);
#ifdef THIS_IS_X
  XLIKE_DISPLAY dpy = GET_XLIKE_X_DISPLAY (d);
  XLIKE_WINDOW x_win = GET_XLIKE_WINDOW (f);
#endif

  /* Output the pixmap. */
  {
    Lisp_Object tmp_pixel;
    XLIKE_COLOR tmp_bcolor, tmp_fcolor;

    tmp_pixel = WINDOW_FACE_CACHEL_FOREGROUND (w, findex);
    tmp_fcolor = XCOLOR_INSTANCE_XLIKE_COLOR (tmp_pixel);
    tmp_pixel = WINDOW_FACE_CACHEL_BACKGROUND (w, findex);
    tmp_bcolor = XCOLOR_INSTANCE_XLIKE_COLOR (tmp_pixel);

    XLIKE_output_xlike_pixmap (f, p, db->xpos, db->ypos,
			       dga->xoffset, dga->yoffset,
			       dga->width, dga->height,
			       tmp_fcolor, tmp_bcolor);
  }

  /* Draw a cursor over top of the pixmap. */
  if (cursor_width && cursor_height && (cursor_start >= db->xpos)
      && !NILP (w->text_cursor_visible_p)
      && (cursor_start < db->xpos + dga->width))
    {
      int focus = EQ (w->frame, DEVICE_FRAME_WITH_FOCUS_REAL (d));
      struct face_cachel *cursor_cachel =
	WINDOW_FACE_CACHEL (w,
			    get_builtin_face_cache_index
			    (w, Vtext_cursor_face));
      if (cursor_width > db->xpos + dga->width - cursor_start)
	cursor_width = db->xpos + dga->width - cursor_start;

#ifdef THIS_IS_X
      {
        GC gc = x_get_gc (f, Qnil, cursor_cachel->background, Qnil, Qnil,
                          Qnil, Qnil);

	if (focus)
	  {
	    XFillRectangle (dpy, x_win, gc, cursor_start, db->ypos,
                            cursor_width, cursor_height);
	  }
	else
	  {
	    XLIKE_DRAW_RECTANGLE (dpy, x_win, gc, cursor_start, db->ypos,
				  cursor_width, cursor_height);
	  }
      }
#endif
#ifdef THIS_IS_GTK
      {
	GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (f);
	GdkWindow *window = gtk_widget_get_window (widget);
#if GTK_CHECK_VERSION(3, 22, 0)
	cairo_region_t *region = gdk_window_get_visible_region (window);
	GdkDrawingContext *ctx = gdk_window_begin_draw_frame (window,
							      region);
        cairo_t *cr = gdk_drawing_context_get_cairo_context (ctx);
#else
	cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (widget));
#endif

	cr_set_foreground (cr, cursor_cachel->background);

	if (focus)
	  {
	    gtk_fill_rectangle (cr, cursor_start, db->ypos,
				cursor_width, cursor_height);
	  }
	else
	  {
	    gtk_draw_rectangle (cr, cursor_start, db->ypos,
				cursor_width, cursor_height);
	  }
#if GTK_CHECK_VERSION(3, 22, 0)
	gdk_window_end_draw_frame (gtk_widget_get_window (widget), ctx);
	cairo_region_destroy (region);
#else
	cairo_destroy (cr);
#endif
      }
#endif
    }
}

static void
XLIKE_clear_frame_window (Lisp_Object window)
{
  struct window *w = XWINDOW (window);

  if (!NILP (w->vchild))
    {
      XLIKE_clear_frame_windows (w->vchild);
      return;
    }

  if (!NILP (w->hchild))
    {
      XLIKE_clear_frame_windows (w->hchild);
      return;
    }

  redisplay_clear_to_window_end (w, WINDOW_TEXT_TOP (w),
				 WINDOW_TEXT_BOTTOM (w));
}

static void
XLIKE_clear_frame_windows (Lisp_Object window)
{
  for (; !NILP (window); window = XWINDOW (window)->next)
    XLIKE_clear_frame_window (window);
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
console_type_create_redisplay_XLIKE (void)
{
  /* redisplay methods */
  XLIKE_CONSOLE_HAS_METHOD (text_width);
  XLIKE_CONSOLE_HAS_METHOD (output_display_block);
  XLIKE_CONSOLE_HAS_METHOD (divider_height);
  XLIKE_CONSOLE_HAS_METHOD (eol_cursor_width);
  XLIKE_CONSOLE_HAS_METHOD (output_vertical_divider);
  XLIKE_CONSOLE_HAS_METHOD (clear_region);
  XLIKE_CONSOLE_HAS_METHOD (clear_frame);
  XLIKE_CONSOLE_HAS_METHOD (flash);
  XLIKE_CONSOLE_HAS_METHOD (ring_bell);
  XLIKE_CONSOLE_HAS_METHOD (bevel_area);
  XLIKE_CONSOLE_HAS_METHOD (output_string);
  XLIKE_CONSOLE_HAS_METHOD (output_pixmap);

#ifdef THIS_IS_X
  XLIKE_CONSOLE_HAS_METHOD (window_output_begin);
  XLIKE_CONSOLE_HAS_METHOD (window_output_end);
#endif
}
