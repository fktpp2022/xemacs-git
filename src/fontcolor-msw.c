/* mswindows-specific Lisp objects.
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995 Tinker Systems.
   Copyright (C) 1995, 1996, 2000, 2001, 2002, 2004, 2005, 2010 Ben Wing.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 1997 Jonathan Harris.

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

/* Authorship:

   This file created by Jonathan Harris, November 1997 for 21.0; based
   heavily on fontcolor-x.c (see authorship there).  Much further work
   by Ben Wing.
 */

/* This function Mule-ized by Ben Wing, 3-24-02. */

/* TODO: palette handling */

#include <config.h>
#include "lisp.h"

#include "console-msw-impl.h"
#include "fontcolor-msw-impl.h"

#include "buffer.h"
#include "charset.h"
#include "device-impl.h"
#include "elhash.h"
#include "insdel.h"
#include "opaque.h"



typedef struct fontmap_t
{
  const Ascbyte *name;
  int value;
} fontmap_t;

/* Default weight first, preferred names listed before synonyms */
static const fontmap_t fontweight_map[] =
{
  {"Regular"		, FW_REGULAR},	/* The standard font weight */
  {"Thin"		, FW_THIN},
  {"Extra Light"	, FW_EXTRALIGHT},
  {"Ultra Light"	, FW_ULTRALIGHT},
  {"Light"		, FW_LIGHT},
  {"Normal"		, FW_NORMAL},
  {"Medium"		, FW_MEDIUM},
  {"Semi Bold"		, FW_SEMIBOLD},
  {"Demi Bold"		, FW_DEMIBOLD},
  {"Bold"		, FW_BOLD},	/* The standard bold font weight */
  {"Extra Bold"		, FW_EXTRABOLD},
  {"Ultra Bold"		, FW_ULTRABOLD},
  {"Heavy"		, FW_HEAVY},
  {"Black"		, FW_BLACK}
};

/* Default charset must be listed first, no synonyms allowed because these
 * names are matched against the names reported by win32 by match_font() */
static const fontmap_t charset_map[] =
{
  {"Western"		, ANSI_CHARSET}, /* Latin 1 */
  {"Central European"	, EASTEUROPE_CHARSET},
  {"Cyrillic"		, RUSSIAN_CHARSET},
  {"Greek"		, GREEK_CHARSET},
  {"Turkish"		, TURKISH_CHARSET},
  {"Hebrew"		, HEBREW_CHARSET},
  {"Arabic"		, ARABIC_CHARSET},
  {"Baltic"		, BALTIC_CHARSET},
  {"Viet Nam"		, VIETNAMESE_CHARSET},
  {"Thai"		, THAI_CHARSET},
  {"Japanese"		, SHIFTJIS_CHARSET},
  {"Korean"		, HANGEUL_CHARSET},
  {"Simplified Chinese"	, GB2312_CHARSET},
  {"Traditional Chinese", CHINESEBIG5_CHARSET},

  {"Symbol"		, SYMBOL_CHARSET},
  {"Mac"		, MAC_CHARSET},
  {"Korean Johab"	, JOHAB_CHARSET},
  {"OEM/DOS"		, OEM_CHARSET}
};

/* This table comes from MSDN, Unicode Subset Bitfields [Platform SDK
   Documentation, Base Services, International Features, Unicode and
   Character Sets, Unicode and Character Set Reference, Unicode and
   Character Set Constants]. */

#define FOR_ALL_UNICODE_FONT_SUBRANGES(FROB, FROB_LAST)         \
  FROB (0, 0x0020, 0x007e)   /* Basic Latin */                  \
  FROB (1, 0x00a0, 0x00ff)   /* Latin-1 Supplement */           \
  FROB (2, 0x0100, 0x017f)   /* Latin Extended-A */             \
  FROB (3, 0x0180, 0x024f)   /* Latin Extended-B */             \
  FROB (4, 0x0250, 0x02af,   /* IPA Extensions */               \
           0x1d00, 0x1d7f,   /* Phonetic Extensions */          \
           0x1d80, 0x1dbf)   /* Phonetic Extensions             \
                                Supplement */                   \
  FROB (5, 0x02b0, 0x02ff,   /* Spacing Modifier Letters */     \
           0xa700, 0xa71f)   /* Modifier Tone Letters */        \
  FROB (6, 0x0300, 0x036f,   /* Combining Diacritical Marks */  \
           0x1dc0, 0x1dff)   /* Combining Diacritical Marks     \
                                Supplement */                   \
  FROB (7, 0x0370, 0x03ff)   /* Greek and Coptic */             \
  FROB (8, 0x2c80, 0x2cff)   /* Coptic */                       \
  FROB (9, 0x0400, 0x04ff,   /* Cyrillic */                     \
           0x0500, 0x052F,   /* Cyrillic Supplement */          \
           0x2de0, 0x2DFF,   /* Cyrillic Extended-A */          \
           0xa640, 0xA69F)   /* Cyrillic Extended-B */          \
  FROB (10, 0x0530, 0x058f)  /* Armenian */                     \
  FROB (11, 0x0590, 0x05ff)  /* Basic Hebrew */                 \
  FROB (12, 0xa500, 0xa63f)  /* Vai */                          \
  FROB (13, 0x0600, 0x06ff,  /* Basic Arabic */                 \
            0x0750, 0x077f)  /* Arabic Supplement */            \
  FROB (14, 0x07c0, 0x07ff)  /* NKo */                          \
  FROB (15, 0x0900, 0x097f)  /* Devanagari */                   \
  FROB (16, 0x0980, 0x09ff)  /* Bangla */                       \
  FROB (17, 0x0a00, 0x0a7f)  /* Gurmukhi */                     \
  FROB (18, 0x0a80, 0x0aff)  /* Gujarati */                     \
  FROB (19, 0x0b00, 0x0b7f)  /* Odia */                         \
  FROB (20, 0x0b80, 0x0bff)  /* Tamil */                        \
  FROB (21, 0x0c00, 0x0c7f)  /* Telugu */                       \
  FROB (22, 0x0c80, 0x0cff)  /* Kannada */                      \
  FROB (23, 0x0d00, 0x0d7f)  /* Malayalam */                    \
  FROB (24, 0x0e00, 0x0e7f)  /* Thai */                         \
  FROB (25, 0x0e80, 0x0eff)  /* Lao */                          \
  FROB (26, 0x10a0, 0x10ff,  /* Georgian */                     \
            0x2d00, 0x2d2f)  /* Georgian Supplement */          \
  FROB (27, 0x1b00, 0x1b7f)  /* Balinese */                     \
  FROB (28, 0x1100, 0x11ff)  /* Hangul Jamo */                  \
  FROB (29, 0x1e00, 0x1eff,  /* Latin Extended Additional */    \
            0x2c60, 0x2C7F,  /* Latin Extended-C */             \
            0xa720, 0xA7FF)  /* Latin Extended-D */             \
  FROB (30, 0x1f00, 0x1fff)  /* Greek Extended */               \
  FROB (31, 0x2000, 0x206f,  /* General Punctuation */          \
            0x2e00, 0x2e7f)  /* Supplemental Punctuation */     \
  FROB (32, 0x2070, 0x209f)  /* Subscripts and Superscripts */  \
  FROB (33, 0x20a0, 0x20cf)  /* Currency Symbols */             \
  FROB (34, 0x20d0, 0x20ff)  /* Combining Diacritical Marks     \
                                for Symbols */                  \
  FROB (35, 0x2100, 0x214f)  /* Letter-like Symbols */          \
  FROB (36, 0x2150, 0x218f)  /* Number Forms */                 \
  FROB (37, 0x2190, 0x21ff,  /* Arrows */                       \
            0x27f0, 0x27ff,  /* Supplemental Arrows-A */        \
            0x2900, 0x297f,  /* Supplemental Arrows-B */        \
            0x2b00, 0x2bff)  /* Miscellaneous Symbols and       \
                                Arrows */                       \
  FROB (38, 0x2200, 0x22ff,  /* Mathematical Operators */       \
            0x27c0, 0x27ef,  /* Miscellaneous Mathematical      \
                                Symbols-A */                    \
            0x2980, 0x29ff,  /* Miscellaneous Mathematical      \
                                Symbols-B */                    \
            0x2a00, 0x2aff)  /* Supplemental Mathematical       \
                                Operators */                    \
  FROB (39, 0x2300, 0x23ff)  /* Miscellaneous Technical */      \
  FROB (40, 0x2400, 0x243f)  /* Control Pictures */             \
  FROB (41, 0x2440, 0x245f)  /* Optical Character Recognition */\
  FROB (42, 0x2460, 0x24ff)  /* Enclosed Alphanumerics */       \
  FROB (43, 0x2500, 0x257f)  /* Box Drawing */                  \
  FROB (44, 0x2580, 0x259f)  /* Block Elements */               \
  FROB (45, 0x25a0, 0x25ff)  /* Geometric Shapes */             \
  FROB (46, 0x2600, 0x26ff)  /* Miscellaneous Symbols */        \
  FROB (47, 0x2700, 0x27bf)  /* Dingbats */                     \
  FROB (48, 0x3000, 0x303f)  /* Chinese, Japanese, and Korean   \
                                (CJK) Symbols and Punctuation */\
  FROB (49, 0x3040, 0x309f)  /* Hiragana */                     \
  FROB (50, 0x30a0, 0x30ff,  /* Katakana */                     \
            0x31f0, 0x31ff)  /* Katakana Phonetic Extensions */ \
  FROB (51, 0x3100, 0x312f,  /* Bopomofo */                     \
            0x31a0, 0x31bf)  /* Extended Bopomofo */            \
  FROB (52, 0x3130, 0x318f)  /* Hangul Compatibility Jamo */    \
  FROB (53, 0xA840, 0xA87F)  /* Phags-pa */                     \
  FROB (54, 0x3200, 0x32ff)  /* Enclosed CJK Letters and        \
                                Months */                       \
  FROB (55, 0x3300, 0x33ff)  /* CJK Compatibility */            \
  FROB (56, 0xac00, 0xd7a3)  /* Hangul Syllables */             \
  FROB (57, 0xd800, 0xdfff)  /* Surrogates. Note that setting   \
                                this bit implies that there is  \
                                at least one codepoint beyond   \
                                the Basic Multilingual Plane    \
                                that is supported by this       \
                                font.  */                       \
  FROB (58, 0x10900, 0x1091F)/* Phoenician */                   \
  FROB (59, 0x2e80, 0x2eff,  /* CJK Radicals Supplement */      \
            0x2f00, 0x2fdf,  /* Kangxi Radicals */              \
            0x2ff0, 0x2fff,  /* Ideographic Description         \
                                Characters */                   \
            0x3190, 0x319f,  /* Kanbun */                       \
            0x3400, 0x4dbf,  /* CJK Unified Ideographs          \
                                Extension A */                  \
            0x4e00, 0x9fff,  /* CJK Unified Ideographs */       \
            0x20000, 0x2a6df) /* CJK Unified Ideographs         \
                                 Extension B */                 \
  FROB (60, 0xe000, 0xf8ff)  /* Private Use Area */             \
  FROB (61, 0x31c0, 0x31ef,  /* CJK Strokes */                  \
            0xf900, 0xfaff,  /* CJK Compatibility Ideographs */ \
            0x2f800, 0x2fa1f) /* CJK Compatibility Ideographs   \
                                 Supplement */                  \
  FROB (62, 0xfb00, 0xfb4f)  /* Alphabetic Presentation Forms */\
  FROB (63, 0xfb50, 0xfdff)  /* Arabic Presentation Forms-A */  \
  FROB (64, 0xfe20, 0xfe2f)  /* Combining Half Marks */         \
  FROB (65, 0xfe10, 0xfe1f,  /* Vertical Forms */               \
            0xfe30, 0xfe4f)  /* CJK Compatibility Forms */      \
  FROB (66, 0xfe50, 0xfe6f)  /* Small Form Variants */          \
  FROB (67, 0xfe70, 0xfeff)  /* Arabic Presentation Forms-B */  \
  FROB (68, 0xff00, 0xffef)  /* Halfwidth and Fullwidth Forms */\
  FROB (69, 0xfff0, 0xfffd)  /* Specials */                     \
  FROB (70, 0x0f00, 0x0fff)  /* Tibetan */                      \
  FROB (71, 0x0700, 0x074f)  /* Syriac */                       \
  FROB (72, 0x0780, 0x07bf)  /* Thaana */                       \
  FROB (73, 0x0d80, 0x0dff)  /* Sinhala */                      \
  FROB (74, 0x1000, 0x109f)  /* Myanmar */                      \
  FROB (75, 0x1200, 0x137f,  /* Ethiopic */                     \
            0x1380, 0x139f,  /* Ethiopic Supplement */          \
            0x2d80, 0x2ddf)  /* Ethiopic Extended */            \
  FROB (76, 0x13a0, 0x13ff)  /* Cherokee */                     \
  FROB (77, 0x1400, 0x167f)  /* Unified Canadian Aboriginal     \
                                Syllabics */                    \
  FROB (78, 0x1680, 0x169f)  /* Ogham */                        \
  FROB (79, 0x16a0, 0x16ff)  /* Runic */                        \
  FROB (80, 0x1780, 0x17ff,  /* Khmer */                        \
            0x19e0, 0x19ff)  /* Khmer Symbols */                \
  FROB (81, 0x1800, 0x18af)  /* Mongolian */                    \
  FROB (82, 0x2800, 0x28ff)  /* Braille */                      \
  FROB (83, 0xa000, 0xa48c,  /* Yi Syllables */                 \
            0xa490, 0xa4cf)  /* Yi Radicals */                  \
  FROB (84, 0x1700, 0x171f,  /* Tagalog */                      \
            0x1720, 0x173f,  /* Hanunoo */                      \
            0x1740, 0x175f,  /* Buhid */                        \
            0x1760, 0x177f)  /* Tagbanwa */                     \
  FROB (85, 0x10300, 0x1032f)  /* Old Italic */                 \
  FROB (86, 0x10330, 0x1034f)  /* Gothic */                     \
  FROB (87, 0x10400, 0x1044f)  /* Deseret */                    \
  FROB (88, 0x1d000, 0x1d0ff,  /* Byzantine Musical Symbols */  \
            0x1d100, 0x1d1ff,  /* Musical Symbols */            \
            0x1d200, 0x1d24f)  /* Ancient Greek Musical         \
                                  Notation */                   \
  FROB (89, 0x1d400, 0x1d7ff)  /* Mathematical Alphanumeric     \
                                  Symbols */                    \
  FROB (90, 0xff000, 0xffffd,  /* Private Use (plane 15) */     \
            0x100000, 0x10fffd)/* Private Use (plane 16) */     \
  FROB (91, 0xfe00, 0xfe0f,    /* Variation Selectors */        \
            0xe0100, 0xe01ef)  /* Variation Selectors           \
                                  Supplement */                 \
  FROB (92, 0xe0000, 0xe007f)  /* Tags */                       \
  FROB (93, 0x1900, 0x194f)    /* Limbu */                      \
  FROB (94, 0x1950, 0x197f)    /* Tai Le */                     \
  FROB (95, 0x1980, 0x19df)    /* New Tai Lue */                \
  FROB (96, 0x1a00, 0x1a1f)    /* Buginese */                   \
  FROB (97, 0x2c00, 0x2c5f)    /* Glagolitic */                 \
  FROB (98, 0x2d30, 0x2d7f)    /* Tifinagh */                   \
  FROB (99, 0x4dc0, 0x4dff)    /* Yijing Hexagram Symbols */    \
  FROB (100, 0xa800, 0xa82f)   /* Syloti Nagri */               \
  FROB (101, 0x10000, 0x1007f, /* Linear B Syllabary */         \
             0x10080, 0x100FF, /* Linear B Ideograms */         \
             0x10100, 0x1013F) /* Aegean Numbers */             \
  FROB (102, 0x10140, 0x1018f) /* Ancient Greek Numbers */      \
  FROB (103, 0x10380, 0x1039f) /* Ugaritic */                   \
  FROB (104, 0x103a0, 0x103df) /* Old Persian */                \
  FROB (105, 0x10450, 0x1047f) /* Shavian */                    \
  FROB (106, 0x10480, 0x104af) /* Osmanya */                    \
  FROB (107, 0x10800, 0x1083f) /* Cypriot Syllabary */          \
  FROB (108, 0x10a00, 0x10a5f) /* Kharoshthi */                 \
  FROB (109, 0x1d300, 0x1d35f) /* Tai Xuan Jing Symbols */      \
  FROB (110, 0x12000, 0x123ff, /* Cuneiform */                  \
             0x12400, 0x1247F) /* Cuneiform Numbers and         \
                                  Punctuation */                \
  FROB (111, 0x1d360, 0x1d37f) /* Counting Rod Numerals */      \
  FROB (112, 0x1b80, 0x1bbf)   /* Sundanese */                  \
  FROB (113, 0x1c00, 0x1c4f)   /* Lepcha */                     \
  FROB (114, 0x1c50, 0x1c7f)   /* Ol Chiki */                   \
  FROB (115, 0xa880, 0xa8df)   /* Saurashtra */                 \
  FROB (116, 0xa900, 0xa92f)   /* Kayah Li */                   \
  FROB (117, 0xa930, 0xa95f)   /* Rejang */                     \
  FROB (118, 0xaa00, 0xaa5f)   /* Cham */                       \
  FROB (119, 0x10190, 0x101cf) /* Ancient Symbols */            \
  FROB (120, 0x101d0, 0x101ff) /* Phaistos Disc */              \
  FROB (121, 0x10280, 0x1029f, /* Lycian */                     \
             0x102a0, 0x102df, /* Carian */                     \
             0x10920, 0x1093f) /* Lydian */                     \
  FROB_LAST (122, 0x1f000, 0x1f02f, /* Mahjong Tiles */         \
                  0x1f030, 0x1f09f) /* Domino Tiles */
  /* 123 Windows 2000 and later: Layout progress: horizontal from right to
     left */
  /* 124 Windows 2000 and later: Layout progress: vertical before
     horizontal */
  /* 125 Windows 2000 and later: Layout progress: vertical bottom to top */
  /* 126-127	Reserved for process-internal usage */

typedef struct unicode_subrange_raw_t
{
  const int start; /* first Unicode codepoint */
  const int end; /* last Unicode codepoint */
} unicode_subrange_raw_t;

#define SUBFROB_RAW_SUBRANGE_TWO(first, last) { first, last }
#define SUBFROB_RAW_SUBRANGE_NOTTWO(first, last, ...)                   \
  { first, last }, PREPROCESSOR_EXPAND                                  \
    (PREPROCESSOR_CONCATENATE (SUBFROB_RAW_SUBRANGE_,                   \
                               PREPROCESSOR_TWO_OR_NOTTWO(__VA_ARGS__)) \
     (__VA_ARGS__)

#define FROB_RAW_SUBRANGE_ONE(N) /* Nothing */
#define FROB_RAW_SUBRANGE_NOTONE(N, ...)                                    \
  static const struct unicode_subrange_raw_t unicode_subrange_raw_##N[] = { \
    PREPROCESSOR_EXPAND (PREPROCESSOR_CONCATENATE                           \
                         (SUBFROB_RAW_SUBRANGE_,                            \
                          PREPROCESSOR_TWO_OR_NOTTWO(__VA_ARGS__))          \
                         (__VA_ARGS__))                                     \
  };

#define FROB_RAW_SUBRANGE(...)                                          \
  PREPROCESSOR_EXPAND (PREPROCESSOR_CONCATENATE                         \
                       (FROB_RAW_SUBRANGE_,                             \
                        PREPROCESSOR_ONE_OR_NOTONE (__VA_ARGS__))       \
                       (__VA_ARGS__))

/* Initialize the subrange arrays. */
FOR_ALL_UNICODE_FONT_SUBRANGES(FROB_RAW_SUBRANGE, FROB_RAW_SUBRANGE)

typedef struct unicode_subrange_t
{
  const int no_subranges;
  const unicode_subrange_raw_t *subranges;
} unicode_subrange_t;

#define FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY_ONE(N) { 0, NULL }

#define FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY_NOTONE(N, ...) \
 { countof (unicode_subrange_raw_##N), unicode_subrange_raw_##N }

#define FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY(...)                     \
  PREPROCESSOR_EXPAND (PREPROCESSOR_CONCATENATE                         \
                       (FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY_,        \
                        PREPROCESSOR_ONE_OR_NOTONE(__VA_ARGS__))        \
                       (__VA_ARGS__))

#define FROB_UNICODE_SUBRANGE_TABLE_ENTRY(...) \
  FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY (__VA_ARGS__) ,

/* Now initialize the general subrange table, which points at all the subrange
   arrays and gives counts for each of them. */
static const unicode_subrange_t unicode_subrange_table[] = {
  FOR_ALL_UNICODE_FONT_SUBRANGES (FROB_UNICODE_SUBRANGE_TABLE_ENTRY,
                                  FROB_LAST_UNICODE_SUBRANGE_TABLE_ENTRY)
};

/* Hash table mapping font specs (strings) to font signature data
   (FONTSIGNATURE structures stored in opaques), as determined by
   GetTextCharsetInfo().  I presume this is somewhat expensive because it
   involves creating a font object.  At the very least, with no hashing, it
   definitely took awhile (a few seconds) when encountering characters from
   charsets needing stage 2 processing. */
Lisp_Object Vfont_signature_data;

/************************************************************************/
/*                               helpers                                */
/************************************************************************/
COLORREF
mswindows_string_to_color (const Ibyte *name)
{
  Rgbref color = shared_X_string_to_color (name);
  if (rgb_valid_p (color))
    return PALETTERGB (rgb_red (color),
		       rgb_green (color),
		       rgb_blue (color));
  else
    return (COLORREF) -1;
}

Lisp_Object
mswindows_color_to_string (COLORREF color)
{
  Rgbref pcolor = rgb_pack (GetRValue (color), GetGValue (color),
				GetBValue (color));

  return shared_X_color_to_string (pcolor);
}

/*
 * Returns non-zero if the two supplied font patterns match.
 * If they match and fontname is not NULL, copies the logical OR of the
 * patterns to fontname (which is assumed to be at least MSW_FONTSIZE in size).
 *
 * The patterns 'match' iff for each field that is not blank in either pattern,
 * the corresponding field in the other pattern is either identical or blank.
 */
static int
match_font (Ibyte *pattern1, Ibyte *pattern2,
	    Ibyte *fontname)
{
  Ibyte *c1 = pattern1, *c2 = pattern2, *e1 = 0, *e2 = 0;
  int i;

  if (fontname)
    fontname[0] = '\0';

  for (i = 0; i < 5; i++)
    {
      if (c1 && (e1 = qxestrchr (c1, ':')))
        *(e1) = '\0';
      if (c2 && (e2 = qxestrchr (c2, ':')))
        *(e2) = '\0';

      if (c1 && c1[0] != '\0')
        {
	  if (c2 && c2[0] != '\0' && qxestrcasecmp (c1, c2))
	    {
	      if (e1) *e1 = ':';
	      if (e2) *e2 = ':';
	      return 0;
	    }
	  else if (fontname)
	    qxestrcat_ascii (qxestrcat (fontname, c1), ":");
	}
      else if (fontname)
        {
	  if (c2 && c2[0] != '\0')
	    qxestrcat_ascii (qxestrcat (fontname, c2), ":");
	  else
	    qxestrcat_ascii (fontname, ":");
	}

      if (e1) *(e1++) = ':';
      if (e2) *(e2++) = ':';
      c1 = e1;
      c2 = e2;
    }

  if (fontname)
    fontname[qxestrlen (fontname) - 1] = '\0';	/* Trim trailing ':' */
  return 1;
}


/************************************************************************/
/*                                 exports                              */
/************************************************************************/

struct font_enum_t
{
  HDC hdc;
  Lisp_Object list;
};

static int CALLBACK
font_enum_callback_2 (ENUMLOGFONTEXW *lpelfe, NEWTEXTMETRICEXW *lpntme,
		      int FontType, struct font_enum_t *font_enum)
{
  Ibyte fontname[MSW_FONTSIZE * 2 * MAX_ICHAR_LEN]; /* should be enough :)*/
  Lisp_Object fontname_lispstr;
  int i;
  Ibyte *facename;

  /*
   * The enumerated font weights are not to be trusted because:
   *  a) lpelfe->elfStyle is only filled in for TrueType fonts.
   *  b) Not all Bold and Italic styles of all fonts (including some Vector,
   *     Truetype and Raster fonts) are enumerated.
   * I guess that fonts for which Bold and Italic styles are generated
   * 'on-the-fly' are not enumerated. It would be overly restrictive to
   * disallow Bold And Italic weights for these fonts, so we just leave
   * weights unspecified. This means that we have to weed out duplicates of
   * those fonts that do get enumerated with different weights.
   */
  facename = TSTR_TO_ITEXT (lpelfe->elfLogFont.lfFaceName);
  if (itext_ichar (facename) == '@')
    /* This is a font for writing vertically. We ignore it. */
    return 1;

  if (FontType == 0 /*vector*/ || FontType & TRUETYPE_FONTTYPE)
    /* Scalable, so leave pointsize blank */
    emacs_snprintf (fontname, sizeof (fontname), "%s::::", facename);
  else
    /* Formula for pointsize->height from LOGFONT docs in Platform SDK */
    emacs_snprintf (fontname, sizeof (fontname), "%s::%d::", facename,
		MulDiv (lpntme->ntmTm.tmHeight -
			lpntme->ntmTm.tmInternalLeading,
			72, GetDeviceCaps (font_enum->hdc, LOGPIXELSY)));

  /*
   * The enumerated font character set strings are not to be trusted because
   * lpelfe->elfScript is returned in the host language and not in English.
   * We can't know a priori the translations of "Western", "Central European"
   * etc into the host language, so we must use English. The same argument
   * applies to the font weight string when matching fonts.
   */
  for (i = 0; i < countof (charset_map); i++)
    if (lpelfe->elfLogFont.lfCharSet == charset_map[i].value)
      {
	qxestrcat_ascii (fontname, charset_map[i].name);
	break;
      }
  if (i == countof (charset_map))
    return 1;

  /* Add the font name to the list if not already there */
  fontname_lispstr = build_istring (fontname);
  if (NILP (assoc_no_quit (fontname_lispstr, font_enum->list)))
    font_enum->list =
      Fcons (Fcons (fontname_lispstr,
		    /* TMPF_FIXED_PITCH is backwards from what you expect!
		       If set, it means NOT fixed pitch. */
		    (lpntme->ntmTm.tmPitchAndFamily & TMPF_FIXED_PITCH) ?
		    Qnil : Qt),
	     font_enum->list);

  return 1;
}

static int CALLBACK
font_enum_callback_1 (ENUMLOGFONTEXW *lpelfe,
		      NEWTEXTMETRICEXW *UNUSED (lpntme),
		      int UNUSED (FontType), struct font_enum_t *font_enum)
{
  /* This function gets called once per facename per character set.
   * We call a second callback to enumerate the fonts in each facename */
  return qxeEnumFontFamiliesEx (font_enum->hdc, &lpelfe->elfLogFont,
				(FONTENUMPROCW) font_enum_callback_2,
				(LPARAM) font_enum, 0);
}

/* Function for sorting lists of fonts as obtained from
   mswindows_enumerate_fonts().  These come in a known format:
   "family::::charset" for TrueType fonts, "family::size::charset"
   otherwise. */

static Boolint
sort_font_list_function (Lisp_Object UNUSED (pred), Lisp_Object UNUSED (key),
			 Lisp_Object obj1, Lisp_Object obj2)
{
  Ibyte *font1, *font2;
  Ibyte *c1, *c2;
  int t1, t2;

  /*
    1. fixed over proportional.
    2. Western over other charsets.
    3. TrueType over non-TrueType.
    4. Within non-TrueType, sizes closer to 10pt over sizes farther from 10pt.
    5. Courier New over other families.
  */

  /* The sort function should return non-zero if OBJ1 < OBJ2, zero
     otherwise. */

  t1 = !NILP (XCDR (obj1));
  t2 = !NILP (XCDR (obj2));

  if (t1 && !t2)
    return 1;
  if (t2 && !t1)
    return 0;

  font1 = XSTRING_DATA (XCAR (obj1));
  font2 = XSTRING_DATA (XCAR (obj2));

  c1 = qxestrrchr (font1, ':');
  c2 = qxestrrchr (font2, ':');

  t1 = !qxestrcasecmp_ascii (c1 + 1, "western");
  t2 = !qxestrcasecmp_ascii (c2 + 1, "western");

  if (t1 && !t2)
    return 1;
  if (t2 && !t1)
    return 0;

  c1 -= 2;
  c2 -= 2;
  t1 = *c1 == ':';
  t2 = *c2 == ':';

  if (t1 && !t2)
    return 1;
  if (t2 && !t1)
    return 0;

  if (!t1 && !t2)
    {
      while (isdigit (*c1))
	c1--;
      while (isdigit (*c2))
	c2--;

      t1 = qxeatoi (c1 + 1) - 10;
      t2 = qxeatoi (c2 + 1) - 10;

      if (abs (t1) < abs (t2))
	return 1;
      else if (abs (t2) < abs (t1))
	return 0;
      else if (t1 < t2)
	/* Prefer a smaller font over a larger one just as far away
	   because the smaller one won't upset the total line height if it's
	   just a few chars. */
	return 1;
    }

  t1 = !qxestrncasecmp_ascii (font1, "courier new:", 12);
  t2 = !qxestrncasecmp_ascii (font2, "courier new:", 12);

  if (t1 && !t2)
    return 1;
  if (t2 && !t1)
    return 0;

  return 0;
}

/*
 * Enumerate the available on the HDC fonts and return a list of string
 * font names.
 */
Lisp_Object
mswindows_enumerate_fonts (HDC hdc)
{
  /* This cannot GC */
  LOGFONTW logfont;
  struct font_enum_t font_enum;

  assert (hdc != NULL);
  logfont.lfCharSet = DEFAULT_CHARSET;
  logfont.lfFaceName[0] = '\0';
  logfont.lfPitchAndFamily = DEFAULT_PITCH;
  font_enum.hdc = hdc;
  font_enum.list = Qnil;
  /* EnumFontFamilies seems to enumerate only one charset per font, which
     is not what we want.  We aren't supporting NT 3.5x, so no need to
     worry about this not existing. */
  qxeEnumFontFamiliesEx (hdc, &logfont, (FONTENUMPROCW) font_enum_callback_1,
			 (LPARAM) (&font_enum), 0);

  return list_sort (font_enum.list, sort_font_list_function, Qnil, Qidentity);
}

static HFONT
mswindows_create_font_variant (Lisp_Font_Instance *f,
			       int under, int strike)
{
  /* Cannot GC */
  LOGFONTW lf;
  HFONT hfont;

  assert (FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, under, strike) == NULL);

  if (qxeGetObject (FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, 0, 0),
		    sizeof (lf), (void *) &lf) == 0)
    {
      hfont = MSWINDOWS_BAD_HFONT;
    }
  else
    {
      lf.lfUnderline = under;
      lf.lfStrikeOut = strike;

      hfont = qxeCreateFontIndirect (&lf);
      if (hfont == NULL)
	hfont = MSWINDOWS_BAD_HFONT;
    }

  FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, under, strike) = hfont;
  return hfont;
}

HFONT
mswindows_get_hfont (Lisp_Font_Instance *f,
		     int under, int strike)
{
  /* Cannot GC */
  HFONT hfont = FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, under, strike);

  if (hfont == NULL)
    hfont = mswindows_create_font_variant (f, under, strike);

  /* If strikeout/underline variant of the font could not be
     created, then use the base version of the font */
  if (hfont == MSWINDOWS_BAD_HFONT)
    hfont = FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, 0, 0);

  assert (hfont != NULL && hfont != MSWINDOWS_BAD_HFONT);

  return hfont;
}

/************************************************************************/
/*                               methods                                */
/************************************************************************/

static int
mswindows_initialize_color_instance (Lisp_Color_Instance *c, Lisp_Object name,
				     Lisp_Object UNUSED (device),
				     Error_Behavior errb)
{
  COLORREF color;

  color = mswindows_string_to_color (XSTRING_DATA (name));
  if (color != (COLORREF) -1)
    {
      c->data = xnew (struct mswindows_color_instance_data);
      COLOR_INSTANCE_MSWINDOWS_COLOR (c) = color;
      return 1;
    }
  maybe_signal_error (Qinvalid_constant,
		      "Unrecognized color", name, Qcolor, errb);
  return(0);
}

static void
mswindows_print_color_instance (Lisp_Color_Instance *c,
				Lisp_Object printcharfun,
				int UNUSED (escapeflag))
{
  COLORREF color = COLOR_INSTANCE_MSWINDOWS_COLOR (c);
  write_fmt_string (printcharfun,
		    " %06d=(%04X,%04X,%04X)", (int) (color & 0xffffff),
		    GetRValue (color) * 257, GetGValue (color) * 257,
		    GetBValue (color) * 257);
}

static void
mswindows_finalize_color_instance (Lisp_Color_Instance *c)
{
  if (c->data)
    {
      xfree (c->data);
      c->data = 0;
    }
}

static int
mswindows_color_instance_equal (Lisp_Color_Instance *c1,
				Lisp_Color_Instance *c2,
				int UNUSED (depth))
{
  return (COLOR_INSTANCE_MSWINDOWS_COLOR (c1) ==
	  COLOR_INSTANCE_MSWINDOWS_COLOR (c2));
}

static Hashcode
mswindows_color_instance_hash (Lisp_Color_Instance *c, int UNUSED (depth))
{
  return (unsigned long) COLOR_INSTANCE_MSWINDOWS_COLOR (c);
}

static Lisp_Object
mswindows_color_instance_rgb_components (Lisp_Color_Instance *c)
{
  COLORREF color = COLOR_INSTANCE_MSWINDOWS_COLOR (c);
  return list3 (make_fixnum (GetRValue (color) * 257),
		make_fixnum (GetGValue (color) * 257),
		make_fixnum (GetBValue (color) * 257));
}

static int
mswindows_valid_color_name_p (struct device *UNUSED (d), Lisp_Object color)
{
  return (mswindows_string_to_color (XSTRING_DATA (color)) != (COLORREF) -1);
}



static void
mswindows_finalize_font_instance (Lisp_Font_Instance *f);

/* Parse the font spec in NAMESTR.  Maybe issue errors, according to ERRB;
   NAME_FOR_ERRORS is the Lisp string to use when issuing errors.  Store
   the five parts of the font spec into the given strings, which should be
   declared as

   Ibyte fontname[LF_FACESIZE], weight[LF_FACESIZE], points[8];
   Ibyte effects[LF_FACESIZE], charset[LF_FACESIZE];

   If LOGFONT is given, store the necessary information in LOGFONT to
   create a font object.  If LOGFONT is given, HDC must also be given;
   else, NULL can be given for both.
   
   Return 1 if ok, 0 if error.
   */
static int
parse_font_spec (const Ibyte *namestr,
		 HDC hdc,
		 Lisp_Object name_for_errors,
		 Error_Behavior errb,
		 LOGFONTW *logfont,
		 Ibyte *fontname,
		 Ibyte *weight,
		 Ibyte *points,
		 Ibyte *effects,
		 Ibyte *charset)
{
  int fields, i;
  int pt;
  Ibyte *style;
  Ibyte *c;

  /*
   * mswindows fonts look like:
   *	fontname[:[weight ][style][:pointsize[:effects]]][:charset]
   * The font name field shouldn't be empty.
   *
   * ie:
   *	Lucida Console:Regular:10
   * minimal:
   *	Courier New
   * maximal:
   *	Courier New:Bold Italic:10:underline strikeout:western
   */

  fontname[0] = 0;
  weight[0] = 0;
  points[0] = 0;
  effects[0] = 0;
  charset[0] = 0;

  if (logfont)
    xzero (*logfont);

  fields = sscanf ((CIbyte *) namestr, "%31[^:]:%31[^:]:%7[^:]:%31[^:]:%31s",
		   fontname, weight, points, effects, charset);

  /* This function is implemented in a fairly ad-hoc manner.
   * The general idea is to validate and canonicalize each of the above fields
   * at the same time as we build up the win32 LOGFONT structure. This enables
   * us to use match_font() on a canonicalized font string to check the
   * availability of the requested font */

  if (fields < 0)
    {
      maybe_signal_error (Qinvalid_argument, "Invalid font", name_for_errors,
			  Qfont, errb);
      return 0;
    }

  if (fields > 0 && qxestrlen (fontname))
    {
      Extbyte *extfontname;

      extfontname = ITEXT_TO_TSTR (fontname);
      if (logfont)
	{
          qxetcsncpy ((Extbyte *) logfont->lfFaceName, extfontname,
	             LF_FACESIZE - 1);
	  logfont->lfFaceName[LF_FACESIZE - 1] = 0;
	}
    }

  /* weight */
  if (fields < 2)
    qxestrcpy_ascii (weight, fontweight_map[0].name);

  /* Maybe split weight into weight and style */
  if ((c = qxestrchr (weight, ' ')))
    {
      *c = '\0';
      style = c + 1;
    }
  else
    style = NULL;

  for (i = 0; i < countof (fontweight_map); i++)
    if (!qxestrcasecmp_ascii (weight, fontweight_map[i].name))
      {
	if (logfont)
	  logfont->lfWeight = fontweight_map[i].value;
	break;
      }
  if (i == countof (fontweight_map))	/* No matching weight */
    {
      if (!style)
	{
	  if (logfont)
	    logfont->lfWeight = FW_REGULAR;
	  style = weight;	/* May have specified style without weight */
	}
      else
	{
	  maybe_signal_error (Qinvalid_constant, "Invalid font weight",
			      name_for_errors, Qfont, errb);
	  return 0;
	}
    }

  if (style)
    {
      /* #### what about oblique? */
      if (qxestrcasecmp_ascii (style, "italic") == 0)
	{
	  if (logfont)
	    logfont->lfItalic = TRUE;
	}
      else
	{
	  maybe_signal_error (Qinvalid_constant,
			      "Invalid font weight or style",
			      name_for_errors, Qfont, errb);
	  return 0;
      }

      /* Glue weight and style together again */
      if (weight != style)
        *c = ' ';
    }
  else if (logfont)
    logfont->lfItalic = FALSE;

  if (fields < 3 || !qxestrcmp_ascii (points, ""))
    ;
  else if (points[0] == '0' ||
	   qxestrspn (points, "0123456789") < qxestrlen (points))
    {
      maybe_signal_error (Qinvalid_argument, "Invalid font pointsize",
			  name_for_errors, Qfont, errb);
      return 0;
    }
  else
    {
      pt = qxeatoi (points);

      if (logfont)
	{
	  /* Formula for pointsize->height from LOGFONT docs in MSVC5 Platform
	     SDK */
	  logfont->lfHeight = -MulDiv (pt, GetDeviceCaps (hdc, LOGPIXELSY),
				       72);
	  logfont->lfWidth = 0;
	}
    }

  /* Effects */
  if (logfont)
    {
      logfont->lfUnderline = FALSE;
      logfont->lfStrikeOut = FALSE;
    }

  if (fields >= 4 && effects[0] != '\0')
    {
      Ibyte *effects2;
      int underline = FALSE, strikeout = FALSE;

      /* Maybe split effects into effects and effects2 */
      if ((c = qxestrchr (effects, ' ')))
        {
          *c = '\0';
          effects2 = c + 1;
        }
      else
        effects2 = NULL;

      if (qxestrcasecmp_ascii (effects, "underline") == 0)
	underline = TRUE;
      else if (qxestrcasecmp_ascii (effects, "strikeout") == 0)
	strikeout = TRUE;
      else
        {
          maybe_signal_error (Qinvalid_constant, "Invalid font effect",
			      name_for_errors, Qfont, errb);
	  return 0;
	}

      if (effects2 && effects2[0] != '\0')
	{
	  if (qxestrcasecmp_ascii (effects2, "underline") == 0)
	    underline = TRUE;
	  else if (qxestrcasecmp_ascii (effects2, "strikeout") == 0)
	    strikeout = TRUE;
	  else
	    {
	      maybe_signal_error (Qinvalid_constant, "Invalid font effect",
				  name_for_errors, Qfont, errb);
	      return 0;
	    }
        }

      /* Regenerate sanitized effects string */
      if (underline)
	{
	  if (strikeout)
	    qxestrcpy_ascii (effects, "underline strikeout");
	  else
	    qxestrcpy_ascii (effects, "underline");
	}
      else if (strikeout)
	qxestrcpy_ascii (effects, "strikeout");

      if (logfont)
	{
	  logfont->lfUnderline = underline;
	  logfont->lfStrikeOut = strikeout;
	}
    }

  /* Charset */
  /* charset can be specified even if earlier fields haven't been */
  if (fields < 5)
    {
      if ((c = qxestrchr (namestr, ':')) && (c = qxestrchr (c + 1, ':')) &&
	  (c = qxestrchr (c + 1, ':')) && (c = qxestrchr (c + 1, ':')))
	{
	  qxestrncpy (charset, c + 1, LF_FACESIZE);
	  charset[LF_FACESIZE - 1] = '\0';
	}
    }

  /* NOTE: If you give a blank charset spec, we will normally not get here
     under Mule unless we explicitly call `make-font-instance'!  This is
     because the C code instantiates fonts using particular charsets, by
     way of specifier_matching_instance().  Before instantiating the font,
     font_instantiate() calls the devmeth find_matching_font(), which gets
     a truename font spec with the registry (i.e. the charset spec) filled
     in appropriately to the charset. */
  if (!qxestrcmp_ascii (charset, ""))
    ;
  else
    {
      for (i = 0; i < countof (charset_map); i++)
	if (!qxestrcasecmp_ascii (charset, charset_map[i].name))
	  {
	    if (logfont)
	      logfont->lfCharSet = charset_map[i].value;
	    break;
	  }

      if (i == countof (charset_map))	/* No matching charset */
	{
	  maybe_signal_error (Qinvalid_argument, "Invalid charset",
			      name_for_errors, Qfont, errb);
	  return 0;
	}
    }

  if (logfont)
    {
      /* Misc crud */
#if 1
      logfont->lfOutPrecision = OUT_DEFAULT_PRECIS;
      logfont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
      logfont->lfQuality = DEFAULT_QUALITY;
#else
      logfont->lfOutPrecision = OUT_STROKE_PRECIS;
      logfont->lfClipPrecision = CLIP_STROKE_PRECIS;
      logfont->lfQuality = PROOF_QUALITY;
#endif
      /* Default to monospaced if the specified fontname doesn't exist. */
      logfont->lfPitchAndFamily = FF_MODERN;
    }

  return 1;
}

/*
  mswindows fonts look like:
  	[fontname[:style[:pointsize[:effects]]]][:charset]
   A maximal mswindows font spec looks like:
  	Courier New:Bold Italic:10:underline strikeout:Western

  A missing weight/style field is the same as Regular, and a missing
  effects field is left alone, and means no effects; but a missing
  fontname, pointsize or charset field means any will do.  We prefer
  Courier New, 10, Western.  See sort function above. */

static HFONT
create_hfont_from_font_spec (const Ibyte *namestr,
			     HDC hdc,
			     Lisp_Object name_for_errors,
			     Lisp_Object device_font_list,
			     Error_Behavior errb,
			     Lisp_Object *truename_ret)
{
  LOGFONTW logfont;
  HFONT hfont;
  Ibyte fontname[LF_FACESIZE], weight[LF_FACESIZE], points[8];
  Ibyte effects[LF_FACESIZE], charset[LF_FACESIZE];
  Ibyte truename[MSW_FONTSIZE];
  Ibyte truername[MSW_FONTSIZE];

  /* Windows will silently substitute a default font if the fontname
     specifies a non-existent font.  This is bad for screen fonts because
     it doesn't allow higher-level code to see the error and to act
     appropriately.  For instance complex_vars_of_faces() sets up a
     fallback list of fonts for the default face.  Instead, we look at all
     the possibilities and pick one that works, handling missing pointsize
     and charset fields appropriately.

     For printer fonts, we used to go ahead and let Windows choose the
     font, and for those devices, then, DEVICE_FONT_LIST would be nil.
     However, this causes problems with the font-matching code below, which
     needs a list of fonts so it can pick the right one for Mule.

     Thus, the code below to handle a nil DEVICE_FONT_LIST is not currently
     used. */

  if (!NILP (device_font_list))
    {
      Lisp_Object fonttail = Qnil;

      if (!parse_font_spec (namestr, 0, name_for_errors,
			    errb, 0, fontname, weight, points,
			    effects, charset))
	return 0;

      /* The fonts in the device font list always specify fontname and
	 charset, but often times not the size; so if we don't have the
	 size specified either, do a round with size 10 so we'll always end
	 up with a size in the truename (if we fail this one but succeed
	 the next one, we'll have chosen a non-TrueType font, and in those
	 cases the size is specified in the font list item. */

      if (!points[0])
	{
          emacs_snprintf (truename, sizeof (truename), "%s:%s:10:%s:%s",
                          fontname, weight, effects, charset);

	  LIST_LOOP (fonttail, device_font_list)
	    {
	      if (match_font (XSTRING_DATA (XCAR (XCAR (fonttail))),
			      truename, truername))
		break;
	    }
	}

      if (NILP (fonttail))
	{
          emacs_snprintf (truename, sizeof (truename), "%s:%s:%s:%s:%s",
                          fontname, weight, points, effects, charset);

	  LIST_LOOP (fonttail, device_font_list)
	    {
	      if (match_font (XSTRING_DATA (XCAR (XCAR (fonttail))),
			      truename, truername))
		break;
	    }
	}

      if (NILP (fonttail))
	{
	  maybe_signal_error (Qinvalid_argument, "No matching font",
			      name_for_errors, Qfont, errb);
	  return 0;
	}

      if (!parse_font_spec (truername, hdc, name_for_errors,
			    ERROR_ME_DEBUG_WARN, &logfont, fontname, weight,
			    points, effects, charset))
	signal_error (Qinternal_error, "Bad value in device font list?",
		      build_istring (truername));
    }
  else if (!parse_font_spec (namestr, hdc, name_for_errors,
			     errb, &logfont, fontname, weight, points,
			     effects, charset))
    return 0;

  if ((hfont = qxeCreateFontIndirect (&logfont)) == NULL)
    {
      maybe_signal_error (Qgui_error, "Couldn't create font",
			  name_for_errors, Qfont, errb);
      return 0;
    }

  /* #### *truename_ret will not have all its fields filled in when we have no
     list of fonts.  Doesn't really matter now, since we always have one.  See
     above. */
  *truename_ret = emacs_sprintf_string ("%s:%s:%s:%s:%s", fontname,
                                        weight, points, effects, charset);
  return hfont;
}

/*
 * This is a work horse for both mswindows_initialize_font_instance and
 * msprinter_initialize_font_instance.
 */
static int
initialize_font_instance (Lisp_Font_Instance *f, Lisp_Object name,
			  Lisp_Object device_font_list, HDC hdc,
			  Error_Behavior errb)
{
  HFONT hfont, hfont2;
  TEXTMETRICW metrics;
  Ibyte *namestr = XSTRING_DATA (name);
  Lisp_Object truename;

  hfont = create_hfont_from_font_spec (namestr, hdc, name, device_font_list,
				       errb, &truename);
  if (!hfont)
    return 0;
  f->truename = truename;
  f->data = xnew_and_zero (struct mswindows_font_instance_data);
  FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, 0, 0) = hfont;

  /* Some underlined fonts have the descent of one pixel more than their
     non-underlined counterparts. Font variants though are assumed to have
     identical metrics. So get the font metrics from the underlined variant
     of the font */
  hfont2 = mswindows_create_font_variant (f, 1, 0);
  if (hfont2 != MSWINDOWS_BAD_HFONT)
    hfont = hfont2;

  hfont2 = (HFONT) SelectObject (hdc, hfont);
  if (!hfont2)
    {
      mswindows_finalize_font_instance (f);
      maybe_signal_error (Qgui_error, "Couldn't map font", name, Qfont, errb);
      return 0;
    }
  qxeGetTextMetrics (hdc, &metrics);
  SelectObject (hdc, hfont2);

  f->width = (unsigned short) metrics.tmAveCharWidth;
  f->height = (unsigned short) metrics.tmHeight;
  f->ascent = (unsigned short) metrics.tmAscent;
  f->descent = (unsigned short) metrics.tmDescent;
  f->proportional_p = (metrics.tmPitchAndFamily & TMPF_FIXED_PITCH);

  return 1;
}

static int
mswindows_initialize_font_instance (Lisp_Font_Instance *f, Lisp_Object name,
				    Lisp_Object device, Error_Behavior errb)
{
  HDC hdc = CreateCompatibleDC (NULL);
  Lisp_Object font_list = DEVICE_MSWINDOWS_FONTLIST (XDEVICE (device));
  int res = initialize_font_instance (f, name, font_list, hdc, errb);
  DeleteDC (hdc);
  return res;
}

static int
msprinter_initialize_font_instance (Lisp_Font_Instance *f, Lisp_Object name,
				    Lisp_Object device, Error_Behavior errb)
{
  HDC hdc = DEVICE_MSPRINTER_HDC (XDEVICE (device));
  Lisp_Object font_list = DEVICE_MSPRINTER_FONTLIST (XDEVICE (device));
  return initialize_font_instance (f, name, font_list, hdc, errb);
}

static void
mswindows_finalize_font_instance (Lisp_Font_Instance *f)
{
  int i;

  if (f->data)
    {
      for (i = 0; i < MSWINDOWS_NUM_FONT_VARIANTS; i++)
	{
	  if (FONT_INSTANCE_MSWINDOWS_HFONT_I (f, i) != NULL
	      && FONT_INSTANCE_MSWINDOWS_HFONT_I (f, i) != MSWINDOWS_BAD_HFONT)
	    DeleteObject (FONT_INSTANCE_MSWINDOWS_HFONT_I (f, i));
	}

      xfree (f->data);
      f->data = 0;
   }
}

static void
mswindows_print_font_instance (Lisp_Font_Instance *f,
			       Lisp_Object printcharfun,
			       int UNUSED (escapeflag))
{
  write_fmt_string (printcharfun, " %zx",
		    (EMACS_UINT) FONT_INSTANCE_MSWINDOWS_HFONT_VARIANT (f, 0,
									0));

}

static Lisp_Object
mswindows_font_list (Lisp_Object pattern, Lisp_Object device,
		      Lisp_Object UNUSED (maxnumber))
{
  struct device *d = XDEVICE (device);
  Lisp_Object font_list = Qnil, fonttail, result = Qnil;

  if (DEVICE_MSWINDOWS_P (d))
    font_list = DEVICE_MSWINDOWS_FONTLIST (d);
  else if (DEVICE_MSPRINTER_P (d))
    font_list = DEVICE_MSPRINTER_FONTLIST (d);
  else
    ABORT ();

  LIST_LOOP (fonttail, font_list)
    {
      Ibyte fontname[MSW_FONTSIZE];

      if (match_font (XSTRING_DATA (XCAR (XCAR (fonttail))),
		      XSTRING_DATA (pattern),
		      fontname))
	result = Fcons (build_istring (fontname), result);
    }

  return Fnreverse (result);
}

static Lisp_Object
mswindows_font_instance_truename (Lisp_Font_Instance *f,
				  Error_Behavior UNUSED (errb))
{
  return f->truename;
}


static int
mswindows_font_spec_matches_charset_stage_1 (struct device *UNUSED (d),
					     Lisp_Object charset,
					     const Ibyte *nonreloc,
					     Lisp_Object reloc,
					     Bytecount offset,
					     Bytecount length)
{
  int i;
  Lisp_Object charset_registry;
  const Ibyte *font_charset;
  const Ibyte *the_nonreloc = nonreloc;
  const Ibyte *c;
  Bytecount the_length = length;

  if (NILP (charset))
    return 1;

  if (!the_nonreloc)
    the_nonreloc = XSTRING_DATA (reloc);
  fixup_internal_substring (nonreloc, reloc, offset, &the_length);
  the_nonreloc += offset;

  c = the_nonreloc;
  for (i = 0; i < 4; i++)
    {
      Ibyte *newc = (Ibyte *) memchr (c, ':', the_length);
      if (!newc)
	break;
      newc++;
      the_length -= (newc - c);
      c = newc;
    }

  if (i < 4)
    return 0;

  font_charset = c;

  /* For border-glyph use */
  if (!qxestrcasecmp_ascii (font_charset, "symbol"))
    font_charset = (const Ibyte *) "western";

  /* Get code page for the charset */
  charset_registry = Fmswindows_charset_registry (charset);
  if (!STRINGP (charset_registry))
    return 0;

  return !qxestrcasecmp (XSTRING_DATA (charset_registry), font_charset);
}

/*

#### The following comment is old and probably not applicable any longer.

1. handle standard mapping and inheritance vectors properly in Face-frob-property.
2. finish impl of mswindows-charset-registry.
3. see if everything works under fixup, now that i copied the stuff over.
4. consider generalizing Face-frob-property to frob-specifier.
5. maybe extract some of the flets out of Face-frob-property as useful specifier frobbing. 
6. eventually this stuff's got to be checked in!!!!
*/

static int
mswindows_font_spec_matches_charset_stage_2 (struct device *d,
					     Lisp_Object charset,
					     const Ibyte *nonreloc,
					     Lisp_Object reloc,
					     Bytecount offset,
					     Bytecount length)
{
  const Ibyte *the_nonreloc = nonreloc;
  FONTSIGNATURE fs;
  FONTSIGNATURE *fsp = &fs;
  struct gcpro gcpro1;
  Lisp_Object fontsig;
  Bytecount the_length = length;
  int i;

  if (NILP (charset))
    return 1;

  if (!the_nonreloc)
    the_nonreloc = XSTRING_DATA (reloc);
  fixup_internal_substring (nonreloc, reloc, offset, &the_length);
  the_nonreloc += offset;

  /* Get the list of Unicode subranges corresponding to the font.  This
     is contained inside of FONTSIGNATURE data, obtained by calling
     GetTextCharsetInfo on a font object, which we need to create from the
     spec.  See if the FONTSIGNATURE data is already cached.  If not, get
     it and cache it. */
  if (!STRINGP (reloc) || the_nonreloc != XSTRING_DATA (reloc))
    reloc = build_istring (the_nonreloc);
  GCPRO1 (reloc);
  fontsig = Fgethash (reloc, Vfont_signature_data, Qunbound);

  if (!UNBOUNDP (fontsig))
    {
      fsp = (FONTSIGNATURE *) XOPAQUE_DATA (fontsig);
      UNGCPRO;
    }
  else
    {
      HDC hdc = CreateCompatibleDC (NULL);
      Lisp_Object font_list = Qnil, truename; 
      HFONT hfont;

      if (DEVICE_TYPE_P (d, mswindows))
	{
	  font_list = DEVICE_MSWINDOWS_FONTLIST (d);
	}
      else if (DEVICE_TYPE_P (d, msprinter))
	{
	  font_list = DEVICE_MSPRINTER_FONTLIST (d);
	}
      else
	{
	  assert(0);
	}

      hfont = create_hfont_from_font_spec (the_nonreloc, hdc, Qnil,
					   font_list,
					   ERROR_ME_DEBUG_WARN,
					   &truename);

      if (!hfont || !(hfont = (HFONT) SelectObject (hdc, hfont)))
	{
	nope:
	  DeleteDC (hdc);
	  UNGCPRO;
	  return 0;
	}
      
      if (GetTextCharsetInfo (hdc, &fs, 0) == DEFAULT_CHARSET)
	{
	  SelectObject (hdc, hfont);
	  goto nope;
	}
      SelectObject (hdc, hfont);
      DeleteDC (hdc);
      Fputhash (reloc, make_opaque (&fs, sizeof (fs)), Vfont_signature_data);
      UNGCPRO;
    }

  {
    int l1, l2, h1, h2;
    int j, cp = -1;

    /* Try to find a Unicode char in the charset.  #### This is somewhat
       bogus.  See below.

       #### Cache me baby!!!!!!!!!!!!!
    */
    get_charset_limits (charset, &l1, &l2, &h1, &h2);

    /* @@#### This needs major fixing.  We need to be passed the character,
       not the charset. */
    for (i = l1; i <= h1; i++)
      for (j = l2; j <= h2; j++)
	if ((cp = charset_codepoint_to_unicode (charset, i, j, CONVERR_FAIL))
	    >= 0)
	  goto multi_break;

  multi_break:
    if (cp < 0)
      return 0;

    /* Check to see, for each subrange supported by the font,
       whether the Unicode char is within that subrange.  If any match,
       the font supports the char (whereby, the charset, bogusly). */
      
    for (i = 0; i < countof (unicode_subrange_table); i++)
      {
	if (fsp->fsUsb[i >> 5] & (1 << (i & 32)))
	  {
	    for (j = 0; j < unicode_subrange_table[i].no_subranges; j++)
	      if (cp >= unicode_subrange_table[i].subranges[j].start &&
		  cp <= unicode_subrange_table[i].subranges[j].end)
		return 1;
	  }
      }

    return 0;
  }
}

/*
  Given a truename font spec, does it match CHARSET?

  We try two stages:

  -- First see if the charset corresponds to one of the predefined Windows
  charsets; if so, we see if the registry (that's the last element of the
  font spec) is that same charset.  If so, this means that the font is
  specifically designed for the charset, and we prefer it.

  -- However, there are only a limited number of defined Windows charsets,
  and new ones aren't being defined; so if we fail the first stage, we find
  a character from the charset with a Unicode equivalent, and see if the
  font can display this character.  we do that by retrieving the Unicode
  ranges that the font supports, to see if the character comes from that
  subrange.

  #### Note: We really want to be doing all these checks at the character
  level, not the charset level.  There's no guarantee that a charset covers
  a single Unicode range.  Furthermore, this is extremely wasteful.  We
  should be doing this when we're about to redisplay and already have the
  Unicode codepoints in hand.
*/

static int
mswindows_font_spec_matches_charset (struct device *d, Lisp_Object charset,
				     const Ibyte *nonreloc,
				     Lisp_Object reloc,
				     Bytecount offset, Bytecount length,
				     enum font_specifier_matchspec_stages stage)
{
  return stage == STAGE_FINAL ?
     mswindows_font_spec_matches_charset_stage_2 (d, charset, nonreloc,
						  reloc, offset, length)
    : mswindows_font_spec_matches_charset_stage_1 (d, charset, nonreloc,
						   reloc, offset, length);
}


/* Find a font spec that matches font spec FONT and also matches
   (the registry of) CHARSET. */

static Lisp_Object
mswindows_find_charset_font (Lisp_Object device, Lisp_Object font,
			     Lisp_Object charset,
			     enum font_specifier_matchspec_stages stage)
{
  Lisp_Object fontlist, fonttail;

  /* If FONT specifies a particular charset, this will only list fonts with
     that charset; otherwise, it will list fonts with all charsets. */
  fontlist = mswindows_font_list (font, device, Qnil);

  if (stage == STAGE_INITIAL)
    {
      LIST_LOOP (fonttail, fontlist)
	{
	  if (mswindows_font_spec_matches_charset_stage_1
	      (XDEVICE (device), charset, 0, XCAR (fonttail), 0, -1))
	    return XCAR (fonttail);
	}
    }
  else
    {
      LIST_LOOP (fonttail, fontlist)
	{
	  if (mswindows_font_spec_matches_charset_stage_2
	      (XDEVICE (device), charset, 0, XCAR (fonttail), 0, -1))
	    return XCAR (fonttail);
	}
    }

  return Qnil;
}



/************************************************************************/
/*                             non-methods                              */
/************************************************************************/

static Lisp_Object
mswindows_color_list (Lisp_Object UNUSED (device))
{
  return shared_X_color_list ();
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_fontcolor_mswindows (void)
{
}

void
console_type_create_fontcolor_mswindows (void)
{
  /* object methods */
  CONSOLE_HAS_METHOD (mswindows, initialize_color_instance);
  CONSOLE_HAS_METHOD (mswindows, print_color_instance);
  CONSOLE_HAS_METHOD (mswindows, finalize_color_instance);
  CONSOLE_HAS_METHOD (mswindows, color_instance_equal);
  CONSOLE_HAS_METHOD (mswindows, color_instance_hash);
  CONSOLE_HAS_METHOD (mswindows, color_instance_rgb_components);
  CONSOLE_HAS_METHOD (mswindows, valid_color_name_p);
  CONSOLE_HAS_METHOD (mswindows, color_list);

  CONSOLE_HAS_METHOD (mswindows, initialize_font_instance);
  CONSOLE_HAS_METHOD (mswindows, print_font_instance);
  CONSOLE_HAS_METHOD (mswindows, finalize_font_instance);
  CONSOLE_HAS_METHOD (mswindows, font_instance_truename);
  CONSOLE_HAS_METHOD (mswindows, font_list);
  CONSOLE_HAS_METHOD (mswindows, font_spec_matches_charset);
  CONSOLE_HAS_METHOD (mswindows, find_charset_font);

  /* Printer methods - delegate most to windows methods,
     since graphical objects behave the same way. */

  CONSOLE_INHERITS_METHOD (msprinter, mswindows, initialize_color_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, print_color_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, finalize_color_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, color_instance_equal);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, color_instance_hash);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, color_instance_rgb_components);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, valid_color_name_p);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, color_list);

  CONSOLE_HAS_METHOD (msprinter, initialize_font_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, print_font_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, finalize_font_instance);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, font_instance_truename);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, font_list);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, font_spec_matches_charset);
  CONSOLE_INHERITS_METHOD (msprinter, mswindows, find_charset_font);
}

void
vars_of_fontcolor_mswindows (void)
{
  Vfont_signature_data =
    make_lisp_hash_table (100, HASH_TABLE_NON_WEAK, Qequal);
  staticpro (&Vfont_signature_data);
}
