/* Indentation functions.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1985, 1986, 1987, 1988, 1992, 1993, 1994, 1995
   Free Software Foundation, Inc.
   Copyright (C) 2002, 2005 Ben Wing.

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

/* This file has been Mule-ized. */

/* Synched up with: 19.30.  Diverges significantly from FSF. */


#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "device.h"
#include "extents.h"
#include "extents-impl.h"
#include "faces.h"
#include "frame.h"
#include "glyphs.h"
#include "insdel.h"
#ifdef REGION_CACHE_NEEDS_WORK
#include "region-cache.h"
#endif
#include "window.h"

/* Indentation can insert tabs if this is non-zero;
   otherwise always uses spaces */
Boolint indent_tabs_mode;

/* Avoid recalculation by remembering things in these variables. */

/* Last value returned by current_column.

   Some things set last_known_column_point to -1
   to mark the memoized value as invalid */
static Charcount last_known_column;

/* Last buffer searched by current_column */
static struct buffer *last_known_column_buffer;

/* Value of point when current_column was called */
static Bytebpos last_known_column_point;

/* Value of MODIFF when current_column was called */
static EMACS_INT last_known_column_modified;

static Bytebpos
last_visible_position (Bytebpos pos, struct buffer *buf)
{
  Bytebpos value = next_previous_single_property_change (pos, Qinvisible,
                                                         wrap_buffer (buf),
                                                         -1, 0, 0);
  Lisp_Object extent;

  if (value < BYTE_BUF_BEG (buf))
    return 0; /* no visible position found */

  /* Old comment:

     [ bug bug bug!!! This will return the position of the beginning of an
       invisible extent; this extent is very likely to be start-closed, and
       thus the spaces inserted in `indent-to' will go inside the invisible
       extent.

       Not sure what the correct solution is here.  Rethink indent-to?]

     The correct solution is likely what I have implemented here, which is to
     return VALUE when the corresponding extent is start-open, to return the
     position before VALUE when it is not and that position exists within the
     buffer, and to return zero otherwise.

     The vast, vast majority of the time there are no invisible extents and
     this function is never called. There is no significant performance impact
     from this extra extent_at () call.

     Aidan Kehoe, Fr  4 Nov 2022 20:40:45 GMT */
  extent = extent_at (value, wrap_buffer (buf), Qinvisible, 0,
                      EXTENT_AT_BEFORE, 0);

  if (extent_start_open_p (XEXTENT (extent)))
    {
      return value;
    }

  if (value > BYTE_BUF_BEG (buf))
    {
      return prev_bytebpos (buf, value);
    }

  return 0;
}

#ifdef REGION_CACHE_NEEDS_WORK

/* Allocate or free the width run cache, as requested by the current
   state of current_buffer's cache_long_line_scans variable.  */
static void
width_run_cache_on_off (struct buffer *buf)
{
  if (NILP (buf->cache_long_line_scans))
    {
      /* It should be off.  */
      if (buf->width_run_cache)
        {
          free_region_cache (buf->width_run_cache);
          buf->width_run_cache = 0;
          buf->width_table = Qnil;
        }
    }
  else
    {
      /* It should be on.  */
      if (buf->width_run_cache == 0)
        {
          buf->width_run_cache = new_region_cache ();
          recompute_width_table (buf, buffer_display_table ());
        }
    }
}

#endif /* REGION_CACHE_NEEDS_WORK */


/* Cancel any recorded value of the horizontal position.  */

void
invalidate_current_column (void)
{
  last_known_column_point = -1;
}

Charcount
column_at_point (struct buffer *buf, Bytebpos init_pos, Charcount cur_col)
{
  Charcount col;
  Charcount tab_seen;
  Charcount tab_width = XFIXNUM (buf->tab_width);
  Charcount post_tab;
  Bytebpos pos = init_pos;
  Ichar c;

  if (tab_width <= 0 || tab_width > 1000) tab_width = 8;
  col = tab_seen = post_tab = 0;

  while (1)
    {
      if (pos <= BYTE_BUF_BEGV (buf))
	break;

      DEC_BYTEBPOS (buf, pos);
      c = BYTE_BUF_FETCH_CHAR (buf, pos);
      if (c == '\t')
	{
	  if (tab_seen)
	    col = ((col + tab_width) / tab_width) * tab_width;

	  post_tab += col;
	  col = 0;
	  tab_seen = 1;
	}
      else if (c == '\n' ||
	       (EQ (buf->selective_display, Qt) && c == '\r'))
	break;
      else
	{
	  /* #### This needs updating to handle the new redisplay. */
	  /* #### FSFmacs looks at ctl_arrow, display tables.
	     We need to do similar. */
#if 0
	  displayed_glyphs = glyphs_from_bytebpos (sel_frame, buf,
						 XWINDOW (selected_window),
						 pos, dp, 0, col, 0, 0, 0);
	  col += (displayed_glyphs->columns
		  - (displayed_glyphs->begin_columns
		     + displayed_glyphs->end_columns));
#else /* XEmacs */
	  col += ichar_columns (c);
#endif /* XEmacs */
	}
    }

  if (tab_seen)
    {
      col = ((col + tab_width) / tab_width) * tab_width;
      col += post_tab;
    }

  if (cur_col)
    {
      last_known_column_buffer = buf;
      last_known_column = col;
      last_known_column_point = init_pos;
      last_known_column_modified = BUF_MODIFF (buf);
    }

  return col;
}

Charcount
string_column_at_point (Lisp_Object s, Bytecount init_pos, Charcount tab_width)
{
  Charcount col;
  Charcount tab_seen;
  Charcount post_tab;
  Bytecount pos = init_pos;
  Ichar c;

  if (tab_width <= 0 || tab_width > 1000) tab_width = 8;
  col = tab_seen = post_tab = 0;

  while (1)
    {
      if (pos <= 0)
	break;

      pos = prev_string_index (s, pos);
      c = itext_ichar (string_byte_addr (s, pos));
      if (c == '\t')
	{
	  if (tab_seen)
	    col = ((col + tab_width) / tab_width) * tab_width;

	  post_tab += col;
	  col = 0;
	  tab_seen = 1;
	}
      else if (c == '\n')
	break;
      else
	col += ichar_columns (c);
    }

  if (tab_seen)
    {
      col = ((col + tab_width) / tab_width) * tab_width;
      col += post_tab;
    }

  return col;
}

Charcount
current_column (struct buffer *buf)
{
  if (buf == last_known_column_buffer
      && BYTE_BUF_PT (buf) == last_known_column_point
      && BUF_MODIFF (buf) == last_known_column_modified)
    return last_known_column;

  return column_at_point (buf, BYTE_BUF_PT (buf), 1);
}

DEFUN ("current-column", Fcurrent_column, 0, 1, 0, /*
Return the horizontal position of point.  Beginning of line is column 0.
This is calculated by adding together the widths of all the displayed
 representations of the character between the start of the previous line
 and point. (e.g. control characters will have a width of 2 or 4, tabs
 will have a variable width.)
Ignores finite width of frame, which means that this function may return
 values greater than (frame-width).
Whether the line is visible (if `selective-display' is t) has no effect;
 however, ^M is treated as end of line when `selective-display' is t.
If BUFFER is nil, the current buffer is assumed.
*/
       (buffer))
{
  return make_fixnum (current_column (decode_buffer (buffer, 0)));
}


DEFUN ("indent-to", Findent_to, 1, 3, "NIndent to column: ", /*
Indent from point with tabs and spaces until COLUMN is reached.
Optional second argument MINIMUM says always do at least MINIMUM spaces
 even if that goes past COLUMN; by default, MINIMUM is zero.
If BUFFER is nil, the current buffer is assumed.
*/
       (column, minimum, buffer))
{
  /* This function can GC */
  Charcount mincol;
  Charcount fromcol;
  struct buffer *buf = decode_buffer (buffer, 0);
  Charcount tab_width = XFIXNUM (buf->tab_width);
  Bytebpos opoint = 0;
  Charbpos ocpoint;

  CHECK_FIXNUM (column);
  if (NILP (minimum))
    minimum = Qzero;
  else
    CHECK_FIXNUM (minimum);

  buffer = wrap_buffer (buf);

  fromcol = current_column (buf);
  mincol = fromcol + XFIXNUM (minimum);
  if (mincol < XFIXNUM (column)) mincol = XFIXNUM (column);

  if (fromcol == mincol)
    return make_fixnum (mincol);

  if (tab_width <= 0 || tab_width > 1000) tab_width = 8;

  if (!NILP (extent_at (BYTE_BUF_PT (buf), buffer, Qinvisible, NULL,
                        EXTENT_AT_AFTER, 0)))
    {
      Bytebpos last_visible = last_visible_position (BYTE_BUF_PT (buf), buf);

      opoint = BYTE_BUF_PT (buf);
      ocpoint = BUF_PT (buf);
      if (last_visible >= BYTE_BUF_BEGV (buf))
	BYTE_BUF_SET_PT (buf, last_visible);
      else
        invalid_operation ("Visible portion of buffer not modifiable",
                           Qunbound);
    }

  if (indent_tabs_mode)
    {
      Charcount n = mincol / tab_width - fromcol / tab_width;
      if (n != 0)
	{
	  Finsert_char (make_char ('\t'), make_fixnum (n), Qnil, buffer);

	  fromcol = (mincol / tab_width) * tab_width;
	}
    }

  Finsert_char (make_char (' '), make_fixnum (mincol - fromcol), Qnil, buffer);

  last_known_column_buffer = buf;
  last_known_column = mincol;
  last_known_column_point = BYTE_BUF_PT (buf);
  last_known_column_modified = BUF_MODIFF (buf);

  /* Not in FSF: */
  if (opoint > 0)
    BOTH_BUF_SET_PT (buf, ocpoint, opoint);

  return make_fixnum (mincol);
}

Charcount
byte_spaces_at_point (struct buffer *b, Bytebpos byte_pos)
{
  Bytebpos byte_end = BYTE_BUF_ZV (b);
  Charcount col = 0;
  Ichar c;
  Charcount tab_width = XFIXNUM (b->tab_width);

  if (tab_width <= 0 || tab_width > 1000)
    tab_width = 8;

  while (byte_pos < byte_end &&
	 (c = BYTE_BUF_FETCH_CHAR (b, byte_pos),
	  (c == '\t'
	   ? (col += tab_width - col % tab_width)
	   : (c == ' ' ? ++col : 0))))
    INC_BYTEBPOS (b, byte_pos);

  return col;
}


DEFUN ("current-indentation", Fcurrent_indentation, 0, 1, 0, /*
Return the indentation of the current line.
This is the horizontal position of the character
following any initial whitespace.
*/
       (buffer))
{
  struct buffer *buf = decode_buffer (buffer, 0);
  Bytebpos pos = byte_find_next_newline (buf, BYTE_BUF_PT (buf), -1);

  buffer = wrap_buffer (buf);

  if (!NILP (extent_at (pos, buffer, Qinvisible, NULL, EXTENT_AT_AFTER, 0)))
    return Qzero;

  return make_fixnum (byte_spaces_at_point (buf, pos));
}


DEFUN ("move-to-column", Fmove_to_column, 1, 3, 0, /*
Move point to column COLUMN in the current line.
The column of a character is calculated by adding together the widths
as displayed of the previous characters in the line.
This function ignores line-continuation;
there is no upper limit on the column number a character can have
and horizontal scrolling has no effect.

If specified column is within a character, point goes after that character.
If it's past end of line, point goes to end of line.

A value of `coerce' for the second (optional) argument FORCE means if
COLUMN is in the middle of a tab character, change it to spaces.
Any other non-nil value means the same, plus if the line is too short to
reach column COLUMN, then add spaces/tabs to get there.

Returns the actual column that it moved to.
*/
       (column, force, buffer))
{
  /* This function can GC */
  Bytebpos pos, end;
  Charbpos cpos;
  struct buffer *buf = decode_buffer (buffer, 0);
  Charcount col = current_column (buf), prev_col = 0, goal;
  Charcount tab_width = XFIXNUM (buf->tab_width);
  Ichar c = 0;

  buffer = wrap_buffer (buf);
  if (tab_width <= 0 || tab_width > 1000) tab_width = 8;

  CHECK_NATNUM (column);
  if (BIGNUMP (column))
    {
      goal = 1 + MOST_POSITIVE_FIXNUM;
    }
  else
    {
      goal = XFIXNUM (column);
    }

 retry:
  pos = BYTE_BUF_PT (buf);
  cpos = BUF_PT (buf);
  end = BYTE_BUF_ZV (buf);

  /* If we're starting past the desired column,
     back up to beginning of line and scan from there.  */
  if (col > goal)
    {
      pos = byte_find_next_newline (buf, pos, -1);
      cpos = bytebpos_to_charbpos (buf, pos);
      col = 0;
    }

  while (col < goal && pos < end)
    {
      c = BYTE_BUF_FETCH_CHAR (buf, pos);
      if (c == '\n')
	break;
      if (c == '\r' && EQ (buf->selective_display, Qt))
	break;
      if (c == '\t')
	{
	  prev_col = col;
	  col += tab_width;
	  col = col / tab_width * tab_width;
	}
      else
	{
	  /* #### oh for the days of the complete new redisplay */
	  /* #### FSFmacs looks at ctl_arrow, display tables.
	     We need to do similar. */
#if 0
	  displayed_glyphs = glyphs_from_charbpos (selected_frame (),
						 buf,
						 XWINDOW (Fselected_window (Qnil)),
						 pos, dp, 0, col, 0, 0, 0);
	  col += (displayed_glyphs->columns
		  - (displayed_glyphs->begin_columns
		     + displayed_glyphs->end_columns));
#else /* XEmacs */
	  col += ichar_columns (c);
#endif /* XEmacs */
	}

      INC_BYTEBPOS (buf, pos);
      cpos++;
    }

  BOTH_BUF_SET_PT (buf, cpos, pos);

  /* If a tab char made us overshoot, change it to spaces
     and scan through it again.  */
  if (!NILP (force) && col > goal && c == '\t' && prev_col < goal)
    {
      buffer_delete_range (buf, BUF_PT (buf) - 1, BUF_PT (buf), 0);
      Findent_to (make_fixnum (col - 1), Qzero, buffer);
      buffer_insert_emacs_char (buf, ' ');
      goto retry;
    }

  /* If line ends prematurely, add space to the end.  */
  if (col < goal && !NILP (force) && !EQ (force, Qcoerce))
    {
      col = goal;
      Findent_to (make_fixnum (col), Qzero, buffer);
    }

  last_known_column_buffer = buf;
  last_known_column = col;
  last_known_column_point = BYTE_BUF_PT (buf);
  last_known_column_modified = BUF_MODIFF (buf);

  return make_fixnum (col);
}

/* GNU has compute_motion() and Fcompute_motion() in this file; this is barely
   used by their Lisp code and involves, basically, re-implementing redisplay
   to give much the same information as pixel_to_glyph_translation() does in
   XEmacs. They use compute_motion() to implement the vertical_motion() and
   related functions; we just use the redisplay infrastructure.

   Attempting to implement compute_motion() in XEmacs will give loads of
   difficult-to-shake-out bugs for no particular benefit. */

/* Helper for vmotion_1 - compute vertical pixel motion between
   START and END in the line start cache CACHE.  This just sums
   the line heights, including both the starting and ending lines.
*/
static int
vpix_motion (line_start_cache_dynarr *cache, int start, int end)
{
  int i, vpix;

  assert (start <= end);
  assert (start >= 0);
  assert (end < Dynarr_length (cache));

  vpix = 0;
  for (i = start; i <= end; i++)
    vpix += Dynarr_atp (cache, i)->height;

  return vpix;
}

/*****************************************************************************
 vmotion_1

 Given a starting position ORIG, move point VTARGET lines in WINDOW.
 Returns the new value for point.  If the arg ret_vpos is not NULL, it is
 taken to be a pointer to a Charcount and the number of lines actually moved is
 returned in it.  If the arg ret_vpix is not NULL, it is taken to be a
 pointer to an int and the vertical pixel height of the motion which
 took place is returned in it.
 ****************************************************************************/
static Bytebpos
vmotion_1 (struct window *w, Bytebpos orig, Charcount vtarget,
           Charcount *ret_vpos, int *ret_vpix)
{
  struct buffer *b = XBUFFER (w->buffer);
  int elt;

  elt = point_in_line_start_cache (w, orig, (vtarget < 0
					     ? -vtarget
					     : vtarget));

  /* #### This assertion must be true before the if statements are hit
     but may possibly be wrong after the call to
     point_in_line_start_cache if orig is outside of the visible
     region of the buffer.  Handle this. */
  assert (elt >= 0);

  /* Moving downward. */
  if (vtarget > 0)
    {
      int cur_line = Dynarr_length (w->line_start_cache) - 1 - elt;
      Bytebpos ret_pt;

      if (cur_line > vtarget)
	cur_line = vtarget;

      /* The traditional FSF behavior is to return the end of buffer
         position if we couldn't move far enough because we hit it.  */
      if (cur_line < vtarget)
	ret_pt = BYTE_BUF_ZV (b);
      else
	ret_pt = Dynarr_atp (w->line_start_cache, cur_line + elt)->start;

      while (ret_pt > BYTE_BUF_ZV (b) && cur_line > 0)
	{
	  cur_line--;
	  ret_pt = Dynarr_atp (w->line_start_cache, cur_line + elt)->start;
	}

      if (ret_vpos) *ret_vpos = cur_line;
      if (ret_vpix)
        *ret_vpix = vpix_motion (w->line_start_cache, elt, cur_line + elt);
      return ret_pt;
    }
  else if (vtarget < 0)
    {
      if (elt < -vtarget)
	{
	  if (ret_vpos) *ret_vpos = -elt;
          if (ret_vpix)
            *ret_vpix = vpix_motion (w->line_start_cache, 0, elt);
	  /* #### This should be BYTE_BUF_BEGV (b), right? */
	  return Dynarr_begin (w->line_start_cache)->start;
	}
      else
	{
	  if (ret_vpos) *ret_vpos = vtarget;
          if (ret_vpix)
            *ret_vpix = vpix_motion (w->line_start_cache, elt + vtarget, elt);
	  return Dynarr_atp (w->line_start_cache, elt + vtarget)->start;
	}
    }
  else
    {
      /* No vertical motion requested so we just return the position
         of the beginning of the current line. */
      if (ret_vpos) *ret_vpos = 0;
      if (ret_vpix)
        *ret_vpix = vpix_motion (w->line_start_cache, elt, elt);

      return Dynarr_atp (w->line_start_cache, elt)->start;
    }

  RETURN_NOT_REACHED(0);	/* shut up compiler */
}

/*****************************************************************************
 vmotion

 Given a starting position ORIG, move point VTARGET lines in WINDOW.
 Returns the new value for point.  If the arg ret_vpos is not NULL, it is
 taken to be a pointer to a Charcount and the number of lines actually moved
 is returned in it.
 ****************************************************************************/
Bytebpos
vmotion (struct window *w, Bytebpos orig, Charcount vtarget,
         Charcount *ret_vpos)
{
  if (!redisplayable_window_p (w))
    {
      if (ret_vpos)
	{
	  *ret_vpos = 0;
	}
      return orig;
    }

  return vmotion_1 (w, orig, vtarget, ret_vpos, NULL);
}

/* Helper for Fvertical_motion. */
static Lisp_Object
vertical_motion_1 (Lisp_Object lines, Lisp_Object window,
                   Boolint pixels)
{
  Bytebpos bpos, orig;
  Boolint selected = 0;
  Charcount *vpos;
  int *vpix;
  Charcount value = 0;
  int pixvalue = 0;
  struct window *w;

  if (NILP (window))
    {
      window = Fselected_window (Qnil);
      selected = 1;
    }

  CHECK_LIVE_WINDOW (window);
  CHECK_FIXNUM (lines);

  selected = selected || EQ (window, Fselected_window (Qnil));

  w = XWINDOW (window);

  if (!redisplayable_window_p (w))
    {
      return Qzero;
    }

  orig = selected ? BYTE_BUF_PT (XBUFFER (w->buffer))
                  : marker_byte_position (w->pointm[CURRENT_DISP]);

  vpos = pixels ? NULL   : &value;
  vpix = pixels ? &pixvalue : NULL;

  bpos = vmotion_1 (w, orig, XFIXNUM (lines), vpos, vpix);

  /* Note that the buffer's point is set, not the window's point. */
  if (selected)
    BYTE_BUF_SET_PT (XBUFFER (w->buffer), bpos);
  else
    set_marker_byte_position_restricted (w->pointm[CURRENT_DISP],
                                         bpos, w->buffer);

  if (pixels)
    {
      return make_fixnum (pixvalue);
    }

  return make_fixnum (value);
}

DEFUN ("vertical-motion", Fvertical_motion, 1, 3, 0, /*
Move to start of frame line LINES lines down.
If LINES is negative, this is moving up.
Optional second argument is WINDOW to move in,
the default is the selected window.

Sets point to position found; this may be start of line
or just the start of a continuation line.
If optional third argument PIXELS is nil, returns number
of lines moved; may be closer to zero than LINES if beginning
or end of buffer was reached.  If PIXELS is non-nil, the
vertical pixel height of the motion which took place is
returned instead of the actual number of lines moved.  A
motion of zero lines returns the height of the current line.

NOTE NOTE NOTE: GNU Emacs/XEmacs difference.

What `vertical-motion' actually does is set WINDOW's buffer's point
if WINDOW is the selected window; else, it sets WINDOW's point.
This is unfortunately somewhat tricky to work with, and different
from GNU Emacs, which always uses the current buffer, not WINDOW's
buffer, always sets current buffer's point, and, from the
perspective of this function, temporarily makes WINDOW display
the current buffer if it wasn't already.
*/
       (lines, window, pixels))
{
  return vertical_motion_1 (lines, window, !NILP (pixels));
}

/*
 * Like vmotion() but requested and returned movement is in pixels.
 * HOW specifies the stopping condition.  Positive means move at least
 * PIXELS.  Negative means at most.  Zero means as close as possible.
 */
Bytebpos
vmotion_pixels (Lisp_Object window, Bytebpos start, int pixels, int how,
                int *motion)
{
  struct window *w;
  Bytebpos eobuf, bobuf;
  int defheight;
  int needed;
  int line, next;
  int remain, abspix, dirn;
  int elt, nelt;
  int i;
  line_start_cache_dynarr *cache;
  int previous = -1;
  int lines;

  if (NILP (window))
    window = Fselected_window (Qnil);

  CHECK_LIVE_WINDOW (window);
  w = XWINDOW (window);

  eobuf = BYTE_BUF_ZV (XBUFFER (w->buffer));
  bobuf = BYTE_BUF_BEGV (XBUFFER (w->buffer));

  default_face_width_and_height (window, NULL, &defheight);

  /* guess num lines needed in line start cache + a few extra */
  abspix = abs (pixels);
  needed = (abspix + defheight-1)/defheight + 3;

  dirn = (pixels >= 0) ? 1 : -1;

  while (1)
    {
      elt = point_in_line_start_cache (w, start, needed);
      assert (elt >= 0); /* in the cache */

      cache = w->line_start_cache;
      nelt  = Dynarr_length (cache);

      *motion = 0;

      if (pixels == 0)
        /* No vertical motion requested so we just return the position
           of the beginning of the current display line. */
        return Dynarr_atp (cache, elt)->start;

      if ((dirn < 0 && elt == 0      &&
           Dynarr_atp (cache, elt)->start <= bobuf) ||
          (dirn > 0 && elt == nelt-1 &&
           Dynarr_atp (cache, elt)->end   >= eobuf))
        return Dynarr_atp (cache, elt)->start;

      remain = abspix;
      for (i = elt; (dirn > 0) ? (i < nelt) : (i > 0); i += dirn)
        {
          /* cache line we're considering moving over */
          int ii = (dirn > 0) ? i : i-1;

          if (remain < 0)
            return Dynarr_atp (cache, i)->start;

          line = Dynarr_atp (cache, ii)->height;
          next = remain - line;

          /* is stopping condition satisfied? */
          if ((how >  0  &&  remain <= 0)  ||       /* at least */
              (how <  0  &&  next < 0)     ||       /* at most */
              (how == 0  &&  remain <= abs (next))) /* closest */
            return Dynarr_atp (cache, i)->start;

          /* moving down and nowhere left to go? */
          if (dirn > 0 && Dynarr_atp (cache, ii)->end >= eobuf)
            return Dynarr_atp (cache, ii)->start;

          /* take the step */
          remain   = next;
          *motion += dirn * line;

          /* moving up and nowhere left to go? */
          if (dirn < 0 && Dynarr_atp (cache, ii)->start <= bobuf)
            return Dynarr_atp (cache, ii)->start;
        }

      /* get here => need more cache lines.  try again. */
      assert (abs (*motion) > previous); /* progress? */
      previous = abs (*motion);

      lines   = (pixels < 0) ? elt : (nelt - elt);
      needed += (remain*lines + abspix-1)/abspix + 3;
    }

  RETURN_NOT_REACHED(0);	/* shut up compiler */
}

DEFUN ("vertical-motion-pixels", Fvertical_motion_pixels, 1, 3, 0, /*
Move to start of frame line PIXELS vertical pixels down.
If PIXELS is negative, this is moving up.
The actual vertical motion in pixels is returned.

Optional second argument is WINDOW to move in,
the default is the selected window.

Optional third argument HOW specifies when to stop.  A value
less than zero indicates that the motion should be no more
than PIXELS.  A value greater than zero indicates that the
motion should be at least PIXELS.  Any other value indicates
that the motion should be as close as possible to PIXELS.
*/
       (pixels, window, how))
{
  Bytebpos bpos, orig;
  Boolint selected;
  int motion;
  int howto = 0;
  struct window *w;

  if (NILP (window))
    window = Fselected_window (Qnil);

  CHECK_LIVE_WINDOW (window);
  CHECK_FIXNUM (pixels);

  selected = (EQ (window, Fselected_window (Qnil)));

  w = XWINDOW (window);

  orig = selected ? BYTE_BUF_PT (XBUFFER (w->buffer))
                  : marker_byte_position (w->pointm[CURRENT_DISP]);

  if (INTEGERP (how))
    {
      if (FIXNUMP (how))
	{
	  howto = XFIXNUM (how) < 0 ? -1 : (XFIXNUM (how) > 0);
	}
#ifdef HAVE_BIGNUM
      else
	{
	  howto = bignum_sign (XBIGNUM_DATA (how));
	}
#endif
    }

  bpos = vmotion_pixels (window, orig, XFIXNUM (pixels), howto, &motion);

  if (selected)
    BYTE_BUF_SET_PT (XBUFFER (w->buffer), bpos);
  else
    set_window_point (w->pointm[CURRENT_DISP], bpos);

  return make_fixnum (motion);
}


void
syms_of_indent (void)
{
  DEFSUBR (Fcurrent_indentation);
  DEFSUBR (Findent_to);
  DEFSUBR (Fcurrent_column);
  DEFSUBR (Fmove_to_column);
  DEFSUBR (Fvertical_motion);
  DEFSUBR (Fvertical_motion_pixels);
}

void
vars_of_indent (void)
{
  DEFVAR_BOOL ("indent-tabs-mode", &indent_tabs_mode /*
*Indentation can insert tabs if this is non-nil.
Setting this variable automatically makes it local to the current buffer.
*/ );
  indent_tabs_mode = 1;
}
