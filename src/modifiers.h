/* Include file for iterating over all modifiers.
   Copyright (C) 2023 Free Software Foundation, Inc.

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

/* Synched up with: Not in FSF.  Split out of events.h, events.c, keymap.c. */

/* This file provides code that deals with event modifiers structured access
   to the modifier names (in upper case and lower case) and the associated
   enumeration values (enum event_modifiers in events.h.) This would all be
   fairly trival but for the fact that mouse-motion events can have chords of
   mouse buttons associated with them.

   The implementation currently ignores button modifiers for keysym events.

   To use this file, define FROB_MODIFIER to do something with the modifier.
   No need to undefine, it happens automatically at the end of this file.  If
   you want button 0 included, define INCLUDE_BUTTON_ZERO (also undefined
   automatically). */

FROB_MODIFIER (CONTROL, control, (1<<0))
FROB_MODIFIER (META,    meta,    (1<<1))
FROB_MODIFIER (SUPER,   super,   (1<<2))
FROB_MODIFIER (HYPER,   hyper,   (1<<3))
FROB_MODIFIER (ALT,     alt,     (1<<4))
FROB_MODIFIER (SHIFT,   shift,   (1<<5))

#define FROB(num) FROB_MODIFIER (BUTTON##num, button##num, (1 << (num + 5)))
#include "keymap-buttons.h"

#undef FROB_MODIFIER
