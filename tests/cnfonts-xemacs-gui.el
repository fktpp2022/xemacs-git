;;; cnfonts-xemacs-gui.el --- cnfonts end-to-end gate (GUI build only)

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

;; Milestone (C)'s success gate: upstream cnfonts, through its own public
;; `cnfonts-set-font', configures XEmacs's font layer end to end.  Run it
;; against a GUI-enabled build of this tree -- the mapped one lives at
;; ~/projects/xemacs/build (HAVE_X_WINDOWS, HAVE_XFT) and its display comes
;; from WSLg (:0):
;;
;;     ~/projects/xemacs/build/bin/xemacs -no-init-file -no-site-file \
;;         -l tests/cnfonts-xemacs-gui.el
;;
;; Exits 0 when every check passes, 1 otherwise.  A build with no
;; graphical frame prints SKIP and exits 0.
;;
;; The platform's font naming, listing and availability probe are stubbed
;; with fixture family names, so the gate needs no CJK fonts installed and
;; asserts configuration, not rendering; `tests/font-layer-gui.el' owns the
;; real versions of all three.  The actual render check is manual, on a
;; machine with real fonts:
;;
;;   - install a Latin and a CJK font, choose both in a cnfonts profile,
;;     and run `(require 'cnfonts-xemacs)' `(require 'cnfonts)' then
;;     `(cnfonts-enable)';
;;   - open a buffer with Latin and Chinese lines of the same nominal
;;     size, e.g. "mmmm mmmm" over "中文 中文";
;;   - the Chinese glyphs must use the profile's CJK font and sit on the
;;     same baseline grid as the Latin text, not fall back to the Latin
;;     font and not render as empty boxes.
;;
;; A human records PASS or FAIL for that check on the milestone issue.

;;; Code:

(require 'font)
(require 'cnfonts-xemacs)

;; In an interactive XEmacs `standard-output' is the echo area, where the
;; results would be invisible.
(setq standard-output 'external-debugging-output)

(defvar cnfonts-xemacs-gui-failures 0)
(defvar cnfonts-xemacs-gui-checks 0)

(defconst cnfonts-xemacs-gui-tarball
  (expand-file-name "elpa/melpa-fixture/cnfonts-20251223.636.tar"
		    (file-name-directory
		     (or load-file-name buffer-file-name)))
  "The vendored upstream cnfonts tarball the gate installs from.")

(defconst cnfonts-xemacs-gui-package-dir
  (file-name-sans-extension
   (file-name-nondirectory cnfonts-xemacs-gui-tarball))
  "Top-level directory inside the vendored tarball.")

(defconst cnfonts-xemacs-gui-families
  '("FixtureLatin" "FixtureCJK" "FixtureEXTB" "FixtureSymbol"
    "FixtureOrnament")
  "Fixture family names the gate's font listing answers with.
The order is cnfonts's: Latin, CJK, EXT-B, symbol, ornament.  No hyphens or
spaces: cnfonts recovers a family by joining the XLFD fields with \"-\", so
a fixture name must survive that round trip unchanged.")

(defun cnfonts-xemacs-gui-check (form description)
  "Evaluate FORM; print PASS or FAIL for DESCRIPTION."
  (setq cnfonts-xemacs-gui-checks (1+ cnfonts-xemacs-gui-checks))
  (condition-case err
      (cond ((eval form)
	     (princ (format "PASS: %s\n" description)))
	    (t
	     (setq cnfonts-xemacs-gui-failures
		   (1+ cnfonts-xemacs-gui-failures))
	     (princ (format "FAIL: %s\n" description))))
    (error
     (setq cnfonts-xemacs-gui-failures
	   (1+ cnfonts-xemacs-gui-failures))
     (princ (format "FAIL: %s (%S)\n" description err)))))

(defun cnfonts-xemacs-gui-instantiators (tag-set)
  "Return the instantiators for TAG-SET on the `default' face's font.
`specifier-specs' answers one inst-pair when a tag-set has one
instantiator and a list of them otherwise; both shapes are normalized."
  (let ((specs (specifier-specs (face-property 'default 'font)
				'global tag-set t)))
    (cond ((null specs) nil)
	  ((consp (cdr specs)) (mapcar #'cdr specs))
	  (t (list (cdr specs))))))

(defmacro cnfonts-xemacs-gui-with-stubs (&rest body)
  "Run BODY with the platform's font naming, listing and probe answered
by fixtures.  The stubs stand in for installed fonts: a name carries its
family, the lister returns the fixture families, and the probe accepts
a name.  `font-layer-gui.el' owns the real versions."
  `(flet ((font-family-list (&optional _device)
	     cnfonts-xemacs-gui-families)
	  (font-list (&rest _args)
	     cnfonts-xemacs-gui-families)
	  (font-create-name (fontobj &optional _device)
	     (concat (or (car (font-family fontobj)) "font") "-device"))
	  (try-font-name (name &optional _device)
	     name))
     ,@body))

(defun cnfonts-xemacs-gui-set-font-from-fixtures (workdir)
  "Configure fonts through upstream `cnfonts-set-font' with fixtures.
WORKDIR keeps cnfonts's profile and config files out of the user's home.
The fixture profile is seeded with cnfonts's own profile writer and then
read back by `cnfonts-set-font', so its profile loading and merging run;
everything below them is cnfonts's untouched path."
  (cnfonts-xemacs-gui-with-stubs
   (let ((cnfonts-directory (expand-file-name "cnfonts/" workdir))
	 (cnfonts-profiles '("gate"))
	 (cnfonts-use-face-font-rescale nil)
	 (cnfonts--custom-set-fontnames
	  (mapcar #'list cnfonts-xemacs-gui-families))
	 (cnfonts--custom-set-fontsizes
	  '((12.5 15.0 15.0 12.5 12.5))))
     (make-directory cnfonts-directory t)
     (cnfonts--save-profile "gate")
     (setq cnfonts--custom-set-fontnames nil
	   cnfonts--custom-set-fontsizes nil)
     (cnfonts-set-font))))

(defun cnfonts-xemacs-gui-run ()
  "Run the checks; failures are counted in the globals."
  (let ((workdir (make-temp-file "cnfonts-gate-" t)))
    (unwind-protect
	(progn
	  (unless (zerop (call-process "tar" nil nil nil
				       "-xf" cnfonts-xemacs-gui-tarball
				       "-C" workdir))
	    (error "could not unpack %s" cnfonts-xemacs-gui-tarball))
	  (push (expand-file-name cnfonts-xemacs-gui-package-dir workdir)
		load-path)
	  (require 'cnfonts)

	  (cnfonts-xemacs-gui-check
	   '(eq 0 (string-match
		   (concat "^"
			   (regexp-quote
			    (expand-file-name cnfonts-xemacs-gui-package-dir
					      workdir)))
		   (or (locate-library "cnfonts") "")))
	   "cnfonts loads from the vendored tarball, not an installed copy")

	  (cnfonts-xemacs-gui-set-font-from-fixtures workdir)

	  (cnfonts-xemacs-gui-check
	   '(let ((descriptor (face-property 'default 'font-specification)))
	      (and (vectorp descriptor) (eql 12 (length descriptor))))
	   "the face's font specification holds a descriptor, not a string")

	  (cnfonts-xemacs-gui-check
	   '(equal (font-family (face-property 'default 'font-specification))
		   '("FixtureLatin"))
	   "the plain descriptor is the Latin font, not the last tagged call")

	  (cnfonts-xemacs-gui-check
	   '(member "FixtureCJK-device"
		    (cnfonts-xemacs-gui-instantiators
		     '(cnfonts-dimension-2)))
	   "the CJK descriptor sits under the dimension-2 tag")

	  (cnfonts-xemacs-gui-check
	   '(member "FixtureEXTB-device"
		    (cnfonts-xemacs-gui-instantiators
		     '(cnfonts-jit-charset)))
	   "the EXT-B descriptor sits under the JIT-charset tag")

	  (cnfonts-xemacs-gui-check
	   '(funcall (specifier-tag-charset-predicate 'cnfonts-dimension-2)
		     (find-charset 'chinese-gb2312))
	   "the dimension-2 predicate matches a Chinese charset")

	  (cnfonts-xemacs-gui-check
	   '(not (funcall (specifier-tag-charset-predicate
			   'cnfonts-dimension-2)
			  (find-charset 'ascii)))
	   "the dimension-2 predicate rejects ASCII"))
      (call-process "rm" nil nil nil "-rf" workdir)))
  cnfonts-xemacs-gui-failures)

(defun cnfonts-xemacs-gui-main ()
  "Print SKIP on a non-graphical frame, else run the checks and exit."
  (if (not (display-graphic-p))
      (progn
	(princ "SKIP: no graphical frame; run this against a GUI build\n")
	(kill-emacs 0))
    (cnfonts-xemacs-gui-run)
    (princ (format "%d checks, %d failures\n"
		   cnfonts-xemacs-gui-checks cnfonts-xemacs-gui-failures))
    (kill-emacs (if (> cnfonts-xemacs-gui-failures 0) 1 0))))

;; An interactive XEmacs would otherwise open its debugger and wait, so a
;; broken gate must still report and exit.
(condition-case err
    (cnfonts-xemacs-gui-main)
  (error
   (princ (format "FAIL: %S\n" err))
   (kill-emacs 1)))

;;; cnfonts-xemacs-gui.el ends here
