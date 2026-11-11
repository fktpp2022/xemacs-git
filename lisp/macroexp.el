;;; macroexp.el --- XEmacs compatibility subset of GNU Emacs's macroexp -*- lexical-binding: t -*-

;; Copyright (C) 2026  Free Software Foundation, Inc.
;; See the file `xemacs' for copyright and disclaimer details.

;; This file is part of XEmacs.
;;
;; XEmacs has no `macroexp' feature, but GNU packages (notably `use-package')
;; call `macroexp-progn' as a function.  Only what those callers use is
;; implemented.  `macroexp-progn' is deliberately a `defun', not a `defmacro',
;; because callers immediately backquote and evaluate its return value; the
;; function returns the `progn' form they expect.

;;; Code:

(defun macroexp-progn (exps)
  "Return a form that evaluates EXPS sequentially and returns the last value."
  (if (cdr exps)
      (cons 'progn exps)
    (car exps)))

(provide 'macroexp)

;;; macroexp.el ends here