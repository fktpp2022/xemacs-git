;;; cnfonts-gui-harness.el --- shared checks and fixtures for the cnfonts GUI gates

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

;; The harness of the cnfonts GUI gates: the counting of checks, the
;; fixtures that stand in for installed fonts, and the reporting every
;; gate ends with.  It is loaded, not run --
;;
;;     ~/projects/xemacs/build/bin/xemacs -no-init-file -no-site-file \
;;         -l tests/cnfonts-xemacs-gui.el
;;     ~/projects/xemacs/build/bin/xemacs -no-init-file -no-site-file \
;;         -l tests/cnfonts-command-matrix-gui.el
;;
;; Each gate requires it with a path resolved from `load-file-name', so it
;; travels with the gates and nothing on the load path is consulted.
;;
;; The harness holds no assertions of its own about cnfonts; the gates do.
;; `font-layer-gui.el' owns the real font naming, listing and availability
;; probe these fixtures fake.

;;; Code:

(require 'font)

(defconst cnfonts-gui-tarball
  (expand-file-name "elpa/melpa-fixture/cnfonts-20251223.636.tar"
		    (file-name-directory
		     (or load-file-name buffer-file-name)))
  "The vendored upstream cnfonts tarball the gates install from.")

(defconst cnfonts-gui-package-dir
  (file-name-sans-extension
   (file-name-nondirectory cnfonts-gui-tarball))
  "Top-level directory inside the vendored tarball.")

(defconst cnfonts-gui-families
  '("FixtureLatin" "FixtureCJK" "FixtureEXTB" "FixtureSymbol"
    "FixtureOrnament")
  "Fixture family names the gates' font listing answers with.
The order is cnfonts's: Latin, CJK, EXT-B, symbol, ornament.  No hyphens or
spaces: cnfonts recovers a family by joining the XLFD fields with \"-\", so
a fixture name must survive that round trip unchanged.")

(defvar cnfonts-gui-failures 0)
(defvar cnfonts-gui-checks 0)

(defun cnfonts-gui-check (form description)
  "Evaluate FORM; print PASS or FAIL for DESCRIPTION."
  (setq cnfonts-gui-checks (1+ cnfonts-gui-checks))
  (condition-case err
      (cond ((eval form)
	     (princ (format "PASS: %s\n" description)))
	    (t
	     (setq cnfonts-gui-failures
		   (1+ cnfonts-gui-failures))
	     (princ (format "FAIL: %s\n" description))))
    (error
     (setq cnfonts-gui-failures
	   (1+ cnfonts-gui-failures))
     (princ (format "FAIL: %s (%S)\n" description err)))))

(defun cnfonts-gui-instantiators (tag-set)
  "Return the instantiators for TAG-SET on the `default' face's font.
`specifier-specs' answers one inst-pair when a tag-set has one
instantiator and a list of them otherwise; both shapes are normalized."
  (let ((specs (specifier-specs (face-property 'default 'font)
				'global tag-set t)))
    (cond ((null specs) nil)
	  ((consp (cdr specs)) (mapcar #'cdr specs))
	  (t (list (cdr specs))))))

(defmacro cnfonts-gui-with-stubs (&rest body)
  "Run BODY with the platform's font naming, listing and probe answered
by fixtures.  The stubs stand in for installed fonts: a name carries its
family, the lister returns the fixture families, and the probe accepts
a name.  `font-layer-gui.el' owns the real versions."
  `(flet ((font-family-list (&optional _device)
	     cnfonts-gui-families)
	  (font-list (&rest _args)
	     cnfonts-gui-families)
	  (font-create-name (fontobj &optional _device)
	     (concat (or (car (font-family fontobj)) "font") "-device"))
	  (try-font-name (name &optional _device)
	     name))
     ,@body))

(defun cnfonts-gui-unpack (workdir)
  "Install cnfonts from the vendored tarball into WORKDIR and load it.
WORKDIR is a temp directory the caller owns, so nothing here reads or
writes the user's own ELPA area or cnfonts profile directory."
  (unless (zerop (call-process "tar" nil nil nil
			       "-xf" cnfonts-gui-tarball
			       "-C" workdir))
    (error "could not unpack %s" cnfonts-gui-tarball))
  (push (expand-file-name cnfonts-gui-package-dir workdir) load-path)
  (require 'cnfonts))

(defun cnfonts-gui-set-font-from-fixtures (workdir &optional sizes)
  "Configure fonts through upstream `cnfonts-set-font' with fixtures.
WORKDIR keeps cnfonts's profile and config files out of the user's home.
SIZES is the (Latin CJK EXT-B symbol ornament) list to save, defaulting
to the canonical 12.5/15/15/12.5/12.5.  The fixture profile is seeded
with cnfonts's own profile writer and then read back by
`cnfonts-set-font', so its profile loading and merging run; everything
below them is cnfonts's untouched path."
  (cnfonts-gui-with-stubs
   (let ((cnfonts-directory (expand-file-name "cnfonts/" workdir))
	 (cnfonts-profiles '("gate"))
	 (cnfonts-use-face-font-rescale nil)
	 (sizes (or sizes '(12.5 15.0 15.0 12.5 12.5)))
	 (cnfonts--custom-set-fontnames
	  (mapcar #'list cnfonts-gui-families))
	 (cnfonts--custom-set-fontsizes (list sizes)))
     (make-directory cnfonts-directory t)
     (cnfonts--save-profile "gate")
     ;; `cnfonts-set-font' takes the English size from the cached config;
     ;; keep it in step with the profile just saved, as the UI would.
     (cnfonts--update-and-save-config "gate" (car sizes))
     (setq cnfonts--custom-set-fontnames nil
	   cnfonts--custom-set-fontsizes nil)
     (cnfonts-set-font))))

(defun cnfonts-gui-finish (run)
  "Print SKIP on a non-graphical frame, else run RUN and exit with its verdict.
Every gate ends the same way, so `ctest -L gui' grades them alike: exit 0
with no display, exit 1 when any check failed."
  (if (not (display-graphic-p))
      (progn
	(princ "SKIP: no graphical frame; run this against a GUI build\n")
	(kill-emacs 0))
    (funcall run)
    (princ (format "%d checks, %d failures\n"
		   cnfonts-gui-checks cnfonts-gui-failures))
    (kill-emacs (if (> cnfonts-gui-failures 0) 1 0))))

(provide 'cnfonts-gui-harness)

;;; cnfonts-gui-harness.el ends here
