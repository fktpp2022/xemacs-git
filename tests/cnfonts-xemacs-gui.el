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
;;     font and not render as empty boxes; the symbol and ornament rows
;;     must draw at the profile's symbol and ornament sizes, and a later
;;     `cnfonts-set-font' with edited sizes must re-render them at the new
;;     sizes without restarting.
;;
;; A human records PASS or FAIL for that check on the milestone issue.
;;
;; The symbol and ornament requests have no XEmacs charset to select them;
;; the shim lowers them into `xft-font-overrides', which the Xft
;; per-character fallback (src/redisplay-x.c) consults before charset
;; coverage.  The checks below assert that map's family and size for both
;; rows, and that re-applying an edited profile updates it in place.

;;; Code:

(require 'font)
(require 'cnfonts-xemacs)
;; The harness lives beside this file: load it by path, not by feature name,
;; so nothing on the load path is consulted.
(load (expand-file-name "cnfonts-gui-harness"
			(file-name-directory
			 (or load-file-name buffer-file-name))))

;; In an interactive XEmacs `standard-output' is the echo area, where the
;; results would be invisible.
(setq standard-output 'external-debugging-output)

(defun cnfonts-xemacs-gui-stale-range-posn ()
  "Read `posn-at-point' at a position the display has not reached.
The align page's rebuild empties its buffer and refills it, and
`cnfonts-ui--move-mouse' asks for the position before anything
redisplays, so `window-end' is the value the erased buffer left and the
new page below it is outside the displayed range.  Answers (POS . POSN)
when that staleness was reproduced, and nil when it was not -- a check
that cannot make the range stale would prove nothing.
The position is a few lines down, not near the window's bottom: the
estimate is a line count times the font's pixel height, so one taken
from the last visible line lands outside the window whenever the font is
tall enough for the window to hold fewer lines than the buffer."
  (let ((win (selected-window)))
    (let ((old-buffer (window-buffer win))
	  (buf (generate-new-buffer " *cnfonts-gui-stale*"))
	  (seen nil))
      (unwind-protect
	  (with-current-buffer buf
	    (set-window-buffer win buf)
	    (insert "\n")
	    (redisplay)
	    (erase-buffer)
	    (dotimes (i 40) (insert (format "line %d\n" i)))
	    (let ((pos (save-excursion
			 (goto-char (point-min))
			 (forward-line 3)
			 (point))))
	      (when (< (window-end win) pos)
		(setq seen (cons pos (posn-at-point pos win))))))
	(set-window-buffer win old-buffer)
	(kill-buffer buf))
      seen)))

(defun cnfonts-xemacs-gui-run ()
  "Run the checks; failures are counted in the globals."
  (let ((workdir (make-temp-file "cnfonts-gate-" t)))
    (unwind-protect
	(progn
	  (cnfonts-gui-unpack workdir)

	  (cnfonts-gui-check
	   '(eq 0 (string-match
		   (concat "^"
			   (regexp-quote
			    (expand-file-name cnfonts-gui-package-dir
					      workdir)))
		   (or (locate-library "cnfonts") "")))
	   "cnfonts loads from the vendored tarball, not an installed copy")

	  (cnfonts-gui-set-font-from-fixtures workdir)

	  (cnfonts-gui-check
	   '(let ((descriptor (face-property 'default 'font-specification)))
	      (and (vectorp descriptor) (eql 12 (length descriptor))))
	   "the face's font specification holds a descriptor, not a string")

	  (cnfonts-gui-check
	   '(equal (font-family (face-property 'default 'font-specification))
		   '("FixtureLatin"))
	   "the plain descriptor is the Latin font, not the last tagged call")

	  (cnfonts-gui-check
	   '(member "FixtureCJK-15-device"
		    (cnfonts-gui-instantiators
		     '(cnfonts-dimension-2)))
	   "the CJK descriptor sits under the dimension-2 tag")

	  (cnfonts-gui-check
	   '(member "FixtureEXTB-15-device"
		    (cnfonts-gui-instantiators
		     '(cnfonts-jit-charset)))
	   "the EXT-B descriptor sits under the JIT-charset tag")

	  (cnfonts-gui-check
	   '(funcall (specifier-tag-charset-predicate 'cnfonts-dimension-2)
		     (find-charset 'chinese-gb2312))
	   "the dimension-2 predicate matches a Chinese charset")

	  (cnfonts-gui-check
	   '(not (funcall (specifier-tag-charset-predicate
			   'cnfonts-dimension-2)
			  (find-charset 'ascii)))
	   "the dimension-2 predicate rejects ASCII")

	  (cnfonts-gui-check
	   '(let* ((seen (cnfonts-xemacs-gui-stale-range-posn))
		   (posn (cdr seen))
		   (edges (window-pixel-edges (selected-window))))
	      (and seen
		   (consp posn)
		   (>= (car posn) (nth 0 edges))
		   (< (car posn) (nth 2 edges))
		   (>= (cdr posn) (nth 1 edges))
		   (< (cdr posn) (nth 3 edges))))
	   "posn-at-point answers a position outside the stale displayed range")

	  (cnfonts-gui-check
	   '(equal (xft-font-override #x03b1) '("FixtureSymbol" . 12.5))
	   "the Greek override names the profile's symbol font and size")

	  (cnfonts-gui-check
	   '(equal (xft-font-override #x028c) '("FixtureSymbol" . 12.5))
	   "the IPA override names the profile's symbol font and size")

	  (cnfonts-gui-check
	   '(equal (xft-font-override #x2776)
		   '("FixtureOrnament" . 12.5))
	   "the ornament override names the profile's ornament font and size")

	  (cnfonts-gui-check
	   '(and (= 7 (length xft-font-overrides))
		 (equal (mapcar #'car xft-font-overrides)
			'((#x0370 . #x03FF) (#x0250 . #x02AF)
			  (#x1D00 . #x1DBF) (#x2776 . #x2793)
			  (#x24B6 . #x24FE) (#x2295 . #x22A1)
			  (#x2190 . #x2200))))
	   "the override map holds the symbol ranges then cnfonts's ornaments")

	  ;; Re-applying an edited profile must update the entries in place:
	  ;; the fallback cache is keyed on family and points, so a new size
	  ;; renders without restarting the editor.
	  (cnfonts-gui-set-font-from-fixtures
	   workdir '(10.0 15.0 15.0 20.0 22.0))

	  (cnfonts-gui-check
	   '(equal (xft-font-override #x03b1) '("FixtureSymbol" . 20.0))
	   "editing the symbol size updates the override")

	  (cnfonts-gui-check
	   '(equal (xft-font-override #x2776)
		   '("FixtureOrnament" . 22.0))
	   "editing the ornament size updates the override in place")

	  (cnfonts-gui-check
	   '(= 7 (length xft-font-overrides))
	   "re-applying the profile leaves one entry per range"))
      (call-process "rm" nil nil nil "-rf" workdir)))
  cnfonts-gui-failures)

(defun cnfonts-xemacs-gui-main ()
  "Print SKIP on a non-graphical frame, else run the checks and exit."
  (cnfonts-gui-finish 'cnfonts-xemacs-gui-run))

;; An interactive XEmacs would otherwise open its debugger and wait, so a
;; broken gate must still report and exit.
(condition-case err
    (cnfonts-xemacs-gui-main)
  (error
   (princ (format "FAIL: %S\n" err))
   (kill-emacs 1)))

;;; cnfonts-xemacs-gui.el ends here
