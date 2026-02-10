/* Sound functions.
   Copyright (C) 2000 Free Software Foundation, Inc.
   Copyright (C) 2001 Ben Wing.

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

/* This file Mule-ized by Ben Wing, 5-15-01. */

#include "buffer.h"

/* Defined in *play.c */
void play_sound_file (Extbyte *name, int volume);
void nt_play_sound_file (Lisp_Object name, int volume);
int play_sound_data (Binbyte *data, int length, int volume);

#ifdef HAVE_ALSA_SOUND
extern int alsa_play_sound_file (const Extbyte *file, int vol);
extern int alsa_play_sound_data (const Binbyte *data, int length, int vol);
# define DEVICE_CONNECTED_TO_ALSA_P(x) 1 /* #### better check */
#endif

#ifdef HAVE_ESD_SOUND
extern int esd_play_sound_file (Extbyte *file, int vol);
extern int esd_play_sound_data (Binbyte *data, size_t length, int vol);
# define DEVICE_CONNECTED_TO_ESD_P(x) 1 /* #### better check */
#endif

#ifdef HAVE_NAS_SOUND
extern int nas_play_sound_file (Extbyte *name, int volume);
extern int nas_play_sound_data (Binbyte *data, int length, int volume);
extern void nas_wait_for_sounds (void);
/* If --with-sound=nas is specified, configure warns and removes NAS support
   if X11 is unavailable, this is fine. */
# include <X11/Xlib.h>
extern Extbyte *nas_init_play (Display *);
#endif

# define sound_perror(string)						 \
do {									 \
  Ibyte *errmess;							 \
  Ibyte *string_int;							 \
  GET_STRERROR (errmess, errno);					 \
  string_int = EXTERNAL_TO_ITEXT (string, Qerror_message_encoding);	 \
  warn_when_safe (Qsound, Qerror, "audio: %s, %s", string_int, errmess); \
} while (0)

# define sound_warn(string) warn_when_safe (Qsound, Qwarning, "audio: %s", \
                                            string)

/* sound.h ends here */
