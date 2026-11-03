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
;;
;; The C-side choice of fallback font is not visible from Lisp.  What this
;; gate can assert is the face instance for each charset (a real instance,
;; with a string truename naming the expected family) and that fontconfig
;; lists a covering font for each fallback codepoint; rendering the mixed
;; line is then the crash check for the UTF-32 split and fallback code.

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
    ;; 한 (U+D55C), charset korean-ksc5601
    (hangul (#xd55c))
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
configures are dimension-2 (CJK and Korean), JIT (EXT-B), plain (Latin),
and the dropped symbol/phonetic/ornament requests.")

(defconst xft-rendering-gui-mixed-line
  '(#x41 #x4e2d #x20109 #x20100 #x221a)
  "A single line with Latin (A), Han (中), an EXT-B codepoint that maps to
a real 2D charset (U+20109, chinese-cns11643-7), an EXT-B JIT codepoint
(U+20100, jit-ucs-charset-0) and a symbol, so redisplay must split it into
several fonts and draw it without crashing.")

(defconst xft-rendering-gui-all-lines
  (append xft-rendering-gui-lines
	  (list (list 'mixed xft-rendering-gui-mixed-line)))
  "The matrix plus the mixed-coverage line, in render order.")

(defconst xft-rendering-gui-charset-families
  '((ascii . "DejaVu Sans")
    (chinese-gb2312 . "SimSun")
    (japanese-jisx0208 . "SimSun")
    (japanese-jisx0212 . "SimSun")
    (chinese-cns11643-7 . "SimSun")
    ;; The hangul sample lands in korean-ksc5601 (the review note called
    ;; it chinese-ksc5601, but that charset does not exist in XEmacs).
    (korean-ksc5601 . "SimSun")
    (jit-ucs-charset-0 . "SimSun-ExtB"))
  "The family each matrix charset must resolve to.  A missing backstop
would let a charset pass on any real instance; every charset the matrix
produces is listed here.")

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

(defun xft-rendering-gui-tagged-specs (tag)
  "Return the instantiators filed under TAG, a tag symbol.
The tag is looked for with `memq' inside the spec's tag set, so tag
ordering and normalization cannot hide it."
  (let (result)
    (dolist (spec (xft-rendering-gui-global-specs) (nreverse result))
      (when (and (consp spec) (memq tag (car spec)))
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
    (dolist (entry xft-rendering-gui-all-lines)
      (dolist (codepoint (xft-rendering-gui-line-codepoints (nth 1 entry)))
	(unless (eq codepoint #x20)
	  (let* ((char (if (< codepoint 256)
			   codepoint
			 (unicode-to-char codepoint)))
		 (charset (char-charset char)))
	    (unless (assq charset seen)
	      (push (cons charset char) seen))))))
    (nreverse seen)))

(defun xft-rendering-gui-instance (charset)
  "Return CHARSET's default-face font instance on this device."
  (face-font-instance 'default (selected-device) charset))

(defun xft-rendering-gui-real-instance-p (charset)
  "Return non-nil when CHARSET resolves to a real font instance.
`Vthe_null_font_instance' passes `font-instance-p' but has a nil
truename, so a string truename is what separates it from a real font."
  (let ((instance (xft-rendering-gui-instance charset)))
    (and instance
	 (font-instance-p instance)
	 (stringp (font-instance-truename instance)))))

(defun xft-rendering-gui-instance-family-p (charset family)
  "Return non-nil when CHARSET's real instance names FAMILY."
  (and (xft-rendering-gui-real-instance-p charset)
       (xft-rendering-gui-truename-family-p
	(font-instance-truename (xft-rendering-gui-instance charset))
	family)))

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

    ;; This gate needs the real fonts.  A machine without them cannot say
    ;; anything about rendering, so skip rather than fail.
    (let ((families (font-family-list device)))
      (dolist (family '("DejaVu Sans" "SimSun" "SimSun-ExtB"))
	(unless (member family families)
	  (xft-rendering-gui-skip "%s is not installed" family)
	  (kill-emacs 0))))

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

    ;; Render the matrix and the mixed-coverage line.  A face with no
    ;; instance for a character is what `add_ichar_rune_1' substitutes `~'
    ;; for; redisplaying here is the crash check for the UTF-32 runs and
    ;; per-character fallback.  Running redisplay inside the check makes an
    ;; error in the draw path a FAIL instead of aborting the gate.
    (let* ((expected
	    (concat (mapconcat
		     #'(lambda (entry)
			 (xft-rendering-gui-string (nth 1 entry)))
		     xft-rendering-gui-all-lines "\n")
		    "\n"))
	   (buffer (get-buffer-create "*xft-rendering-gui*")))
      (switch-to-buffer buffer)
      (erase-buffer)
      (insert expected)
      (xft-rendering-gui-check
       `(progn
	  (goto-char (point-min))
	  (redisplay t)
	  (sit-for 0)
	  (equal (buffer-string) ,expected))
       "redisplay and sit-for render the matrix and the mixed line"))

    (let ((charsets (xft-rendering-gui-charsets)))
      ;; Every matrix charset needs a backstop; a new line must not slip
      ;; through without one.
      (dolist (entry charsets)
	(xft-rendering-gui-check
	 `(assq ',(car entry) xft-rendering-gui-charset-families)
	 (format "%s has a family backstop" (car entry))))

      ;; Every matrix charset, and every backstop the matrix does not
      ;; reach (chinese-gb2312), must resolve to a real instance naming
      ;; its family.
      (dolist (charset (sort (delete-duplicates
			      (append
			       (mapcar #'car charsets)
			       (mapcar #'car xft-rendering-gui-charset-families)))
			     #'(lambda (a b) (string< (symbol-name a)
						      (symbol-name b)))))
	(let ((family (cdr (assq charset xft-rendering-gui-charset-families))))
	  (xft-rendering-gui-check
	   `(xft-rendering-gui-real-instance-p ',charset)
	   (format "%s has a real instance, not `~'" charset))
	  (xft-rendering-gui-check
	   `(and ,family
		 (xft-rendering-gui-instance-family-p ',charset ,family))
	   (format "the %s instance's truename names %s"
		   charset (or family "a missing backstop"))))))
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
