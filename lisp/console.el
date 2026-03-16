;;; console.el --- miscellaneous console functions not written in C

;; Copyright (C) 1994-5, 1997 Free Software Foundation, Inc.
;; Copyright (C) 1995, 1996 Ben Wing

;; Maintainer: XEmacs Development Team
;; Keywords: internal, dumped

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

;;; Commentary:

;; This file is dumped with XEmacs.

;;; Code:

(defun consolep (object)
  "Return non-nil if OBJECT is a console."
  (eq (type-of object) 'console))

(defun quit-char (&optional console)
  "Return the character that causes a QUIT to happen.
This is normally C-g.  Optional arg CONSOLE specifies the console
that the information is returned for; nil means the current console."
  (nth 3 (current-input-mode console)))

(defun resume-pid-console (pid)
  "Resume the consoles with a controlling process of PID."
  (mapc (lambda (c) 
	  (if (and (eq (console-type c) 'tty)
		   (eql pid
			(declare-fboundp (console-tty-controlling-process c))))
	      (resume-console c)))
	(console-list))
  nil)

(macrolet
    ((define-console-type-predicates (&rest arguments)
       (cons
        'progn
        (mapcar
         #'(lambda (symbol)
             `(when (valid-console-type-p ',symbol)
                (defalias (intern ,(format "console-%s-p" symbol))
                    (lambda (object)
                      ,(format
                        "Return non-nil if OBJECT is a%.*s %s console."
                        (if (member* (aref (symbol-name symbol) 0)
                                     '(?x ?m))
                            1
                          0)
                        "n"
                        (upcase (symbol-name symbol)))
                      (and (consolep object) (eq ',symbol
                                                 (console-type object)))))))
         arguments))))
  ;; CONCHECK_CONSOLE_TYPE() in console-impl.h requires the existence of the
  ;; associated predicates at runtime (console-x-p, console-tty-p,
  ;; console-mswindows-p ...) if the specified console exists.
  ;;
  ;; Now, CONCHECK_CONSOLE_TYPE() is not actually used anywhere, so the
  ;; existence of the relevant symbols is only for the sake of the
  ;; dead_wrong_type_argument() call in CHECK_CONSOLE_TYPE(), and the
  ;; associated predciate does not need to exist. It is confusing for it not
  ;; to exist, create them at loadup type.
  (define-console-type-predicates x tty mswindows msprinter gtk stream))

;;; console.el ends here
