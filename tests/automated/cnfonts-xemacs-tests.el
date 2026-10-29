;;; cnfonts-xemacs-tests.el --- tests for the cnfonts compatibility layer

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

;; The device-free half of ticket #94: the load-bearing five GNU calls that
;; `cnfonts' makes, answered from the font layer.  ctest runs `-batch',
;; whose device has no row in `font-window-system-mappings', so the probe is
;; stubbed with fixtures: the assertions are about the shapes the
;; compatibility layer promises, not about the fonts installed on this
;; machine.

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
