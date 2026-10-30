;;; cnfonts-xemacs.el --- GNU cnfonts's font queries on the XEmacs font layer

;; Copyright (C) 2026 XEmacs Development Team

;; Author: XEmacs Development Team
;; Keywords: fonts, i18n

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

;;; Commentary:

;; `cnfonts' (a GNU Emacs package) chooses fonts through seven APIs that
;; XEmacs does not have.  This layer answers them from the font layer's own
;; data, as decided in docs/adr/0002 and 0003:
;;
;;   `font-spec'           the 12-element descriptor `make-font' returns
;;   `x-list-fonts'        synthetic XLFDs for families the honest probe accepts
;;   `list-fonts'          availability through the same descriptor route
;;   `font-family-list'    supplied by lisp/font.el; loaded, not redefined
;;   `string-join'         GNU's `subr-x' helper
;;   `set-face-attribute'  a descriptor lowered onto the face's plain font
;;   `set-fontset-font'    a descriptor lowered under one of the two tags below
;;
;; Loading this file does not load or require `cnfonts', and it works on a
;; build with no window system: the queries come from the device-neutral
;; descriptor route, and a device with no row in
;; `font-window-system-mappings' simply answers "no".  The accepted
;; approximations are marked at their branches and in the XEmacs user
;; manual under "Per-script fonts with cnfonts".

;;; Code:

(require 'font)

(defconst cnfonts-xemacs-weights
  '((ultra-light . :extra-light)
    (extra-light . :extra-light)
    (light       . :light)
    (semi-light  . :demi-light)
    (regular     . :normal)
    (normal      . :normal)
    (book        . :book)
    (medium      . :medium)
    (semi-bold   . :demi-bold)
    (bold        . :bold)
    (extra-bold  . :extra-bold)
    (ultra-bold  . :extra-bold)
    (black       . :extra-bold))
  "Map GNU `font-spec' weight symbols onto XEmacs descriptor keywords.")

(defconst cnfonts-xemacs-slants
  `((italic . ,font-italic-mask)
    (oblique . ,font-oblique-mask))
  "Map GNU `font-spec' slant symbols onto descriptor style bitmasks.")

;; `cnfonts' keeps `(cl-subseq (split-string xlfd "-") 2 -12)' from the
;; lister's answer, so the synthetic name carries a foundry, the family, and
;; exactly twelve wildcard fields after it.
(defconst cnfonts-xemacs--xlfd-tail "-*-*-*-*-*-*-*-*-*-*-*-*")

(defun cnfonts-xemacs--weight (weight)
  "Translate GNU WEIGHT to an XEmacs descriptor weight keyword.
Unknown symbols pass through unchanged; the X name producer has no
keyword for them and leaves the weight unconstrained."
  (and weight
       (or (cdr (assq weight cnfonts-xemacs-weights))
           weight)))

(defun font-spec (&rest args)
  "Return an XEmacs font descriptor describing GNU-style ARGS.
ARGS is a plist; `:family' and `:size' are the descriptor's own slots.
`:name' names the family -- `cnfonts' only ever passes a family name,
never an XLFD.  `:weight' and `:slant' use GNU's names and are translated
to the descriptor's keyword weight and `font-style' bitmask."
  (let ((slant (plist-get args :slant)))
    (make-font
     :family (or (plist-get args :family)
                 (plist-get args :name))
     :size (plist-get args :size)
     :weight (cnfonts-xemacs--weight (plist-get args :weight))
     :style (if slant
                (or (cdr (assq slant cnfonts-xemacs-slants)) 0)
              0))))

(defun cnfonts-xemacs--available-name (font)
  "Return FONT's device name if the honest probe accepts it, else nil.
FONT is a descriptor; lower it with `font-create-name', then ask
`try-font-name' -- the platform's own matcher decides."
  (let* ((device (selected-device))
         (name (font-create-name font device)))
    (and name (try-font-name name device))))

(defun cnfonts-xemacs--families (pattern)
  "Return the families matching PATTERN, which `x-list-fonts' supplies.
A shell wildcard (`*', `?' or `[...]') is matched against the font
layer's family list; a plain PATTERN names the single family to try."
  (if (not (string-match "[*?[]" pattern))
      (list pattern)
    (let ((regexp (wildcard-to-regexp pattern))
          (case-fold-search t)
          families)
      (dolist (family (font-family-list (selected-device)))
        (if (string-match regexp family)
            (push family families)))
      (nreverse families))))

(defun x-list-fonts (pattern &optional _face _frame maximum)
  "Return synthetic XLFDs for the families matching PATTERN.
PATTERN is a family name, or a shell wildcard over the font layer's
family list.  Each family goes through `font-create-name' and
`try-font-name', so only families the honest probe accepts are listed.
The answers are of the form `-*-FAMILY-*...' because `cnfonts' splits
them on `-' and keeps the family field; returning a raw device name (a
fontconfig pattern on Xft) would make that split signal instead of
parse.  MAXIMUM, when non-nil, limits the length of the answer."
  (let ((families (cnfonts-xemacs--families pattern))
        result)
    (while (and families
                (or (null maximum) (< (length result) maximum)))
      (if (cnfonts-xemacs--available-name (make-font :family (car families)))
          (push (concat "-*-" (car families) cnfonts-xemacs--xlfd-tail)
                result))
      (setq families (cdr families)))
    (nreverse result)))

(defun list-fonts (font-spec &optional _frame _num)
  "Return FONT-SPEC's device name if it is available, else nil.
`cnfonts' only asks whether the answer is non-nil; availability goes
through the same `font-create-name' plus `try-font-name' route as
`x-list-fonts'."
  (and (font-family font-spec)
       (cnfonts-xemacs--available-name font-spec)))

(defun string-join (strings &optional separator)
  "Join STRINGS with SEPARATOR, which defaults to the empty string."
  (mapconcat #'identity strings (or separator "")))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; The sink half: cnfonts's configuration, lowered onto face fonts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; The layer's own tags, not the built-in `two-dimensional'/`jit-charset':
;; their names are this layer's contract, and sharing a built-in would
;; entangle cnfonts with any other tag-set under that name.
(define-specifier-tag 'cnfonts-dimension-2 nil
  'specifier-tag-two-dimensional-p)

;; `specifier-tag-jit-charset-p' is compiled out under UNICODE_INTERNAL,
;; where no JIT charset exists; a tag that never matches is then honest.
(define-specifier-tag 'cnfonts-jit-charset nil
  (if (fboundp 'specifier-tag-jit-charset-p)
      'specifier-tag-jit-charset-p
    (lambda (_charset) nil)))

(defconst cnfonts-xemacs-descriptor-keys
  '(:family :size :weight :slant :name)
  "The `font-spec' keywords `set-face-attribute' builds a descriptor from.")

(defun cnfonts-xemacs--gtk-p ()
  "Return non-nil when the selected device is a GTK device."
  (eq (device-type (selected-device)) 'gtk))

(defun cnfonts-xemacs--charset-tag (charset)
  "Return the specifier tag that carries CHARSET's font, or nil to drop it.
A nil CHARSET is cnfonts's whole-repertoire bucket, its EXT-B request;
the scope is broader than GNU's, because any unencodable codepoint may
fall to it, not just Ext-B.  A GNU script is coarse: `cjk-misc' and
every other script collapse into the dimension-2 tag, which also offers
the CJK font to non-CJK dimension-2 charsets -- their per-charset
coverage usually rejects it.  `symbol', `phonetic', and the ornament
codepoint ranges have no XEmacs charset to select; a dimension-1 script
\(half-width katakana, Zhuyin) has no tag either, so it keeps the Latin
font."
  (cond
   ((null charset) 'cnfonts-jit-charset)
   ((or (eq charset 'symbol) (eq charset 'phonetic) (consp charset)) nil)
   ((let ((found (find-charset charset)))
      (and found (= 1 (charset-dimension found))))
    nil)
   (t 'cnfonts-dimension-2)))

(defun set-fontset-font (fontset charset font-spec &optional frame how-to-add)
  "Lower a GNU fontset font request onto the `default' face, for `cnfonts'.
CHARSET nil selects the JIT-charset tag; a script selects the
dimension-2 tag.  HOW-TO-ADD (`prepend' or `append') passes through to
`set-face-font'.  Requests this layer cannot honour are dropped with a
warning rather than reported as success."
  (if (not (equal fontset "fontset-default"))
      (warn "cnfonts-xemacs: only fontset-default is mapped; dropping %S"
            fontset)
    (when frame
      (warn "cnfonts-xemacs: the frame argument %S is ignored" frame))
    (let ((tag (cnfonts-xemacs--charset-tag charset)))
      (cond
       ((null tag)
        (warn (concat "cnfonts-xemacs: dropping the font request for %S: "
                      "no specifier tag can select it")
              charset))
       ((cnfonts-xemacs--gtk-p)
        ;; GTK has no per-charset fallback (ADR 0003).  Only the
        ;; script-level requests apply, as the plain font; in cnfonts's
        ;; sequence the last one is the Chinese font, and the Latin font
        ;; never takes this slot.
        (if (and charset (null how-to-add))
            (progn
              (warn (concat "cnfonts-xemacs: GTK cannot select fonts per "
                            "charset; setting the CJK font as the plain font"))
              (set-face-font 'default font-spec))
          (warn (concat "cnfonts-xemacs: GTK cannot apply the %S fallback "
                        "for %S; dropped")
                how-to-add charset)))
       (t
        ;; cnfonts adds the hangul EXT-B font with HOW-TO-ADD `append'; it
        ;; lands after the Chinese font, which already matches, so it never
        ;; wins.  The pass-through keeps GNU's ordering, no-op and all.
        (set-face-font 'default font-spec 'global (list tag) how-to-add))))))

(defun set-face-attribute (face frame &rest args)
  "Set FACE's font from GNU-style keyword ARGS, for `cnfonts'.
`:font' carries a descriptor; without it the descriptor keywords
\(`:family', `:size', `:weight', `:slant', `:name') build one.  Other
attributes and a non-nil FRAME are ignored with a warning."
  (when frame
    (warn "cnfonts-xemacs: set-face-attribute ignores FRAME %S" frame))
  (let ((rest args))
    (while rest
      (unless (memq (car rest) (cons :font cnfonts-xemacs-descriptor-keys))
        (warn "cnfonts-xemacs: set-face-attribute ignores %S" (car rest)))
      (setq rest (cddr rest))))
  (let ((font (plist-get args :font))
        descriptor-args)
    (dolist (key cnfonts-xemacs-descriptor-keys)
      (let ((value (plist-get args key)))
        (when value
          (if font
              (warn "cnfonts-xemacs: set-face-attribute ignores %S: :font wins"
                    key)
            (setq descriptor-args (append descriptor-args (list key value)))))))
    (unless font
      (when descriptor-args
        (setq font (apply 'font-spec descriptor-args))))
    (cond
     ((null font)
      (warn (concat "cnfonts-xemacs: set-face-attribute needs :font or a "
                    "descriptor keyword")))
     ((cnfonts-xemacs--gtk-p)
      ;; Without a per-charset fallback a plain Latin font renders CJK as
      ;; boxes (ADR 0003); the CJK request that follows owns the slot.
      (warn (concat "cnfonts-xemacs: GTK has no per-charset fonts; not "
                    "setting %s's Latin font")
            face))
     (t
      (set-face-font face font)))))

(provide 'cnfonts-xemacs)

;;; cnfonts-xemacs.el ends here
