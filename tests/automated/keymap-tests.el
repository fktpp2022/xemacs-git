;; Copyright (C) 2012 Free Software Foundation, Inc. -*- coding: utf-8 -*-

;; Author: Aidan Kehoe <kehoea@parhasard.net>
;; Maintainers: Aidan Kehoe <kehoea@parhasard.net>
;; Created: 2012
;; Keywords: tests

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

;;; Synched up with: Not in FSF.

(let* ((map (make-keymap 'help-map-copy))
       (parent-map (make-keymap 'help-map-copy-parent))
       (help-map-copy t)
       (minor-mode-map-alist (acons 'help-map-copy map minor-mode-map-alist)))
  (set-keymap-parent map parent-map)
  ;; Take care in choosing commands for these tests!  Many of the commands
  ;; here are defined in packages, which may not be available to make check.
  (loop for (keys def) on '((shift tab) help-prev-symbol
			    tab help-next-symbol
			    c customize-variable
			    V find-variable-at-point
			    q help-mode-quit
			    f find-function-at-point
			    d describe-function-at-point
			    D describe-function-at-point
			    v describe-variable-at-point
			    i Info-elisp-ref
			    F find-function-at-point
			    Q help-mode-bury
			    button2 help-mouse-find-source-or-track
			    p help-prev-section
			    n help-next-section
			    return help-activate-function-or-scroll-up)
        by #'cddr
        do (define-key map (vector keys) def))
  (loop for (keys def) on '(u view-scroll-some-lines-down
			    % view-goto-percent
                            \2 digit-argument
			    p view-goto-percent
			    \?  view-search-backward
			    - negative-argument
			    k view-scroll-lines-down
			    backspace scroll-down
			    G view-last-windowful
			    f scroll-up
			    \5 digit-argument
			    s view-repeat-search
			    \0 digit-argument
			    n view-repeat-search
			    = what-line
                            \\ view-search-backward
			    delete scroll-down
			    \8 digit-argument
			    E view-file
			    d view-scroll-some-lines-up
			    \3 digit-argument
			    q view-quit
			    ! shell-command
			    (control j) view-scroll-lines-up
			    (control m) view-scroll-lines-up
			    y view-scroll-lines-down
                            linefeed view-scroll-lines-up
			    g view-goto-line
                            \6 digit-argument
			    t toggle-truncate-lines
			    C view-cleanup-backspaces
			    b scroll-down
			    \1 digit-argument
			    P view-buffer
			    return view-scroll-lines-up
			    | shell-command-on-region
			    j view-scroll-lines-up
			    \9 digit-argument
			    \' register-to-point
			    e view-scroll-lines-up
			    \4 digit-argument
			    r recenter
			    space scroll-up
			    / view-search-forward
			    N view-buffer
			    m point-to-register
			    h view-mode-describe
			    \7 digit-argument
                            describe-function-at-point view-mode-describe)
        by #'cddr
        do (define-key parent-map (vector keys) def))
  (Assert (eq (key-binding [F]) 'find-function-at-point)
          "checking normal key lookup works, F")
  (Assert (eq (key-binding [c]) 'customize-variable)
          "checking normal key lookup works, c")
  (Assert (eq (key-binding [\2]) 'digit-argument)
          "checking normal key parent lookup works, \\2")
  (Assert (eq (key-binding [|]) 'shell-command-on-region)
          "checking normal key parent lookup works, |")
  (define-key map [remap describe-function-at-point] #'find-file)
  (Assert (eq (key-binding [D]) 'find-file)
          "checking remapped key lookup works, d")
  (Assert (eq (key-binding [d]) 'find-file)
          "checking remapped key lookup works, f")
  (Assert (eq (key-binding [\2]) 'digit-argument)
          "checking normal key parent lookup works, \\2")
  (Assert (eq (key-binding [|]) 'shell-command-on-region)
          "checking normal key parent lookup works, |")
  (Assert (eq (key-binding [describe-function-at-point]) 'view-mode-describe)
          "checking remapped function doesn't affect key name mapping")
  (define-key parent-map [remap help-next-symbol] #'find-file)
  (Assert (eq (key-binding [tab]) 'find-file)
          "checking remapping in parent extends to child")
  (Assert (equal (commands-remapped-to 'find-file)
		 '(help-next-symbol describe-function-at-point))
	  "checking #'commands-remapped-to is sane")
  (Check-Error wrong-type-argument (commands-remapped-to pi))
  (Check-Error wrong-type-argument (commands-remapped-to 'find-file pi))
  (Check-Error wrong-type-argument (commands-remapped-to 'find-file nil pi))
  (Assert (eq (command-remapping 'describe-function-at-point) 'find-file)
	  "checking #'command-remapping is sane")
  (Check-Error wrong-type-argument (command-remapping pi))
  (Check-Error wrong-type-argument (command-remapping 'describe-function-at-point
						      pi))
  (Check-Error wrong-type-argument (command-remapping 'describe-function-at-point
						      nil pi)))

;; Not quite a keymap test, but this function is in keymap.c, so add a test
;; here:
 
(Check-Error circular-list (single-key-description '#1=(control shift . #1#)))

;; Test #'canonicalize-keysym, also in keymap.c.

(Assert (eq (canonicalize-keysym 'a) ?a))
(Assert (eq (canonicalize-keysym ?a) ?a))
(Assert (eq (canonicalize-keysym (char-int ?a)) ?a))
(Assert (eq (canonicalize-keysym '(a)) ?a))
(Assert (eq (canonicalize-keysym '(?a)) ?a))
(Assert (eq (canonicalize-keysym (list (char-int ?a))) ?a))
(Check-Error invalid-argument (canonicalize-keysym [?a]))

(Assert (equal (canonicalize-keysym '(control shift ?a)) '(control ?A)))
(Assert (equal (canonicalize-keysym '(control A)) '(control ?A)))
(Assert (equal (canonicalize-keysym '(control ?A)) '(control ?A)))
(Assert (equal (canonicalize-keysym (list 'control (char-int ?A)))
	       '(control ?A)))
(Check-Error invalid-argument (canonicalize-keysym [(control a)]))

(Check-Error invalid-argument (canonicalize-keysym '(control ueber b)))

(let ((control-meta-end '(control meta end))
      (control-meta-meta-end '(control meta meta end))
      (control-b '(control b))
      (control-?b '(control ?b))
      (control-shift-?b '(control shift ?b))
      (control-?B '(control ?B)))
  (Assert (eq control-meta-end (canonicalize-keysym control-meta-end)))
  (Assert (not (eq control-meta-meta-end
                   (canonicalize-keysym control-meta-meta-end))))
  (Assert (equal control-meta-end
                 (canonicalize-keysym control-meta-meta-end)))
  (Assert (not (eq control-b (canonicalize-keysym control-b))))
  (Assert (eq control-?b (canonicalize-keysym control-?b)))
  (Assert (eq control-?B (canonicalize-keysym control-?B)))
  (Assert (not (eq control-shift-?b (canonicalize-keysym control-shift-?b))))
  (Assert (equal control-?B (canonicalize-keysym control-shift-?b)))

  (if (featurep 'mule)
      (let ((control-rho (list 'control
                               (intern (string (decode-char 'ucs #x03c1)))))
            (control-?rho (list 'control (decode-char 'ucs #x03c1)))
            (control-shift-?rho (list 'control 'shift
                                      (decode-char 'ucs #x03c1)))
            (control-?RHO (list 'control (decode-char 'ucs #x03a1))))
        (Assert (not (eq control-rho (canonicalize-keysym control-rho))))
        (Assert (eq control-?rho (canonicalize-keysym control-?rho)))
        (Assert (eq control-?RHO (canonicalize-keysym control-?RHO)))
        (Assert (not (eq control-shift-?rho
                         (canonicalize-keysym control-shift-?rho))))
        (Assert (equal control-?RHO
                       (canonicalize-keysym control-shift-?rho))))))

;;; end of keymap-tests.el
