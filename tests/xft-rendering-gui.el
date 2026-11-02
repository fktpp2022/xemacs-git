;;; xft-rendering-gui.el --- Xft rendering gate: no `~' on the align page

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

;; The rendering half of the Xft font plan (#107, decision #112): after
;; the UTF-32 draw path (#113) and the JIT-charset tag split (#114), the
;; align page's characters reach redisplay with a font instance instead
;; of CANT_DISPLAY_CHAR (`~').  Run it against a GUI build with Xft on a
;; display that has the real Windows fonts -- on WSLg (:0):
;;
;;     DISPLAY=:0 build-gui/bin/xemacs -no-init-file -no-site-file \
;;         -l tests/xft-rendering-gui.el
;;
;; Exits 0 when every check passes, 1 otherwise.  A build with no
;; graphical frame prints SKIP and exits 0.  The checks are Xft-specific:
;; on a core-X device the Xft checks fail, they do not skip.
;;
;; The `default' face is configured through the cnfonts shim's tag route,
;; exactly as cnfonts's own calls lower onto it: a plain Latin family, a
;; SimSun request for the scripts, a prepended SimSun-ExtB request for
;; the nil charset (the JIT tag), and symbol/phonetic/ornament requests
;; the shim drops so that per-character fallback supplies those glyphs.
;; `font-layer-gui.el' owns the font-layer contract and
;; `cnfonts-xemacs-gui.el' the configuration contract; this file owns
;; what redisplay may substitute.
;;
;; The one human visual check this environment cannot automate (there is
;; no screenshot tool): open the align page with a SimSun / SimSun-ExtB
;; profile, e.g. `(require 'cnfonts-xemacs)' `(require 'cnfonts)' then
;; `(cnfonts-edit-profile)' and the align page.  Chinese must render in
;; SimSun, the EXT-B samples in SimSun-ExtB, symbols and ornaments must
;; be visible, and no character may appear as `~'.  A human records PASS
;; or FAIL for that check on the milestone issue.

;;; Code:

(require 'font)
(require 'cnfonts-xemacs)

;; In an interactive XEmacs `standard-output' is the echo area, where the
;; results would be invisible.
(setq standard-output 'external-debugging-output)

(defvar xft-rendering-gui-failures 0)
(defvar xft-rendering-gui-checks 0)

(defun xft-rendering-gui-check (form description)
  "Evaluate FORM; print PASS or FAIL for DESCRIPTION."
  (setq xft-rendering-gui-checks (1+ xft-rendering-gui-checks))
  (condition-case err
      (cond ((eval form)
	     (princ (format "PASS: %s\n" description)))
	    (t
	     (setq xft-rendering-gui-failures
		   (1+ xft-rendering-gui-failures))
	     (princ (format "FAIL: %s\n" description))))
    (error
     (setq xft-rendering-gui-failures
	   (1+ xft-rendering-gui-failures))
     (princ (format "FAIL: %s (%S)\n" description err)))))

(defun xft-rendering-gui-skip (format-string &rest args)
  (princ (apply #'format (concat "SKIP: " format-string "\n") args)))

(defconst xft-rendering-gui-lines
  '((latin "ABC xyz")
    ;; 中文 你好
    (cjk (#x4e2d #x6587 #x20 #x4f60 #x597d))
    ;; EXT-B: U+20100 and U+20109.  XEmacs maps the first to the JIT
    ;; charset and the second to chinese-cns11643-7; both must render.
    (ext-b (#x20100 #x20109))
    ;; √ ← ϕ ʌ ɯ ʊ ❶ Ⓐ
    (symbol (#x221a #x2190 #x03d5 #x028c #x026f #x028a
	     #x2776 #x24b6))
    ;; cnfonts's ornament ranges, distinct from the symbol line:
    ;; ❷ ➓ ⓩ ⊖ ⊡ ⇒ ∀
    (ornament (#x2777 #x2793 #x24e9 #x2296 #x22a1 #x21d2 #x2200)))
  "One line per rendering class; codepoints are given numerically so the
file's own encoding cannot change the matrix.  The classes cnfonts
configures are dimension-2 (CJK), JIT (EXT-B), plain (Latin), and the
dropped symbol/phonetic/ornament requests.")

(defconst xft-rendering-gui-covered-codepoints
  (append '(#x20100 #x20109)			; EXT-B
	  (cadr (assq 'symbol xft-rendering-gui-lines))
	  (cadr (assq 'ornament xft-rendering-gui-lines)))
  "The codepoints per-character fallback must supply a covering font for.")

(defun xft-rendering-gui-string (line)
  "Return LINE's display string, a string or a list of codepoints."
  (if (stringp line)
      line
    (mapconcat #'(lambda (codepoint)
		   (string (unicode-to-char codepoint)))
	       line "")))

(defun xft-rendering-gui-line-codepoints (line)
  "Return LINE's codepoints as a list."
  (if (stringp line)
      (append line nil)
    line))

(defun xft-rendering-gui-truename-family-p (truename family)
  "Return non-nil when fontconfig TRUENAME names FAMILY.
Fontconfig escapes a hyphen inside a family as `\\-', so an escaped or
bare hyphen is accepted between the family's parts.  The trailing
boundary is a comma, an unescaped hyphen, or the end of the family
field, which keeps `SimSun' from accepting `SimSun\\-ExtB'."
  (and (stringp truename)
       (let ((regexp (concat "\\`"
			     (mapconcat #'regexp-quote
					(split-string family "-")
					"\\\\?-")
			     "\\([-,]\\|$\\)")))
	 (string-match regexp truename))))

(defun xft-rendering-gui-global-specs ()
  "Return the default face's global font instantiators."
  (specifier-specs (face-property 'default 'font) 'global))

(defun xft-rendering-gui-tagged-specs (&rest tag-set)
  "Return the instantiators under TAG-SET."
  (let (result)
    (dolist (spec (xft-rendering-gui-global-specs) (nreverse result))
      (when (and (consp spec) (equal (car spec) tag-set))
	(push (cdr spec) result)))))

(defun xft-rendering-gui-plain-specs ()
  "Return the untagged instantiators."
  (let (result)
    (dolist (spec (xft-rendering-gui-global-specs) (nreverse result))
      (when (stringp spec)
	(push spec result)))))

(defun xft-rendering-gui-charsets ()
  "Return an alist of each matrix charset to its first character."
  (let (seen)
    (dolist (entry xft-rendering-gui-lines)
      (dolist (codepoint (xft-rendering-gui-line-codepoints (nth 1 entry)))
	(unless (eq codepoint #x20)
	  (let* ((char (if (< codepoint 256)
			   codepoint
			 (unicode-to-char codepoint)))
		 (charset (char-charset char)))
	    (unless (assq charset seen)
	      (push (cons charset char) seen))))))
    (nreverse seen)))

(defun xft-rendering-gui-fc-covered-p (codepoint)
  "Return non-nil when fontconfig lists an installed font covering CODEPOINT."
  (with-temp-buffer
    (let ((status (call-process "fc-list" nil (current-buffer) nil
				(format ":charset=%x" codepoint)
				"family")))
      (and (eq status 0)
	   (not (string= "" (buffer-string)))))))

(defun xft-rendering-gui-configure ()
  "Configure the default face through the cnfonts shim's tag route."
  (set-face-attribute 'default nil
		      :font (font-spec :name "DejaVu Sans" :size 12))
  (dolist (script '(kana han cjk-misc bopomofo hangul))
    (set-fontset-font "fontset-default" script
		      (font-spec :name "SimSun" :size 12)))
  (set-fontset-font "fontset-default" nil
		    (font-spec :name "SimSun-ExtB" :size 12) nil 'prepend))

(defun xft-rendering-gui-run ()
  "Run the checks; failures are counted in the globals."
  (let ((device (selected-device)))
    (xft-rendering-gui-check '(featurep 'xft-fonts)
			      "the device renders with Xft")

    (let ((families (font-family-list device)))
      (dolist (family '("DejaVu Sans" "SimSun" "SimSun-ExtB"))
	(xft-rendering-gui-check
	 `(member ,family families)
	 (format "the %s family is installed (this gate needs the real font)"
		 family))))

    (xft-rendering-gui-configure)

    (xft-rendering-gui-check
     '(member-if #'(lambda (name)
		     (xft-rendering-gui-truename-family-p name "DejaVu Sans"))
		 (xft-rendering-gui-plain-specs))
     "the plain font is DejaVu Sans")

    (xft-rendering-gui-check
     '(member-if #'(lambda (name)
		     (xft-rendering-gui-truename-family-p name "SimSun"))
		 (xft-rendering-gui-tagged-specs 'cnfonts-dimension-2))
     "the dimension-2 tag carries SimSun")

    (xft-rendering-gui-check
     '(member-if #'(lambda (name)
		     (xft-rendering-gui-truename-family-p
		      name "SimSun-ExtB"))
		 (xft-rendering-gui-tagged-specs 'cnfonts-jit-charset))
     "the JIT-charset tag carries SimSun-ExtB")

    ;; The shim must drop the requests no XEmacs charset can select; a
    ;; stray instantiator would shadow the tag route asserted above.
    (let ((before (xft-rendering-gui-global-specs)))
      (dolist (script '(symbol phonetic))
	(set-fontset-font "fontset-default" script
			  (font-spec :name "Segoe UI Symbol" :size 12)
			  nil 'prepend))
      (xft-rendering-gui-check
       `(equal ',before (xft-rendering-gui-global-specs))
       "the shim drops the symbol and phonetic requests"))

    (let ((before (xft-rendering-gui-global-specs)))
      (dolist (range '((#x2776 . #x2793) (#x24b6 . #x24fe)
		       (#x2295 . #x22a1) (#x2190 . #x2200)))
	(set-fontset-font "fontset-default" range
			  (font-spec :name "MS Gothic" :size 12)
			  nil 'prepend))
      (xft-rendering-gui-check
       `(equal ',before (xft-rendering-gui-global-specs))
       "the shim drops the ornament range requests"))

    ;; Render the whole matrix once.  A face with no instance for a
    ;; character is what `add_ichar_rune_1' substitutes `~' for, and the
    ;; draw path itself must survive the UTF-32 runs and fallback.
    (let ((buffer (get-buffer-create "*xft-rendering-gui*")))
      (switch-to-buffer buffer)
      (erase-buffer)
      (dolist (entry xft-rendering-gui-lines)
	(insert (xft-rendering-gui-string (nth 1 entry)) "\n"))
      (goto-char (point-min))
      (redisplay t)
      (sit-for 0)
      (xft-rendering-gui-check
       `(> (buffer-size) 0)
       "the rendering matrix displays without a crash"))

    (dolist (entry (xft-rendering-gui-charsets))
      (let ((charset (car entry))
	    (char (cdr entry)))
	(xft-rendering-gui-check
	 `(let ((instance (face-font-instance 'default (selected-device)
					      ',charset)))
	    (and instance (font-instance-p instance)))
	 (format "%s (U+%04X) has a font instance, not `~'"
		 charset (char-to-unicode char)))))

    (dolist (charset '(japanese-jisx0208 japanese-jisx0212
		       chinese-gb2312))
      (xft-rendering-gui-check
       `(let ((instance (face-font-instance 'default (selected-device)
					    ',charset)))
	  (and instance
	       (xft-rendering-gui-truename-family-p
		(font-instance-truename instance) "SimSun")))
       (format "the %s instance's truename names SimSun" charset)))

    (xft-rendering-gui-check
     '(let ((instance (face-font-instance 'default (selected-device)
					  'jit-ucs-charset-0)))
	(and instance
	     (xft-rendering-gui-truename-family-p
	      (font-instance-truename instance) "SimSun-ExtB")))
     "the jit-ucs-charset-0 instance's truename names SimSun-ExtB")

    (dolist (codepoint xft-rendering-gui-covered-codepoints)
      (xft-rendering-gui-check
       `(xft-rendering-gui-fc-covered-p ,codepoint)
       (format (concat "U+%04X has an installed covering font"
		       " (fc-list :charset=%x; needs fontconfig)")
	       codepoint codepoint)))))

(defun xft-rendering-gui-main ()
  "Print SKIP on a non-graphical frame, else run the checks and exit."
  (if (not (display-graphic-p))
      (progn
	(xft-rendering-gui-skip
	 "no graphical frame; run this against an Xft GUI build\n")
	(kill-emacs 0))
    (xft-rendering-gui-run)
    (princ (format "%d checks, %d failures\n"
		   xft-rendering-gui-checks xft-rendering-gui-failures))
    (kill-emacs (if (> xft-rendering-gui-failures 0) 1 0))))

;; An interactive XEmacs would otherwise open its debugger and wait, so a
;; broken gate must still report and exit.
(condition-case err
    (xft-rendering-gui-main)
  (error
   (princ (format "FAIL: %S\n" err))
   (kill-emacs 1)))

;;; xft-rendering-gui.el ends here
