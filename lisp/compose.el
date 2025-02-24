;;; compose.el --- Compose-key processing in XEmacs

;; Copyright (C) 1992, 1993, 1997, 2005, 2025 Free Software Foundation, Inc.

;; Author: Jamie Zawinski <jwz@jwz.org>
;; Maintainer: XEmacs Development Team
;; Rewritten by Martin Buchholz far too many times.

;; Keywords: i18n

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.

;; You should have received a copy of the GNU General Public License
;; along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Synched up with: Not in FSF.

;;; Commentary:

;; created by jwz, 14-jun-92.
;;; changed by Jan Vroonhof, July 1997: Use function-key-map instead
;;;                                     of global map.
;;;                                     Preliminary support for
;;;                                     XFree86 deadkeys

;; This file implements DEC-, OpenWindows-, and HP-compatible "Compose"
;; processing for XEmacs.  It was formerly named x-compose.el.

;; If you are running a version of X which already does compose processing,
;; then you don't need this file.  But the MIT R4 and R5 distributions don't
;; do compose processing, so you may want to fake it by using this code.

;; The basic idea is that there are several ways to generate keysyms which
;; do not have keys devoted to them on your keyboard.

;; The first method is by using "dead" keys.  A dead key is a key which,
;; when typed, does not insert a character.  Instead it modifies the
;; following character typed.  So if you typed "dead-tilde" followed by "A",
;; then "A-tilde" would be inserted.  Of course, this requires you to modify
;; your keyboard to include a "dead-tilde" key on it somewhere.

;; The second method is by using a "Compose" key.  With a Compose key, you
;; would type "Compose" then "tilde" then "A" to insert "A-tilde".

;; There are a small number of dead keys: acute, grave, cedilla, diaeresis,
;; circumflex, tilde, and ring.  There are a larger number of accented and
;; other characters accessible via the Compose key, so both are useful.

;; To use this code, you will need to have a Compose key on your keyboard.
;; The default configuration of most X keyboards doesn't contain one.  You
;; can, for example, turn the right "Meta" key into a "Compose" key with
;; this command:

;;    xmodmap -e "remove mod1 = Meta_R" -e "keysym Meta_R = Multi_key"

;; Multi-key is the name that X (and emacs) know the "Compose" key by.
;; The "remove..." command is necessary because the "Compose" key must not
;; have any modifier bits associated with it.  This exact command may not
;; work, depending on what system and keyboard you are using.  If it
;; doesn't, you'll have to read the man page for xmodmap.  You might want
;; to get the "xkeycaps" program from
;; <URL:http://www.jwz.org/xkeycaps/>,
;; which is a graphical front end to xmodmap
;; that hides xmodmap's arcane syntax from you.

;; If for some reason you don't want to have a dedicated compose key on your
;; keyboard, you can use some other key as the prefix.  For example, to make
;; "Meta-Shift-C" act as a compose key (so that "M-C , c" would insert the
;; character "ccedilla") you could do

;;    (global-set-key "\M-C" compose-map)

;; I believe the bindings encoded in this file are the same as those used
;; by OpenWindows versions 2 and 3, and DEC VT320 terminals.  Please let me
;; know if you think otherwise.

;; Much thanks to Justin Bur <justin@crim.ca> for helping me understand how
;; this stuff is supposed to work.

;; You also might want to consider getting Justin's patch for the MIT Xlib
;; that implements compose processing in the library.  This will enable
;; compose processing in applications other than emacs as well.  You can
;; get it from export.lcs.mit.edu in contrib/compose.tar.Z.

;; This code has one feature that a more "builtin" Compose mechanism could
;; not have: at any point you can type C-h to get a list of the possible
;; completions of what you have typed so far.

;; ----------------------------------------------------------------------
;;
;; Notes from Aidan Kehoe, Thu Feb 12 16:21:18 GMT 2009 (these conflict to
;; some extent with the above):

;; Giacomo Boffi's problem of
;; 20050324103919.8D22E4901@boffi95.stru.polimi.it is caused by Xlib doing
;; the compose processing. To turn that off, you need to recompile without
;; XIM support, or start up XEmacs in a locale that the system supports but
;; X11 does not (for me, ru_RU.CP866 works for this). This will be
;; preferable anyway for some people, because the XIM support drops
;; sequences we would prefer to see. E.g. in the following situation, with
;; an XIM build:

;;    $ LC_CTYPE=de_DE.ISO8859-1 ./xemacs -vanilla &
      
;;    Input: dead-acute a 
;;    Seen by XEmacs: aacute (thanks to XIM)
;;    Action: U+00E1 is inserted in the buffer 
      
;;    Input: dead-abovedot o
;;    Seen by XEmacs: dead-abovedot o (XIM does not intervene, since no
;;                    characters in this locale are generated with
;;                    dead-abovedot)
;;    Action: U+022F is inserted in the buffer (thanks to this file)
      
;;    Input: dead-acute r
;;    Seen by XEmacs: nothing (thanks to XIM, it considers U+0155 unavailable)
;;    Action: nothing

;; Without XIM, all the above inputs would work fine, independent of your
;; locale.

;; Also, XIM does not intervene at all with the second or subsequent X11
;; devices created, and this file is needed for compose processing
;; there. This may be a bug in our use of XIM, or it may a bug in XIM
;; itself.

;; @@#### This should probably be integrated into general Unicode handling
;; of composition sequences.

;;; Code:

(macrolet
    ((define-compose-map (&rest keymap-symbols)
       (loop
         for keymap-symbol in keymap-symbols
         with result = nil
         do
	  ;; Required to tell XEmacs the keymaps were actually autoloaded.
	  ;; #### Make this unnecessary!
         (push `(fset ',keymap-symbol ,keymap-symbol) result)
         (push `(defconst ,keymap-symbol (make-sparse-keymap ',keymap-symbol))
               result)
         finally return (cons 'progn result))))
  (define-compose-map compose-map compose-acute-map compose-grave-map
    compose-cedilla-map compose-diaeresis-map compose-circumflex-map
    compose-tilde-map compose-ring-map compose-caron-map compose-macron-map
    compose-breve-map compose-dot-map compose-doubleacute-map
    compose-ogonek-map compose-hook-map compose-horn-map compose-stroke-map))

(define-key compose-map 'acute	    compose-acute-map)
(define-key compose-map 'grave	    compose-grave-map)
(define-key compose-map 'cedilla    compose-cedilla-map)
(define-key compose-map 'diaeresis  compose-diaeresis-map)
(define-key compose-map 'circumflex compose-circumflex-map)
(define-key compose-map 'tilde      compose-tilde-map)
(define-key compose-map 'degree	    compose-ring-map)
(define-key compose-map 'caron	    compose-caron-map)
(define-key compose-map 'macron	    compose-macron-map)
(define-key compose-map 'doubleacute compose-doubleacute-map)
(define-key compose-map 'ogonek     compose-ogonek-map)
(define-key compose-map 'breve      compose-breve-map)
(define-key compose-map 'abovedot   compose-dot-map)
(define-key compose-map 'stroke     compose-stroke-map)

;;(define-key function-key-map [multi-key] compose-map)

(define-key compose-map [acute]		compose-acute-map)
(define-key compose-map [?']		compose-acute-map)
(define-key compose-map [grave]		compose-grave-map)
(define-key compose-map [?`]		compose-grave-map)
(define-key compose-map [cedilla]	compose-cedilla-map)
(define-key compose-map [?,]		compose-cedilla-map)
(define-key compose-map [diaeresis]	compose-diaeresis-map)
(define-key compose-map [?\"]		compose-diaeresis-map)
(define-key compose-map [circumflex]	compose-circumflex-map)
(define-key compose-map [?^]		compose-circumflex-map)
(define-key compose-map [tilde]		compose-tilde-map)
(define-key compose-map [~]		compose-tilde-map)
(define-key compose-map [degree]	compose-ring-map)
(define-key compose-map [?*]		compose-ring-map)
(define-key compose-map [stroke]		compose-stroke-map)

(loop
  for (keysym character map)
  in '((caron ?\u02C7 compose-caron-map)
       (macron ?\u00AF compose-macron-map)
       (doubleacute ?\u02DD compose-doubleacute-map)
       (ogonek ?\u02db compose-ogonek-map)
       (breve ?\u0306 compose-breve-map)
       (abovedot ?\u0307 compose-dot-map)
       (U031b ?\u031b compose-horn-map))
  do
  (define-key compose-map (vector keysym) map)
  (define-key compose-map (vector (list character)) map))


;;; The contents of the "dead key" maps.  These are shared by the
;;; compose-map.

;;; Against the spirit of Unicode, which says that the precomposed
;;; characters are just there for round-trip compatibility with other
;;; encodings and don't reflect that they're necessarily used much, these
;;; are just the precomposed Latin characters in UnicodeData.txt; we don't
;;; support any decomposed characters here. (Not least because in general we
;;; don't have worthwhile support for precomposed characters.)

(define-key compose-acute-map [space] [(?\u0027)]) ;; APOSTROPHE
(define-key compose-acute-map [?\'] [(?\u00B4)]) ;; ACUTE ACCENT
(define-key compose-acute-map [?A] [(?\u00C1)]) ;; CAPITAL A WITH ACUTE
(define-key compose-acute-map [?C] [(?\u0106)]) ;; CAPITAL C WITH ACUTE
(define-key compose-acute-map [?E] [(?\u00C9)]) ;; CAPITAL E WITH ACUTE
(define-key compose-acute-map [?G] [(?\u01F4)]) ;; CAPITAL G WITH ACUTE
(define-key compose-acute-map [?I] [(?\u00CD)]) ;; CAPITAL I WITH ACUTE
(define-key compose-acute-map [?K] [(?\u1E30)]) ;; CAPITAL K WITH ACUTE
(define-key compose-acute-map [?L] [(?\u0139)]) ;; CAPITAL L WITH ACUTE
(define-key compose-acute-map [?M] [(?\u1E3E)]) ;; CAPITAL M WITH ACUTE
(define-key compose-acute-map [?N] [(?\u0143)]) ;; CAPITAL N WITH ACUTE
(define-key compose-acute-map [?O] [(?\u00D3)]) ;; CAPITAL O WITH ACUTE
(define-key compose-acute-map [?P] [(?\u1E54)]) ;; CAPITAL P WITH ACUTE
(define-key compose-acute-map [?R] [(?\u0154)]) ;; CAPITAL R WITH ACUTE
(define-key compose-acute-map [?S] [(?\u015A)]) ;; CAPITAL S WITH ACUTE
(define-key compose-acute-map [?U] [(?\u00DA)]) ;; CAPITAL U WITH ACUTE
(define-key compose-acute-map [?W] [(?\u1E82)]) ;; CAPITAL W WITH ACUTE
(define-key compose-acute-map [?Y] [(?\u00DD)]) ;; CAPITAL Y WITH ACUTE
(define-key compose-acute-map [?Z] [(?\u0179)]) ;; CAPITAL Z WITH ACUTE
(define-key compose-acute-map [?a] [(?\u00E1)]) ;; SMALL A WITH ACUTE
(define-key compose-acute-map [?c] [(?\u0107)]) ;; SMALL C WITH ACUTE
(define-key compose-acute-map [?e] [(?\u00E9)]) ;; SMALL E WITH ACUTE
(define-key compose-acute-map [?g] [(?\u01F5)]) ;; SMALL G WITH ACUTE
(define-key compose-acute-map [?i] [(?\u00ED)]) ;; SMALL I WITH ACUTE
(define-key compose-acute-map [?k] [(?\u1E31)]) ;; SMALL K WITH ACUTE
(define-key compose-acute-map [?l] [(?\u013A)]) ;; SMALL L WITH ACUTE
(define-key compose-acute-map [?m] [(?\u1E3F)]) ;; SMALL M WITH ACUTE
(define-key compose-acute-map [?n] [(?\u0144)]) ;; SMALL N WITH ACUTE
(define-key compose-acute-map [?o] [(?\u00F3)]) ;; SMALL O WITH ACUTE
(define-key compose-acute-map [?p] [(?\u1E55)]) ;; SMALL P WITH ACUTE
(define-key compose-acute-map [?r] [(?\u0155)]) ;; SMALL R WITH ACUTE
(define-key compose-acute-map [?s] [(?\u015B)]) ;; SMALL S WITH ACUTE
(define-key compose-acute-map [?u] [(?\u00FA)]) ;; SMALL U WITH ACUTE
(define-key compose-acute-map [?w] [(?\u1E83)]) ;; SMALL W WITH ACUTE
(define-key compose-acute-map [?y] [(?\u00FD)]) ;; SMALL Y WITH ACUTE
(define-key compose-acute-map [?z] [(?\u017A)]) ;; SMALL Z WITH ACUTE

(define-key compose-grave-map [space] [(?\u0060)]) ;; GRAVE ACCENT
(define-key compose-grave-map [?\`] [(?\u0060)]) ;; GRAVE ACCENT
(define-key compose-grave-map [?A] [(?\u00C0)]) ;; CAPITAL A WITH GRAVE
(define-key compose-grave-map [?E] [(?\u00C8)]) ;; CAPITAL E WITH GRAVE
(define-key compose-grave-map [?I] [(?\u00CC)]) ;; CAPITAL I WITH GRAVE
(define-key compose-grave-map [?N] [(?\u01F8)]) ;; CAPITAL N WITH GRAVE
(define-key compose-grave-map [?O] [(?\u00D2)]) ;; CAPITAL O WITH GRAVE
(define-key compose-grave-map [?U] [(?\u00D9)]) ;; CAPITAL U WITH GRAVE
(define-key compose-grave-map [?W] [(?\u1E80)]) ;; CAPITAL W WITH GRAVE
(define-key compose-grave-map [?Y] [(?\u1EF2)]) ;; CAPITAL Y WITH GRAVE
(define-key compose-grave-map [?a] [(?\u00E0)]) ;; SMALL A WITH GRAVE
(define-key compose-grave-map [?e] [(?\u00E8)]) ;; SMALL E WITH GRAVE
(define-key compose-grave-map [?i] [(?\u00EC)]) ;; SMALL I WITH GRAVE
(define-key compose-grave-map [?n] [(?\u01F9)]) ;; SMALL N WITH GRAVE
(define-key compose-grave-map [?o] [(?\u00F2)]) ;; SMALL O WITH GRAVE
(define-key compose-grave-map [?u] [(?\u00F9)]) ;; SMALL U WITH GRAVE
(define-key compose-grave-map [?w] [(?\u1E81)]) ;; SMALL W WITH GRAVE
(define-key compose-grave-map [?y] [(?\u1EF3)]) ;; SMALL Y WITH GRAVE

(define-key compose-cedilla-map [space] [(?\u002C)]) ;; COMMA
(define-key compose-cedilla-map [?\,] [(?\u00B8)]) ;; CEDILLA
(define-key compose-cedilla-map [C] [(?\u00C7)]) ;; CAPITAL C WITH CEDILLA
(define-key compose-cedilla-map [D] [(?\u1E10)]) ;; CAPITAL D WITH CEDILLA
(define-key compose-cedilla-map [E] [(?\u0228)]) ;; CAPITAL E WITH CEDILLA
(define-key compose-cedilla-map [G] [(?\u0122)]) ;; CAPITAL G WITH CEDILLA
(define-key compose-cedilla-map [H] [(?\u1E28)]) ;; CAPITAL H WITH CEDILLA
(define-key compose-cedilla-map [K] [(?\u0136)]) ;; CAPITAL K WITH CEDILLA
(define-key compose-cedilla-map [L] [(?\u013B)]) ;; CAPITAL L WITH CEDILLA
(define-key compose-cedilla-map [N] [(?\u0145)]) ;; CAPITAL N WITH CEDILLA
(define-key compose-cedilla-map [R] [(?\u0156)]) ;; CAPITAL R WITH CEDILLA
(define-key compose-cedilla-map [S] [(?\u015E)]) ;; CAPITAL S WITH CEDILLA
(define-key compose-cedilla-map [T] [(?\u0162)]) ;; CAPITAL T WITH CEDILLA
(define-key compose-cedilla-map [c] [(?\u00E7)]) ;; SMALL C WITH CEDILLA
(define-key compose-cedilla-map [d] [(?\u1E11)]) ;; SMALL D WITH CEDILLA
(define-key compose-cedilla-map [e] [(?\u0229)]) ;; SMALL E WITH CEDILLA
(define-key compose-cedilla-map [g] [(?\u0123)]) ;; SMALL G WITH CEDILLA
(define-key compose-cedilla-map [h] [(?\u1E29)]) ;; SMALL H WITH CEDILLA
(define-key compose-cedilla-map [k] [(?\u0137)]) ;; SMALL K WITH CEDILLA
(define-key compose-cedilla-map [l] [(?\u013C)]) ;; SMALL L WITH CEDILLA
(define-key compose-cedilla-map [n] [(?\u0146)]) ;; SMALL N WITH CEDILLA
(define-key compose-cedilla-map [r] [(?\u0157)]) ;; SMALL R WITH CEDILLA
(define-key compose-cedilla-map [s] [(?\u015F)]) ;; SMALL S WITH CEDILLA
(define-key compose-cedilla-map [t] [(?\u0163)]) ;; SMALL T WITH CEDILLA

(define-key compose-diaeresis-map [space] [(?\u00A8)]) ;; DIAERESIS
(define-key compose-diaeresis-map [?\"] [(?\u00A8)]) ;; DIAERESIS
(define-key compose-diaeresis-map [?s] [(?\u00DF)]) ;; SMALL SHARP S
(define-key compose-diaeresis-map [?A] [(?\u00C4)]) ;; CAPITAL A WITH DIAERESIS
(define-key compose-diaeresis-map [?E] [(?\u00CB)]) ;; CAPITAL E WITH DIAERESIS
(define-key compose-diaeresis-map [?H] [(?\u1E26)]) ;; CAPITAL H WITH DIAERESIS
(define-key compose-diaeresis-map [?I] [(?\u00CF)]) ;; CAPITAL I WITH DIAERESIS
(define-key compose-diaeresis-map [?O] [(?\u00D6)]) ;; CAPITAL O WITH DIAERESIS
(define-key compose-diaeresis-map [?U] [(?\u00DC)]) ;; CAPITAL U WITH DIAERESIS
(define-key compose-diaeresis-map [?W] [(?\u1E84)]) ;; CAPITAL W WITH DIAERESIS
(define-key compose-diaeresis-map [?X] [(?\u1E8C)]) ;; CAPITAL X WITH DIAERESIS
(define-key compose-diaeresis-map [?Y] [(?\u0178)]) ;; CAPITAL Y WITH DIAERESIS
(define-key compose-diaeresis-map [?a] [(?\u00E4)]) ;; SMALL A WITH DIAERESIS
(define-key compose-diaeresis-map [?e] [(?\u00EB)]) ;; SMALL E WITH DIAERESIS
(define-key compose-diaeresis-map [?h] [(?\u1E27)]) ;; SMALL H WITH DIAERESIS
(define-key compose-diaeresis-map [?i] [(?\u00EF)]) ;; SMALL I WITH DIAERESIS
(define-key compose-diaeresis-map [?o] [(?\u00F6)]) ;; SMALL O WITH DIAERESIS
(define-key compose-diaeresis-map [?t] [(?\u1E97)]) ;; SMALL T WITH DIAERESIS
(define-key compose-diaeresis-map [?u] [(?\u00FC)]) ;; SMALL U WITH DIAERESIS
(define-key compose-diaeresis-map [?w] [(?\u1E85)]) ;; SMALL W WITH DIAERESIS
(define-key compose-diaeresis-map [?x] [(?\u1E8D)]) ;; SMALL X WITH DIAERESIS
(define-key compose-diaeresis-map [?y] [(?\u00FF)]) ;; SMALL Y WITH DIAERESIS

(define-key compose-circumflex-map [space] [(?\u005e)]) ;; CIRCUMFLEX ACCENT
(define-key compose-circumflex-map [?A] [(?\u00C2)]) ;; CAPITAL A WITH CIRCUMFLEX
(define-key compose-circumflex-map [?C] [(?\u0108)]) ;; CAPITAL C WITH CIRCUMFLEX
(define-key compose-circumflex-map [?E] [(?\u00CA)]) ;; CAPITAL E WITH CIRCUMFLEX
(define-key compose-circumflex-map [?G] [(?\u011C)]) ;; CAPITAL G WITH CIRCUMFLEX
(define-key compose-circumflex-map [?H] [(?\u0124)]) ;; CAPITAL H WITH CIRCUMFLEX
(define-key compose-circumflex-map [?I] [(?\u00CE)]) ;; CAPITAL I WITH CIRCUMFLEX
(define-key compose-circumflex-map [?J] [(?\u0134)]) ;; CAPITAL J WITH CIRCUMFLEX
(define-key compose-circumflex-map [?O] [(?\u00D4)]) ;; CAPITAL O WITH CIRCUMFLEX
(define-key compose-circumflex-map [?S] [(?\u015C)]) ;; CAPITAL S WITH CIRCUMFLEX
(define-key compose-circumflex-map [?U] [(?\u00DB)]) ;; CAPITAL U WITH CIRCUMFLEX
(define-key compose-circumflex-map [?W] [(?\u0174)]) ;; CAPITAL W WITH CIRCUMFLEX
(define-key compose-circumflex-map [?Y] [(?\u0176)]) ;; CAPITAL Y WITH CIRCUMFLEX
(define-key compose-circumflex-map [?Z] [(?\u1E90)]) ;; CAPITAL Z WITH CIRCUMFLEX
(define-key compose-circumflex-map [?a] [(?\u00e2)]) ;; SMALL A WITH CIRCUMFLEX
(define-key compose-circumflex-map [?c] [(?\u0109)]) ;; SMALL C WITH CIRCUMFLEX
(define-key compose-circumflex-map [?e] [(?\u00ea)]) ;; SMALL E WITH CIRCUMFLEX
(define-key compose-circumflex-map [?g] [(?\u011d)]) ;; SMALL G WITH CIRCUMFLEX
(define-key compose-circumflex-map [?h] [(?\u0125)]) ;; SMALL H WITH CIRCUMFLEX
(define-key compose-circumflex-map [?i] [(?\u00ee)]) ;; SMALL I WITH CIRCUMFLEX
(define-key compose-circumflex-map [?j] [(?\u0135)]) ;; SMALL J WITH CIRCUMFLEX
(define-key compose-circumflex-map [?o] [(?\u00f4)]) ;; SMALL O WITH CIRCUMFLEX
(define-key compose-circumflex-map [?s] [(?\u015d)]) ;; SMALL S WITH CIRCUMFLEX
(define-key compose-circumflex-map [?u] [(?\u00fb)]) ;; SMALL U WITH CIRCUMFLEX
(define-key compose-circumflex-map [?w] [(?\u0175)]) ;; SMALL W WITH CIRCUMFLEX
(define-key compose-circumflex-map [?y] [(?\u0177)]) ;; SMALL Y WITH CIRCUMFLEX
(define-key compose-circumflex-map [?z] [(?\u1e91)]) ;; SMALL Z WITH CIRCUMFLEX

(define-key compose-tilde-map [space] [(?\u007E)]) ;; TILDE
(define-key compose-tilde-map [?A] [(?\u00C3)]) ;; CAPITAL A WITH TILDE
(define-key compose-tilde-map [?E] [(?\u1EBC)]) ;; CAPITAL E WITH TILDE
(define-key compose-tilde-map [?I] [(?\u0128)]) ;; CAPITAL I WITH TILDE
(define-key compose-tilde-map [?N] [(?\u00D1)]) ;; CAPITAL N WITH TILDE
(define-key compose-tilde-map [?O] [(?\u00D5)]) ;; CAPITAL O WITH TILDE
(define-key compose-tilde-map [?U] [(?\u0168)]) ;; CAPITAL U WITH TILDE
(define-key compose-tilde-map [?V] [(?\u1E7C)]) ;; CAPITAL V WITH TILDE
(define-key compose-tilde-map [?Y] [(?\u1EF8)]) ;; CAPITAL Y WITH TILDE
(define-key compose-tilde-map [?a] [(?\u00E3)]) ;; SMALL A WITH TILDE
(define-key compose-tilde-map [?e] [(?\u1EBD)]) ;; SMALL E WITH TILDE
(define-key compose-tilde-map [?i] [(?\u0129)]) ;; SMALL I WITH TILDE
(define-key compose-tilde-map [?n] [(?\u00F1)]) ;; SMALL N WITH TILDE
(define-key compose-tilde-map [?o] [(?\u00F5)]) ;; SMALL O WITH TILDE
(define-key compose-tilde-map [?u] [(?\u0169)]) ;; SMALL U WITH TILDE
(define-key compose-tilde-map [?v] [(?\u1E7D)]) ;; SMALL V WITH TILDE
(define-key compose-tilde-map [?y] [(?\u1EF9)]) ;; SMALL Y WITH TILDE

(define-key compose-ring-map [space] [(?\u00B0)]) ;; DEGREE SIGN
(define-key compose-ring-map [?A] [(?\u00C5)]) ;; CAPITAL A WITH RING ABOVE
(define-key compose-ring-map [?U] [(?\u016E)]) ;; CAPITAL U WITH RING ABOVE
(define-key compose-ring-map [?a] [(?\u00E5)]) ;; SMALL A WITH RING ABOVE
(define-key compose-ring-map [?u] [(?\u016F)]) ;; SMALL U WITH RING ABOVE
(define-key compose-ring-map [?w] [(?\u1E98)]) ;; SMALL W WITH RING ABOVE
(define-key compose-ring-map [?y] [(?\u1E99)]) ;; SMALL Y WITH RING ABOVE

(define-key compose-caron-map [space] [(?\u02C7)]) ;; CARON
(define-key compose-caron-map [?A] [(?\u01CD)]) ;; CAPITAL A WITH CARON
(define-key compose-caron-map [?C] [(?\u010C)]) ;; CAPITAL C WITH CARON
(define-key compose-caron-map [?D] [(?\u010E)]) ;; CAPITAL D WITH CARON
(define-key compose-caron-map [?\u01f1] [(?\u01C4)]) ;; CAPITAL DZ WITH CARON
(define-key compose-caron-map [?E] [(?\u011A)]) ;; CAPITAL E WITH CARON
(define-key compose-caron-map [?\u01b7] [(?\u01EE)]) ;; CAPITAL EZH WITH CARON
(define-key compose-caron-map [?G] [(?\u01E6)]) ;; CAPITAL G WITH CARON
(define-key compose-caron-map [?H] [(?\u021E)]) ;; CAPITAL H WITH CARON
(define-key compose-caron-map [?I] [(?\u01CF)]) ;; CAPITAL I WITH CARON
(define-key compose-caron-map [?K] [(?\u01E8)]) ;; CAPITAL K WITH CARON
(define-key compose-caron-map [?L] [(?\u013D)]) ;; CAPITAL L WITH CARON
(define-key compose-caron-map [?N] [(?\u0147)]) ;; CAPITAL N WITH CARON
(define-key compose-caron-map [?O] [(?\u01D1)]) ;; CAPITAL O WITH CARON
(define-key compose-caron-map [?R] [(?\u0158)]) ;; CAPITAL R WITH CARON
(define-key compose-caron-map [?S] [(?\u0160)]) ;; CAPITAL S WITH CARON
(define-key compose-caron-map [?T] [(?\u0164)]) ;; CAPITAL T WITH CARON
(define-key compose-caron-map [?U] [(?\u01D3)]) ;; CAPITAL U WITH CARON
(define-key compose-caron-map [?Z] [(?\u017D)]) ;; CAPITAL Z WITH CARON
(define-key compose-caron-map [?a] [(?\u01CE)]) ;; SMALL A WITH CARON
(define-key compose-caron-map [?c] [(?\u010D)]) ;; SMALL C WITH CARON
(define-key compose-caron-map [?d] [(?\u010F)]) ;; SMALL D WITH CARON
(define-key compose-caron-map [?\u01F3] [(?\u01C6)]) ;; SMALL DZ WITH CARON
(define-key compose-caron-map [?e] [(?\u011B)]) ;; SMALL E WITH CARON
(define-key compose-caron-map [?\u0292] [(?\u01EF)]) ;; SMALL EZH WITH CARON
(define-key compose-caron-map [?g] [(?\u01E7)]) ;; SMALL G WITH CARON
(define-key compose-caron-map [?h] [(?\u021F)]) ;; SMALL H WITH CARON
(define-key compose-caron-map [?i] [(?\u01D0)]) ;; SMALL I WITH CARON
(define-key compose-caron-map [?j] [(?\u01F0)]) ;; SMALL J WITH CARON
(define-key compose-caron-map [?k] [(?\u01E9)]) ;; SMALL K WITH CARON
(define-key compose-caron-map [?l] [(?\u013E)]) ;; SMALL L WITH CARON
(define-key compose-caron-map [?n] [(?\u0148)]) ;; SMALL N WITH CARON
(define-key compose-caron-map [?o] [(?\u01D2)]) ;; SMALL O WITH CARON
(define-key compose-caron-map [?r] [(?\u0159)]) ;; SMALL R WITH CARON
(define-key compose-caron-map [?s] [(?\u0161)]) ;; SMALL S WITH CARON
(define-key compose-caron-map [?t] [(?\u0165)]) ;; SMALL T WITH CARON
(define-key compose-caron-map [?u] [(?\u01D4)]) ;; SMALL U WITH CARON
(define-key compose-caron-map [?z] [(?\u017E)]) ;; SMALL Z WITH CARON

(define-key compose-macron-map [space] [(?\u00AF)]) ;; MACRON
(define-key compose-macron-map [?A] [(?\u0100)]) ;; CAPITAL A WITH MACRON  
(define-key compose-macron-map [AE] [(?\u01E2)]) ;; CAPITAL AE WITH MACRON 
(define-key compose-macron-map [?E] [(?\u0112)]) ;; CAPITAL E WITH MACRON  
(define-key compose-macron-map [?G] [(?\u1E20)]) ;; CAPITAL G WITH MACRON  
(define-key compose-macron-map [?I] [(?\u012A)]) ;; CAPITAL I WITH MACRON  
(define-key compose-macron-map [?O] [(?\u014C)]) ;; CAPITAL O WITH MACRON  
(define-key compose-macron-map [?U] [(?\u016A)]) ;; CAPITAL U WITH MACRON  
(define-key compose-macron-map [?Y] [(?\u0232)]) ;; CAPITAL Y WITH MACRON  
(define-key compose-macron-map [?a] [(?\u0101)]) ;; SMALL A WITH MACRON    
(define-key compose-macron-map [ae] [(?\u01E3)]) ;; SMALL AE WITH MACRON   
(define-key compose-macron-map [?e] [(?\u0113)]) ;; SMALL E WITH MACRON    
(define-key compose-macron-map [?g] [(?\u1E21)]) ;; SMALL G WITH MACRON    
(define-key compose-macron-map [?i] [(?\u012B)]) ;; SMALL I WITH MACRON    
(define-key compose-macron-map [?o] [(?\u014D)]) ;; SMALL O WITH MACRON    
(define-key compose-macron-map [?u] [(?\u016B)]) ;; SMALL U WITH MACRON    
(define-key compose-macron-map [?y] [(?\u0233)]) ;; SMALL Y WITH MACRON    

(define-key compose-doubleacute-map [space] [(?\u02DD)]) ;; DOUBLE ACUTE ACCENT
(define-key compose-doubleacute-map [?O] [(?\u0150)]) ;; CAPITAL O WITH DOUBLE ACUTE
(define-key compose-doubleacute-map [?U] [(?\u0170)]) ;; CAPITAL U WITH DOUBLE ACUTE
(define-key compose-doubleacute-map [?o] [(?\u0151)]) ;; SMALL O WITH DOUBLE ACUTE
(define-key compose-doubleacute-map [?u] [(?\u0171)]) ;; SMALL U WITH DOUBLE ACUTE

(define-key compose-ogonek-map [space] [(?\u02DB)]) ;; OGONEK
(define-key compose-ogonek-map [?A] [(?\u0104)]) ;; CAPITAL A WITH OGONEK
(define-key compose-ogonek-map [?E] [(?\u0118)]) ;; CAPITAL E WITH OGONEK
(define-key compose-ogonek-map [?I] [(?\u012E)]) ;; CAPITAL I WITH OGONEK
(define-key compose-ogonek-map [?O] [(?\u01EA)]) ;; CAPITAL O WITH OGONEK
(define-key compose-ogonek-map [?U] [(?\u0172)]) ;; CAPITAL U WITH OGONEK
(define-key compose-ogonek-map [?a] [(?\u0105)]) ;; SMALL A WITH OGONEK
(define-key compose-ogonek-map [?e] [(?\u0119)]) ;; SMALL E WITH OGONEK
(define-key compose-ogonek-map [?i] [(?\u012F)]) ;; SMALL I WITH OGONEK
(define-key compose-ogonek-map [?o] [(?\u01EB)]) ;; SMALL O WITH OGONEK
(define-key compose-ogonek-map [?u] [(?\u0173)]) ;; SMALL U WITH OGONEK

(define-key compose-breve-map [space] [(?\u02D8)]) ;; BREVE
(define-key compose-breve-map [?A] [(?\u0102)]) ;; CAPITAL A WITH BREVE
(define-key compose-breve-map [?E] [(?\u0114)]) ;; CAPITAL E WITH BREVE
(define-key compose-breve-map [?G] [(?\u011E)]) ;; CAPITAL G WITH BREVE
(define-key compose-breve-map [?I] [(?\u012C)]) ;; CAPITAL I WITH BREVE
(define-key compose-breve-map [?O] [(?\u014E)]) ;; CAPITAL O WITH BREVE
(define-key compose-breve-map [?U] [(?\u016C)]) ;; CAPITAL U WITH BREVE
(define-key compose-breve-map [?a] [(?\u0103)]) ;; SMALL A WITH BREVE
(define-key compose-breve-map [?e] [(?\u0115)]) ;; SMALL E WITH BREVE
(define-key compose-breve-map [?g] [(?\u011F)]) ;; SMALL G WITH BREVE
(define-key compose-breve-map [?i] [(?\u012D)]) ;; SMALL I WITH BREVE
(define-key compose-breve-map [?o] [(?\u014F)]) ;; SMALL O WITH BREVE
(define-key compose-breve-map [?u] [(?\u016D)]) ;; SMALL U WITH BREVE

(define-key compose-dot-map [space] [(?\u02D9)]) ;; DOT ABOVE
(define-key compose-dot-map [?A] [(?\u0226)]) ;; CAPITAL A WITH DOT ABOVE
(define-key compose-dot-map [?B] [(?\u1E02)]) ;; CAPITAL B WITH DOT ABOVE
(define-key compose-dot-map [?C] [(?\u010A)]) ;; CAPITAL C WITH DOT ABOVE
(define-key compose-dot-map [?D] [(?\u1E0A)]) ;; CAPITAL D WITH DOT ABOVE
(define-key compose-dot-map [?E] [(?\u0116)]) ;; CAPITAL E WITH DOT ABOVE
(define-key compose-dot-map [?F] [(?\u1E1E)]) ;; CAPITAL F WITH DOT ABOVE
(define-key compose-dot-map [?G] [(?\u0120)]) ;; CAPITAL G WITH DOT ABOVE
(define-key compose-dot-map [?H] [(?\u1E22)]) ;; CAPITAL H WITH DOT ABOVE
(define-key compose-dot-map [?I] [(?\u0130)]) ;; CAPITAL I WITH DOT ABOVE
(define-key compose-dot-map [?M] [(?\u1E40)]) ;; CAPITAL M WITH DOT ABOVE
(define-key compose-dot-map [?N] [(?\u1E44)]) ;; CAPITAL N WITH DOT ABOVE
(define-key compose-dot-map [?O] [(?\u022E)]) ;; CAPITAL O WITH DOT ABOVE
(define-key compose-dot-map [?P] [(?\u1E56)]) ;; CAPITAL P WITH DOT ABOVE
(define-key compose-dot-map [?R] [(?\u1E58)]) ;; CAPITAL R WITH DOT ABOVE
(define-key compose-dot-map [?S] [(?\u1E60)]) ;; CAPITAL S WITH DOT ABOVE
(define-key compose-dot-map [?T] [(?\u1E6A)]) ;; CAPITAL T WITH DOT ABOVE
(define-key compose-dot-map [?W] [(?\u1E86)]) ;; CAPITAL W WITH DOT ABOVE
(define-key compose-dot-map [?X] [(?\u1E8A)]) ;; CAPITAL X WITH DOT ABOVE
(define-key compose-dot-map [?Y] [(?\u1E8E)]) ;; CAPITAL Y WITH DOT ABOVE
(define-key compose-dot-map [?Z] [(?\u017B)]) ;; CAPITAL Z WITH DOT ABOVE
(define-key compose-dot-map [?a] [(?\u0227)]) ;; SMALL A WITH DOT ABOVE
(define-key compose-dot-map [?b] [(?\u1E03)]) ;; SMALL B WITH DOT ABOVE
(define-key compose-dot-map [?c] [(?\u010B)]) ;; SMALL C WITH DOT ABOVE
(define-key compose-dot-map [?d] [(?\u1E0B)]) ;; SMALL D WITH DOT ABOVE
(define-key compose-dot-map [?e] [(?\u0117)]) ;; SMALL E WITH DOT ABOVE
(define-key compose-dot-map [?f] [(?\u1E1F)]) ;; SMALL F WITH DOT ABOVE
(define-key compose-dot-map [?g] [(?\u0121)]) ;; SMALL G WITH DOT ABOVE
(define-key compose-dot-map [?h] [(?\u1E23)]) ;; SMALL H WITH DOT ABOVE
(define-key compose-dot-map [?\u017F] [(?\u1E9B)]) ;; SMALL LONG S WITH DOT ABOVE
(define-key compose-dot-map [?m] [(?\u1E41)]) ;; SMALL M WITH DOT ABOVE
(define-key compose-dot-map [?n] [(?\u1E45)]) ;; SMALL N WITH DOT ABOVE
(define-key compose-dot-map [?o] [(?\u022F)]) ;; SMALL O WITH DOT ABOVE
(define-key compose-dot-map [?p] [(?\u1E57)]) ;; SMALL P WITH DOT ABOVE
(define-key compose-dot-map [?r] [(?\u1E59)]) ;; SMALL R WITH DOT ABOVE
(define-key compose-dot-map [?s] [(?\u1E61)]) ;; SMALL S WITH DOT ABOVE
(define-key compose-dot-map [?t] [(?\u1E6B)]) ;; SMALL T WITH DOT ABOVE
(define-key compose-dot-map [?w] [(?\u1E87)]) ;; SMALL W WITH DOT ABOVE
(define-key compose-dot-map [?x] [(?\u1E8B)]) ;; SMALL X WITH DOT ABOVE
(define-key compose-dot-map [?y] [(?\u1E8F)]) ;; SMALL Y WITH DOT ABOVE
(define-key compose-dot-map [?z] [(?\u017C)]) ;; SMALL Z WITH DOT ABOVE
(define-key compose-dot-map [?i] [(?\u0131)]) ;; SMALL DOTLESS I
(define-key compose-dot-map [?j] [(?\u0237)]) ;; SMALL DOTLESS J

;; There is nothing obvious we can bind space to on compose-hook-map, these are
;; IPA characters that are in Unicode theory not precomposed.
(define-key compose-hook-map [?B] [(?\u0181)]) ;; CAPITAL B WITH HOOK
(define-key compose-hook-map [?C] [(?\u0187)]) ;; CAPITAL C WITH HOOK
(define-key compose-hook-map [?D] [(?\u018A)]) ;; CAPITAL D WITH HOOK
(define-key compose-hook-map [?F] [(?\u0191)]) ;; CAPITAL F WITH HOOK
(define-key compose-hook-map [?G] [(?\u0193)]) ;; CAPITAL G WITH HOOK
(define-key compose-hook-map [?K] [(?\u0198)]) ;; CAPITAL K WITH HOOK
(define-key compose-hook-map [?P] [(?\u01A4)]) ;; CAPITAL P WITH HOOK
(define-key compose-hook-map [?T] [(?\u01AC)]) ;; CAPITAL T WITH HOOK
(define-key compose-hook-map [?V] [(?\u01B2)]) ;; CAPITAL V WITH HOOK
(define-key compose-hook-map [?Y] [(?\u01B3)]) ;; CAPITAL Y WITH HOOK
(define-key compose-hook-map [?Z] [(?\u0224)]) ;; CAPITAL Z WITH HOOK
(define-key compose-hook-map [?\u0262] [(?\u029B)]) ;; SMALL CAPITAL G WITH HOOK
(define-key compose-hook-map [?b] [(?\u0253)]) ;; SMALL B WITH HOOK
(define-key compose-hook-map [?c] [(?\u0188)]) ;; SMALL C WITH HOOK
(define-key compose-hook-map [?d] [(?\u0257)]) ;; SMALL D WITH HOOK
(define-key compose-hook-map [?f] [(?\u0192)]) ;; SMALL F WITH HOOK
(define-key compose-hook-map [?g] [(?\u0260)]) ;; SMALL G WITH HOOK
(define-key compose-hook-map [?h] [(?\u0266)]) ;; SMALL H WITH HOOK
(define-key compose-hook-map [?\u0266] [(?\u0267)]) ;; SMALL HENG WITH HOOK
(define-key compose-hook-map [?k] [(?\u0199)]) ;; SMALL K WITH HOOK
(define-key compose-hook-map [?m] [(?\u0271)]) ;; SMALL M WITH HOOK
(define-key compose-hook-map [?p] [(?\u01A5)]) ;; SMALL P WITH HOOK
(define-key compose-hook-map [?q] [(?\u02A0)]) ;; SMALL Q WITH HOOK
(define-key compose-hook-map [?\u025C] [(?\u025D)]) ;; SMALL REVERSED OPEN E WITH HOOK
(define-key compose-hook-map [?s] [(?\u0282)]) ;; SMALL S WITH HOOK
(define-key compose-hook-map [?\u0259] [(?\u025A)]) ;; SMALL SCHWA WITH HOOK
(define-key compose-hook-map [?t] [(?\u01AD)]) ;; SMALL T WITH HOOK
(define-key compose-hook-map [?\u0279] [(?\u027B)]) ;; SMALL TURNED R WITH HOOK
(define-key compose-hook-map [?v] [(?\u028B)]) ;; SMALL V WITH HOOK
(define-key compose-hook-map [?y] [(?\u01B4)]) ;; SMALL Y WITH HOOK
(define-key compose-hook-map [?z] [(?\u0225)]) ;; SMALL Z WITH HOOK

(define-key compose-horn-map [space] [(?\u031b)])
(define-key compose-horn-map [?O] [(?\u01A0)]) ;; CAPITAL O WITH HORN
(define-key compose-horn-map [?U] [(?\u01AF)]) ;; CAPITAL U WITH HORN
(define-key compose-horn-map [?o] [(?\u01A1)]) ;; SMALL O WITH HORN
(define-key compose-horn-map [?u] [(?\u01B0)]) ;; SMALL U WITH HORN

(define-key compose-stroke-map [?A] [(?\u023a)]) ;; CAPITAL A WITH STROKE
(define-key compose-stroke-map [?a] [(?\u2c65)]) ;; SMALL A WITH STROKE
(define-key compose-stroke-map [?B] [(?\u0243)]) ;; CAPITAL B WITH STROKE
(define-key compose-stroke-map [?b] [(?\u0180)]) ;; SMALL B WITH STROKE
(define-key compose-stroke-map [?C] [(?\u023b)]) ;; CAPITAL C WITH STROKE
(define-key compose-stroke-map [?c] [(?\u023c)]) ;; SMALL C WITH STROKE
(define-key compose-stroke-map [?D] [(?\u0110)]) ;; CAPITAL D WITH STROKE
(define-key compose-stroke-map [?d] [(?\u0111)]) ;; SMALL D WITH STROKE
(define-key compose-stroke-map [?E] [(?\u0246)]) ;; CAPITAL E WITH STROKE
(define-key compose-stroke-map [?e] [(?\u0247)]) ;; SMALL E WITH STROKE
(define-key compose-stroke-map [?G] [(?\u01e4)]) ;; CAPITAL G WITH STROKE
(define-key compose-stroke-map [?g] [(?\u01e5)]) ;; SMALL G WITH STROKE
(define-key compose-stroke-map [?H] [(?\u0126)]) ;; CAPITAL H WITH STROKE
(define-key compose-stroke-map [?h] [(?\u0127)]) ;; SMALL H WITH STROKE
(define-key compose-stroke-map [?I] [(?\u0197)]) ;; CAPITAL I WITH STROKE
(define-key compose-stroke-map [?i] [(?\u0268)]) ;; SMALL I WITH STROKE
(define-key compose-stroke-map [?J] [(?\u0248)]) ;; CAPITAL J WITH STROKE
(define-key compose-stroke-map [?j] [(?\u0249)]) ;; SMALL J WITH STROKE
(define-key compose-stroke-map [?K] [(?\ua740)]) ;; CAPITAL K WITH STROKE
(define-key compose-stroke-map [?k] [(?\ua741)]) ;; SMALL K WITH STROKE
(define-key compose-stroke-map [?L] [(?\u0141)]) ;; CAPITAL L WITH STROKE
(define-key compose-stroke-map [?l] [(?\u0142)]) ;; SMALL L WITH STROKE
(define-key compose-stroke-map [?O] [(?\u00d8)]) ;; CAPITAL O WITH STROKE
(define-key compose-stroke-map [?o] [(?\u00f8)]) ;; SMALL O WITH STROKE
(define-key compose-stroke-map [?P] [(?\u2c63)]) ;; CAPITAL P WITH STROKE
(define-key compose-stroke-map [?p] [(?\u1d7d)]) ;; SMALL P WITH STROKE
(define-key compose-stroke-map [?R] [(?\u024c)]) ;; CAPITAL R WITH STROKE
(define-key compose-stroke-map [?r] [(?\u024d)]) ;; SMALL R WITH STROKE
(define-key compose-stroke-map [?T] [(?\u0166)]) ;; CAPITAL T WITH STROKE
(define-key compose-stroke-map [?t] [(?\u0167)]) ;; SMALL T WITH STROKE
(define-key compose-stroke-map [?Y] [(?\u024e)]) ;; CAPITAL Y WITH STROKE
(define-key compose-stroke-map [?y] [(?\u024f)]) ;; SMALL Y WITH STROKE
(define-key compose-stroke-map [?Z] [(?\u01b5)]) ;; CAPITAL Z WITH STROKE
(define-key compose-stroke-map [?z] [(?\u01b6)]) ;; SMALL Z WITH STROKE

;;; The rest of the compose-map.  These are the composed characters
;;; that are not accessible via "dead" keys.

(define-key compose-map " '"	"'")
(define-key compose-map " ^"	"^")
(define-key compose-map " `"	"`")
(define-key compose-map " ~"	"~")
(define-key compose-map "  "	[(?\240)])
(define-key compose-map " \""	[(?\250)])
(define-key compose-map " :"	[(?\250)])
(define-key compose-map " *"	[(?\260)])

(define-key compose-map "!!"	[(?\241)])
(define-key compose-map "!^"	[(?\246)])
(define-key compose-map "!S"	[(?\247)])
(define-key compose-map "!s"	[(?\247)])
(define-key compose-map "!P"	[(?\266)])
(define-key compose-map "!p"	[(?\266)])

(define-key compose-map "(("	"[")
(define-key compose-map "(-"	"{")

(define-key compose-map "))"	"]")
(define-key compose-map ")-"	"}")

(define-key compose-map "++"	"#")
(define-key compose-map "+-"	[(?\261)])

(define-key compose-map "-("	"{")
(define-key compose-map "-)"	"}")
(define-key compose-map "--"	"-")
(define-key compose-map "-L"	[(?\243)])
(define-key compose-map "-l"	[(?\243)])
(define-key compose-map "-Y"	[(?\245)])
(define-key compose-map "-y"	[(?\245)])
(define-key compose-map "-,"	[(?\254)])
(define-key compose-map "-|"	[(?\254)])
(define-key compose-map "-^"	[(?\257)])
(define-key compose-map "-+"	[(?\261)])
(define-key compose-map "-:"	[(?\367)])
(define-key compose-map "-D"	[(?\320)])
(define-key compose-map "-d"	[(?\360)])
(define-key compose-map "-a"    [(?\252)])

(define-key compose-map ".^"	[(?\267)])

(define-key compose-map "//"	"\\")
(define-key compose-map "/<"	"\\")
(define-key compose-map "/^"	"|")
(define-key compose-map "/C"	[(?\242)])
(define-key compose-map "/c"	[(?\242)])
(define-key compose-map "/U"	[(?\265)])
(define-key compose-map "/u"	[(?\265)])
(define-key compose-map "/O"	[(?\330)])
(define-key compose-map "/o"	[(?\370)])

(define-key compose-map "0X"	[(?\244)])
(define-key compose-map "0x"	[(?\244)])
(define-key compose-map "0S"	[(?\247)])
(define-key compose-map "0s"	[(?\247)])
(define-key compose-map "0C"	[(?\251)])
(define-key compose-map "0c"	[(?\251)])
(define-key compose-map "0R"	[(?\256)])
(define-key compose-map "0r"	[(?\256)])
(define-key compose-map "0^"	[(?\260)])

(define-key compose-map "1^"	[(?\271)])
(define-key compose-map "14"	[(?\274)])
(define-key compose-map "12"	[(?\275)])

(define-key compose-map "2^"	[(?\262)])

(define-key compose-map "3^"	[(?\263)])
(define-key compose-map "34"	[(?\276)])

(define-key compose-map ":-"	[(?\367)])

(define-key compose-map "</"	"\\")
(define-key compose-map "<<"	[(?\253)])

(define-key compose-map "=L"	[(?\243)])
(define-key compose-map "=l"	[(?\243)])
(define-key compose-map "=Y"	[(?\245)])
(define-key compose-map "=y"	[(?\245)])

(define-key compose-map ">>"	[(?\273)])

(define-key compose-map "??"	[(?\277)])

(define-key compose-map "AA"	"@")
(define-key compose-map "Aa"	"@")
(define-key compose-map "A_"	[(?\252)])
(define-key compose-map "A`"	[(?\300)])
(define-key compose-map "A'"	[(?\301)])
(define-key compose-map "A^"	[(?\302)])
(define-key compose-map "A~"	[(?\303)])
(define-key compose-map "A\""	[(?\304)])
(define-key compose-map "A*"	[(?\305)])
(define-key compose-map "AE"	[(?\306)])

(define-key compose-map "C/"	[(?\242)])
(define-key compose-map "C|"	[(?\242)])
(define-key compose-map "C0"	[(?\251)])
(define-key compose-map "CO"	[(?\251)])
(define-key compose-map "Co"	[(?\251)])
(define-key compose-map "C,"	[(?\307)])

(define-key compose-map "D-"	[(?\320)])

(define-key compose-map "E`"	[(?\310)])
(define-key compose-map "E'"	[(?\311)])
(define-key compose-map "E^"	[(?\312)])
(define-key compose-map "E\""	[(?\313)])

(define-key compose-map "I`"	[(?\314)])
(define-key compose-map "I'"	[(?\315)])
(define-key compose-map "I^"	[(?\316)])
(define-key compose-map "I\""	[(?\317)])

(define-key compose-map "L-"	[(?\243)])
(define-key compose-map "L="	[(?\243)])

(define-key compose-map "N~"	[(?\321)])

(define-key compose-map "OX"	[(?\244)])
(define-key compose-map "Ox"	[(?\244)])
(define-key compose-map "OS"	[(?\247)])
(define-key compose-map "Os"	[(?\247)])
(define-key compose-map "OC"	[(?\251)])
(define-key compose-map "Oc"	[(?\251)])
(define-key compose-map "OR"	[(?\256)])
(define-key compose-map "Or"	[(?\256)])
(define-key compose-map "O_"	[(?\272)])
(define-key compose-map "O`"	[(?\322)])
(define-key compose-map "O'"	[(?\323)])
(define-key compose-map "O^"	[(?\324)])
(define-key compose-map "O~"	[(?\325)])
(define-key compose-map "O\""	[(?\326)])
(define-key compose-map "O/"	[(?\330)])

(define-key compose-map "P!"	[(?\266)])

(define-key compose-map "R0"	[(?\256)])
(define-key compose-map "RO"	[(?\256)])
(define-key compose-map "Ro"	[(?\256)])

(define-key compose-map "S!"	[(?\247)])
(define-key compose-map "S0"	[(?\247)])
(define-key compose-map "SO"	[(?\247)])
(define-key compose-map "So"	[(?\247)])
(define-key compose-map "SS"	[(?\337)])

(define-key compose-map "TH"	[(?\336)])

(define-key compose-map "U`"	[(?\331)])
(define-key compose-map "U'"	[(?\332)])
(define-key compose-map "U^"	[(?\333)])
(define-key compose-map "U\""	[(?\334)])

(define-key compose-map "X0"	[(?\244)])
(define-key compose-map "XO"	[(?\244)])
(define-key compose-map "Xo"	[(?\244)])

(define-key compose-map "Y-"	[(?\245)])
(define-key compose-map "Y="	[(?\245)])
(define-key compose-map "Y'"	[(?\335)])

(define-key compose-map "_A"	[(?\252)])
(define-key compose-map "_a"	[(?\252)])
(define-key compose-map "_^"	[(?\257)])
(define-key compose-map "_O"	[(?\272)])
(define-key compose-map "_o"	[(?\272)])

(define-key compose-map "aA"	"@")
(define-key compose-map "aa"	"@")
(define-key compose-map "a_"	[(?\252)])
(define-key compose-map "a-"    [(?\252)])
(define-key compose-map "a`"	[(?\340)])
(define-key compose-map "a'"	[(?\341)])
(define-key compose-map "a^"	[(?\342)])
(define-key compose-map "a~"	[(?\343)])
(define-key compose-map "a\""	[(?\344)])
(define-key compose-map "a*"	[(?\345)])
(define-key compose-map "ae"	[(?\346)])

(define-key compose-map "c/"	[(?\242)])
(define-key compose-map "c|"	[(?\242)])
(define-key compose-map "c0"	[(?\251)])
(define-key compose-map "cO"	[(?\251)])
(define-key compose-map "co"	[(?\251)])
(define-key compose-map "c,"	[(?\347)])

(define-key compose-map "d-"	[(?\360)])

(define-key compose-map "e`"	[(?\350)])
(define-key compose-map "e'"	[(?\351)])
(define-key compose-map "e^"	[(?\352)])
(define-key compose-map "e\""	[(?\353)])

(define-key compose-map "i`"	[(?\354)])
(define-key compose-map "i'"	[(?\355)])
(define-key compose-map "i^"	[(?\356)])
(define-key compose-map "i\""	[(?\357)])
(define-key compose-map "i:"	[(?\357)])

(define-key compose-map "l-"	[(?\243)])
(define-key compose-map "l="	[(?\243)])

(define-key compose-map "n~"	[(?\361)])

(define-key compose-map "oX"	[(?\244)])
(define-key compose-map "ox"	[(?\244)])
(define-key compose-map "oC"	[(?\251)])
(define-key compose-map "oc"	[(?\251)])
(define-key compose-map "oR"	[(?\256)])
(define-key compose-map "or"	[(?\256)])
(define-key compose-map "oS"	[(?\247)])
(define-key compose-map "os"	[(?\247)])
(define-key compose-map "o_"	[(?\272)])
(define-key compose-map "o`"	[(?\362)])
(define-key compose-map "o'"	[(?\363)])
(define-key compose-map "o^"	[(?\364)])
(define-key compose-map "o~"	[(?\365)])
(define-key compose-map "o\""	[(?\366)])
(define-key compose-map "o/"	[(?\370)])

(define-key compose-map "p!"	[(?\266)])

(define-key compose-map "r0"	[(?\256)])
(define-key compose-map "rO"	[(?\256)])
(define-key compose-map "ro"	[(?\256)])

(define-key compose-map "s!"	[(?\247)])
(define-key compose-map "s0"	[(?\247)])
(define-key compose-map "sO"	[(?\247)])
(define-key compose-map "so"	[(?\247)])
(define-key compose-map "ss"	[(?\337)])

(define-key compose-map "th"	[(?\376)])

(define-key compose-map "u`"	[(?\371)])
(define-key compose-map "u'"	[(?\372)])
(define-key compose-map "u^"	[(?\373)])
(define-key compose-map "u\""	[(?\374)])
(define-key compose-map "u/"	[(?\265)])

(define-key compose-map "x0"	[(?\244)])
(define-key compose-map "xO"	[(?\244)])
(define-key compose-map "xo"	[(?\244)])
(define-key compose-map "xx"	[(?\327)])

(define-key compose-map "y-"	[(?\245)])
(define-key compose-map "y="	[(?\245)])
(define-key compose-map "y'"	[(?\375)])
(define-key compose-map "y\""	[(?\377)])

(define-key compose-map "|C"	[(?\242)])
(define-key compose-map "|c"	[(?\242)])
(define-key compose-map "||"	[(?\246)])


;; Make colon equivalent to doublequote for diaeresis processing.  Some
;; Xlibs do this.
(labels ((alias-colon-to-doublequote (keymap)
           (map-keymap
            #'(lambda (key value)
                (when (keymapp value)
                  (alias-colon-to-doublequote value))
                (when (eq key '\")
                  (define-key keymap ":" value)))
            keymap)))
  (alias-colon-to-doublequote compose-map))

;;; Electric dead keys: making a' mean a-acute.


(defun electric-diacritic (&optional count)
  "Modify the previous character with an accent.
For example, if `:' is bound to this command, then typing `a:'
will first insert `a' and then turn it into `\344' (adiaeresis).
The minimum list of keys to which this command may be bound (and the accents
which it understands) are:

   '  (acute)       \301\311\315\323\332\335 \341\351\355\363\372\375
   `  (grave)       \300\310\314\322\331 \340\350\354\362\371
   :  (diaeresis)   \304\313\317\326\334 \344\353\357\366\374\377
   ^  (circumflex)  \302\312\316\324\333 \342\352\356\364\373
   ,  (cedilla)     \307\347
   .  (ring)        \305\345"
  (interactive "p")
  (or count (setq count 1))

  (if (not (eq last-command 'self-insert-command))
      ;; Only do the magic if the two chars were typed in succession.
      (self-insert-command count)

    ;; This is so that ``a : C-x u'' will transform `adiaeresis' back into `a:'
    (self-insert-command count)
    (undo-boundary)
    (delete-char (- count))

    (let* ((c last-command-char)
	   (map (cond ((eq c ?') compose-acute-map)
		      ((eq c ?`) compose-grave-map)
		      ((eq c ?,) compose-cedilla-map)
		      ((eq c ?:) compose-diaeresis-map)
		      ((eq c ?^) compose-circumflex-map)
		      ((eq c ?~) compose-tilde-map)
		      ((eq c ?.) compose-ring-map)
		      (t (error "unknown diacritic: %s (%c)" c c))))
	   (base-char (char-before))
	   (mod-char (and (>= (downcase base-char) ?a) ; only do alphabetics?
			  (<= (downcase base-char) ?z)
			  (lookup-key map (make-string 1 base-char)))))
      (when (and (vectorp mod-char) (eql (length mod-char) 1))
        (setq mod-char (aref mod-char 0))
        (if (and (consp mod-char) (eql (length mod-char) 1)
                 (characterp (car mod-char)))
            (setq mod-char (car mod-char))))
      (if (and mod-char (symbolp mod-char))
	  (setq mod-char (or (get-character-of-keysym mod-char) mod-char)))
      (if (and mod-char (> count 0))
	  (delete-char -1)
	(setq mod-char c))
      (while (> count 0)
	(insert mod-char)
	(setq count (1- count))))))

;; should "::" mean "¨" and ": " mean ":"?
;; should we also do
;;    (?~
;;     (?A "\303")
;;     (?C "\307")
;;     (?D "\320")
;;     (?N "\321")
;;     (?O "\325")
;;     (?a "\343")
;;     (?c "\347")
;;     (?d "\360")
;;     (?n "\361")
;;     (?o "\365")
;;     (?> "\273")
;;     (?< "\253")
;;     (?  "~")) ; no special code
;;    (?\/
;;     (?A "\305") ;; A-with-ring (Norwegian and Danish)
;;     (?E "\306") ;; AE-ligature (Norwegian and Danish)
;;     (?O "\330")
;;     (?a "\345") ;; a-with-ring (Norwegian and Danish)
;;     (?e "\346") ;; ae-ligature (Norwegian and Danish)
;;     (?o "\370")
;;     (?  "/")) ; no special code


(provide 'x-compose)
(provide 'compose)

;;; compose.el ends here
