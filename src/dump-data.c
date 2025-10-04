/* Static array to put the dumped data in and its management
   Copyright (C) 2003 Olivier Galibert, 2025 Free Software Foundation

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

/* Mule-ized? Not relevant. */

#include <config.h>
#include "lisp.h"
#include "dump-data.h"

#ifdef __has_embed
#include <stdalign.h>

alignas (16) static CRawbyte dumped_data[] = {
#embed EMACS_DUMP_FILE_NAME
};

size_t
dumped_data_size (void)
{
  return sizeof (dumped_data);
}
#else /* !__has_embed */

#define INCBIN_PREFIX /* Nothing. */
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_OUTPUT_SECTION ".data"
#define INCBIN_ALIGNMENT_INDEX 4 /* 16-byte alignment. */
#define INCBIN_SILENCE_BITCODE_WARNING /* Unlikely to be building XEmacs for an iPhone. */
#include "incbin.h"

INCBIN (dumped, EMACS_DUMP_FILE_NAME);

size_t
dumped_data_size (void)
{
  return dumped_size;
}
#endif /* __has_embed */

Rawbyte *
dumped_data_get (void)
{
  return (Rawbyte *) dumped_data;
}

/* dump-data.c ends here. */
