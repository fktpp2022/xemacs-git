/* Line number cache.
   Copyright (C) 1997 Free Software Foundation, Inc.

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

/* To calculate the line numbers, redisplay must count the newlines
   from a known position.  This used to be BUF_BEGV, but this made the
   line numbering extremely slow for large buffers, because Emacs had
   to rescan the whole buffer at each redisplay.

   To make line numbering efficient, we maintain a buffer-local cache of
   recently used positions and their line numbers.  The cache is implemented
   as a small unordered ring of cache positions.  A cache position is either
   nil or a cons of a buffer position (zero-length extent) and the
   corresponding line number.

   When calculating the line numbers, this cache is consulted if it
   would otherwise take too much time to count the newlines in the
   buffer (see the comment to buffer_line_number().)

   Insertion and deletions that contain/delete newlines invalidate the
   cached positions after the insertion point.  This guarantees
   relatively fast line numbers caching (even in buffers where point
   moves a lot), and low memory usage.  All of this is done only in
   the buffers where the cache is actually initialized -- i.e. where
   line-numbering is on (or there is a vertical scrollbar), and you
   move the point farther than LINE_NUMBER_FAR from the beginning of
   buffer.  In this sense, the cache is lazy -- if you don't use it,
   you don't pay for it.

   NOTE: line-number cache should not be confused with line-start
   cache.  Line-start cache (a part of redisplay) works with the
   display lines, whereas this works with the buffer lines (literally
   counting the newlines).  */

#include <config.h>
#include "lisp.h"
#include "buffer.h"

#include "line-number.h"
#include "extents-impl.h"

/* Size of the ring.  The current code expects this to be a small
   number.  If you make it larger, you should probably optimize the
   code below to keep it sorted.

   As of October 2020, newlines are usually calculated for point and
   for point-max (if the vertical scrollbar is turned on, as it
   usually is if you have a GUI). Other positions frequently,
   predictably accessed are the line number for a screen down or a
   screen up, and the line number for the beginning of the visible
   part of the buffer, which is trivial. No indication for a bigger
   ring size than 8. */
#define LINE_NUMBER_RING_SIZE 8

/* How much traversal has to be exceeded for two points to be
   considered "far" from each other.  When two points are far, this
   cache will be used, and when they are near, the level 1 processor
   cache is likely to be used.

   As of October 2020 with a 2006 Mac Mini and GCC 8.4 -Os -flto
   -momit-leaf-frame-pointer -mfpmath=both -march=native, this
   threshold means performance for the non-LINE_NUMBER_FAR case is
   much the same as performace for the LINE_NUMBER_FAR case. */
#define LINE_NUMBER_FAR 2730

/* How large a string has to be to give up searching it for newlines,
   before change.

   The calls to insert_adjust_line_number_cache are roughly bimodal;
   there are insertions because the user typed something, where the
   string is of length < MAX_ICHAR_LEN, and there are longer
   programmatic insertions, from inferior shells or similar, where
   there is a big cluster of lengths at 1024. The former occasionally
   will have newlines, rarely more than one; the latter will basically
   always have multiple newlines. With the current design it's
   completely appropriate to have this threshold at 256; the only
   question is whether it should be smaller. */
#define LINE_NUMBER_LARGE_STRING 256

/* To be used only when you *know* the cache has been allocated!  */
#define LINE_NUMBER_RING(b) (XCAR ((b)->text->line_number_cache))
#define LINE_NUMBER_BEGV(b) (XCDR ((b)->text->line_number_cache))


/* Initialize the cache.  Cache is (in pseudo-BNF):

   CACHE		= nil | INITIALIZED-CACHE
   INITIALIZED-CACHE	= cons (RING, BEGV-LINE)
   RING			= vector (*RING-ELEMENT)
   RING-ELEMENT		= nil | RING-PAIR
   RING-PAIR		= cons (extent, integer)
   BEGV-LINE		= integer

   Line number cache should never, ever, be visible to Lisp (because
   destructively modifying its elements can cause crashes.)  Debug it
   using debug_print (current_buffer->text->line_number_cache).  */
static void
allocate_line_number_cache (struct buffer *b)
{
  b->text->line_number_cache = Fcons (make_vector (LINE_NUMBER_RING_SIZE, Qnil),
				      Qzero);
  narrow_line_number_cache (b);
}

/* Flag LINE_NUMBER_BEGV (b) as dirty.  Do it only if the line number
   cache is already initialized.  */
void
narrow_line_number_cache (struct buffer *b)
{
  if (NILP (b->text->line_number_cache))
    return;

  if (BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b))
    /* The is the case Fwiden and save_restriction_restore.  Since we
       know the correct value, we can update it now.  */
    LINE_NUMBER_BEGV (b) = Qzero;
  else
    /* Calculating the line number of BUF_BEGV here is a bad idea,
       because there is absolutely no reason to do it before the next
       redisplay.  We simply mark it as dirty instead.  */
    LINE_NUMBER_BEGV (b) = make_fixnum (-1);
}

/* Invalidate the line number cache positions that lie after POS. */
static void
invalidate_line_number_cache (struct buffer *b, Bytebpos pos)
{
  EMACS_INT i, j;
  Lisp_Object *ring = XVECTOR_DATA (LINE_NUMBER_RING (b));
  Memxpos mpos = bytebpos_to_membpos (b, pos);

  for (i = 0; i < LINE_NUMBER_RING_SIZE; i++)
    {
      if (!CONSP (ring[i]))
	break;

      if (extent_start (XEXTENT (XCAR (ring[i]))) >= mpos)
	{
	  /* Delete the extent.  */
	  Fdelete_extent (XCAR (ring[i]));
	  /* ...and shift the ring elements, up to the first nil.  */
	  for (j = i; !NILP (ring[j]) && j < LINE_NUMBER_RING_SIZE - 1; j++)
	    ring[j] = ring[j + 1];
	  ring[j] = Qnil;
	  /* Must recheck position i. */
	  i--;
	}
    }
}

/* If the string to be inserted is short and contains no newlines, do
   nothing. If it is short and contains one newline, adjust the
   positions in the cache to reflect that. If it contains more than
   one newline, or if LENGTH is greater than than
   LINE_NUMBER_LARGE_STRING, invalidate the cache positions after POS
   without prior search.

   This will do nothing if the cache is uninitialized.  */
void
insert_adjust_line_number_cache (struct buffer *b, Bytebpos pos,
                                 const Ibyte *nonreloc, Bytecount length)
{
  if (NILP (b->text->line_number_cache))
    return;

  if (length > LINE_NUMBER_LARGE_STRING)
    {
      invalidate_line_number_cache (b, pos);
      return;
    }

  {
    const Ibyte *newline
      = (const Ibyte *) memchr ((const void *)nonreloc, '\n', length);

    if (!newline)
      {
        return;
      }
          
    INC_IBYTEPTR (newline);

    if (memchr ((const void *) newline, '\n', length - (newline - nonreloc)))
      {
        /* More than one newline, throw up our hands. */
        invalidate_line_number_cache (b, pos);
        return;
      }

    /* If there is only one newline (the commonest case with insertions),
       increment the line numbers following POS. */
    {
      EMACS_INT i;
      Lisp_Object *ring = XVECTOR_DATA (LINE_NUMBER_RING (b));
      Memxpos mpos = bytebpos_to_membpos (b, pos);

      for (i = 0; i < LINE_NUMBER_RING_SIZE; i++)
        {
          if (!CONSP (ring[i]))
            break;

          if (extent_start (XEXTENT (XCAR (ring[i]))) > mpos)
            {
              XSETCDR (ring[i], FIXNUM_PLUS1 (XCDR (ring[i])));
            }
        }
    }
  }
}

/* Invalidate the cache positions after FROM, if the region to be
   deleted contains a newline.  If the region-to-be-deleted is larger
   than LINE_NUMBER_LARGE_STRING, invalidate the cache positions after
   FROM without unconditionally.

   This will do nothing if the cache is uninitialized.  */
void
delete_invalidate_line_number_cache (struct buffer *b, Bytebpos from,
                                     Bytebpos to)
{
  if (NILP (b->text->line_number_cache))
    return;

  if ((to - from) > LINE_NUMBER_LARGE_STRING)
    invalidate_line_number_cache (b, from);
  else
    {
      EMACS_INT shortage;
      byte_scan_buffer (b, '\n', from, to, 1, &shortage, 0);
      if (!shortage)
	invalidate_line_number_cache (b, from);
    }
}

/* Get the nearest known position we know the line number of
   (i.e. BUF_BEGV, and cached positions).  The return position will be
   either closer than BEG, or BEG.  The line of this known position
   will be stored in LINE.

   *LINE should be initialized to the line number of BEG (normally,
   BEG will be BUF_BEGV, and *LINE will be XFIXNUM (LINE_NUMBER_BEGV).
   This will initialize the cache, if necessary.  */
static void
get_nearest_line_number (struct buffer *b, Bytebpos *beg, Bytebpos pos,
			 Charcount *line)
{
  EMACS_INT i;
  Lisp_Object *ring = XVECTOR_DATA (LINE_NUMBER_RING (b));
  Bytecount length = pos - *beg;

  if (length < 0)
    length = -length;

  /* Find the ring entry closest to POS, if it is closer than BEG. */
  for (i = 0; i < LINE_NUMBER_RING_SIZE && CONSP (ring[i]); i++)
    {
      if (extent_detached_p (XEXTENT (XCAR (ring[i]))))
        {
          continue;
        }
      else
        {
          Bytebpos newpos
            = membpos_to_bytebpos (b, extent_start (XEXTENT (XCAR (ring[i]))));
          Bytecount howfar = newpos - pos;
          if (howfar < 0)
            howfar = -howfar;
          if (howfar < length)
            {
              length = howfar;
              *beg = newpos;
              *line = XFIXNUM (XCDR (ring[i]));
            }
        }
    }
}

/* Add a (POS . LINE) pair to the ring, and rotate it. */
static void
add_position_to_cache (struct buffer *b, Bytebpos pos, Charcount line)
{
  Lisp_Object *ring = XVECTOR_DATA (LINE_NUMBER_RING (b));
  int i = LINE_NUMBER_RING_SIZE - 1;
  struct extent *e;
  Lisp_Object extent;

  if (CONSP (ring[i]))
    {
      /* Re-use the last extent in the ring. */
      extent = XCAR (ring[i]);
      e = XEXTENT (extent);
      ring[i] = Qnil;
    }
  else
    {
      /* Make a fresh extent. */
      extent = Fmake_extent (Qnil, Qnil, Qnil);
      e = XEXTENT (extent);
    }

  /* Make the extent reflect POS. */
  set_extent_endpoints (e, pos, pos, wrap_buffer (b));

  /* Rotate the ring... */
  for (; i > 0; i--)
    ring[i] = ring[i - 1];

  /* And add a fresh entry. */
  ring[0] = Fcons (extent, make_fixnum (line));
}

/* Calculate the line number in buffer B at position POS.  If CACHEP is
   non-zero, initialize and facilitate the line-number cache.  The line number
   of the first line is 0.  If narrowing is in effect, count the lines from
   the beginning of the visible portion of the buffer.

   The cache works as follows: To calculate the line number, we need
   two positions: position of point (POS) and the position from which
   to count newlines (BEG).  We start by setting BEG to BUF_BEGV.  If
   this would require too much searching (i.e. pos - BUF_BEGV >
   LINE_NUMBER_FAR), try to find a closer position in the ring.  If it
   is found, use that position for BEG, and increment the line number
   appropriately.

   If the calculation (with or without the cache lookup) required more
   than LINE_NUMBER_FAR characters of traversal, update the cache.  */
Charcount
buffer_line_number (struct buffer *b, Bytebpos pos, Boolint cachep,
                    Boolint respect_narrowing)
{
  Bytebpos beg = respect_narrowing ? BYTE_BUF_BEGV (b) : BYTE_BUF_BEG (b);
  Charcount cached_lines = 0;
  EMACS_INT shortage, line;

  if ((pos > beg ? pos - beg : beg - pos) <= LINE_NUMBER_FAR)
    cachep = 0;

  if (cachep && (respect_narrowing || BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b)))
    {
      if (NILP (b->text->line_number_cache))
	allocate_line_number_cache (b);
      /* If we don't know the line number of BUF_BEGV, calculate it now.  */
      if (XFIXNUM (LINE_NUMBER_BEGV (b)) == -1)
	{
	  LINE_NUMBER_BEGV (b) = Qzero;
	  /* #### This has a side-effect of changing the cache.  */
	  LINE_NUMBER_BEGV (b) =
	    make_fixnum (buffer_line_number (b, BYTE_BUF_BEGV (b), 1, 0));
	}
      cached_lines = XFIXNUM (LINE_NUMBER_BEGV (b));
      get_nearest_line_number (b, &beg, pos, &cached_lines);
    }

  byte_scan_buffer (b, '\n', beg, pos,
               pos > beg ? MOST_POSITIVE_FIXNUM : -MOST_POSITIVE_FIXNUM,
	       &shortage, 0);

  line = MOST_POSITIVE_FIXNUM - shortage;
  if (beg > pos)
    line = -line;
  line += cached_lines;

  if (cachep && (respect_narrowing || BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b)))
    {
      /* If too far, update the cache. */
      if ((pos > beg ? pos - beg : beg - pos) > LINE_NUMBER_FAR)
	add_position_to_cache (b, pos, line);
      /* Account for narrowing.  If cache is not used, this is
	 unnecessary, because we counted from BUF_BEGV anyway.  */
      line -= XFIXNUM (LINE_NUMBER_BEGV (b));
    }

  return line;
}

/* Calculate and return the Bytebpos of the beginning of LINE_NUMBER, counting
   from zero. If CACHEP is non-zero, initialize and facilitate the line-number
   cache.

   If narrowing is in effect and RESPECT_NARROWING is non-zero, count the
   lines from the beginning of the visible portion of the buffer.

   If the calculation (with or without the cache lookup) required more than
   LINE_NUMBER_FAR characters of traversal, update the cache.

   If there are not LINE_NUMBER lines in the region of interest, return -1. */
Bytebpos
byte_beginning_of_line (struct buffer *b, Charcount line_number,
                        Boolint cachep, Boolint respect_narrowing)
{
  Bytebpos beg = respect_narrowing ? BYTE_BUF_BEGV (b) : BYTE_BUF_BEG (b);
  Bytebpos result, cached_pos = beg;
  Charcount cached_line = 0;
  EMACS_INT shortage = 0;

  if (line_number == 0)
    {
      return respect_narrowing ? BYTE_BUF_BEGV (b) : BYTE_BUF_BEG (b);
    }

  if (cachep && (respect_narrowing || BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b)))
    {
      if (NILP (b->text->line_number_cache))
	allocate_line_number_cache (b);

      /* If we don't know the line number of BUF_BEGV, calculate it now.  */
      if (XFIXNUM (LINE_NUMBER_BEGV (b)) == -1)
	{
	  LINE_NUMBER_BEGV (b) = Qzero;
	  /* #### This has a side-effect of changing the cache.  */
	  LINE_NUMBER_BEGV (b) =
	    make_fixnum (buffer_line_number (b, BYTE_BUF_BEGV (b), 1, 0));
	}

      {
        EMACS_INT ii;
        Lisp_Object *ring = XVECTOR_DATA (LINE_NUMBER_RING (b));
        Charcount closest;

        cached_line = XFIXNUM (LINE_NUMBER_BEGV (b));
        closest = EMACS_INT_ABS (line_number - cached_line);

          /* Find the ring entry closest to LINE_NUMBER. */
        for (ii = 0; ii < LINE_NUMBER_RING_SIZE && CONSP (ring[ii]); ii++)
          {
            if (extent_detached_p (XEXTENT (XCAR (ring[ii]))))
              {
                continue;
              }
            else
              {
                Charcount howfar = XFIXNUM (XCDR (ring[ii])) - line_number;

                if (howfar == 0)
                  {
                    return
                      membpos_to_bytebpos (b,
                                           extent_start
                                           (XEXTENT (XCAR (ring[ii]))));

                  }
                else if (howfar < 0)
                  {
                    howfar = -howfar;
                  }

                if (howfar < closest)
                  {
                    closest = howfar;
                    cached_line = XFIXNUM (XCDR (ring[ii]));
                    cached_pos = 
                      membpos_to_bytebpos (b,
                                           extent_start
                                           (XEXTENT (XCAR (ring[ii]))));
                  }
              }
          }
      }
    }

  if (cached_line < line_number)
    {
      result
        = byte_scan_buffer (b, '\n', cached_pos,
                            respect_narrowing ?
                            BYTE_BUF_ZV (b) : BYTE_BUF_Z (b),
                            line_number - cached_line, &shortage, 0);
      if (shortage == 0)
        {
          if (cachep && (respect_narrowing
                         || BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b)))
            {
              /* If too far, update the cache. */
              if (result - cached_pos > LINE_NUMBER_FAR)
                {
                  add_position_to_cache (b, result, line_number);
                }
            }
          return result;
        }
      return -1;
    }

  result = byte_scan_buffer (b, '\n', cached_pos,
                             respect_narrowing ?
                             BYTE_BUF_BEGV (b) : BYTE_BUF_BEG (b),
                             line_number - cached_line, &shortage, 0);
  if (shortage == 0)
    {
      if (cachep && (respect_narrowing
                     || BYTE_BUF_BEG (b) == BYTE_BUF_BEGV (b)))
        {
          /* If too far, update the cache. */
          if (cached_pos - result > LINE_NUMBER_FAR)
            add_position_to_cache (b, result, line_number);
        }
      return result;
    }

  return -1;
}

/* line-number.c ends here */
