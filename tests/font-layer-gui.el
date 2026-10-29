;;; font-layer-gui.el --- device-level font-layer checks (GUI build only)

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

;; The device-level half of the font layer's verification plan (ADR 0003).
;; The ctest harness runs `-batch', which gives a `stream' device even in a
;; GUI build, so these checks need their own run:
;;
;;     xemacs -l tests/font-layer-gui.el
;;
;; Exits 0 when every check passes, 1 otherwise.  On an Xft build the checks
;; are the Xft contract; on a core-X build they are the compatibility gate.

;;; Code:

(require 'font)

;; In an interactive XEmacs `standard-output' is the echo area, where the
;; results would be invisible.
(setq standard-output 'external-debugging-output)

(defvar font-layer-gui-failures 0)
(defvar font-layer-gui-checks 0)

(defun font-layer-gui-check (form description)
  "Evaluate FORM; print PASS or FAIL for DESCRIPTION."
  (setq font-layer-gui-checks (1+ font-layer-gui-checks))
  (condition-case err
      (cond ((eval form)
	     (princ (format "PASS: %s\n" description)))
	    (t
	     (setq font-layer-gui-failures (1+ font-layer-gui-failures))
	     (princ (format "FAIL: %s\n" description))))
    (error
     (setq font-layer-gui-failures (1+ font-layer-gui-failures))
     (princ (format "FAIL: %s (%S)\n" description err)))))

(defun font-layer-gui-skip (format-string &rest args)
  (princ (apply #'format (concat "SKIP: " format-string "\n") args)))

(defun font-layer-gui-family-has-slant-p (family slant device)
  (let ((pattern (make-fc-pattern)))
    (fc-pattern-add-family pattern family)
    (fc-pattern-add-slant pattern slant)
    (consp (fc-try-font pattern device))))

(defun font-layer-gui-family-has-weight-p (family weight device)
  (let ((pattern (make-fc-pattern)))
    (fc-pattern-add-family pattern family)
    (fc-pattern-add-weight pattern weight)
    (consp (fc-try-font pattern device))))

(defun font-layer-gui-roundtrip (font device)
  (font-create-object (font-create-name font device) device))

(let* ((device (selected-device))
       (xft (featurep 'xft-fonts))
       (names (font-list "*" device 50))
       (families (font-family-list device))
       (family (or (car (member "DejaVu Sans" families))
		   (car families))))

  (font-layer-gui-check '(eq 'x (device-type (selected-device)))
			"the selected device is an x device")

  (font-layer-gui-check 'names "font-list returns fonts")

  (if xft
      (progn
	(font-layer-gui-check
	 '(every #'(lambda (name) (not (eq ?- (aref name 0)))) names)
	 "font-list returns no XLFDs on an Xft build")

	(font-layer-gui-check
	 '(every #'(lambda (name) (make-font-instance name device t)) names)
	 "the first listed fonts all load on an Xft build")

	(font-layer-gui-check 'families "font-family-list returns families")

	(font-layer-gui-check
	 '(equal (font-family (font-layer-gui-roundtrip
			       (make-font :family family :size 14) device))
		 (font-family (make-font :family family :size 14)))
	 "the family round-trips through name and object")

	(if (font-layer-gui-family-has-weight-p
	     family fc-font-name-weight-bold device)
	    (font-layer-gui-check
	     '(eq :bold (font-weight
			 (font-layer-gui-roundtrip
			  (make-font :family family :weight :bold :size 14)
			  device)))
	     "the weight round-trips through name and object")
	  (font-layer-gui-skip "weight round-trip: %s has no bold variant"
			       family))

	(let ((slant-family
	       (cond ((font-layer-gui-family-has-slant-p
		       family fc-font-name-slant-italic device)
		      family)
		     ((and (member "Latin Modern Roman" families)
			   (font-layer-gui-family-has-slant-p
			    "Latin Modern Roman" fc-font-name-slant-italic device))
		      "Latin Modern Roman"))))
	  (if slant-family
	      (font-layer-gui-check
	       '(font-italic-p
		 (font-layer-gui-roundtrip
		  (let ((font (make-font :family slant-family :size 14)))
		    (set-font-italic-p font t)
		    font)
		  device))
	       "the slant round-trips through name and object")
	    (font-layer-gui-skip "slant round-trip: no italic family to test")))

	(font-layer-gui-check
	 '(let ((size (font-size
		       (font-layer-gui-roundtrip
			(make-font :family family :size "14pt") device))))
	    (and (numberp size) (< (abs (- size 14)) 0.5)))
	 "a size string lowers to points")

	(font-layer-gui-check
	 '(null (try-font-name "definitely-not-a-font" device))
	 "try-font-name rejects a font that does not exist")

	(let ((face (make-face 'font-layer-gui-face)))
	  (make-face-family face family)
	  (font-layer-gui-check
	   '(equal (list family)
		   (font-family (font-create-object (face-font-name face device)
						    device)))
	   "make-face-family changes the rendered font's family"))

	(let ((cjk (or (car (member "Noto Sans CJK SC" families))
		       (car (member-if #'(lambda (f)
					   (string-match "CJK\\|AR PL\\|Baekmuk\\|WenQuanYi" f))
				       families)))))
	  (if (and cjk (find-charset 'chinese-gb2312))
	      (let ((face (make-face 'font-layer-gui-tagged-face)))
		(set-face-font face (format "%s-14" family) device)
		(set-face-font face (format "%s-14" cjk) device
			       '(two-dimensional final))
		(font-layer-gui-check
		 '(equal (list cjk)
			 (font-family
			  (font-create-object
			   (face-font-name face device 'chinese-gb2312)
			   device)))
		 "per-charset selection still routes through a tag-set"))
	    (font-layer-gui-skip
	     "per-charset selection: needs a CJK family and chinese-gb2312"))))

    ;; Core-X build: every existing behaviour must be unchanged.
    (progn
      (font-layer-gui-check
       '(every #'(lambda (name) (eq ?- (aref name 0))) names)
       "font-list returns XLFDs on a core-X build")
      (font-layer-gui-check
       '(null (try-font-name "definitely-not-a-font" device))
       "try-font-name is a real existence check on a core-X build"))))

(princ (format "%d checks, %d failures\n"
	       font-layer-gui-checks font-layer-gui-failures))
(kill-emacs (if (> font-layer-gui-failures 0) 1 0))
