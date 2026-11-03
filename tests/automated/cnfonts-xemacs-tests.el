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

;; The device-free half of tickets #94, #95 and #104: the seven GNU calls
;; that `cnfonts' makes, answered from the font layer, the two specifier
;; tags the sink half installs, and the GNU calls `cnfonts-ui' makes to
;; open and navigate the editor.  ctest runs `-batch', whose device has no
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

;;; The editor half: the GNU calls cnfonts-ui makes at load time

;; XEmacs searches a keymap's parents in order, so a composed map is a
;; sparse map with MAPS as its ordered parents.
(let* ((widget-like (make-sparse-keymap))
       (special-like (make-sparse-keymap))
       (composed (make-composed-keymap (list widget-like special-like))))
  (define-key widget-like "t" 'widget-command)
  (define-key special-like "t" 'special-command)
  (define-key special-like "q" 'special-quit)
  (Assert (eq (lookup-key composed "t") 'widget-command)
          "the first map wins in a composed keymap")
  (Assert (eq (lookup-key composed "q") 'special-quit)
          "a later map answers where the first is silent"))

(let* ((map (make-sparse-keymap))
       (parent (make-sparse-keymap))
       (composed (make-composed-keymap map parent)))
  (define-key map "a" 'map-command)
  (define-key parent "b" 'parent-command)
  (Assert (eq (lookup-key composed "a") 'map-command)
          "a single keymap argument is accepted")
  (Assert (eq (lookup-key composed "b") 'parent-command)
          "PARENT is searched after MAPS"))

(Assert (null (face-valid-attribute-values :box))
        "the value-list stand-in answers nil")

;; The widget variables `custom--initialize-widget-variables' sets are
;; defined by `cus-edit'; `cnfonts-ui' loads it before building a page.
(require 'cus-edit)
(with-temp-buffer
  (let ((custom-raised-buttons t))
    (custom--initialize-widget-variables)
    (Assert (eq widget-documentation-face 'custom-documentation-face)
            "widget documentation uses the custom face")
    (Assert (eq widget-button-face 'custom-button-face)
            "widget buttons use the custom face")
    (Assert (equal widget-push-button-prefix "")
            "raised buttons have no bracketing prefix")
    (Assert (equal widget-push-button-suffix "")
            "raised buttons have no bracketing suffix")))

(with-temp-buffer
  (custom--initialize-widget-variables)
  (Assert (equal widget-push-button-prefix "[")
          "without custom-raised-buttons the prefixes stay"))

(Assert (equal (char-to-string ?中) "中")
        "a character is passed through")
(Assert (equal (char-to-string 65) "A")
        "an ASCII integer names its character")
(Assert (equal (char-to-string 10102) (string (unicode-to-char 10102)))
        "a raw Unicode codepoint is converted to an XEmacs character")
(Assert (condition-case nil
            (progn (char-to-string #x110000) nil)
          (error t))
        "a codepoint beyond Unicode signals")
(Assert (condition-case nil
            (progn (char-to-string -1) nil)
          (error t))
        "a negative codepoint signals")

;;; Navigation: `cnfonts-ui-forward' needs an optional POS and a
;;; character-wise scan that can reach a tab stop at `point-min'.

(require 'wid-edit)
(with-temp-buffer
  (let* ((first (widget-create 'push-button :tag "first"
                               :tab-stop-point t))
         (_ (widget-insert " "))
         (second (widget-create 'push-button :tag "second")))
    (widget-setup)
    (goto-char (point-min))
    (Assert (eq (widget-at) first) "widget-at defaults to point")
    (Assert (eq (widget-at (point)) first) "widget-at takes POS")
    (widget-move 1)
    (Assert (eq (widget-at) second) "widget-move advances one widget")
    (widget-move 1)
    (Assert (and (= (point) (point-min)) (eq (widget-at) first))
            "widget-move wraps forward to a tab stop at point-min")
    (widget-move -1)
    (Assert (eq (widget-at) second) "widget-move backs up one widget")
    (widget-move -1)
    (Assert (and (= (point) (point-min)) (eq (widget-at) first))
            "widget-move wraps backward to the tab stop at point-min")))

;; The two degenerate buffers are what made XEmacs's own `widget-move'
;; unusable: `cnfonts-ui-forward' calls it until it reaches a tab stop, so
;; a buffer with no tabable widget must give up with an error and a
;; single-widget buffer must stop on that widget instead of looping.
(with-temp-buffer
  (Assert (condition-case nil
              (progn (widget-move 1) nil)
            (error t))
          "widget-move without any tabable widget signals"))

(with-temp-buffer
  (let ((message-log-max nil)
        (only (widget-create 'push-button :tag "only" :tab-stop-point t)))
    (widget-setup)
    (goto-char (point-min))
    (widget-move 1)
    (Assert (eq (widget-at) only)
            "widget-move stops on the only tabable widget")))

(with-temp-buffer
  (insert "plain text")
  (Assert (null (widget-at (point-min)))
          "widget-at answers nil where no widget sits"))

;;; GUI buttons: a window-system console without native widgets (this
;;; --with-widgets=no build) has no `button' image instantiator, so a
;;; push button must fall back to its text label instead of warning for
;;; every button cnfonts creates.

(require 'cl)
(with-temp-buffer
  (let ((widget-push-button-gui t))
    (flet ((console-on-window-system-p () t)
           (valid-image-instantiator-format-p (&rest _) nil))
      (widget-create 'push-button :tag "fallback"))
    (Assert (string-match "fallback" (buffer-string))
            "a push button is text when the button instantiator is missing")))

;;; The align page's mouse warp: window-relative pixels and a frame-taking
;;; `set-mouse-pixel-position'.

(Assert (integerp (default-font-width))
        "default-font-width answers a metric")
(Assert (integerp (default-font-height))
        "default-font-height answers a metric")
(Assert (or (null (posn-at-point (point-min)))
            (consp (posn-at-point (point-min))))
        "posn-at-point answers a cons or nil")
(with-temp-buffer
  (insert "one line")
  (Assert (null (posn-at-point (1+ (point-max))))
          "a position outside the buffer answers nil"))
(Assert (equal (posn-x-y '(7 . 9)) '(7 . 9))
        "posn-x-y returns a pixel pair unchanged")
(Assert (null (posn-x-y nil))
        "posn-x-y answers nil for a nil position")
(Assert (null (posn-x-y '(widget . field)))
        "posn-x-y answers nil for something that is not a pixel pair")

(let ((cnfonts-xemacs--mouse-pixel-position-primitive
       (lambda (window x y) (list window x y))))
  (Assert (equal (set-mouse-pixel-position (selected-frame) 3 4)
                 (list (frame-selected-window (selected-frame)) 3 4))
          "a frame maps to its selected window")
  (Assert (equal (set-mouse-pixel-position (selected-window) 3 4)
                 (list (selected-window) 3 4))
          "a window reaches the primitive unchanged"))

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
