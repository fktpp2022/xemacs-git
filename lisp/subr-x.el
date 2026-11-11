;;; subr-x.el --- XEmacs compatibility subset of GNU Emacs's subr-x -*- lexical-binding: t -*-

;; Copyright (C) 2026  Free Software Foundation, Inc.
;; See the file `xemacs' for copyright and disclaimer details.

;; This file is part of XEmacs.
;;
;; XEmacs folds two of GNU's `subr-x' macros (`when-let' and `if-let') into
;; the dumped `subr.el', but provides no `subr-x' feature and lacks the rest.
;; This file provides the feature and the symbols that packages reasonably
;; require and use (notably `hash-table-keys').  These are called only when
;; a program explicitly requires `subr-x'.

;;; Code:

(unless (fboundp 'hash-table-keys)
  (defun hash-table-keys (hash-table)
    "Return a list of keys in HASH-TABLE."
    (let (keys)
      (maphash (lambda (key _value) (push key keys)) hash-table)
      (nreverse keys))))

(unless (fboundp 'string-suffix-p)
  (defun string-suffix-p (suffix string &optional ignore-case)
    "Return non-nil if SUFFIX is a suffix of STRING.
If IGNORE-CASE is non-nil, compare case-insensitively."
    (let ((start-pos (- (length string) (length suffix))))
      (and (>= start-pos 0)
           (eq t (compare-strings suffix nil nil
                                  string start-pos nil ignore-case))))))

(unless (fboundp 'string-trim)
  (defun string-trim (string &optional trim-left trim-right)
    "Remove leading and trailing whitespace from STRING.
TRIM-LEFT and TRIM-RIGHT, if given, are regexps matching what to strip
instead of the default whitespace class."
    (let ((trim-left (or trim-left "[ \t\n\r]+"))
          (trim-right (or trim-right "[ \t\n\r]+")))
      (when (string-match (concat "\\`" trim-left) string)
        (setq string (substring string (match-end 0))))
      (when (string-match (concat trim-right "\\'") string)
        (setq string (substring string 0 (match-beginning 0))))
      string)))

;; XEmacs's dumped `subr.el' `if-let'/`when-let' already bind sequentially
;; over a binding list, which is GNU's starred behaviour, so the starred
;; names are thin wrappers rather than a second implementation.
(unless (fboundp 'if-let*)
  (defmacro if-let* (spec then &rest else)
    "Like `if-let' for a SPEC of sequential bindings."
    (declare (indent 2) (debug t))
    `(if-let ,spec ,then ,@else)))

(unless (fboundp 'when-let*)
  (defmacro when-let* (spec &rest body)
    "Like `when-let' for a SPEC of sequential bindings."
    (declare (indent 1) (debug t))
    `(when-let ,spec ,@body)))

(provide 'subr-x)

;;; subr-x.el ends here