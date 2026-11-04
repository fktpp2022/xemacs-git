;;; cnfonts-command-matrix-gui.el --- every cnfonts command runs on XEmacs (GUI build only)

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

;; The command-matrix gate: upstream cnfonts's thirty interactive commands,
;; each invoked in a live GUI XEmacs and each asserted on observable state,
;; not merely on the absence of an error.  Run it against a GUI-enabled
;; build of this tree:
;;
;;     ~/projects/xemacs/build/bin/xemacs -no-init-file -no-site-file \
;;         -l tests/cnfonts-command-matrix-gui.el
;;
;; Exits 0 when every check passes, 1 otherwise.  A build with no
;; graphical frame prints SKIP and exits 0.
;;
;; Two sections, one process.  The SWEEP is the command matrix, one row
;; per command in an order that satisfies the side effects the rows have on
;; each other.  The SESSION then works the editor as a user does, so that
;; at least one widget press travels the whole event path with nothing
;; stubbed underneath it.
;;
;; The assertion is per class: a tagged font instantiator for the size
;; commands, a saved profile for the profile commands, the named buffer for
;; the page commands, widget state for the widget commands -- and for the
;; two commands with no XEmacs equivalent, a message naming the missing GNU
;; command rather than a signal.
;;
;; The commands come from the vendored tarball and the platform's font
;; layer is answered by the harness's fixture families, so this gate needs
;; no CJK fonts installed and asserts configuration, not rendering: glyph
;; appearance is the human walkthrough's, and `tests/cnfonts-xemacs-gui.el'
;; owns the rendering contract this gate depends on.  Nothing here creates
;; a frame or moves the pointer -- the align page's row buttons do not warp
;; at all, and the two page buttons that do bind `cnfonts-ui--move-mouse'
;; only inside their own `let', which no swept command reaches.
;;
;; The swept command list is derived at runtime from the loaded package and
;; compared with the matrix, so a command upstream adds cannot arrive
;; unasserted: the difference is printed as well as failed.

;;; Code:

(load (expand-file-name "cnfonts-gui-harness"
			(file-name-directory
			 (or load-file-name buffer-file-name))))
(require 'cnfonts-xemacs)

;; In an interactive XEmacs `standard-output' is the echo area, where the
;; results would be invisible.
(setq standard-output 'external-debugging-output)

(defconst cnfonts-command-matrix-gui-pages
  '(start-page english-fonts-page chinese-fonts-page extb-fonts-page
    symbol-fonts-page ornament-fonts-page align-page key-page help-page)
  "The nine pages of the editor, in the order the matrix sweeps them.")

(defvar cnfonts-command-matrix-gui-answers nil
  "Prompts the row being run expects, in order, as (PROMPT . ANSWER).")

(defvar cnfonts-command-matrix-gui-moves 0
  "How many times `widget-move' has run in the current row.
`cnfonts-ui-forward' loops until it reaches a tab stop, so an endless
wrap would hang the gate; the guard makes it fail instead.")

(defvar cnfonts-command-matrix-gui-baseline nil
  "The size, font name or profile the row recorded before acting.")

(defvar cnfonts-command-matrix-gui-cell nil
  "The align page widget a later row acts on through its argument.")

;;; The matrix.
;;;
;;; A row is (COMMAND SETUP ARGS CLASS ASSERTION DESCRIPTION ANSWERS):
;;;
;;;   SETUP      run before the command to place point and choose the
;;;              buffer the command expects, recording any before-value
;;;              the assertion needs.
;;;   ARGS       the argument list.  Two commands read `last-input-event'
;;;              through `interactive', so calling them with no argument
;;;              is a wrong-number-of-arguments signal, not a test.
;;;   CLASS      `message' asserts the echo area names a missing GNU
;;;              command; `state' evaluates ASSERTION afterwards.
;;;   ANSWERS    the prompts the command is expected to ask, which
;;;              `with-answers' checks the wording of before answering.
;;;
;;; The order is load-bearing.  The size commands accumulate, so one moves
;;; the profile up and the next brings it back.  The page and widget
;;; commands need the editor open, so `cnfonts-edit-profile' precedes them.
;;; `cnfonts-regenerate-profile' overwrites the chosen font names with
;;; fallbacks, so it comes after every row that reads them, and
;;; `cnfonts-mode' closes the sweep it opened.

(defconst cnfonts-command-matrix-gui-rows
  (append
   `((cnfonts-mode
      nil (list 1) state (bound-and-true-p cnfonts-mode)
      "cnfonts-mode with a positive argument enables the mode")
     (cnfonts-set-font
      nil nil state
      (member "FixtureCJK-device" (cnfonts-gui-instantiators
				   '(cnfonts-dimension-2)))
      "cnfonts-set-font puts the CJK font under the dimension-2 tag")
     (cnfonts-increase-fontsize
      (setq cnfonts-command-matrix-gui-baseline
	    (cnfonts--get-profile-fontsize
	     (cnfonts--get-current-profile t)))
      nil state
      (> (cnfonts--get-profile-fontsize
	  (cnfonts--get-current-profile t))
	 cnfonts-command-matrix-gui-baseline)
      "cnfonts-increase-fontsize moves the profile size up one step")
     (cnfonts-decrease-fontsize
      nil nil state
      (= (cnfonts--get-profile-fontsize
	  (cnfonts--get-current-profile t))
	 cnfonts-command-matrix-gui-baseline)
      "cnfonts-decrease-fontsize brings it back to the step it started from")
     (cnfonts-reset-fontsize
      nil nil state
      (= (cnfonts--get-profile-fontsize
	  (cnfonts--get-current-profile t))
	 cnfonts-command-matrix-gui-baseline)
      "cnfonts-reset-fontsize re-applies that step unchanged")
     (cnfonts-switch-profile
      nil nil state
      (string= (cnfonts--get-current-profile t) "other")
      "cnfonts-switch-profile selects the profile the prompt answered"
      (("Set cnfonts profile to:" . "other")))
     (cnfonts-next-profile
      nil nil state
      (string= (cnfonts--get-current-profile t) "gate")
      "cnfonts-next-profile advances to the following profile")
     (cnfonts-edit-profile
      nil nil state
      (string= (buffer-name) " *cnfonts: start-page*")
      "cnfonts-edit-profile opens the editor on the start page")
     (cnfonts-ui
      nil nil state
      (and (= 9 (length (delq nil (mapcar (lambda (page)
					    (get-buffer
					     (format " *cnfonts: %S*" page)))
					  cnfonts-command-matrix-gui-pages))))
	   (string= (buffer-name) " *cnfonts: start-page*"))
      "cnfonts-ui builds all nine pages and shows the first")
     (cnfonts-ui-mode
      (set-buffer (get-buffer-create " *cnfonts: mode probe*"))
      nil state
      (and (eq major-mode 'cnfonts-ui-mode)
	   (derived-mode-p 'special-mode)
	   (eq (current-local-map) cnfonts-ui-mode-map))
      "cnfonts-ui-mode derives special-mode and installs its keymap")
     ,@(mapcar (lambda (page)
		 `(,(intern (concat "cnfonts-ui-page-" (symbol-name page)))
		   nil nil state
		   (string= (buffer-name)
			    (format " *cnfonts: %S*"
				    ',page))
		   ,(format "the %S page command makes its buffer current"
			    page)))
	       cnfonts-command-matrix-gui-pages))
   `((cnfonts-ui-next-page
      (cnfonts-ui--switch-to-page 'start-page) nil state
      (string= (buffer-name) " *cnfonts: english-fonts-page*")
      "cnfonts-ui-next-page steps from the start page to the next")
     (cnfonts-ui-previous-page
      nil nil state
      (string= (buffer-name) " *cnfonts: start-page*")
      "cnfonts-ui-previous-page steps back")
     (cnfonts-ui-restart
      (cnfonts-ui--switch-to-page 'align-page) nil state
      (and (string= (buffer-name) " *cnfonts: align-page*")
	   (consp cnfonts-ui--widgets-alist))
      "cnfonts-ui-restart rebuilds the page it runs from and stays on it")
     (cnfonts-ui-forward
      (progn
	(cnfonts-ui--switch-to-page 'chinese-fonts-page)
	(goto-char (point-min)))
      nil state
      (widget-get (widget-at) :tab-stop-point)
      "cnfonts-ui-forward stops on a tab stop instead of wrapping past it")
     (cnfonts-ui-backward
      nil nil state
      (widget-get (widget-at) :tab-stop-point)
      "cnfonts-ui-backward stops on a tab stop going the other way")
     (cnfonts-ui-toggle-select-font
      (progn
	(cnfonts-ui--switch-to-page 'chinese-fonts-page)
	(let ((cell (cnfonts-command-matrix-gui-second-row)))
	  (setq cnfonts-command-matrix-gui-baseline
		(widget-get cell :font-name))
	  (goto-char (widget-get cell :from))))
      nil state
      (string= (car (nth 1 cnfonts--custom-set-fontnames))
	       cnfonts-command-matrix-gui-baseline)
      "cnfonts-ui-toggle-select-font puts the clicked font first in its slot")
     (cnfonts-ui-increase-align
      (progn
	(cnfonts-ui--switch-to-page 'align-page)
	(let ((cell (cnfonts-command-matrix-gui-size-button "+")))
	  (setq cnfonts-command-matrix-gui-baseline
		(cnfonts-command-matrix-gui-cjkv-size)
		cnfonts-command-matrix-gui-cell cell)
	  (goto-char (widget-get cell :from))))
      nil state
      (> (cnfonts-command-matrix-gui-cjkv-size)
	 cnfonts-command-matrix-gui-baseline)
      "cnfonts-ui-increase-align raises the size on the row it sits beside")
     (cnfonts-ui-decrease-align
      nil
      (list cnfonts-command-matrix-gui-cell) state
      (= (cnfonts-command-matrix-gui-cjkv-size)
	 cnfonts-command-matrix-gui-baseline)
      "cnfonts-ui-decrease-align lowers it again, passed as a widget")
     (cnfonts-mouse-wheel
      nil (list (cnfonts-command-matrix-gui-event)) message
      "mouse-wheel-text-scale"
      "cnfonts-mouse-wheel names the command this build lacks")
     (cnfonts-touch-screen-pinch
      nil (list (cnfonts-command-matrix-gui-event)) message
      "touch-screen-pinch"
      "cnfonts-touch-screen-pinch names the command this build lacks")
     (cnfonts-regenerate-profile
      nil nil state
      (file-exists-p (cnfonts--get-profile "gate"))
      "cnfonts-regenerate-profile rewrites the profile it confirms"
      (("Regenerate profile: " . "gate")
       ("Regenerate (gate)? " . t)))
     (cnfonts-mode
      nil (list -1) state (not (bound-and-true-p cnfonts-mode))
      "cnfonts-mode with a negative argument disables the mode"))))

;;; Where a row reaches the page's widgets.

(defun cnfonts-command-matrix-gui-cjkv-size ()
  "The CJKV size the align page shows, which its buttons edit."
  (nth 1 (cnfonts--get-fontsizes
	  (cnfonts--get-profile-fontsize
	   (cnfonts--get-current-profile t)))))

(defun cnfonts-command-matrix-gui-page-widgets (predicate)
  "The current page's widgets satisfying PREDICATE, newest row first.
XEmacs's widgets are conses in the page's own
`cnfonts-ui--widgets-alist', not extents over the buffer."
  (mapcar #'car
	  (delq nil (mapcar (lambda (cell)
			      (and (funcall predicate (car cell)) cell))
			    cnfonts-ui--widgets-alist))))

(defun cnfonts-command-matrix-gui-second-row ()
  "The checkbox of the second font on the fonts page shown.
The first is the font already selected: toggling it off would leave the
slot holding a fallback name, and the assertion is about the choice made."
  (let ((cells (cnfonts-command-matrix-gui-page-widgets
		(lambda (widget)
		  (and (widget-get widget :flag)
		       (eq 1 (widget-get widget :index)))))))
    (or (nth 1 cells)
	(error "the fonts page shows %s selectable rows" (length cells)))))

(defun cnfonts-command-matrix-gui-size-button (sign)
  "The [+] or [-] button of the align page's CJKV row."
  (let ((cells (cnfonts-command-matrix-gui-page-widgets
		(lambda (widget)
		  (and (equal (widget-get widget :tag)
			      (format "[%s]" sign))
		       (eq 1 (widget-get widget :index)))))))
    (or (car cells)
	(error "the align page has no [%s] button on its CJKV row" sign))))

(defun cnfonts-command-matrix-gui-event ()
  "A mouse-wheel shaped event object, for the two event commands.
Neither reads it on this build -- they answer their `functionp' guard and
message first -- but `interactive' hands them one, so the argument is
there even though its contents are not looked at."
  (vector 'mouse-absolute 0 0 0))

;;; Answering prompts.

(defun cnfonts-command-matrix-gui-answer (kind prompt)
  "Check PROMPT against the next answer KIND expects, and give that answer.
A reworded prompt fails the gate here instead of silently feeding a wrong
answer to the command."
  (let ((expected (pop cnfonts-command-matrix-gui-answers)))
    (cond ((null expected)
	   (cnfonts-gui-check
	    nil (format "%s asked a prompt the matrix does not expect: %S"
			kind prompt)))
	  ((string= (car expected) prompt) (cdr expected))
	  (t (cnfonts-gui-check
	     nil (format "%s prompt is %S, the matrix expected %S"
			 kind prompt (car expected)))))))

(defmacro cnfonts-command-matrix-gui-with-answers (answers &rest body)
  "Run BODY answering the prompts in ANSWERS, (PROMPT . ANSWER) in order."
  `(let ((cnfonts-command-matrix-gui-answers ,answers))
     (flet ((completing-read (prompt &rest _args)
	      (cnfonts-command-matrix-gui-answer 'completing-read prompt))
	     (yes-or-no-p (prompt)
	       (cnfonts-command-matrix-gui-answer 'yes-or-no-p prompt)))
       ,@body)))

;;; Running the sweep.

(defmacro cnfonts-command-matrix-gui-with-move-guard (&rest body)
  "Run BODY with `widget-move' counted, and stop it after 200 calls.
`cnfonts-ui-forward' walks until it reaches a tab stop, so a wrap that
never lands would hang the gate rather than fail it."
  `(let ((real (symbol-function 'widget-move))
	 (cnfonts-command-matrix-gui-moves 0))
     (unwind-protect
	 (progn
	   (fset 'widget-move
		 (lambda (step)
		   (setq cnfonts-command-matrix-gui-moves
			 (1+ cnfonts-command-matrix-gui-moves))
		   (if (> cnfonts-command-matrix-gui-moves 200)
		       (error "widget-move ran %s times without stopping"
			      cnfonts-command-matrix-gui-moves)
		     (funcall real step))))
	   ,@body)
       (fset 'widget-move real))))

(defun cnfonts-command-matrix-gui-run-row (row)
  "Run the matrix ROW and record its check."
  (let* ((command (nth 0 row))
	 (setup (nth 1 row))
	 (args (nth 2 row))
	 (class (nth 3 row))
	 (assertion (nth 4 row))
	 (description (nth 5 row))
	 (answers (nth 6 row)))
    (setq cnfonts-gui-checks (1+ cnfonts-gui-checks))
    (condition-case err
	(if (cnfonts-command-matrix-gui-with-answers
	       answers
	     ;; `sit-for' is answered so the align buttons' five-round
	     ;; minibuffer jitter loop costs nothing here; the session is
	     ;; where that loop is wanted for real.
	     (flet ((sit-for (&optional _seconds _no-redisplay) t))
	       (cnfonts-command-matrix-gui-with-move-guard
		(eval setup)
		(apply command (eval args))
		(cond ((eq class 'message)
		       (and (stringp (current-message))
			    (string-match assertion (current-message))))
		      ((eq class 'state) (eval assertion))
		      (t t)))))
	    (princ (format "PASS: %s\n" description))
	  (setq cnfonts-gui-failures (1+ cnfonts-gui-failures))
	  (princ (format "FAIL: %s\n" description)))
      (error
       (setq cnfonts-gui-failures (1+ cnfonts-gui-failures))
       (princ (format "FAIL: %s (%S)\n" description err))))))

(defun cnfonts-command-matrix-gui-derived-commands ()
  "Every interactive command the loaded cnfonts defines, sorted."
  (let ((found nil))
    (mapatoms (lambda (symbol)
		(let ((name (symbol-name symbol)))
		  (and (string-match "^cnfonts" name)
		       (not (string-match "--" name))
		       (fboundp symbol)
		       (not (eq 'autoload (car-safe (symbol-function symbol))))
		       (commandp symbol)
		       (push symbol found)))))
    (sort found (lambda (a b) (string< (symbol-name a) (symbol-name b))))))

(defun cnfonts-command-matrix-gui-check-matrix ()
  "Check the matrix against the commands the package actually defines."
  (let* ((derived (cnfonts-command-matrix-gui-derived-commands))
	 (swept (delete-dups (mapcar #'car
				     cnfonts-command-matrix-gui-rows)))
	 (unasserted (delq nil (mapcar (lambda (command)
					 (and (not (memq command swept))
					      command))
				       derived)))
	 (absent (delq nil (mapcar (lambda (command)
				     (and (not (memq command derived))
					  command))
				   swept))))
    (cnfonts-gui-check
     (and (= 30 (length derived)) (null unasserted) (null absent))
     (format "the matrix sweeps every command cnfonts defines (%d found)"
	     (length derived)))
    (when (or unasserted absent)
      (princ (format "     defined but not swept: %S\n" unasserted))
      (princ (format "     swept but not defined: %S\n" absent)))
    (cnfonts-gui-check
     (equal (sort (append cnfonts-command-matrix-gui-pages nil) #'string<)
	    (sort (mapcar #'car cnfonts-ui--pages) #'string<))
     "the matrix sweeps the nine pages cnfonts-ui declares")))

;;; The editor session.
;;;
;;; The machine-checkable half of the walkthrough's checklist, run after
;;; the sweep on the real editor: open, reach every page, press one align
;;; button all the way through the event path with nothing stubbed, restart,
;;; close.  Glyph appearance is what a machine cannot see and stays human.

(defun cnfonts-command-matrix-gui-session ()
  "Work the editor as a user does, and record each check."
  (cnfonts-command-matrix-gui-check-run
   '(progn (cnfonts-edit-profile)
	   (string= (buffer-name) " *cnfonts: start-page*"))
   "the editor opens on the start page")
  (cnfonts-command-matrix-gui-check-run
   '(let ((start (buffer-name)) (visited nil))
      (dotimes (_ 9)
	(cnfonts-ui-next-page)
	(push (buffer-name) visited))
      (and (= 9 (length (delete-dups visited)))
	   (string= start (car visited))))
   "next-page reaches all nine pages and comes round")
  (cnfonts-command-matrix-gui-check-run
   '(let* ((cell (progn (cnfonts-ui--switch-to-page 'align-page)
			(cnfonts-command-matrix-gui-size-button "+")))
	   (before (cnfonts-command-matrix-gui-cjkv-size)))
      ;; The press a user makes: the button's own position, no `sit-for'
      ;; stub, no answer stubs, and the real font layer below the
      ;; `cnfonts-set-font' the button's action calls.
      (widget-button-press (widget-get cell :from))
      (= (+ 0.5 before) (cnfonts-command-matrix-gui-cjkv-size)))
   "pressing the align page's [+] raises the CJKV size half a point")
  (cnfonts-command-matrix-gui-check-run
   '(let ((page-buffer (buffer-name)))
      (cnfonts-ui-restart)
      (and (string= page-buffer (buffer-name))
	   (not (eq major-mode 'fundamental-mode))))
   "cnfonts-ui-restart rebuilds the page and stays on it")
  (cnfonts-command-matrix-gui-check-run
   '(let ((from (buffer-name)))
      (cnfonts-ui-previous-page)
      (and (not (string= from (buffer-name)))
	   (consp (cnfonts-command-matrix-gui-page-widgets
		   (lambda (widget) (widget-get widget :flag))))))
   "the page reached by stepping back carries the rows a user can choose")
  (dolist (page cnfonts-command-matrix-gui-pages)
    (let ((buffer (get-buffer (format " *cnfonts: %S*" page))))
      (and buffer (kill-buffer buffer))))
  (let ((probe (get-buffer " *cnfonts: mode probe*")))
    (and probe (kill-buffer probe))))

(defun cnfonts-command-matrix-gui-check-run (form description)
  "Check FORM against a session step, unstubbed except for the answers.
`sit-for' is left real here, because the session is where cnfonts's
minibuffer jitter loop is supposed to run."
  (cnfonts-gui-check
   `(cnfonts-command-matrix-gui-with-answers nil ,form)
   description))

(defun cnfonts-command-matrix-gui-run ()
  "Sweep the matrix, then run the editor session."
  (let ((workdir (make-temp-file "cnfonts-matrix-" t)))
    (unwind-protect
	(progn
	  (cnfonts-gui-unpack workdir)
	  (require 'cnfonts-ui)
	  (cnfonts-command-matrix-gui-check-matrix)
	  ;; The state the sweep starts from: two profiles seeded with the
	  ;; fixture families, the mode off, and the platform's fonts
	  ;; faked for every command below.
	  (setq cnfonts-profiles '("gate" "other")
		cnfonts-directory (expand-file-name "cnfonts/" workdir))
	  (cnfonts-gui-set-font-from-fixtures workdir)
	  (cnfonts-gui-with-stubs
	   (cnfonts--save-profile "other")
	   (dolist (row cnfonts-command-matrix-gui-rows)
	     (cnfonts-command-matrix-gui-run-row row))
	   (cnfonts-command-matrix-gui-session)))
      (call-process "rm" nil nil nil "-rf" workdir)))
  cnfonts-gui-failures)

(defun cnfonts-command-matrix-gui-main ()
  "Print SKIP on a non-graphical frame, else run the gate and exit."
  (cnfonts-gui-finish 'cnfonts-command-matrix-gui-run))

;; An interactive XEmacs would otherwise open its debugger and wait, so a
;; broken gate must still report and exit.
(condition-case err
    (cnfonts-command-matrix-gui-main)
  (error
   (princ (format "FAIL: %S\n" err))
   (kill-emacs 1)))

;;; cnfonts-command-matrix-gui.el ends here
