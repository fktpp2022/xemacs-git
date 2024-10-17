/* Generic Objects and Functions.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995, 1996, 2002, 2004, 2005, 2010 Ben Wing.
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

/* Synched up with: Not in FSF. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "device-impl.h"
#include "elhash.h"
#include "faces.h"
#include "frame.h"
#include "glyphs.h"
#include "fontcolor-impl.h"
#include "specifier.h"
#include "window.h"

#ifdef HAVE_TTY
#include "console-tty.h"
#endif

/* Objects that are substituted when an instantiation fails.
   If we leave in the Qunbound value, we will probably get crashes. */
Lisp_Object Vthe_null_color_instance, Vthe_null_font_instance;

/* Author: Ben Wing; some earlier code from Chuck Thompson, Jamie
   Zawinski. */

DOESNT_RETURN
finalose (void *ptr)
{
  Lisp_Object obj = wrap_pointer_1 (ptr);

  invalid_operation
    ("Can't dump an emacs containing window system objects", obj);
}

/****************************************************************************
 *                  X color names for use on TTY and MSW                    *
 ****************************************************************************/
const Rgbref invalid_rgb = ULONG_MAX;

typedef struct colormap_t
{
  const Ascbyte *name;
  Rgbref rgbref;
} colormap_t;

/* Colors from X11R6 "XConsortium: rgb.txt,v 10.41 94/02/20 18:39:36 rws Exp" */
/* MSWindows tends to round up the numbers in its palette, ie where X uses
 * 127, MSWindows uses 128. Colors commented as "Adjusted" are tweaked to
 * match the Windows standard palette to increase the likelihood of
 * mswindows_color_to_string() finding a named match.

Sorted case-insensitively by the string name of the color.

 */
static const colormap_t shared_X_color_map[] =
{
  {"AliceBlue"		, RGB_CONSTANT (240, 248, 255) },
  {"AntiqueWhite"	, RGB_CONSTANT (250, 235, 215) },
  {"AntiqueWhite1"	, RGB_CONSTANT (255, 239, 219) },
  {"AntiqueWhite2"	, RGB_CONSTANT (238, 223, 204) },
  {"AntiqueWhite3"	, RGB_CONSTANT (205, 192, 176) },
  {"AntiqueWhite4"	, RGB_CONSTANT (139, 131, 120) },
  {"aquamarine"		, RGB_CONSTANT (127, 255, 212) },
  {"aquamarine1"	, RGB_CONSTANT (127, 255, 212) },
  {"aquamarine2"	, RGB_CONSTANT (118, 238, 198) },
  {"aquamarine3"	, RGB_CONSTANT (102, 205, 170) },
  {"aquamarine4"	, RGB_CONSTANT (69, 139, 116) },
  {"azure"		, RGB_CONSTANT (240, 255, 255) },
  {"azure1"		, RGB_CONSTANT (240, 255, 255) },
  {"azure2"		, RGB_CONSTANT (224, 238, 238) },
  {"azure3"		, RGB_CONSTANT (193, 205, 205) },
  {"azure4"		, RGB_CONSTANT (131, 139, 139) },
  {"beige"		, RGB_CONSTANT (245, 245, 220) },
  {"bisque"		, RGB_CONSTANT (255, 228, 196) },
  {"bisque1"		, RGB_CONSTANT (255, 228, 196) },
  {"bisque2"		, RGB_CONSTANT (238, 213, 183) },
  {"bisque3"		, RGB_CONSTANT (205, 183, 158) },
  {"bisque4"		, RGB_CONSTANT (139, 125, 107) },
  {"black"		, RGB_CONSTANT (0, 0, 0) },
  {"BlanchedAlmond"	, RGB_CONSTANT (255, 235, 205) },
  {"blue"		, RGB_CONSTANT (0, 0, 255) },
  {"blue1"		, RGB_CONSTANT (0, 0, 255) },
  {"blue2"		, RGB_CONSTANT (0, 0, 238) },
  {"blue3"		, RGB_CONSTANT (0, 0, 205) },
  {"blue4"		, RGB_CONSTANT (0, 0, 139) },
  {"BlueViolet"		, RGB_CONSTANT (138, 43, 226) },
  {"brown"		, RGB_CONSTANT (165, 42, 42) },
  {"brown1"		, RGB_CONSTANT (255, 64, 64) },
  {"brown2"		, RGB_CONSTANT (238, 59, 59) },
  {"brown3"		, RGB_CONSTANT (205, 51, 51) },
  {"brown4"		, RGB_CONSTANT (139, 35, 35) },
  {"burlywood"		, RGB_CONSTANT (222, 184, 135) },
  {"burlywood1"		, RGB_CONSTANT (255, 211, 155) },
  {"burlywood2"		, RGB_CONSTANT (238, 197, 145) },
  {"burlywood3"		, RGB_CONSTANT (205, 170, 125) },
  {"burlywood4"		, RGB_CONSTANT (139, 115, 85) },
  {"CadetBlue"		, RGB_CONSTANT (95, 158, 160) },
  {"CadetBlue1"		, RGB_CONSTANT (152, 245, 255) },
  {"CadetBlue2"		, RGB_CONSTANT (144, 220, 240) },	/* Adjusted */
  {"CadetBlue3"		, RGB_CONSTANT (122, 197, 205) },
  {"CadetBlue4"		, RGB_CONSTANT (83, 134, 139) },
  {"chartreuse"		, RGB_CONSTANT (127, 255, 0) },
  {"chartreuse1"	, RGB_CONSTANT (127, 255, 0) },
  {"chartreuse2"	, RGB_CONSTANT (118, 238, 0) },
  {"chartreuse3"	, RGB_CONSTANT (102, 205, 0) },
  {"chartreuse4"	, RGB_CONSTANT (69, 139, 0) },
  {"chocolate"		, RGB_CONSTANT (210, 105, 30) },
  {"chocolate1"		, RGB_CONSTANT (255, 127, 36) },
  {"chocolate2"		, RGB_CONSTANT (238, 118, 33) },
  {"chocolate3"		, RGB_CONSTANT (205, 102, 29) },
  {"chocolate4"		, RGB_CONSTANT (139, 69, 19) },
  {"coral"		, RGB_CONSTANT (255, 127, 80) },
  {"coral1"		, RGB_CONSTANT (255, 114, 86) },
  {"coral2"		, RGB_CONSTANT (238, 106, 80) },
  {"coral3"		, RGB_CONSTANT (205, 91, 69) },
  {"coral4"		, RGB_CONSTANT (139, 62, 47) },
  {"CornflowerBlue"	, RGB_CONSTANT (100, 149, 237) },
  {"cornsilk"		, RGB_CONSTANT (255, 248, 220) },
  {"cornsilk1"		, RGB_CONSTANT (255, 248, 220) },
  {"cornsilk2"		, RGB_CONSTANT (238, 232, 205) },
  {"cornsilk3"		, RGB_CONSTANT (205, 200, 177) },
  {"cornsilk4"		, RGB_CONSTANT (139, 136, 120) },
  {"cyan"		, RGB_CONSTANT (0, 255, 255) },
  {"cyan1"		, RGB_CONSTANT (0, 255, 255) },
  {"cyan2"		, RGB_CONSTANT (0, 238, 238) },
  {"cyan3"		, RGB_CONSTANT (0, 205, 205) },
  {"cyan4"		, RGB_CONSTANT (0, 139, 139) },
  {"DarkBlue"		, RGB_CONSTANT (0, 0, 128) },	/* Adjusted == Navy */
  {"DarkCyan"		, RGB_CONSTANT (0, 128, 128) },	/* Adjusted */
  {"DarkGoldenrod"	, RGB_CONSTANT (184, 134, 11) },
  {"DarkGoldenrod1"	, RGB_CONSTANT (255, 185, 15) },
  {"DarkGoldenrod2"	, RGB_CONSTANT (238, 173, 14) },
  {"DarkGoldenrod3"	, RGB_CONSTANT (205, 149, 12) },
  {"DarkGoldenrod4"	, RGB_CONSTANT (139, 101, 8) },
  {"DarkGray"		, RGB_CONSTANT (169, 169, 169) },
  {"DarkGreen"		, RGB_CONSTANT (0, 128, 0) },	/* Adjusted */
  {"DarkGrey"		, RGB_CONSTANT (169, 169, 169) },
  {"DarkKhaki"		, RGB_CONSTANT (189, 183, 107) },
  {"DarkMagenta"	, RGB_CONSTANT (128, 0, 128) },	/* Adjusted */
  {"DarkOliveGreen"	, RGB_CONSTANT (85, 107, 47) },
  {"DarkOliveGreen1"	, RGB_CONSTANT (202, 255, 112) },
  {"DarkOliveGreen2"	, RGB_CONSTANT (188, 238, 104) },
  {"DarkOliveGreen3"	, RGB_CONSTANT (162, 205, 90) },
  {"DarkOliveGreen4"	, RGB_CONSTANT (110, 139, 61) },
  {"DarkOrange"		, RGB_CONSTANT (255, 140, 0) },
  {"DarkOrange1"	, RGB_CONSTANT (255, 127, 0) },
  {"DarkOrange2"	, RGB_CONSTANT (238, 118, 0) },
  {"DarkOrange3"	, RGB_CONSTANT (205, 102, 0) },
  {"DarkOrange4"	, RGB_CONSTANT (139, 69, 0) },
  {"DarkOrchid"		, RGB_CONSTANT (153, 50, 204) },
  {"DarkOrchid1"	, RGB_CONSTANT (191, 62, 255) },
  {"DarkOrchid2"	, RGB_CONSTANT (178, 58, 238) },
  {"DarkOrchid3"	, RGB_CONSTANT (154, 50, 205) },
  {"DarkOrchid4"	, RGB_CONSTANT (104, 34, 139) },
  {"DarkRed"		, RGB_CONSTANT (128, 0, 0) },	/* Adjusted */
  {"DarkSalmon"		, RGB_CONSTANT (233, 150, 122) },
  {"DarkSeaGreen"	, RGB_CONSTANT (143, 188, 143) },
  {"DarkSeaGreen1"	, RGB_CONSTANT (193, 255, 193) },
  {"DarkSeaGreen2"	, RGB_CONSTANT (180, 238, 180) },
  {"DarkSeaGreen3"	, RGB_CONSTANT (155, 205, 155) },
  {"DarkSeaGreen4"	, RGB_CONSTANT (105, 139, 105) },
  {"DarkSlateBlue"	, RGB_CONSTANT (72, 61, 139) },
  {"DarkSlateGray"	, RGB_CONSTANT (47, 79, 79) },
  {"DarkSlateGray1"	, RGB_CONSTANT (151, 255, 255) },
  {"DarkSlateGray2"	, RGB_CONSTANT (141, 238, 238) },
  {"DarkSlateGray3"	, RGB_CONSTANT (121, 205, 205) },
  {"DarkSlateGray4"	, RGB_CONSTANT (82, 139, 139) },
  {"DarkSlateGrey"	, RGB_CONSTANT (47, 79, 79) },
  {"DarkTurquoise"	, RGB_CONSTANT (0, 206, 209) },
  {"DarkViolet"		, RGB_CONSTANT (148, 0, 211) },
  {"DarkYellow"		, RGB_CONSTANT (128, 128, 0) },
  {"DeepPink"		, RGB_CONSTANT (255, 20, 147) },
  {"DeepPink1"		, RGB_CONSTANT (255, 20, 147) },
  {"DeepPink2"		, RGB_CONSTANT (238, 18, 137) },
  {"DeepPink3"		, RGB_CONSTANT (205, 16, 118) },
  {"DeepPink4"		, RGB_CONSTANT (139, 10, 80) },
  {"DeepSkyBlue"	, RGB_CONSTANT (0, 191, 255) },
  {"DeepSkyBlue1"	, RGB_CONSTANT (0, 191, 255) },
  {"DeepSkyBlue2"	, RGB_CONSTANT (0, 178, 238) },
  {"DeepSkyBlue3"	, RGB_CONSTANT (0, 154, 205) },
  {"DeepSkyBlue4"	, RGB_CONSTANT (0, 104, 139) },
  {"DimGray"		, RGB_CONSTANT (105, 105, 105) },
  {"DimGrey"		, RGB_CONSTANT (105, 105, 105) },
  {"DodgerBlue"		, RGB_CONSTANT (30, 144, 255) },
  {"DodgerBlue1"	, RGB_CONSTANT (30, 144, 255) },
  {"DodgerBlue2"	, RGB_CONSTANT (28, 134, 238) },
  {"DodgerBlue3"	, RGB_CONSTANT (24, 116, 205) },
  {"DodgerBlue4"	, RGB_CONSTANT (16, 78, 139) },
  {"firebrick"		, RGB_CONSTANT (178, 34, 34) },
  {"firebrick1"		, RGB_CONSTANT (255, 48, 48) },
  {"firebrick2"		, RGB_CONSTANT (238, 44, 44) },
  {"firebrick3"		, RGB_CONSTANT (205, 38, 38) },
  {"firebrick4"		, RGB_CONSTANT (139, 26, 26) },
  {"FloralWhite"	, RGB_CONSTANT (255, 250, 240) },
  {"ForestGreen"	, RGB_CONSTANT (34, 139, 34) },
  {"gainsboro"		, RGB_CONSTANT (220, 220, 220) },
  {"GhostWhite"		, RGB_CONSTANT (248, 248, 255) },
  {"gold"		, RGB_CONSTANT (255, 215, 0) },
  {"gold1"		, RGB_CONSTANT (255, 215, 0) },
  {"gold2"		, RGB_CONSTANT (238, 201, 0) },
  {"gold3"		, RGB_CONSTANT (205, 173, 0) },
  {"gold4"		, RGB_CONSTANT (139, 117, 0) },
  {"goldenrod"		, RGB_CONSTANT (218, 165, 32) },
  {"goldenrod1"		, RGB_CONSTANT (255, 193, 37) },
  {"goldenrod2"		, RGB_CONSTANT (238, 180, 34) },
  {"goldenrod3"		, RGB_CONSTANT (205, 155, 29) },
  {"goldenrod4"		, RGB_CONSTANT (139, 105, 20) },
  {"gray"		, RGB_CONSTANT (190, 190, 190) },
  {"gray0"		, RGB_CONSTANT (0, 0, 0) },
  {"gray1"		, RGB_CONSTANT (3, 3, 3) },
  {"gray10"		, RGB_CONSTANT (26, 26, 26) },
  {"gray100"		, RGB_CONSTANT (255, 255, 255) },
  {"gray11"		, RGB_CONSTANT (28, 28, 28) },
  {"gray12"		, RGB_CONSTANT (31, 31, 31) },
  {"gray13"		, RGB_CONSTANT (33, 33, 33) },
  {"gray14"		, RGB_CONSTANT (36, 36, 36) },
  {"gray15"		, RGB_CONSTANT (38, 38, 38) },
  {"gray16"		, RGB_CONSTANT (41, 41, 41) },
  {"gray17"		, RGB_CONSTANT (43, 43, 43) },
  {"gray18"		, RGB_CONSTANT (46, 46, 46) },
  {"gray19"		, RGB_CONSTANT (48, 48, 48) },
  {"gray2"		, RGB_CONSTANT (5, 5, 5) },
  {"gray20"		, RGB_CONSTANT (51, 51, 51) },
  {"gray21"		, RGB_CONSTANT (54, 54, 54) },
  {"gray22"		, RGB_CONSTANT (56, 56, 56) },
  {"gray23"		, RGB_CONSTANT (59, 59, 59) },
  {"gray24"		, RGB_CONSTANT (61, 61, 61) },
  {"gray25"		, RGB_CONSTANT (64, 64, 64) },
  {"gray26"		, RGB_CONSTANT (66, 66, 66) },
  {"gray27"		, RGB_CONSTANT (69, 69, 69) },
  {"gray28"		, RGB_CONSTANT (71, 71, 71) },
  {"gray29"		, RGB_CONSTANT (74, 74, 74) },
  {"gray3"		, RGB_CONSTANT (8, 8, 8) },
  {"gray30"		, RGB_CONSTANT (77, 77, 77) },
  {"gray31"		, RGB_CONSTANT (79, 79, 79) },
  {"gray32"		, RGB_CONSTANT (82, 82, 82) },
  {"gray33"		, RGB_CONSTANT (84, 84, 84) },
  {"gray34"		, RGB_CONSTANT (87, 87, 87) },
  {"gray35"		, RGB_CONSTANT (89, 89, 89) },
  {"gray36"		, RGB_CONSTANT (92, 92, 92) },
  {"gray37"		, RGB_CONSTANT (94, 94, 94) },
  {"gray38"		, RGB_CONSTANT (97, 97, 97) },
  {"gray39"		, RGB_CONSTANT (99, 99, 99) },
  {"gray4"		, RGB_CONSTANT (10, 10, 10) },
  {"gray40"		, RGB_CONSTANT (102, 102, 102) },
  {"gray41"		, RGB_CONSTANT (105, 105, 105) },
  {"gray42"		, RGB_CONSTANT (107, 107, 107) },
  {"gray43"		, RGB_CONSTANT (110, 110, 110) },
  {"gray44"		, RGB_CONSTANT (112, 112, 112) },
  {"gray45"		, RGB_CONSTANT (115, 115, 115) },
  {"gray46"		, RGB_CONSTANT (117, 117, 117) },
  {"gray47"		, RGB_CONSTANT (120, 120, 120) },
  {"gray48"		, RGB_CONSTANT (122, 122, 122) },
  {"gray49"		, RGB_CONSTANT (125, 125, 125) },
  {"gray5"		, RGB_CONSTANT (13, 13, 13) },
  {"gray50"		, RGB_CONSTANT (128, 128, 128) },	/* Adjusted */
  {"gray51"		, RGB_CONSTANT (130, 130, 130) },
  {"gray52"		, RGB_CONSTANT (133, 133, 133) },
  {"gray53"		, RGB_CONSTANT (135, 135, 135) },
  {"gray54"		, RGB_CONSTANT (138, 138, 138) },
  {"gray55"		, RGB_CONSTANT (140, 140, 140) },
  {"gray56"		, RGB_CONSTANT (143, 143, 143) },
  {"gray57"		, RGB_CONSTANT (145, 145, 145) },
  {"gray58"		, RGB_CONSTANT (148, 148, 148) },
  {"gray59"		, RGB_CONSTANT (150, 150, 150) },
  {"gray6"		, RGB_CONSTANT (15, 15, 15) },
  {"gray60"		, RGB_CONSTANT (153, 153, 153) },
  {"gray61"		, RGB_CONSTANT (156, 156, 156) },
  {"gray62"		, RGB_CONSTANT (158, 158, 158) },
  {"gray63"		, RGB_CONSTANT (161, 161, 161) },
  {"gray64"		, RGB_CONSTANT (163, 163, 163) },
  {"gray65"		, RGB_CONSTANT (166, 166, 166) },
  {"gray66"		, RGB_CONSTANT (168, 168, 168) },
  {"gray67"		, RGB_CONSTANT (171, 171, 171) },
  {"gray68"		, RGB_CONSTANT (173, 173, 173) },
  {"gray69"		, RGB_CONSTANT (176, 176, 176) },
  {"gray7"		, RGB_CONSTANT (18, 18, 18) },
  {"gray70"		, RGB_CONSTANT (179, 179, 179) },
  {"gray71"		, RGB_CONSTANT (181, 181, 181) },
  {"gray72"		, RGB_CONSTANT (184, 184, 184) },
  {"gray73"		, RGB_CONSTANT (186, 186, 186) },
  {"gray74"		, RGB_CONSTANT (189, 189, 189) },
  {"gray75"		, RGB_CONSTANT (192, 192, 192) },	/* Adjusted */
  {"gray76"		, RGB_CONSTANT (194, 194, 194) },
  {"gray77"		, RGB_CONSTANT (196, 196, 196) },
  {"gray78"		, RGB_CONSTANT (199, 199, 199) },
  {"gray79"		, RGB_CONSTANT (201, 201, 201) },
  {"gray8"		, RGB_CONSTANT (20, 20, 20) },
  {"gray80"		, RGB_CONSTANT (204, 204, 204) },
  {"gray81"		, RGB_CONSTANT (207, 207, 207) },
  {"gray82"		, RGB_CONSTANT (209, 209, 209) },
  {"gray83"		, RGB_CONSTANT (212, 212, 212) },
  {"gray84"		, RGB_CONSTANT (214, 214, 214) },
  {"gray85"		, RGB_CONSTANT (217, 217, 217) },
  {"gray86"		, RGB_CONSTANT (219, 219, 219) },
  {"gray87"		, RGB_CONSTANT (222, 222, 222) },
  {"gray88"		, RGB_CONSTANT (224, 224, 224) },
  {"gray89"		, RGB_CONSTANT (227, 227, 227) },
  {"gray9"		, RGB_CONSTANT (23, 23, 23) },
  {"gray90"		, RGB_CONSTANT (229, 229, 229) },
  {"gray91"		, RGB_CONSTANT (232, 232, 232) },
  {"gray92"		, RGB_CONSTANT (235, 235, 235) },
  {"gray93"		, RGB_CONSTANT (237, 237, 237) },
  {"gray94"		, RGB_CONSTANT (240, 240, 240) },
  {"gray95"		, RGB_CONSTANT (242, 242, 242) },
  {"gray96"		, RGB_CONSTANT (245, 245, 245) },
  {"gray97"		, RGB_CONSTANT (247, 247, 247) },
  {"gray98"		, RGB_CONSTANT (250, 250, 250) },
  {"gray99"		, RGB_CONSTANT (252, 252, 252) },
  {"green"		, RGB_CONSTANT (0, 255, 0) },
  {"green1"		, RGB_CONSTANT (0, 255, 0) },
  {"green2"		, RGB_CONSTANT (0, 238, 0) },
  {"green3"		, RGB_CONSTANT (0, 205, 0) },
  {"green4"		, RGB_CONSTANT (0, 139, 0) },
  {"GreenYellow"	, RGB_CONSTANT (173, 255, 47) },
  {"grey"		, RGB_CONSTANT (190, 190, 190) },
  {"grey0"		, RGB_CONSTANT (0, 0, 0) },
  {"grey1"		, RGB_CONSTANT (3, 3, 3) },
  {"grey10"		, RGB_CONSTANT (26, 26, 26) },
  {"grey100"		, RGB_CONSTANT (255, 255, 255) },
  {"grey11"		, RGB_CONSTANT (28, 28, 28) },
  {"grey12"		, RGB_CONSTANT (31, 31, 31) },
  {"grey13"		, RGB_CONSTANT (33, 33, 33) },
  {"grey14"		, RGB_CONSTANT (36, 36, 36) },
  {"grey15"		, RGB_CONSTANT (38, 38, 38) },
  {"grey16"		, RGB_CONSTANT (41, 41, 41) },
  {"grey17"		, RGB_CONSTANT (43, 43, 43) },
  {"grey18"		, RGB_CONSTANT (46, 46, 46) },
  {"grey19"		, RGB_CONSTANT (48, 48, 48) },
  {"grey2"		, RGB_CONSTANT (5, 5, 5) },
  {"grey20"		, RGB_CONSTANT (51, 51, 51) },
  {"grey21"		, RGB_CONSTANT (54, 54, 54) },
  {"grey22"		, RGB_CONSTANT (56, 56, 56) },
  {"grey23"		, RGB_CONSTANT (59, 59, 59) },
  {"grey24"		, RGB_CONSTANT (61, 61, 61) },
  {"grey25"		, RGB_CONSTANT (64, 64, 64) },
  {"grey26"		, RGB_CONSTANT (66, 66, 66) },
  {"grey27"		, RGB_CONSTANT (69, 69, 69) },
  {"grey28"		, RGB_CONSTANT (71, 71, 71) },
  {"grey29"		, RGB_CONSTANT (74, 74, 74) },
  {"grey3"		, RGB_CONSTANT (8, 8, 8) },
  {"grey30"		, RGB_CONSTANT (77, 77, 77) },
  {"grey31"		, RGB_CONSTANT (79, 79, 79) },
  {"grey32"		, RGB_CONSTANT (82, 82, 82) },
  {"grey33"		, RGB_CONSTANT (84, 84, 84) },
  {"grey34"		, RGB_CONSTANT (87, 87, 87) },
  {"grey35"		, RGB_CONSTANT (89, 89, 89) },
  {"grey36"		, RGB_CONSTANT (92, 92, 92) },
  {"grey37"		, RGB_CONSTANT (94, 94, 94) },
  {"grey38"		, RGB_CONSTANT (97, 97, 97) },
  {"grey39"		, RGB_CONSTANT (99, 99, 99) },
  {"grey4"		, RGB_CONSTANT (10, 10, 10) },
  {"grey40"		, RGB_CONSTANT (102, 102, 102) },
  {"grey41"		, RGB_CONSTANT (105, 105, 105) },
  {"grey42"		, RGB_CONSTANT (107, 107, 107) },
  {"grey43"		, RGB_CONSTANT (110, 110, 110) },
  {"grey44"		, RGB_CONSTANT (112, 112, 112) },
  {"grey45"		, RGB_CONSTANT (115, 115, 115) },
  {"grey46"		, RGB_CONSTANT (117, 117, 117) },
  {"grey47"		, RGB_CONSTANT (120, 120, 120) },
  {"grey48"		, RGB_CONSTANT (122, 122, 122) },
  {"grey49"		, RGB_CONSTANT (125, 125, 125) },
  {"grey5"		, RGB_CONSTANT (13, 13, 13) },
  {"grey50"		, RGB_CONSTANT (128, 128, 128) },	/* Adjusted */
  {"grey51"		, RGB_CONSTANT (130, 130, 130) },
  {"grey52"		, RGB_CONSTANT (133, 133, 133) },
  {"grey53"		, RGB_CONSTANT (135, 135, 135) },
  {"grey54"		, RGB_CONSTANT (138, 138, 138) },
  {"grey55"		, RGB_CONSTANT (140, 140, 140) },
  {"grey56"		, RGB_CONSTANT (143, 143, 143) },
  {"grey57"		, RGB_CONSTANT (145, 145, 145) },
  {"grey58"		, RGB_CONSTANT (148, 148, 148) },
  {"grey59"		, RGB_CONSTANT (150, 150, 150) },
  {"grey6"		, RGB_CONSTANT (15, 15, 15) },
  {"grey60"		, RGB_CONSTANT (153, 153, 153) },
  {"grey61"		, RGB_CONSTANT (156, 156, 156) },
  {"grey62"		, RGB_CONSTANT (158, 158, 158) },
  {"grey63"		, RGB_CONSTANT (161, 161, 161) },
  {"grey64"		, RGB_CONSTANT (163, 163, 163) },
  {"grey65"		, RGB_CONSTANT (166, 166, 166) },
  {"grey66"		, RGB_CONSTANT (168, 168, 168) },
  {"grey67"		, RGB_CONSTANT (171, 171, 171) },
  {"grey68"		, RGB_CONSTANT (173, 173, 173) },
  {"grey69"		, RGB_CONSTANT (176, 176, 176) },
  {"grey7"		, RGB_CONSTANT (18, 18, 18) },
  {"grey70"		, RGB_CONSTANT (179, 179, 179) },
  {"grey71"		, RGB_CONSTANT (181, 181, 181) },
  {"grey72"		, RGB_CONSTANT (184, 184, 184) },
  {"grey73"		, RGB_CONSTANT (186, 186, 186) },
  {"grey74"		, RGB_CONSTANT (189, 189, 189) },
  {"grey75"		, RGB_CONSTANT (192, 192, 192) },	/* Adjusted */
  {"grey76"		, RGB_CONSTANT (194, 194, 194) },
  {"grey77"		, RGB_CONSTANT (196, 196, 196) },
  {"grey78"		, RGB_CONSTANT (199, 199, 199) },
  {"grey79"		, RGB_CONSTANT (201, 201, 201) },
  {"grey8"		, RGB_CONSTANT (20, 20, 20) },
  {"grey80"		, RGB_CONSTANT (204, 204, 204) },
  {"grey81"		, RGB_CONSTANT (207, 207, 207) },
  {"grey82"		, RGB_CONSTANT (209, 209, 209) },
  {"grey83"		, RGB_CONSTANT (212, 212, 212) },
  {"grey84"		, RGB_CONSTANT (214, 214, 214) },
  {"grey85"		, RGB_CONSTANT (217, 217, 217) },
  {"grey86"		, RGB_CONSTANT (219, 219, 219) },
  {"grey87"		, RGB_CONSTANT (222, 222, 222) },
  {"grey88"		, RGB_CONSTANT (224, 224, 224) },
  {"grey89"		, RGB_CONSTANT (227, 227, 227) },
  {"grey9"		, RGB_CONSTANT (23, 23, 23) },
  {"grey90"		, RGB_CONSTANT (229, 229, 229) },
  {"grey91"		, RGB_CONSTANT (232, 232, 232) },
  {"grey92"		, RGB_CONSTANT (235, 235, 235) },
  {"grey93"		, RGB_CONSTANT (237, 237, 237) },
  {"grey94"		, RGB_CONSTANT (240, 240, 240) },
  {"grey95"		, RGB_CONSTANT (242, 242, 242) },
  {"grey96"		, RGB_CONSTANT (245, 245, 245) },
  {"grey97"		, RGB_CONSTANT (247, 247, 247) },
  {"grey98"		, RGB_CONSTANT (250, 250, 250) },
  {"grey99"		, RGB_CONSTANT (252, 252, 252) },
  {"honeydew"		, RGB_CONSTANT (240, 255, 240) },
  {"honeydew1"		, RGB_CONSTANT (240, 255, 240) },
  {"honeydew2"		, RGB_CONSTANT (224, 238, 224) },
  {"honeydew3"		, RGB_CONSTANT (193, 205, 193) },
  {"honeydew4"		, RGB_CONSTANT (131, 139, 131) },
  {"HotPink"		, RGB_CONSTANT (255, 105, 180) },
  {"HotPink1"		, RGB_CONSTANT (255, 110, 180) },
  {"HotPink2"		, RGB_CONSTANT (238, 106, 167) },
  {"HotPink3"		, RGB_CONSTANT (205, 96, 144) },
  {"HotPink4"		, RGB_CONSTANT (139, 58, 98) },
  {"IndianRed"		, RGB_CONSTANT (205, 92, 92) },
  {"IndianRed1"		, RGB_CONSTANT (255, 106, 106) },
  {"IndianRed2"		, RGB_CONSTANT (238, 99, 99) },
  {"IndianRed3"		, RGB_CONSTANT (205, 85, 85) },
  {"IndianRed4"		, RGB_CONSTANT (139, 58, 58) },
  {"ivory"		, RGB_CONSTANT (255, 255, 240) },
  {"ivory1"		, RGB_CONSTANT (255, 255, 240) },
  {"ivory2"		, RGB_CONSTANT (240, 240, 208) },	/* Adjusted */
  {"ivory3"		, RGB_CONSTANT (205, 205, 193) },
  {"ivory4"		, RGB_CONSTANT (139, 139, 131) },
  {"khaki"		, RGB_CONSTANT (240, 230, 140) },
  {"khaki1"		, RGB_CONSTANT (255, 246, 143) },
  {"khaki2"		, RGB_CONSTANT (238, 230, 133) },
  {"khaki3"		, RGB_CONSTANT (205, 198, 115) },
  {"khaki4"		, RGB_CONSTANT (139, 134, 78) },
  {"lavender"		, RGB_CONSTANT (230, 230, 250) },
  {"LavenderBlush"	, RGB_CONSTANT (255, 240, 245) },
  {"LavenderBlush1"	, RGB_CONSTANT (255, 240, 245) },
  {"LavenderBlush2"	, RGB_CONSTANT (238, 224, 229) },
  {"LavenderBlush3"	, RGB_CONSTANT (205, 193, 197) },
  {"LavenderBlush4"	, RGB_CONSTANT (139, 131, 134) },
  {"LawnGreen"		, RGB_CONSTANT (124, 252, 0) },
  {"LemonChiffon"	, RGB_CONSTANT (255, 250, 205) },
  {"LemonChiffon1"	, RGB_CONSTANT (255, 250, 205) },
  {"LemonChiffon2"	, RGB_CONSTANT (238, 233, 191) },
  {"LemonChiffon3"	, RGB_CONSTANT (205, 201, 165) },
  {"LemonChiffon4"	, RGB_CONSTANT (139, 137, 112) },
  {"LightBlue"		, RGB_CONSTANT (173, 216, 230) },
  {"LightBlue1"		, RGB_CONSTANT (191, 239, 255) },
  {"LightBlue2"		, RGB_CONSTANT (178, 223, 238) },
  {"LightBlue3"		, RGB_CONSTANT (154, 192, 205) },
  {"LightBlue4"		, RGB_CONSTANT (104, 131, 139) },
  {"LightCoral"		, RGB_CONSTANT (240, 128, 128) },
  {"LightCyan"		, RGB_CONSTANT (224, 255, 255) },
  {"LightCyan1"		, RGB_CONSTANT (224, 255, 255) },
  {"LightCyan2"		, RGB_CONSTANT (209, 238, 238) },
  {"LightCyan3"		, RGB_CONSTANT (180, 205, 205) },
  {"LightCyan4"		, RGB_CONSTANT (122, 139, 139) },
  {"LightGoldenrod"	, RGB_CONSTANT (238, 221, 130) },
  {"LightGoldenrod1"	, RGB_CONSTANT (255, 236, 139) },
  {"LightGoldenrod2"	, RGB_CONSTANT (238, 220, 130) },
  {"LightGoldenrod3"	, RGB_CONSTANT (205, 190, 112) },
  {"LightGoldenrod4"	, RGB_CONSTANT (139, 129, 76) },
  {"LightGoldenrodYellow", RGB_CONSTANT (250, 250, 210) },
  {"LightGray"		, RGB_CONSTANT (211, 211, 211) },
  {"LightGreen"		, RGB_CONSTANT (144, 238, 144) },
  {"LightGrey"		, RGB_CONSTANT (211, 211, 211) },
  {"LightPink"		, RGB_CONSTANT (255, 182, 193) },
  {"LightPink1"		, RGB_CONSTANT (255, 174, 185) },
  {"LightPink2"		, RGB_CONSTANT (238, 162, 173) },
  {"LightPink3"		, RGB_CONSTANT (205, 140, 149) },
  {"LightPink4"		, RGB_CONSTANT (139, 95, 101) },
  {"LightSalmon"	, RGB_CONSTANT (255, 160, 122) },
  {"LightSalmon1"	, RGB_CONSTANT (255, 160, 122) },
  {"LightSalmon2"	, RGB_CONSTANT (238, 149, 114) },
  {"LightSalmon3"	, RGB_CONSTANT (205, 129, 98) },
  {"LightSalmon4"	, RGB_CONSTANT (139, 87, 66) },
  {"LightSeaGreen"	, RGB_CONSTANT (32, 178, 170) },
  {"LightSkyBlue"	, RGB_CONSTANT (135, 206, 250) },
  {"LightSkyBlue1"	, RGB_CONSTANT (176, 226, 255) },
  {"LightSkyBlue2"	, RGB_CONSTANT (164, 211, 238) },
  {"LightSkyBlue3"	, RGB_CONSTANT (141, 182, 205) },
  {"LightSkyBlue4"	, RGB_CONSTANT (96, 123, 139) },
  {"LightSlateBlue"	, RGB_CONSTANT (132, 112, 255) },
  {"LightSlateGray"	, RGB_CONSTANT (119, 136, 153) },
  {"LightSlateGrey"	, RGB_CONSTANT (119, 136, 153) },
  {"LightSteelBlue"	, RGB_CONSTANT (176, 196, 222) },
  {"LightSteelBlue1"	, RGB_CONSTANT (202, 225, 255) },
  {"LightSteelBlue2"	, RGB_CONSTANT (188, 210, 238) },
  {"LightSteelBlue3"	, RGB_CONSTANT (162, 181, 205) },
  {"LightSteelBlue4"	, RGB_CONSTANT (110, 123, 139) },
  {"LightYellow"	, RGB_CONSTANT (255, 255, 224) },
  {"LightYellow"	, RGB_CONSTANT (255, 255, 225) },	/* Adjusted */
  {"LightYellow1"	, RGB_CONSTANT (255, 255, 224) },
  {"LightYellow2"	, RGB_CONSTANT (238, 238, 209) },
  {"LightYellow3"	, RGB_CONSTANT (205, 205, 180) },
  {"LightYellow4"	, RGB_CONSTANT (139, 139, 122) },
  {"LimeGreen"		, RGB_CONSTANT (50, 205, 50) },
  {"linen"		, RGB_CONSTANT (250, 240, 230) },
  {"magenta"		, RGB_CONSTANT (255, 0, 255) },
  {"magenta1"		, RGB_CONSTANT (255, 0, 255) },
  {"magenta2"		, RGB_CONSTANT (238, 0, 238) },
  {"magenta3"		, RGB_CONSTANT (205, 0, 205) },
  {"magenta4"		, RGB_CONSTANT (139, 0, 139) },
  {"maroon"		, RGB_CONSTANT (176, 48, 96) },
  {"maroon1"		, RGB_CONSTANT (255, 52, 179) },
  {"maroon2"		, RGB_CONSTANT (238, 48, 167) },
  {"maroon3"		, RGB_CONSTANT (205, 41, 144) },
  {"maroon4"		, RGB_CONSTANT (139, 28, 98) },
  {"MediumAquamarine"	, RGB_CONSTANT (102, 205, 170) },
  {"MediumBlue"		, RGB_CONSTANT (0, 0, 205) },
  {"MediumOrchid"	, RGB_CONSTANT (186, 85, 211) },
  {"MediumOrchid1"	, RGB_CONSTANT (224, 102, 255) },
  {"MediumOrchid2"	, RGB_CONSTANT (209, 95, 238) },
  {"MediumOrchid3"	, RGB_CONSTANT (180, 82, 205) },
  {"MediumOrchid4"	, RGB_CONSTANT (122, 55, 139) },
  {"MediumPurple"	, RGB_CONSTANT (147, 112, 219) },
  {"MediumPurple1"	, RGB_CONSTANT (171, 130, 255) },
  {"MediumPurple2"	, RGB_CONSTANT (159, 121, 238) },
  {"MediumPurple3"	, RGB_CONSTANT (137, 104, 205) },
  {"MediumPurple4"	, RGB_CONSTANT (93, 71, 139) },
  {"MediumSeaGreen"	, RGB_CONSTANT (60, 179, 113) },
  {"MediumSlateBlue"	, RGB_CONSTANT (123, 104, 238) },
  {"MediumSpringGreen"	, RGB_CONSTANT (0, 250, 154) },
  {"MediumTurquoise"	, RGB_CONSTANT (72, 209, 204) },
  {"MediumVioletRed"	, RGB_CONSTANT (199, 21, 133) },
  {"MidnightBlue"	, RGB_CONSTANT (25, 25, 112) },
  {"MintCream"		, RGB_CONSTANT (245, 255, 250) },
  {"MistyRose"		, RGB_CONSTANT (255, 228, 225) },
  {"MistyRose1"		, RGB_CONSTANT (255, 228, 225) },
  {"MistyRose2"		, RGB_CONSTANT (238, 213, 210) },
  {"MistyRose3"		, RGB_CONSTANT (205, 183, 181) },
  {"MistyRose4"		, RGB_CONSTANT (139, 125, 123) },
  {"moccasin"		, RGB_CONSTANT (255, 228, 181) },
  {"NavajoWhite"	, RGB_CONSTANT (255, 222, 173) },
  {"NavajoWhite1"	, RGB_CONSTANT (255, 222, 173) },
  {"NavajoWhite2"	, RGB_CONSTANT (238, 207, 161) },
  {"NavajoWhite3"	, RGB_CONSTANT (205, 179, 139) },
  {"NavajoWhite4"	, RGB_CONSTANT (139, 121, 94) },
  {"navy"		, RGB_CONSTANT (0, 0, 128) },
  {"NavyBlue"		, RGB_CONSTANT (0, 0, 128) },
  {"OldLace"		, RGB_CONSTANT (253, 245, 230) },
  {"OliveDrab"		, RGB_CONSTANT (107, 142, 35) },
  {"OliveDrab1"		, RGB_CONSTANT (192, 255, 62) },
  {"OliveDrab2"		, RGB_CONSTANT (179, 238, 58) },
  {"OliveDrab3"		, RGB_CONSTANT (154, 205, 50) },
  {"OliveDrab4"		, RGB_CONSTANT (105, 139, 34) },
  {"orange"		, RGB_CONSTANT (255, 165, 0) },
  {"orange1"		, RGB_CONSTANT (255, 165, 0) },
  {"orange2"		, RGB_CONSTANT (238, 154, 0) },
  {"orange3"		, RGB_CONSTANT (205, 133, 0) },
  {"orange4"		, RGB_CONSTANT (139, 90, 0) },
  {"OrangeRed"		, RGB_CONSTANT (255, 69, 0) },
  {"OrangeRed1"		, RGB_CONSTANT (255, 69, 0) },
  {"OrangeRed2"		, RGB_CONSTANT (238, 64, 0) },
  {"OrangeRed3"		, RGB_CONSTANT (205, 55, 0) },
  {"OrangeRed4"		, RGB_CONSTANT (139, 37, 0) },
  {"orchid"		, RGB_CONSTANT (218, 112, 214) },
  {"orchid1"		, RGB_CONSTANT (255, 131, 250) },
  {"orchid2"		, RGB_CONSTANT (238, 122, 233) },
  {"orchid3"		, RGB_CONSTANT (205, 105, 201) },
  {"orchid4"		, RGB_CONSTANT (139, 71, 137) },
  {"PaleGoldenrod"	, RGB_CONSTANT (238, 232, 170) },
  {"PaleGreen"		, RGB_CONSTANT (152, 251, 152) },
  {"PaleGreen1"		, RGB_CONSTANT (154, 255, 154) },
  {"PaleGreen2"		, RGB_CONSTANT (144, 238, 144) },
  {"PaleGreen3"		, RGB_CONSTANT (124, 205, 124) },
  {"PaleGreen4"		, RGB_CONSTANT (84, 139, 84) },
  {"PaleTurquoise"	, RGB_CONSTANT (175, 238, 238) },
  {"PaleTurquoise1"	, RGB_CONSTANT (187, 255, 255) },
  {"PaleTurquoise2"	, RGB_CONSTANT (174, 238, 238) },
  {"PaleTurquoise3"	, RGB_CONSTANT (150, 205, 205) },
  {"PaleTurquoise4"	, RGB_CONSTANT (102, 139, 139) },
  {"PaleVioletRed"	, RGB_CONSTANT (219, 112, 147) },
  {"PaleVioletRed1"	, RGB_CONSTANT (255, 130, 171) },
  {"PaleVioletRed2"	, RGB_CONSTANT (238, 121, 159) },
  {"PaleVioletRed3"	, RGB_CONSTANT (205, 104, 137) },
  {"PaleVioletRed4"	, RGB_CONSTANT (139, 71, 93) },
  {"PaleYellow"		, RGB_CONSTANT (255, 255, 128) },
  {"PapayaWhip"		, RGB_CONSTANT (255, 239, 213) },
  {"PeachPuff"		, RGB_CONSTANT (255, 218, 185) },
  {"PeachPuff1"		, RGB_CONSTANT (255, 218, 185) },
  {"PeachPuff2"		, RGB_CONSTANT (238, 203, 173) },
  {"PeachPuff3"		, RGB_CONSTANT (205, 175, 149) },
  {"PeachPuff4"		, RGB_CONSTANT (139, 119, 101) },
  {"peru"		, RGB_CONSTANT (205, 133, 63) },
  {"pink"		, RGB_CONSTANT (255, 192, 203) },
  {"pink1"		, RGB_CONSTANT (255, 181, 197) },
  {"pink2"		, RGB_CONSTANT (238, 169, 184) },
  {"pink3"		, RGB_CONSTANT (205, 145, 158) },
  {"pink4"		, RGB_CONSTANT (139, 99, 108) },
  {"plum"		, RGB_CONSTANT (221, 160, 221) },
  {"plum1"		, RGB_CONSTANT (255, 187, 255) },
  {"plum2"		, RGB_CONSTANT (238, 174, 238) },
  {"plum3"		, RGB_CONSTANT (205, 150, 205) },
  {"plum4"		, RGB_CONSTANT (139, 102, 139) },
  {"PowderBlue"		, RGB_CONSTANT (176, 224, 230) },
  {"purple"		, RGB_CONSTANT (160, 32, 240) },
  {"purple1"		, RGB_CONSTANT (155, 48, 255) },
  {"purple2"		, RGB_CONSTANT (145, 44, 238) },
  {"purple3"		, RGB_CONSTANT (125, 38, 205) },
  {"purple4"		, RGB_CONSTANT (85, 26, 139) },
  {"red"		, RGB_CONSTANT (255, 0, 0) },
  {"red1"		, RGB_CONSTANT (255, 0, 0) },
  {"red2"		, RGB_CONSTANT (238, 0, 0) },
  {"red3"		, RGB_CONSTANT (205, 0, 0) },
  {"red4"		, RGB_CONSTANT (139, 0, 0) },
  {"RosyBrown"		, RGB_CONSTANT (188, 143, 143) },
  {"RosyBrown1"		, RGB_CONSTANT (255, 193, 193) },
  {"RosyBrown2"		, RGB_CONSTANT (238, 180, 180) },
  {"RosyBrown3"		, RGB_CONSTANT (205, 155, 155) },
  {"RosyBrown4"		, RGB_CONSTANT (139, 105, 105) },
  {"RoyalBlue"		, RGB_CONSTANT (65, 105, 225) },
  {"RoyalBlue1"		, RGB_CONSTANT (72, 118, 255) },
  {"RoyalBlue2"		, RGB_CONSTANT (67, 110, 238) },
  {"RoyalBlue3"		, RGB_CONSTANT (58, 95, 205) },
  {"RoyalBlue4"		, RGB_CONSTANT (39, 64, 139) },
  {"SaddleBrown"	, RGB_CONSTANT (139, 69, 19) },
  {"salmon"		, RGB_CONSTANT (250, 128, 114) },
  {"salmon1"		, RGB_CONSTANT (255, 140, 105) },
  {"salmon2"		, RGB_CONSTANT (238, 130, 98) },
  {"salmon3"		, RGB_CONSTANT (205, 112, 84) },
  {"salmon4"		, RGB_CONSTANT (139, 76, 57) },
  {"SandyBrown"		, RGB_CONSTANT (244, 164, 96) },
  {"SeaGreen"		, RGB_CONSTANT (46, 139, 87) },
  {"SeaGreen1"		, RGB_CONSTANT (84, 255, 159) },
  {"SeaGreen2"		, RGB_CONSTANT (78, 238, 148) },
  {"SeaGreen3"		, RGB_CONSTANT (67, 205, 128) },
  {"SeaGreen4"		, RGB_CONSTANT (46, 139, 87) },
  {"seashell"		, RGB_CONSTANT (255, 245, 238) },
  {"seashell1"		, RGB_CONSTANT (255, 245, 238) },
  {"seashell2"		, RGB_CONSTANT (238, 229, 222) },
  {"seashell3"		, RGB_CONSTANT (205, 197, 191) },
  {"seashell4"		, RGB_CONSTANT (139, 134, 130) },
  {"sienna"		, RGB_CONSTANT (160, 82, 45) },
  {"sienna1"		, RGB_CONSTANT (255, 130, 71) },
  {"sienna2"		, RGB_CONSTANT (238, 121, 66) },
  {"sienna3"		, RGB_CONSTANT (205, 104, 57) },
  {"sienna4"		, RGB_CONSTANT (139, 71, 38) },
  {"SkyBlue"		, RGB_CONSTANT (135, 206, 235) },
  {"SkyBlue1"		, RGB_CONSTANT (135, 206, 255) },
  {"SkyBlue2"		, RGB_CONSTANT (126, 192, 238) },
  {"SkyBlue3"		, RGB_CONSTANT (108, 166, 205) },
  {"SkyBlue4"		, RGB_CONSTANT (74, 112, 139) },
  {"SlateBlue"		, RGB_CONSTANT (106, 90, 205) },
  {"SlateBlue1"		, RGB_CONSTANT (131, 111, 255) },
  {"SlateBlue2"		, RGB_CONSTANT (122, 103, 238) },
  {"SlateBlue3"		, RGB_CONSTANT (105, 89, 205) },
  {"SlateBlue4"		, RGB_CONSTANT (71, 60, 139) },
  {"SlateGray"		, RGB_CONSTANT (112, 128, 144) },
  {"SlateGray1"		, RGB_CONSTANT (198, 226, 255) },
  {"SlateGray2"		, RGB_CONSTANT (185, 211, 238) },
  {"SlateGray3"		, RGB_CONSTANT (159, 182, 205) },
  {"SlateGray4"		, RGB_CONSTANT (108, 123, 139) },
  {"SlateGrey"		, RGB_CONSTANT (112, 128, 144) },
  {"snow"		, RGB_CONSTANT (255, 250, 250) },
  {"snow1"		, RGB_CONSTANT (255, 250, 250) },
  {"snow2"		, RGB_CONSTANT (238, 233, 233) },
  {"snow3"		, RGB_CONSTANT (205, 201, 201) },
  {"snow4"		, RGB_CONSTANT (139, 137, 137) },
  {"SpringGreen"	, RGB_CONSTANT (0, 255, 127) },
  {"SpringGreen1"	, RGB_CONSTANT (0, 255, 127) },
  {"SpringGreen2"	, RGB_CONSTANT (0, 238, 118) },
  {"SpringGreen3"	, RGB_CONSTANT (0, 205, 102) },
  {"SpringGreen4"	, RGB_CONSTANT (0, 139, 69) },
  {"SteelBlue"		, RGB_CONSTANT (70, 130, 180) },
  {"SteelBlue1"		, RGB_CONSTANT (99, 184, 255) },
  {"SteelBlue2"		, RGB_CONSTANT (92, 172, 238) },
  {"SteelBlue3"		, RGB_CONSTANT (79, 148, 205) },
  {"SteelBlue4"		, RGB_CONSTANT (54, 100, 139) },
  {"tan"		, RGB_CONSTANT (210, 180, 140) },
  {"tan1"		, RGB_CONSTANT (255, 165, 79) },
  {"tan2"		, RGB_CONSTANT (238, 154, 73) },
  {"tan3"		, RGB_CONSTANT (205, 133, 63) },
  {"tan4"		, RGB_CONSTANT (139, 90, 43) },
  {"thistle"		, RGB_CONSTANT (216, 191, 216) },
  {"thistle1"		, RGB_CONSTANT (255, 225, 255) },
  {"thistle2"		, RGB_CONSTANT (238, 210, 238) },
  {"thistle3"		, RGB_CONSTANT (205, 181, 205) },
  {"thistle4"		, RGB_CONSTANT (139, 123, 139) },
  {"tomato"		, RGB_CONSTANT (255, 99, 71) },
  {"tomato1"		, RGB_CONSTANT (255, 99, 71) },
  {"tomato2"		, RGB_CONSTANT (238, 92, 66) },
  {"tomato3"		, RGB_CONSTANT (205, 79, 57) },
  {"tomato4"		, RGB_CONSTANT (139, 54, 38) },
  {"turquoise"		, RGB_CONSTANT (64, 224, 208) },
  {"turquoise1"		, RGB_CONSTANT (0, 245, 255) },
  {"turquoise2"		, RGB_CONSTANT (0, 229, 238) },
  {"turquoise3"		, RGB_CONSTANT (0, 197, 205) },
  {"turquoise4"		, RGB_CONSTANT (0, 134, 139) },
  {"violet"		, RGB_CONSTANT (238, 130, 238) },
  {"VioletRed"		, RGB_CONSTANT (208, 32, 144) },
  {"VioletRed1"		, RGB_CONSTANT (255, 62, 150) },
  {"VioletRed2"		, RGB_CONSTANT (238, 58, 140) },
  {"VioletRed3"		, RGB_CONSTANT (205, 50, 120) },
  {"VioletRed4"		, RGB_CONSTANT (139, 34, 82) },
  {"wheat"		, RGB_CONSTANT (245, 222, 179) },
  {"wheat1"		, RGB_CONSTANT (255, 231, 186) },
  {"wheat2"		, RGB_CONSTANT (238, 216, 174) },
  {"wheat3"		, RGB_CONSTANT (205, 186, 150) },
  {"wheat4"		, RGB_CONSTANT (139, 126, 102) },
  {"white"		, RGB_CONSTANT (255, 255, 255) },
  {"WhiteSmoke"		, RGB_CONSTANT (245, 245, 245) },
  {"yellow"		, RGB_CONSTANT (255, 255, 0) },
  {"yellow1"		, RGB_CONSTANT (255, 255, 0) },
  {"yellow2"		, RGB_CONSTANT (238, 238, 0) },
  {"yellow3"		, RGB_CONSTANT (205, 205, 0) },
  {"yellow4"		, RGB_CONSTANT (139, 139, 0) },
  {"YellowGreen"	, RGB_CONSTANT (154, 205, 50) }
};

static int
hexval (Ibyte c)
{
  /* assumes ASCII and isxdigit (c) */
  if (c >= 'a')
    return c - 'a' + 10;
  else if (c >= 'A')
    return c - 'A' + 10;
  else
    return c - '0';
}

static int
colormap_t_compare (const void *a, const void *b)
{
  return ascii_strcasecmp (((colormap_t *)a)->name,
                           ((colormap_t *)b)->name);
}

Rgbref
shared_X_string_to_color (const Ibyte *name)
{
  int i;

  if (*name == '#')
    {
      /* numeric names look like "#RRGGBB", "#RRRGGGBBB" or "#RRRRGGGGBBBB"
	 or "rgb:rrrr/gggg/bbbb" */
      unsigned int r, g, b;

      for (i = 1; i < qxestrlen (name); i++)
	{
	  if (!byte_ascii_p (name[i]) || !isxdigit ((int) name[i]))
	    return invalid_rgb;
	}
      if (qxestrlen (name) == 7)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[3]) * 16 + hexval (name[4]);
	  b = hexval (name[5]) * 16 + hexval (name[6]);
	  return (rgb_pack (r, g, b));
	}
      else if (qxestrlen (name) == 10)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[4]) * 16 + hexval (name[5]);
	  b = hexval (name[7]) * 16 + hexval (name[8]);
	  return (rgb_pack (r, g, b));
	}
      else if (qxestrlen (name) == 13)
	{
	  r = hexval (name[1]) * 16 + hexval (name[2]);
	  g = hexval (name[5]) * 16 + hexval (name[6]);
	  b = hexval (name[9]) * 16 + hexval (name[10]);
	  return (rgb_pack (r, g, b));
	}
    }
  else if (!qxestrncmp_ascii (name, "rgb:", 4))
    {
      unsigned int r, g, b;

      if (sscanf ((CIbyte *) name, "rgb:%04x/%04x/%04x", &r, &g, &b) == 3)
	{
	  int len = qxestrlen (name);
	  if (len == 18)
	    {
	      r /= 257;
	      g /= 257;
	      b /= 257;
	    }
	  else if (len == 15)
	    {
	      r /= 17;
	      g /= 17;
	      b /= 17;
	    }
	  return (rgb_pack (r, g, b));
	}
      else
	return invalid_rgb;
    }
  else if (*name)	/* Can't be an empty string */
    {
      colormap_t key = { alloca_ascbytes (qxestrlen (name) + 1),
                         rgb_pack (255, 255, 255) }, *res;
      Ascbyte *c = (Ascbyte *)(key.name);

      while (*name)
        {
          if (*name != ' ')
            {
              if (!byte_ascii_p (*name))
                {
                  return invalid_rgb;
                }

              *c++ = *name++;
            }
          else
            {
              name++;
            }
        }
      *c = '\0';

      if ((res = (colormap_t *) bsearch (&key, shared_X_color_map,
                                         countof (shared_X_color_map),
                                         sizeof (shared_X_color_map[0]),
                                         colormap_t_compare)) != NULL)
        {
          return res->rgbref;
        }
    }
  return invalid_rgb;
}

Lisp_Object
shared_X_color_to_string (Rgbref color)
{
  int i;

  for (i = 0; i < countof (shared_X_color_map); i++)
    if (color == (shared_X_color_map[i].rgbref))
      return  build_ascstring (shared_X_color_map[i].name);

  return emacs_sprintf_string ("#%02X%02X%02X", rgb_red (color),
                               rgb_green (color), rgb_blue (color));
}

Lisp_Object
shared_X_color_list (void)
{
  Lisp_Object result = Qnil;
  int i;

  for (i = countof (shared_X_color_map); i != 0;)
    result = Fcons (build_ascstring (shared_X_color_map[--i].name), result);

  return result;
}


/****************************************************************************
 *                       Color-Instance Object                              *
 ****************************************************************************/

Lisp_Object Qcolor_instancep;

static const struct memory_description color_instance_data_description_1 []= {
#ifdef HAVE_TTY
  { XD_BLOCK_PTR, tty_console, 1, { &tty_color_instance_data_description } },
#endif
  { XD_END }
};

static const struct sized_memory_description color_instance_data_description = {
  sizeof (void *), color_instance_data_description_1
};

static const struct memory_description color_instance_description[] = {
  { XD_INT, offsetof (Lisp_Color_Instance, color_instance_type) },
  { XD_LISP_OBJECT, offsetof (Lisp_Color_Instance, name)},
  { XD_LISP_OBJECT, offsetof (Lisp_Color_Instance, device)},
  { XD_UNION, offsetof (Lisp_Color_Instance, data),
    XD_INDIRECT (0, 0), { &color_instance_data_description } },
  {XD_END}
};

static void
print_color_instance (Lisp_Object obj, Lisp_Object printcharfun,
		      int escapeflag)
{
  Lisp_Color_Instance *c = XCOLOR_INSTANCE (obj);
  if (print_readably)
    printing_unreadable_lisp_object (obj, 0);
  write_fmt_string_lisp (printcharfun, "#<color-instance %s", c->name);
  write_fmt_string_lisp (printcharfun, " on %s", c->device);
  if (!NILP (c->device)) /* Vthe_null_color_instance */
    MAYBE_DEVMETH (XDEVICE (c->device), print_color_instance,
		   (c, printcharfun, escapeflag));
  write_fmt_string (printcharfun, " 0x%x>", LISP_OBJECT_UID (obj));
}

static void
finalize_color_instance (Lisp_Object obj)
{
  Lisp_Color_Instance *c = XCOLOR_INSTANCE (obj);

  if (!NILP (c->device))
    MAYBE_DEVMETH (XDEVICE (c->device), finalize_color_instance, (c));
}

static int
color_instance_equal (Lisp_Object obj1, Lisp_Object obj2, int depth,
		      int UNUSED (foldcase))
{
  Lisp_Color_Instance *c1 = XCOLOR_INSTANCE (obj1);
  Lisp_Color_Instance *c2 = XCOLOR_INSTANCE (obj2);

  return (c1 == c2) ||
    (EQ (c1->device, c2->device) &&
     DEVICEP (c1->device) &&
     HAS_DEVMETH_P (XDEVICE (c1->device), color_instance_equal) &&
     DEVMETH (XDEVICE (c1->device), color_instance_equal, (c1, c2, depth)));
}

static Hashcode
color_instance_hash (Lisp_Object obj, int depth, Boolint UNUSED (equalp))
{
  Lisp_Color_Instance *c = XCOLOR_INSTANCE (obj);
  struct device *d = DEVICEP (c->device) ? XDEVICE (c->device) : 0;

  return HASH2 ((Hashcode) d,
		!d ? LISP_HASH (obj)
		: DEVMETH_OR_GIVEN (d, color_instance_hash, (c, depth),
				    LISP_HASH (obj)));
}

DEFINE_NODUMP_LISP_OBJECT ("color-instance", color_instance,
			   print_color_instance,
			   finalize_color_instance, color_instance_equal,
			   color_instance_hash,
			   color_instance_description,
			   Lisp_Color_Instance);

DEFUN ("make-color-instance", Fmake_color_instance, 1, 3, 0, /*
Return a new `color-instance' object named NAME (a string).

Optional argument DEVICE specifies the device this object applies to
and defaults to the selected device.

An error is signaled if the color is unknown or cannot be allocated;
however, if optional argument NOERROR is non-nil, nil is simply
returned in this case. (And if NOERROR is other than t, a warning may
be issued.)

The returned object is a normal, first-class lisp object.  The way you
`deallocate' the color is the way you deallocate any other lisp object:
you drop all pointers to it and allow it to be garbage collected.  When
these objects are GCed, the underlying window-system data (e.g. X object)
is deallocated as well.
*/
       (name, device, noerror))
{
  Lisp_Object obj;
  Lisp_Color_Instance *c;
  int retval;

  CHECK_STRING (name);
  device = wrap_device (decode_device (device));

  obj = ALLOC_NORMAL_LISP_OBJECT (color_instance);
  c = XCOLOR_INSTANCE (obj);
  c->name = name;
  c->device = device;
  c->data = 0;
  c->color_instance_type = get_console_variant (XDEVICE_TYPE (c->device));

  retval = MAYBE_INT_DEVMETH (XDEVICE (device), initialize_color_instance,
			      (c, name, device,
			       decode_error_behavior_flag (noerror)));
  if (!retval)
    return Qnil;

  return obj;
}

DEFUN ("color-instance-p", Fcolor_instance_p, 1, 1, 0, /*
Return non-nil if OBJECT is a color instance.
*/
       (object))
{
  return COLOR_INSTANCEP (object) ? Qt : Qnil;
}

DEFUN ("color-instance-name", Fcolor_instance_name, 1, 1, 0, /*
Return the name used to allocate COLOR-INSTANCE.
*/
       (color_instance))
{
  CHECK_COLOR_INSTANCE (color_instance);
  return XCOLOR_INSTANCE (color_instance)->name;
}

DEFUN ("color-instance-rgb-components", Fcolor_instance_rgb_components, 1, 1, 0, /*
Return a three element list containing the red, green, and blue
color components of COLOR-INSTANCE, or nil if unknown.
Component values range from 0 to 65535.
*/
       (color_instance))
{
  Lisp_Color_Instance *c;

  CHECK_COLOR_INSTANCE (color_instance);
  c = XCOLOR_INSTANCE (color_instance);

  if (NILP (c->device))
    return Qnil;

  return MAYBE_LISP_DEVMETH (XDEVICE (c->device),
			     color_instance_rgb_components,
			     (c));
}

DEFUN ("valid-color-name-p", Fvalid_color_name_p, 1, 2, 0, /*
Return true if COLOR names a valid color for the current device.

Valid color names for X are listed in the file /usr/lib/X11/rgb.txt, or
whatever the equivalent is on your system.

Valid color names for TTY are those which have an ISO 6429 (ANSI) sequence.
In addition to being a color this may be one of a number of attributes
such as `blink'.
*/
       (color, device))
{
  struct device *d = decode_device (device);

  CHECK_STRING (color);
  return MAYBE_INT_DEVMETH (d, valid_color_name_p, (d, color)) ? Qt : Qnil;
}

DEFUN ("color-list", Fcolor_list, 0, 1, 0, /*
Return a list of color names.
DEVICE specifies which device to return names for, and defaults to the
currently selected device.
*/
       (device))
{
  device = wrap_device (decode_device (device));

  return MAYBE_LISP_DEVMETH (XDEVICE (device), color_list, (device));
}


/***************************************************************************
 *                       Font-Instance Object                              *
 ***************************************************************************/

Lisp_Object Qfont_instancep;

static Lisp_Object font_instance_truename_internal (Lisp_Object xfont,
						    Error_Behavior errb);

static const struct memory_description font_instance_data_description_1 []= {
#ifdef HAVE_TTY
  { XD_BLOCK_PTR, tty_console, 1, { &tty_font_instance_data_description } },
#endif
  { XD_END }
};

static const struct sized_memory_description font_instance_data_description = {
  sizeof (void *), font_instance_data_description_1
};

static const struct memory_description font_instance_description[] = {
  { XD_INT, offsetof (Lisp_Font_Instance, font_instance_type) },
  { XD_LISP_OBJECT, offsetof (Lisp_Font_Instance, name)},
  { XD_LISP_OBJECT, offsetof (Lisp_Font_Instance, truename)},
  { XD_LISP_OBJECT, offsetof (Lisp_Font_Instance, device)},
  { XD_LISP_OBJECT, offsetof (Lisp_Font_Instance, charset)},
  { XD_UNION, offsetof (Lisp_Font_Instance, data),
    XD_INDIRECT (0, 0), { &font_instance_data_description } },
  { XD_END }
};

static void
print_font_instance (Lisp_Object obj, Lisp_Object printcharfun, int escapeflag)
{
  Lisp_Font_Instance *f = XFONT_INSTANCE (obj);
  if (print_readably)
    printing_unreadable_lisp_object (obj, 0);
  write_fmt_string_lisp (printcharfun, "#<font-instance %S", f->name);
  write_fmt_string_lisp (printcharfun, " on %s", f->device);
  if (!NILP (f->device))
    {
      MAYBE_DEVMETH (XDEVICE (f->device), print_font_instance,
		     (f, printcharfun, escapeflag));

    }
  write_fmt_string (printcharfun, " 0x%x>", LISP_OBJECT_UID (obj));
}

static void
finalize_font_instance (Lisp_Object obj)
{
  Lisp_Font_Instance *f = XFONT_INSTANCE (obj);

  if (!NILP (f->device))
    {
      MAYBE_DEVMETH (XDEVICE (f->device), finalize_font_instance, (f));
    }
}

/* Fonts are equal if they resolve to the same name.
   Since we call `font-truename' to do this, and since font-truename is lazy,
   this means the `equal' could cause XListFonts to be run the first time.
 */
static int
font_instance_equal (Lisp_Object obj1, Lisp_Object obj2, int depth,
		     int UNUSED (foldcase))
{
  /* #### should this be moved into a device method? */
  return internal_equal (font_instance_truename_internal
			 (obj1, ERROR_ME_DEBUG_WARN),
			 font_instance_truename_internal
			 (obj2, ERROR_ME_DEBUG_WARN),
			 depth + 1);
}

static Hashcode
font_instance_hash (Lisp_Object obj, int depth, Boolint UNUSED (equalp))
{
  return internal_hash (font_instance_truename_internal
			(obj, ERROR_ME_DEBUG_WARN),
			depth + 1, 0);
}

DEFINE_NODUMP_LISP_OBJECT ("font-instance", font_instance,
			   print_font_instance,
			   finalize_font_instance, font_instance_equal,
			   font_instance_hash, font_instance_description,
			   Lisp_Font_Instance);


/* #### Why is this exposed to Lisp?  Used in:
x-frob-font-size, gtk-font-menu-load-font, x-font-menu-load-font-xft,
x-font-menu-load-font-core, mswindows-font-menu-load-font,
mswindows-frob-font-style-and-sizify, mswindows-frob-font-size. */
DEFUN ("make-font-instance", Fmake_font_instance, 1, 4, 0, /*
Return a new `font-instance' object named NAME.
DEVICE specifies the device this object applies to and defaults to the
selected device.  An error is signalled if the font is unknown or cannot
be allocated; however, if NOERROR is non-nil, nil is simply returned in
this case.  CHARSET is used internally.  #### make helper function?

The returned object is a normal, first-class lisp object.  The way you
`deallocate' the font is the way you deallocate any other lisp object:
you drop all pointers to it and allow it to be garbage collected.  When
these objects are GCed, the underlying GUI data is deallocated as well.
*/
       (name, device, noerror, charset))
{
  Lisp_Object obj;
  Lisp_Font_Instance *f;
  int retval = 0;
  Error_Behavior errb = decode_error_behavior_flag (noerror);

  if (ERRB_EQ (errb, ERROR_ME))
    CHECK_STRING (name);
  else if (!STRINGP (name))
    return Qnil;

  device = wrap_device (decode_device (device));

  obj = ALLOC_NORMAL_LISP_OBJECT (font_instance);
  f = XFONT_INSTANCE (obj);
  f->name = name;
  f->truename = Qnil;
  f->device = device;

  f->data = 0;
  f->font_instance_type = get_console_variant (XDEVICE_TYPE (f->device));

  /* Stick some default values here ... */
  f->ascent = f->height = 1;
  f->descent = 0;
  f->width = 1;
  f->charset = charset;
  f->proportional_p = 0;

  retval = MAYBE_INT_DEVMETH (XDEVICE (device), initialize_font_instance,
			      (f, name, device, errb));

  if (!retval)
    return Qnil;

  return obj;
}

DEFUN ("font-instance-p", Ffont_instance_p, 1, 1, 0, /*
Return non-nil if OBJECT is a font instance.
*/
       (object))
{
  return FONT_INSTANCEP (object) ? Qt : Qnil;
}

DEFUN ("font-instance-name", Ffont_instance_name, 1, 1, 0, /*
Return the name used to allocate FONT-INSTANCE.
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return XFONT_INSTANCE (font_instance)->name;
}

DEFUN ("font-instance-ascent", Ffont_instance_ascent, 1, 1, 0, /*
Return the ascent in pixels of FONT-INSTANCE.
The returned value is the maximum ascent for all characters in the font,
where a character's ascent is the number of pixels above (and including)
the baseline.
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return make_fixnum (XFONT_INSTANCE (font_instance)->ascent);
}

DEFUN ("font-instance-descent", Ffont_instance_descent, 1, 1, 0, /*
Return the descent in pixels of FONT-INSTANCE.
The returned value is the maximum descent for all characters in the font,
where a character's descent is the number of pixels below the baseline.
\(Many characters to do not have any descent.  Typical characters with a
descent are lowercase p and lowercase g.)
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return make_fixnum (XFONT_INSTANCE (font_instance)->descent);
}

DEFUN ("font-instance-width", Ffont_instance_width, 1, 1, 0, /*
Return the width in pixels of FONT-INSTANCE.
The returned value is the average width for all characters in the font.
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return make_fixnum (XFONT_INSTANCE (font_instance)->width);
}

DEFUN ("font-instance-proportional-p", Ffont_instance_proportional_p, 1, 1, 0, /*
Return whether FONT-INSTANCE is proportional.
This means that different characters in the font have different widths.
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return XFONT_INSTANCE (font_instance)->proportional_p ? Qt : Qnil;
}

static Lisp_Object
font_instance_truename_internal (Lisp_Object font_instance,
				 Error_Behavior errb)
{
  Lisp_Font_Instance *f = XFONT_INSTANCE (font_instance);

  if (NILP (f->device))
    {
      maybe_signal_error (Qgui_error,
			  "can't determine truename: "
			  "no device for font instance",
			  font_instance, Qfont, errb);
      return Qnil;
    }

  return DEVMETH_OR_GIVEN (XDEVICE (f->device),
			   font_instance_truename, (f, errb), f->name);
}

DEFUN ("font-instance-truename", Ffont_instance_truename, 1, 1, 0, /*
Return the canonical name of FONT-INSTANCE.
Font names are patterns which may match any number of fonts, of which
the first found is used.  This returns an unambiguous name for that font
\(but not necessarily its only unambiguous name).
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return font_instance_truename_internal (font_instance, ERROR_ME);
}

DEFUN ("font-instance-charset", Ffont_instance_charset, 1, 1, 0, /*
Return the Mule charset that FONT-INSTANCE was allocated to handle.
*/
       (font_instance))
{
  CHECK_FONT_INSTANCE (font_instance);
  return XFONT_INSTANCE (font_instance)->charset;
}

DEFUN ("font-instance-properties", Ffont_instance_properties, 1, 1, 0, /*
Return the properties (an alist or nil) of FONT-INSTANCE.
*/
       (font_instance))
{
  Lisp_Font_Instance *f;

  CHECK_FONT_INSTANCE (font_instance);
  f = XFONT_INSTANCE (font_instance);

  if (NILP (f->device))
    return Qnil;

  return MAYBE_LISP_DEVMETH (XDEVICE (f->device),
			     font_instance_properties, (f));
}

DEFUN ("font-list", Ffont_list, 1, 3, 0, /*
Return a list of font names matching the given pattern.
DEVICE specifies which device to search for names, and defaults to the
currently selected device.
*/
       (pattern, device, maxnumber))
{
  CHECK_STRING (pattern);
  device = wrap_device (decode_device (device));

  return MAYBE_LISP_DEVMETH (XDEVICE (device), font_list, (pattern, device,
							    maxnumber));
}


/****************************************************************************
 Color Object
 ***************************************************************************/

static const struct memory_description color_specifier_description[] = {
  { XD_LISP_OBJECT, offsetof (struct color_specifier, face) },
  { XD_LISP_OBJECT, offsetof (struct color_specifier, face_property) },
  { XD_END }
};

DEFINE_SPECIFIER_TYPE_WITH_DATA (color);
/* Qcolor defined in general.c */

static void
color_create (Lisp_Object obj)
{
  Lisp_Specifier *color = XCOLOR_SPECIFIER (obj);

  COLOR_SPECIFIER_FACE (color) = Qnil;
  COLOR_SPECIFIER_FACE_PROPERTY (color) = Qnil;
}

/* No equal or hash methods; ignore the face the color is based off
   of for `equal' */

static Lisp_Object
color_instantiate (Lisp_Object specifier, Lisp_Object UNUSED (matchspec),
		   Lisp_Object domain, Lisp_Object instantiator,
		   Lisp_Object depth, Lisp_Object no_fallback)
{
  /* When called, we're inside of call_with_suspended_errors(),
     so we can freely error. */
  Lisp_Object device = DOMAIN_DEVICE (domain);
  struct device *d = XDEVICE (device);

  if (COLOR_INSTANCEP (instantiator))
    {
      /* If we are on the same device then we're done.  Otherwise change
	 the instantiator to the name used to generate the pixel and let the
	 STRINGP case deal with it. */
      if (NILP (device) /* Vthe_null_color_instance */
	  || EQ (device, XCOLOR_INSTANCE (instantiator)->device))
	return instantiator;
      else
	instantiator = Fcolor_instance_name (instantiator);
    }

  if (STRINGP (instantiator))
    {
      /* First, look to see if we can retrieve a cached value. */
      Lisp_Object instance =
	Fgethash (instantiator, d->color_instance_cache, Qunbound);
      /* Otherwise, make a new one. */
      if (UNBOUNDP (instance))
	{
	  /* make sure we cache the failures, too. */
	  instance = Fmake_color_instance (instantiator, device, Qt);
	  Fputhash (instantiator, instance, d->color_instance_cache);
	}

      return NILP (instance) ? Qunbound : instance;
    }
  else if (VECTORP (instantiator))
    {
      switch (XVECTOR_LENGTH (instantiator))
	{
	case 0:
	  if (DEVICE_TTY_P (d))
	    return Vthe_null_color_instance;
	  else
            error_or_quit_failed_instantiator_in_domain
              ("Color instantiator [] only valid on TTYs", instantiator,
               domain);
	case 1:
	  if (NILP (COLOR_SPECIFIER_FACE (XCOLOR_SPECIFIER (specifier))))
            {
              error_or_quit_failed_instantiator_in_domain
                ("Color specifier not attached to a face", instantiator,
                 domain);
            }
	  return (FACE_PROPERTY_INSTANCE_1
		  (Fget_face (XVECTOR_DATA (instantiator)[0]),
		   COLOR_SPECIFIER_FACE_PROPERTY
		   (XCOLOR_SPECIFIER (specifier)),
                     domain, ERROR_ME, !NILP (no_fallback), depth));

	case 2:
	  return (FACE_PROPERTY_INSTANCE_1
		  (Fget_face (XVECTOR_DATA (instantiator)[0]),
		   XVECTOR_DATA (instantiator)[1], domain, ERROR_ME,
		   !NILP (no_fallback), depth));

	default:
	  ABORT ();
	}
    }
  else if (NILP (instantiator))
    {
      if (DEVICE_TTY_P (d))
	return Vthe_null_color_instance;
      else
        error_or_quit_failed_instantiator_in_domain
          ("Color instantiator [] only valid on TTYs", instantiator, domain);
    }
  else
    ABORT ();	/* The spec validation routines are screwed up. */

  return Qunbound;
}

static void
color_validate (Lisp_Object instantiator)
{
  if (COLOR_INSTANCEP (instantiator) || STRINGP (instantiator))
    return;
  if (VECTORP (instantiator))
    {
      if (XVECTOR_LENGTH (instantiator) > 2)
	sferror ("Inheritance vector must be of size 0 - 2",
			     instantiator);
      else if (XVECTOR_LENGTH (instantiator) > 0)
	{
	  Lisp_Object face = XVECTOR_DATA (instantiator)[0];

	  Fget_face (face);
	  if (XVECTOR_LENGTH (instantiator) == 2)
	    {
	      Lisp_Object field = XVECTOR_DATA (instantiator)[1];
	      if (!EQ (field, Qforeground)
		  && !EQ (field, Qforeback)
		  && !EQ (field, Qbackground))
		invalid_constant
("Inheritance field must be `foreground', `foreback' or `background'",
 field);
	    }
	}
    }
  else
    invalid_argument ("Invalid color instantiator", instantiator);
}

static void
color_after_change (Lisp_Object specifier, Lisp_Object locale)
{
  Lisp_Object face = COLOR_SPECIFIER_FACE (XCOLOR_SPECIFIER (specifier));
  Lisp_Object property =
    COLOR_SPECIFIER_FACE_PROPERTY (XCOLOR_SPECIFIER (specifier));
  if (!NILP (face))
    {
      face_property_was_changed (face, property, locale);
      if (BUFFERP (locale))
	XBUFFER (locale)->buffer_local_face_property = 1;
    }
}

void
set_color_attached_to (Lisp_Object obj, Lisp_Object face, Lisp_Object property)
{
  Lisp_Specifier *color = XCOLOR_SPECIFIER (obj);

  COLOR_SPECIFIER_FACE (color) = face;
  COLOR_SPECIFIER_FACE_PROPERTY (color) = property;
}

DEFUN ("color-specifier-p", Fcolor_specifier_p, 1, 1, 0, /*
Return t if OBJECT is a color specifier.

See `make-color-specifier' for a description of possible color instantiators.
*/
       (object))
{
  return COLOR_SPECIFIERP (object) ? Qt : Qnil;
}


/****************************************************************************
 Font Object
 ***************************************************************************/

static const struct memory_description font_specifier_description[] = {
  { XD_LISP_OBJECT, offsetof (struct font_specifier, face) },
  { XD_LISP_OBJECT, offsetof (struct font_specifier, face_property) },
  { XD_END }
};

DEFINE_SPECIFIER_TYPE_WITH_DATA (font);
/* Qfont defined in general.c */

static void
font_create (Lisp_Object obj)
{
  Lisp_Specifier *font = XFONT_SPECIFIER (obj);

  FONT_SPECIFIER_FACE (font) = Qnil;
  FONT_SPECIFIER_FACE_PROPERTY (font) = Qnil;
}

/* No equal or hash methods; ignore the face the font is based off
   of for `equal' */

#ifdef MULE

/* Given a truename font spec (i.e. the font spec should have its registry
   field filled in), does it support displaying characters from CHARSET? */

static int
font_spec_matches_charset (struct device *d, Lisp_Object charset,
			   const Ibyte *nonreloc, Lisp_Object reloc,
			   Bytecount offset, Bytecount length,
			   enum font_specifier_matchspec_stages stage)
{
  return DEVMETH_OR_GIVEN (d, font_spec_matches_charset,
			   (d, charset, nonreloc, reloc, offset, length,
			    stage),
			   1);
}

static void
font_validate_matchspec (Lisp_Object matchspec)
{
  CHECK_CONS (matchspec);
  Fget_charset (XCAR (matchspec));

  do
    {
      if (EQ(XCDR(matchspec), Qinitial))
	{
	  break;
	}
      if (EQ(XCDR(matchspec), Qfinal))
	{
	  break;
	}

      invalid_argument("Invalid font matchspec stage",
		       XCDR(matchspec));
    } while (0);
}

void
initialize_charset_font_caches (struct device *d)
{
  /* Note that the following tables are bi-level. */
  d->charset_font_cache_stage_1 =
    make_lisp_hash_table (20, HASH_TABLE_NON_WEAK, Qeq);
  d->charset_font_cache_stage_2 =
    make_lisp_hash_table (20, HASH_TABLE_NON_WEAK, Qeq);
}

void
invalidate_charset_font_caches (Lisp_Object charset)
{
  /* Invalidate font cache entries for charset on all devices. */
  Lisp_Object devcons, concons, hash_table;
  DEVICE_LOOP_NO_BREAK (devcons, concons)
    {
      struct device *d = XDEVICE (XCAR (devcons));
      hash_table = Fgethash (charset, d->charset_font_cache_stage_1,
			     Qunbound);
      if (!UNBOUNDP (hash_table))
	Fclrhash (hash_table);
      hash_table = Fgethash (charset, d->charset_font_cache_stage_2,
			     Qunbound);
      if (!UNBOUNDP (hash_table))
	Fclrhash (hash_table);
    }
}

#endif /* MULE */

/* It's a little non-obvious what's going on here.  Specifically:

   MATCHSPEC is a somewhat bogus way in the specifier mechanism of passing
   in additional information needed to instantiate some object.  For fonts,
   it's a cons of (CHARSET . SECOND-STAGE-P).  SECOND-STAGE-P, if set,
   means "try harder to find an appropriate font" and is a very bogus way
   of dealing with the fact that it may not be possible to may a charset
   directly onto a font; it's used esp. under Windows.  @@#### We need to
   change this so that MATCHSPEC is just a character.

   When redisplay is building up its structure, and needs font info, it
   calls functions in faces.c such as ensure_face_cachel_complete() (map
   fonts needed for a string of text) or
   ensure_face_cachel_contains_charset() (map fonts needed for a charset
   derived from a single character).  The former function calls the latter;
   the latter calls face_property_matching_instance(); this constructs the
   MATCHSPEC and calls specifier_instance_no_quit() twice (first stage and
   second stage, updating MATCHSPEC appropriately).  That function, in
   turn, looks up the appropriate specifier method to do the instantiation,
   which, lo and behold, is this function here (because we set it in
   initialization using `SPECIFIER_HAS_METHOD (font, instantiate);').  We
   in turn call the device method `find_charset_font', which maps to
   mswindows_find_charset_font(), x_find_charset_font(), or similar, in
   fontcolor-msw.c or the like.

   --ben */

static Lisp_Object
font_instantiate (Lisp_Object UNUSED (specifier),
		  Lisp_Object USED_IF_MULE (matchspec),
		  Lisp_Object domain, Lisp_Object instantiator,
		  Lisp_Object depth, Lisp_Object no_fallback)
{
  /* When called, we're inside of call_with_suspended_errors(),
     so we can freely error. */
  Lisp_Object device = DOMAIN_DEVICE (domain);
  struct device *d = XDEVICE (device);
  Lisp_Object instance;
  Lisp_Object charset = Qnil;
#ifdef MULE
  enum font_specifier_matchspec_stages stage = STAGE_INITIAL;

  if (!UNBOUNDP (matchspec))
    {
      charset = Fget_charset (XCAR (matchspec));

#define FROB(new_stage, enumstage)			\
          if (EQ(Q##new_stage, XCDR(matchspec)))	\
	    {						\
	      stage = enumstage;			\
	    }

	  FROB (initial, STAGE_INITIAL)
	  else FROB (final, STAGE_FINAL)
	  else assert(0);

#undef FROB

    }
#endif

  if (FONT_INSTANCEP (instantiator))
    {
      if (NILP (device)
	  || EQ (device, XFONT_INSTANCE (instantiator)->device))
	{
#ifdef MULE
	  if (font_spec_matches_charset (d, charset, 0,
					 Ffont_instance_truename
					 (instantiator),
					 0, -1, stage))
#endif
	    return instantiator;
	}
      instantiator = Ffont_instance_name (instantiator);
    }

  if (STRINGP (instantiator))
    {
#ifdef MULE
      /* #### rename these caches. */
      Lisp_Object cache = stage == STAGE_FINAL ?
	d->charset_font_cache_stage_2 :
	d->charset_font_cache_stage_1;
#else
      Lisp_Object cache = d->font_instance_cache;
#endif

#ifdef MULE
      if (!NILP (charset))
	{
	  /* The instantiator is a font spec that could match many
	     different fonts.  We need to find one of those fonts
	     whose registry matches the registry of the charset in
	     MATCHSPEC.  This is potentially a very slow operation,
	     as it involves doing an XListFonts() or equivalent to
	     iterate over all possible fonts, and a regexp match
	     on each one.  So we cache the results. */
	  Lisp_Object matching_font = Qunbound;
	  Lisp_Object hash_table = Fgethash (charset, cache, Qunbound);
	  if (UNBOUNDP (hash_table))
	    {
	      /* need to make a sub hash table. */
	      hash_table = make_lisp_hash_table (20, HASH_TABLE_KEY_WEAK,
						 Qequal);
	      Fputhash (charset, hash_table, cache);
	    }
	  else
	    matching_font = Fgethash (instantiator, hash_table, Qunbound);

	  if (UNBOUNDP (matching_font))
	    {
	      /* make sure we cache the failures, too. */
	      matching_font =
		DEVMETH_OR_GIVEN (d, find_charset_font,
				  (device, instantiator, charset, stage),
				  instantiator);
	      Fputhash (instantiator, matching_font, hash_table);
	    }
	  if (NILP (matching_font))
	    return Qunbound;
	  instantiator = matching_font;
	}
#endif /* MULE */

      /* First, look to see if we can retrieve a cached value. */
      instance = Fgethash (instantiator, cache, Qunbound);
      /* Otherwise, make a new one. */
      if (UNBOUNDP (instance))
	{
	  /* make sure we cache the failures, too. */
	  instance = Fmake_font_instance (instantiator, device, Qt, charset);
	  Fputhash (instantiator, instance, cache);
	}

      return NILP (instance) ? Qunbound : instance;
    }
  else if (VECTORP (instantiator))
    {
      Lisp_Object match_inst = Qunbound;
      assert (XVECTOR_LENGTH (instantiator) == 1);

      match_inst = face_property_matching_instance
	(Fget_face (XVECTOR_DATA (instantiator)[0]), Qfont,
           charset, domain, ERROR_ME, !NILP (no_fallback),
           depth, STAGE_INITIAL);

      if (UNBOUNDP(match_inst))
	{
	  match_inst = face_property_matching_instance
	    (Fget_face (XVECTOR_DATA (instantiator)[0]), Qfont,
               charset, domain, ERROR_ME, !NILP (no_fallback), depth,
               STAGE_FINAL);
	}

      return match_inst;

    }
  else if (NILP (instantiator))
    return Qunbound;
  else
    ABORT ();	/* Eh? */

  return Qunbound;
}

static void
font_validate (Lisp_Object instantiator)
{
  if (FONT_INSTANCEP (instantiator) || STRINGP (instantiator))
    return;
  if (VECTORP (instantiator))
    {
      if (XVECTOR_LENGTH (instantiator) != 1)
	{
	  sferror
	    ("Vector length must be one for font inheritance", instantiator);
	}
      Fget_face (XVECTOR_DATA (instantiator)[0]);
    }
  else
    invalid_argument ("Must be string, vector, or font-instance",
			 instantiator);
}

static void
font_after_change (Lisp_Object specifier, Lisp_Object locale)
{
  Lisp_Object face = FONT_SPECIFIER_FACE (XFONT_SPECIFIER (specifier));
  Lisp_Object property =
    FONT_SPECIFIER_FACE_PROPERTY (XFONT_SPECIFIER (specifier));
  if (!NILP (face))
    {
      face_property_was_changed (face, property, locale);
      if (BUFFERP (locale))
	XBUFFER (locale)->buffer_local_face_property = 1;
    }
}

void
set_font_attached_to (Lisp_Object obj, Lisp_Object face, Lisp_Object property)
{
  Lisp_Specifier *font = XFONT_SPECIFIER (obj);

  FONT_SPECIFIER_FACE (font) = face;
  FONT_SPECIFIER_FACE_PROPERTY (font) = property;
}

DEFUN ("font-specifier-p", Ffont_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a font specifier.

See `make-font-specifier' for a description of possible font instantiators.
*/
       (object))
{
  return FONT_SPECIFIERP (object) ? Qt : Qnil;
}


/*****************************************************************************
 Face Boolean Object
 ****************************************************************************/

static const struct memory_description face_boolean_specifier_description[] = {
  { XD_LISP_OBJECT, offsetof (struct face_boolean_specifier, face) },
  { XD_LISP_OBJECT, offsetof (struct face_boolean_specifier, face_property) },
  { XD_END }
};

DEFINE_SPECIFIER_TYPE_WITH_DATA (face_boolean);
Lisp_Object Qface_boolean;

static void
face_boolean_create (Lisp_Object obj)
{
  Lisp_Specifier *face_boolean = XFACE_BOOLEAN_SPECIFIER (obj);

  FACE_BOOLEAN_SPECIFIER_FACE (face_boolean) = Qnil;
  FACE_BOOLEAN_SPECIFIER_FACE_PROPERTY (face_boolean) = Qnil;
}

/* No equal or hash methods; ignore the face the face-boolean is based off
   of for `equal' */

static Lisp_Object
face_boolean_instantiate (Lisp_Object specifier,
			  Lisp_Object UNUSED (matchspec),
			  Lisp_Object domain, Lisp_Object instantiator,
			  Lisp_Object depth, Lisp_Object no_fallback)
{
  /* When called, we're inside of call_with_suspended_errors(),
     so we can freely error. */
  if (NILP (instantiator) || EQ (instantiator, Qt))
    return instantiator;
  else if (VECTORP (instantiator))
    {
      Lisp_Object retval;
      Lisp_Object prop;
      Elemcount instantiator_len = XVECTOR_LENGTH (instantiator);

      assert (instantiator_len >= 1 && instantiator_len <= 3);
      if (instantiator_len > 1)
	prop = XVECTOR_DATA (instantiator)[1];
      else
	{
	  if (NILP (FACE_BOOLEAN_SPECIFIER_FACE
		    (XFACE_BOOLEAN_SPECIFIER (specifier))))
	    gui_error
	      ("Face-boolean specifier not attached to a face", instantiator);
	  prop = FACE_BOOLEAN_SPECIFIER_FACE_PROPERTY
	    (XFACE_BOOLEAN_SPECIFIER (specifier));
	}

      retval = (FACE_PROPERTY_INSTANCE_1
		(Fget_face (XVECTOR_DATA (instantiator)[0]),
                   prop, domain, ERROR_ME, !NILP (no_fallback), depth));

      if (instantiator_len == 3 && !NILP (XVECTOR_DATA (instantiator)[2]))
	retval = NILP (retval) ? Qt : Qnil;

      return retval;
    }
  else
    ABORT ();	/* Eh? */

  return Qunbound;
}

static void
face_boolean_validate (Lisp_Object instantiator)
{
  if (NILP (instantiator) || EQ (instantiator, Qt))
    return;
  else if (VECTORP (instantiator) &&
	   (XVECTOR_LENGTH (instantiator) >= 1 &&
	    XVECTOR_LENGTH (instantiator) <= 3))
    {
      Lisp_Object face = XVECTOR_DATA (instantiator)[0];

      Fget_face (face);

      if (XVECTOR_LENGTH (instantiator) > 1)
	{
	  Lisp_Object field = XVECTOR_DATA (instantiator)[1];
	  if (!EQ (field, Qunderline)
	      && !EQ (field, Qstrikethru)
	      && !EQ (field, Qhighlight)
	      && !EQ (field, Qdim)
	      && !EQ (field, Qblinking)
	      && !EQ (field, Qreverse)
	      && !EQ (field, Qshrink))
	    invalid_constant ("Invalid face-boolean inheritance field",
			      field);
	}
    }
  else if (VECTORP (instantiator))
    sferror ("Wrong length for face-boolean inheritance spec",
			 instantiator);
  else
    invalid_argument ("Face-boolean instantiator must be nil, t, or vector",
			 instantiator);
}

static void
face_boolean_after_change (Lisp_Object specifier, Lisp_Object locale)
{
  Lisp_Object face =
    FACE_BOOLEAN_SPECIFIER_FACE (XFACE_BOOLEAN_SPECIFIER (specifier));
  Lisp_Object property =
    FACE_BOOLEAN_SPECIFIER_FACE_PROPERTY (XFACE_BOOLEAN_SPECIFIER (specifier));
  if (!NILP (face))
    {
      face_property_was_changed (face, property, locale);
      if (BUFFERP (locale))
	XBUFFER (locale)->buffer_local_face_property = 1;
    }
}

void
set_face_boolean_attached_to (Lisp_Object obj, Lisp_Object face,
			      Lisp_Object property)
{
  Lisp_Specifier *face_boolean = XFACE_BOOLEAN_SPECIFIER (obj);

  FACE_BOOLEAN_SPECIFIER_FACE (face_boolean) = face;
  FACE_BOOLEAN_SPECIFIER_FACE_PROPERTY (face_boolean) = property;
}

DEFUN ("face-boolean-specifier-p", Fface_boolean_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a face-boolean specifier.

See `make-face-boolean-specifier' for a description of possible
face-boolean instantiators.
*/
       (object))
{
  return FACE_BOOLEAN_SPECIFIERP (object) ? Qt : Qnil;
}


/*****************************************************************************
 Face Background Placement Object
 ****************************************************************************/
Lisp_Object Qabsolute, Qrelative;

static const struct memory_description
face_background_placement_specifier_description[] = {
  { XD_LISP_OBJECT, offsetof (struct face_background_placement_specifier,
			      face) },
  { XD_END }
};

DEFINE_SPECIFIER_TYPE_WITH_DATA (face_background_placement);
Lisp_Object Qface_background_placement;

static void
face_background_placement_create (Lisp_Object obj)
{
  Lisp_Specifier *face_background_placement
    = XFACE_BACKGROUND_PLACEMENT_SPECIFIER (obj);

  FACE_BACKGROUND_PLACEMENT_SPECIFIER_FACE (face_background_placement) = Qnil;
}

/* No equal or hash methods; ignore the face the background-placement is based
   off of for `equal' */

extern Lisp_Object Qbackground_placement;

static Lisp_Object
face_background_placement_instantiate (Lisp_Object UNUSED (specifier),
				       Lisp_Object UNUSED (matchspec),
				       Lisp_Object domain,
				       Lisp_Object instantiator,
				       Lisp_Object depth,
                                       Lisp_Object no_fallback)
{
  /* When called, we're inside of call_with_suspended_errors(),
     so we can freely error. */
  if (EQ (instantiator, Qabsolute) || EQ (instantiator, Qrelative))
    return instantiator;
  else if (VECTORP (instantiator))
    {
      assert (XVECTOR_LENGTH (instantiator) == 1);

      return FACE_PROPERTY_INSTANCE_1
	(Fget_face (XVECTOR_DATA (instantiator)[0]),
           Qbackground_placement, domain, ERROR_ME, !NILP (no_fallback), depth);
    }
  else
    ABORT ();	/* Eh? */

  return Qunbound;
}

static void
face_background_placement_validate (Lisp_Object instantiator)
{
  if (EQ (instantiator, Qabsolute) || EQ (instantiator, Qrelative))
    return;
  else if (VECTORP (instantiator) &&
	   (XVECTOR_LENGTH (instantiator) == 1))
    {
      Lisp_Object face = XVECTOR_DATA (instantiator)[0];

      Fget_face (face); /* just to check that the face exists -- dvl */
    }
  else if (VECTORP (instantiator))
    sferror ("Wrong length for background-placement inheritance spec",
	     instantiator);
  else
    invalid_argument
      ("\
Background-placement instantiator must be absolute, relative or vector",
       instantiator);
}

static void
face_background_placement_after_change (Lisp_Object specifier,
					Lisp_Object locale)
{
  Lisp_Object face
    = FACE_BACKGROUND_PLACEMENT_SPECIFIER_FACE
    (XFACE_BACKGROUND_PLACEMENT_SPECIFIER (specifier));

  if (!NILP (face))
    {
      face_property_was_changed (face, Qbackground_placement, locale);
      if (BUFFERP (locale))
	XBUFFER (locale)->buffer_local_face_property = 1;
    }
}

void
set_face_background_placement_attached_to (Lisp_Object obj, Lisp_Object face)
{
  Lisp_Specifier *face_background_placement
    = XFACE_BACKGROUND_PLACEMENT_SPECIFIER (obj);

  FACE_BACKGROUND_PLACEMENT_SPECIFIER_FACE (face_background_placement) = face;
}

DEFUN ("face-background-placement-specifier-p", Fface_background_placement_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a face-background-placement specifier.

See `make-face-background-placement-specifier' for a description of possible
face-background-placement instantiators.
*/
       (object))
{
  return FACE_BACKGROUND_PLACEMENT_SPECIFIERP (object) ? Qt : Qnil;
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_fontcolor (void)
{
  INIT_LISP_OBJECT (color_instance);
  INIT_LISP_OBJECT (font_instance);

  DEFSUBR (Fcolor_specifier_p);
  DEFSUBR (Ffont_specifier_p);
  DEFSUBR (Fface_boolean_specifier_p);
  DEFSUBR (Fface_background_placement_specifier_p);

  DEFSYMBOL_MULTIWORD_PREDICATE (Qcolor_instancep);
  DEFSUBR (Fmake_color_instance);
  DEFSUBR (Fcolor_instance_p);
  DEFSUBR (Fcolor_instance_name);
  DEFSUBR (Fcolor_instance_rgb_components);
  DEFSUBR (Fvalid_color_name_p);
  DEFSUBR (Fcolor_list);

  DEFSYMBOL_MULTIWORD_PREDICATE (Qfont_instancep);
  DEFSUBR (Fmake_font_instance);
  DEFSUBR (Ffont_instance_p);
  DEFSUBR (Ffont_instance_name);
  DEFSUBR (Ffont_instance_ascent);
  DEFSUBR (Ffont_instance_descent);
  DEFSUBR (Ffont_instance_width);
  DEFSUBR (Ffont_instance_charset);
  DEFSUBR (Ffont_instance_proportional_p);
  DEFSUBR (Ffont_instance_truename);
  DEFSUBR (Ffont_instance_properties);
  DEFSUBR (Ffont_list);

  /* Qcolor, Qfont defined in general.c */
  DEFSYMBOL (Qface_boolean);

  DEFSYMBOL (Qface_background_placement);
  DEFSYMBOL (Qabsolute);
  DEFSYMBOL (Qrelative);
}

void
specifier_type_create_fontcolor (void)
{
  INITIALIZE_SPECIFIER_TYPE_WITH_DATA (color, "color", "color-specifier-p");
  INITIALIZE_SPECIFIER_TYPE_WITH_DATA (font, "font", "font-specifier-p");
  INITIALIZE_SPECIFIER_TYPE_WITH_DATA (face_boolean, "face-boolean",
					 "face-boolean-specifier-p");
  INITIALIZE_SPECIFIER_TYPE_WITH_DATA (face_background_placement,
				       "face-background-placement",
				       "\
face-background-placement-specifier-p");

  SPECIFIER_HAS_METHOD (color, instantiate);
  SPECIFIER_HAS_METHOD (font, instantiate);
  SPECIFIER_HAS_METHOD (face_boolean, instantiate);
  SPECIFIER_HAS_METHOD (face_background_placement, instantiate);

  SPECIFIER_HAS_METHOD (color, validate);
  SPECIFIER_HAS_METHOD (font, validate);
  SPECIFIER_HAS_METHOD (face_boolean, validate);
  SPECIFIER_HAS_METHOD (face_background_placement, validate);

  SPECIFIER_HAS_METHOD (color, create);
  SPECIFIER_HAS_METHOD (font, create);
  SPECIFIER_HAS_METHOD (face_boolean, create);
  SPECIFIER_HAS_METHOD (face_background_placement, create);

  SPECIFIER_HAS_METHOD (color, after_change);
  SPECIFIER_HAS_METHOD (font, after_change);
  SPECIFIER_HAS_METHOD (face_boolean, after_change);
  SPECIFIER_HAS_METHOD (face_background_placement, after_change);

#ifdef MULE
  SPECIFIER_HAS_METHOD (font, validate_matchspec);
#endif
}

void
reinit_specifier_type_create_fontcolor (void)
{
  REINITIALIZE_SPECIFIER_TYPE (color);
  REINITIALIZE_SPECIFIER_TYPE (font);
  REINITIALIZE_SPECIFIER_TYPE (face_boolean);
  REINITIALIZE_SPECIFIER_TYPE (face_background_placement);
}

void
reinit_vars_of_fontcolor (void)
{
  {
    Lisp_Object obj = ALLOC_NORMAL_LISP_OBJECT (color_instance);
    Lisp_Color_Instance *c = XCOLOR_INSTANCE (obj);
    c->name = Qnil;
    c->device = Qnil;
    c->data = 0;

    Vthe_null_color_instance = obj;
    staticpro_nodump (&Vthe_null_color_instance);
  }

  {
    Lisp_Object obj = ALLOC_NORMAL_LISP_OBJECT (font_instance);
    Lisp_Font_Instance *f = XFONT_INSTANCE (obj);
    f->name = Qnil;
    f->truename = Qnil;
    f->device = Qnil;
    f->data = 0;

    f->ascent = f->height = 0;
    f->descent = 0;
    f->width = 0;
    f->proportional_p = 0;

    Vthe_null_font_instance = obj;
    staticpro_nodump (&Vthe_null_font_instance);
  }
}

void
vars_of_fontcolor (void)
{
}
