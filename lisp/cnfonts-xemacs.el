;;; cnfonts-xemacs.el --- GNU cnfonts's font queries on the XEmacs font layer

;; Copyright (C) 2026 XEmacs Development Team

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
;; XEmacs does not have.  This layer answers the load-bearing five from the
;; font layer's own data, as decided in docs/adr/0002 and 0003:
;;
;;   `font-spec'         the 12-element descriptor `make-font' returns
;;   `x-list-fonts'      synthetic XLFDs for families the honest probe accepts
;;   `list-fonts'        availability through the same descriptor route
;;   `font-family-list'  supplied by lisp/font.el; loaded, not redefined
;;   `string-join'       GNU's `subr-x' helper
;;
;; The sink half (`set-face-attribute' and `set-fontset-font') lands with
;; ticket #95.  Loading this file does not load or require `cnfonts', and it
;; works on a build with no window system: every answer comes from the
;; device-neutral descriptor route, and a device with no row in
;; `font-window-system-mappings' simply answers "no".

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

(provide 'cnfonts-xemacs)

;;; cnfonts-xemacs.el ends here
