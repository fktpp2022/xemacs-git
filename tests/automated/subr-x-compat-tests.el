;;; subr-x-compat-tests.el --- tests for the XEmacs `subr-x' feature

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;;; Commentary:

;; The `subr-x' compatibility shape promised by the use-package milestone:
;; the five documented upstream symbols become defined and the `subr-x'
;; feature is provided.  Each symbol is unbound first so the test proves the
;; guard-and-define path in lisp/subr-x.el, not merely that something else
;; happened to define them.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(defconst subr-x-compat-symbols
  '(hash-table-keys string-suffix-p string-trim if-let* when-let*)
  "The five upstream `subr-x' symbols shipped as core provision.")

;; Force the preconditions the milestone promises to repair, then load the
;; feature under test from the worktree.
(dolist (symbol subr-x-compat-symbols)
  (when (fboundp symbol)
    (fmakunbound symbol)))
(setq features (delq 'subr-x features))
(load "subr-x" nil t)

;;; Feature and symbol surface

(Assert (featurep 'subr-x) "loading subr-x provides the feature")
(dolist (symbol subr-x-compat-symbols)
  (Assert (fboundp symbol) (format "%s is defined by subr-x" symbol)))

;;; Behavioural spot checks

(Assert (equal (sort (hash-table-keys
                      (let ((table (make-hash-table)))
                        (puthash 'a 1 table)
                        (puthash 'b 2 table)
                        table))
                     (lambda (x y) (string< (symbol-name x) (symbol-name y))))
               '(a b))
        "hash-table-keys returns the table's keys")

(Assert (string-suffix-p "bar" "foobar")
        "string-suffix-p finds a suffix")
(Assert (not (string-suffix-p "baz" "foobar"))
        "string-suffix-p rejects a non-suffix")
(Assert (string-suffix-p "BAR" "foobar" t)
        "string-suffix-p ignores case when asked")

(Assert (equal (string-trim "  hello \t\n") "hello")
        "string-trim removes surrounding whitespace")
(Assert (equal (string-trim "xxhelloxx" "x+" "x+") "hello")
        "string-trim honours explicit trim regexps")

(Assert (equal (if-let* ((a 1) (b (+ a 1))) b 'no) 2)
        "if-let* binds sequentially and returns THEN")
(Assert (equal (if-let* ((a 1) (b nil)) b 'no) 'no)
        "if-let* falls through when a binding is nil")
(Assert (equal (when-let* ((a 1) (b (* a 3))) b) 3)
        "when-let* binds sequentially")

;;; Verdict
;;
;; The batch harness exits 0 even when Asserts fail, so the verdict is
;; computed from the harness's own failure counters, which are dynamically
;; bound while the suite body runs.
(Assert (and (boundp 'passes)
             (boundp 'assertion-failures)
             (boundp 'no-error-failures)
             (boundp 'wrong-error-failures)
             (boundp 'missing-message-failures)
             (boundp 'other-failures))
        "the harness failure counters are visible for the terminal line")
(message
 (if (and (zerop assertion-failures)
          (zerop no-error-failures)
          (zerop wrong-error-failures)
          (zerop missing-message-failures)
          (zerop other-failures))
     "subr-x compat tests: passed"
   "subr-x compat tests: FAILED"))

;;; subr-x-compat-tests.el ends here
