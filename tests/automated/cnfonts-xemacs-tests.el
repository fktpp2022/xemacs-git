;;; cnfonts-xemacs-tests.el --- tests for the cnfonts compatibility layer

;; Copyright (C) 2026 XEmacs Development Team

;; Author: XEmacs Development Team
;; Keywords: tests, fonts

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

;; The device-free half of tickets #94 and #95: the seven GNU calls that
;; `cnfonts' makes, answered from the font layer, and the two specifier
;; tags the sink half installs.  ctest runs `-batch', whose device has no
;; row in `font-window-system-mappings', so the name producer is stubbed
;; with fixtures: the assertions are about the shapes the compatibility
;; layer promises, not about the fonts installed on this machine.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(require 'cl-lib)
(require 'font)

;; The font layer owns `font-family-list' (ADR 0003); the compatibility
;; layer must load it, not replace it with a per-backend stub.
(let ((family-list (symbol-function 'font-family-list)))
  (load "cnfonts-xemacs" nil t)
  (Assert (featurep 'cnfonts-xemacs)
          "loading provides the cnfonts-xemacs feature")
  (Assert (eq family-list (symbol-function 'font-family-list))
          "the layer leaves the font layer's font-family-list in place"))

(Assert (not (featurep 'cnfonts))
        "the layer does not require cnfonts")
(dolist (symbol '(font-spec x-list-fonts list-fonts string-join))
  (Assert (fboundp symbol)
          (format "%s is answered by the compatibility layer" symbol)))

;; The batch device has no backend row, so the device-neutral family list
;; answers "none" rather than a per-backend stub's guess.
(Assert (null (font-family-list (selected-device)))
        "the device-neutral family list answers for the batch device")

;;; font-spec: the descriptor and the GNU keyword mapping

(let ((spec (font-spec :name "Noto Sans CJK SC" :size 12.5
                       :weight 'bold :slant 'italic)))
  (Assert (= 12 (length spec))
          "font-spec returns the 12-element descriptor")
  (Assert (equal (font-family spec) '("Noto Sans CJK SC"))
          "font-spec keeps :name as the family")
  (Assert (eq (font-weight spec) :bold)
          "font-spec maps :weight bold")
  (Assert (= (font-style spec) font-italic-mask)
          "font-spec maps :slant italic onto the italic style bit")
  (Assert (equal (font-size spec) 12.5)
          "font-spec keeps :size direct"))

(let ((spec (font-spec :family "Monospace" :weight 'regular :slant 'oblique)))
  (Assert (equal (font-family spec) '("Monospace"))
          ":family is the family")
  (Assert (eq (font-weight spec) :normal)
          ":weight regular maps onto :normal")
  (Assert (= (font-style spec) font-oblique-mask)
          ":slant oblique maps onto the oblique style bit"))

(let ((spec (font-spec :name "Monospace")))
  (Assert (null (font-weight spec))
          "no :weight leaves the weight unset")
  (Assert (= 0 (font-style spec))
          "no :slant leaves the style unset"))

;;; x-list-fonts: synthetic XLFDs from the honest probe

;; The stubs stand in for the platform: four families, a device name that
;; carries the family, and a probe that rejects the one family whose name
;; contains "Beta".  "Zalpha" exists so an unanchored wildcard would show.
(defmacro cnfonts-xemacs-tests--with-stubs (&rest body)
  `(flet ((font-family-list (&optional _device)
             '("Alpha" "Beta" "Gamma" "Zalpha"))
          (font-create-name (fontobj &optional _device)
             (concat (car (font-family fontobj)) "-device"))
          (try-font-name (name &optional _device)
             (unless (string-match "Beta" name) name)))
     ,@body))

(defun cnfonts-xemacs-tests--family-of (xlfd)
  "Extract the family from XLFD the way `cnfonts--font-exists-p' does."
  (let ((fields (split-string xlfd "-")))
    (mapconcat #'identity
               (cl-subseq fields 2 (- (length fields) 12))
               "-")))

(cnfonts-xemacs-tests--with-stubs
 (Assert (equal (x-list-fonts "Alpha")
                '("-*-Alpha-*-*-*-*-*-*-*-*-*-*-*-*"))
         "a present family yields one synthetic XLFD")
 (Assert (null (x-list-fonts "Beta"))
         "a rejected family is omitted")
 (Assert (equal (x-list-fonts "*")
                '("-*-Alpha-*-*-*-*-*-*-*-*-*-*-*-*"
                  "-*-Gamma-*-*-*-*-*-*-*-*-*-*-*-*"
                  "-*-Zalpha-*-*-*-*-*-*-*-*-*-*-*-*"))
         "a wildcard enumerates the family list, omitting rejected families")
 (Assert (equal (x-list-fonts "Al*")
                '("-*-Alpha-*-*-*-*-*-*-*-*-*-*-*-*"))
         "a wildcard is anchored, so a suffix match is not enough")
 (Assert (equal (x-list-fonts "*" nil nil 1)
                '("-*-Alpha-*-*-*-*-*-*-*-*-*-*-*-*"))
         "maximum truncates the answer")
 (Assert (equal (x-list-fonts "Noto-Sans")
                '("-*-Noto-Sans-*-*-*-*-*-*-*-*-*-*-*-*"))
         "a hyphenated family stays one XLFD field group"))

(let* ((xlfd (car (cnfonts-xemacs-tests--with-stubs (x-list-fonts "Alpha"))))
       (fields (split-string xlfd "-")))
  (Assert (= 15 (length fields))
          "the synthetic XLFD is a foundry, a family, and twelve wildcards")
  (Assert (equal (cnfonts-xemacs-tests--family-of xlfd) "Alpha")
          "cnfonts's `(length-12)' extraction recovers the family"))

(let ((xlfd (car (cnfonts-xemacs-tests--with-stubs
                  (x-list-fonts "Noto-Sans")))))
  (Assert (equal (cnfonts-xemacs-tests--family-of xlfd) "Noto-Sans")
          "cnfonts recovers a hyphenated family unchanged"))

;;; list-fonts: availability through the same route

(cnfonts-xemacs-tests--with-stubs
 (Assert (list-fonts (font-spec :name "Alpha"))
         "an available descriptor answers non-nil")
 (Assert (null (list-fonts (font-spec :name "Beta")))
         "a rejected descriptor answers nil")
 (Assert (null (list-fonts (font-spec)))
         "a descriptor without a family answers nil"))

;;; string-join

(Assert (equal (string-join '("a" "b" "c") "-") "a-b-c")
        "string-join joins with the separator")
(Assert (equal (string-join '("a" "b")) "ab")
        "the separator defaults to the empty string")
(Assert (equal (string-join '("a") "-") "a")
        "a single string is itself")
(Assert (equal (string-join nil "-") "")
        "no strings join to the empty string")

;;; The two specifier tags

(Assert (valid-specifier-tag-p 'cnfonts-dimension-2)
        "the dimension-2 tag is registered")
(Assert (valid-specifier-tag-p 'cnfonts-jit-charset)
        "the JIT-charset tag is registered")
(Assert (and (memq 'cnfonts-dimension-2 (specifier-tag-list))
             (memq 'cnfonts-jit-charset (specifier-tag-list)))
        "both tags are in the tag list, so set-face-font accepts them")

(defun cnfonts-xemacs-tests--tag-matches-p (tag charset)
  "Return non-nil when TAG's charset predicate accepts CHARSET."
  (funcall (specifier-tag-charset-predicate tag) (find-charset charset)))

(Assert (cnfonts-xemacs-tests--tag-matches-p
         'cnfonts-dimension-2 'chinese-gb2312)
        "the dimension-2 tag matches a Chinese charset")
(Assert (not (cnfonts-xemacs-tests--tag-matches-p
              'cnfonts-dimension-2 'ascii))
        "the dimension-2 tag rejects ASCII")
(Assert (not (cnfonts-xemacs-tests--tag-matches-p
              'cnfonts-dimension-2 'katakana-jisx0201))
        "the dimension-2 tag rejects a dimension-1 script")

(when (find-charset 'jit-ucs-charset-0)
  (Assert (cnfonts-xemacs-tests--tag-matches-p
           'cnfonts-jit-charset 'jit-ucs-charset-0)
          "the JIT tag matches the JIT charset")
  (Assert (not (cnfonts-xemacs-tests--tag-matches-p
                'cnfonts-jit-charset 'chinese-gb2312))
          "the JIT tag rejects an ordinary Chinese charset"))

;;; The sink half

;; The batch device produces no font name, so the producer is stubbed to a
;; name carrying the family.  `warn' is captured rather than printed, and
;; the default face is restored afterwards: `cnfonts' configures `default',
;; and the assertions read its font specifier directly.
(defmacro cnfonts-xemacs-tests--with-sink (&rest body)
  "Run BODY against a clean `default' face with stubbed font naming.
`warn' messages are collected in `cnfonts-xemacs-tests--warned'."
  `(let ((cnfonts-xemacs-tests--warned nil))
     (flet ((font-create-name (fontobj &optional _device)
               (concat (car (font-family fontobj)) "-device"))
            (warn (&rest args)
               (push (apply 'format args) cnfonts-xemacs-tests--warned)
               nil))
       (unwind-protect
           (progn ,@body)
         (remove-specifier (face-property 'default 'font) 'global)
         (remove-face-property 'default 'font-specification)))))

(defun cnfonts-xemacs-tests--specs ()
  "Return the default face's global font specifications in order."
  (specifier-specs (face-property 'default 'font) 'global nil nil))

(defun cnfonts-xemacs-tests--spec (tag-set)
  "Return the default face's font specification for TAG-SET, or nil."
  (specifier-specs (face-property 'default 'font) 'global tag-set t))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" 'han (make-font :family "CJK"))
 (Assert (equal (cnfonts-xemacs-tests--spec '(cnfonts-dimension-2))
                '((cnfonts-dimension-2) . "CJK-device"))
         "a script lowers onto the dimension-2 tag"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" nil (make-font :family "EXTB"))
 (Assert (equal (cnfonts-xemacs-tests--spec '(cnfonts-jit-charset))
                '((cnfonts-jit-charset) . "EXTB-device"))
         "the nil charset lowers onto the JIT-charset tag"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" 'han (make-font :family "CJK"))
 (set-fontset-font "fontset-default" nil (make-font :family "EXTB") nil 'prepend)
 (Assert (equal (cnfonts-xemacs-tests--specs)
                '(((cnfonts-jit-charset) . "EXTB-device")
                  ((cnfonts-dimension-2) . "CJK-device")))
         "prepend puts the JIT bucket ahead of the dimension-2 entry"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" 'han (make-font :family "CJK"))
 (set-fontset-font "fontset-default" 'hangul (make-font :family "EXTB")
                   nil 'append)
 (Assert (equal (cnfonts-xemacs-tests--specs)
                '(((cnfonts-dimension-2) . "CJK-device")
                  ((cnfonts-dimension-2) . "EXTB-device")))
         "append lands after the Chinese font: the hangul fallback is a no-op"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" 'han (make-font :family "CJK") 'some-frame)
 (Assert (equal (cnfonts-xemacs-tests--spec '(cnfonts-dimension-2))
                '((cnfonts-dimension-2) . "CJK-device"))
         "a non-nil frame is ignored and the request still applies")
 (Assert (= 1 (length cnfonts-xemacs-tests--warned))
         "the ignored frame warns once"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-default" 'symbol (make-font :family "Symbol"))
 (set-fontset-font "fontset-default" 'phonetic (make-font :family "Phonetic"))
 (set-fontset-font "fontset-default" '(#x2776 . #x2793)
                   (make-font :family "Ornament"))
 (set-fontset-font "fontset-default" 'katakana-jisx0201
                   (make-font :family "Kana"))
 (Assert (null (cnfonts-xemacs-tests--specs))
         "dropped requests add no specification")
 (Assert (= 4 (length cnfonts-xemacs-tests--warned))
         "each dropped request warns once"))

(cnfonts-xemacs-tests--with-sink
 (set-fontset-font "fontset-other" 'han (make-font :family "CJK"))
 (Assert (null (cnfonts-xemacs-tests--specs))
         "a fontset other than fontset-default is dropped")
 (Assert (= 1 (length cnfonts-xemacs-tests--warned))
         "the dropped fontset warns once"))

(cnfonts-xemacs-tests--with-sink
 (set-face-attribute 'default nil :font (make-font :family "Latin"))
 (Assert (equal (cnfonts-xemacs-tests--spec nil) "Latin-device")
         "set-face-attribute sets the plain font")
 (Assert (and (vectorp (face-property 'default 'font-specification))
              (= 12 (length (face-property 'default 'font-specification))))
         "the face records a descriptor, not a name string"))

(cnfonts-xemacs-tests--with-sink
 (set-face-attribute 'default nil :family "Latin" :weight 'bold :slant 'italic)
 (let ((descriptor (face-property 'default 'font-specification)))
   (Assert (equal (font-family descriptor) '("Latin"))
           "a descriptor keyword builds the family")
   (Assert (eq (font-weight descriptor) :bold)
           "a descriptor keyword builds the weight")
   (Assert (= (font-style descriptor) font-italic-mask)
           "a descriptor keyword builds the slant")))

(cnfonts-xemacs-tests--with-sink
 (set-face-attribute 'default nil :font (make-font :family "Latin")
                     :family "Other")
 (Assert (equal (cnfonts-xemacs-tests--spec nil) "Latin-device")
         ":font wins over the descriptor keywords")
 (Assert (= 1 (length cnfonts-xemacs-tests--warned))
         "an ignored descriptor keyword warns once"))

(cnfonts-xemacs-tests--with-sink
 (set-face-attribute 'default 'some-frame :font (make-font :family "Latin"))
 (Assert (equal (cnfonts-xemacs-tests--spec nil) "Latin-device")
         "a non-nil frame is ignored, not fatal")
 (Assert (= 1 (length cnfonts-xemacs-tests--warned))
         "the ignored frame warns once"))

;; GTK has no per-charset fallback, so only the script-level requests apply,
;; as the plain font.  The Latin font must never win there.
(cnfonts-xemacs-tests--with-sink
 (flet ((device-type (&optional _device) 'gtk))
   (set-face-attribute 'default nil :font (make-font :family "Latin"))
   (Assert (null (cnfonts-xemacs-tests--specs))
           "the Latin font is not set on GTK")
   (set-fontset-font "fontset-default" 'han (make-font :family "CJK"))
   (Assert (equal (cnfonts-xemacs-tests--spec nil) "CJK-device")
           "the CJK font becomes the plain font on GTK")
   (Assert (null (cnfonts-xemacs-tests--spec '(cnfonts-dimension-2)))
           "GTK sets no tagged specification")
   (set-fontset-font "fontset-default" nil (make-font :family "EXTB")
                     nil 'prepend)
   (Assert (equal (cnfonts-xemacs-tests--spec nil) "CJK-device")
           "the nil-charset bucket does not replace the CJK font on GTK")
   (Assert (>= (length cnfonts-xemacs-tests--warned) 3)
           "every GTK request warns")))

;; `cnfonts' encodes family names its lister missed as `gbk'; on XEmacs
;; the same encoding is `cp936', which the shim aliases at load time.
(Assert (or (null (find-coding-system 'cp936))
            (find-coding-system 'gbk))
        "the shim supplies the gbk coding system cnfonts encodes with")

;;; Verdict
;;
;; The batch harness exits 0 even when Asserts fail, so the verdict is
;; computed from the harness's own failure counters.  tests/CMakeLists.txt
;; also gates on the "FAIL:" marker so a regression cannot pass ctest.
(Assert (and (boundp 'passes)
             (boundp 'assertion-failures)
             (boundp 'no-error-failures)
             (boundp 'wrong-error-failures)
             (boundp 'missing-message-failures)
             (boundp 'other-failures))
        "the harness failure counters are visible for the terminal line")
(message
 (if (and (zerop assertion-failures)
          (zerop no-error-failures)
          (zerop wrong-error-failures)
          (zerop missing-message-failures)
          (zerop other-failures))
     "cnfonts-xemacs tests: passed"
   "cnfonts-xemacs tests: FAILED"))

;;; cnfonts-xemacs-tests.el ends here
