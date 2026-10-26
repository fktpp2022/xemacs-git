;;; subr-compat-tests.el --- tests for the dumped GNU compatibility macros

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

;; Tests for the `when-let', `if-let', `defvar-local' and
;; `with-eval-after-load' definitions in the dumped lisp/subr.el.  There is
;; deliberately no `(require 'subr-x)' anywhere: these forms must work with
;; no require at load time and at compile time.  The harness compiles this
;; file as well as interpreting it, so the compiled pass proves compile-time
;; availability.  One fresh child XEmacs process proves the same for a
;; process that has loaded nothing but the dump.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(Assert (fboundp 'when-let) "when-let is available without a require")
(Assert (fboundp 'if-let) "if-let is available without a require")
(Assert (fboundp 'defvar-local) "defvar-local is available without a require")
(Assert (fboundp 'with-eval-after-load)
        "with-eval-after-load is available without a require")

;;; when-let

(Assert (eql (when-let ((x 1) (y 2)) (+ x y)) 3)
        "when-let binds every non-nil value")
(Assert (null (when-let ((x 1) (y nil) (z 3)) (+ x z)))
        "when-let stops at the first nil value")
(Assert (null (when-let ((x nil)) 'yes))
        "when-let returns nil for a nil binding")
(Assert (eq (when-let ((x 'bound)) x) 'bound)
        "when-let returns the last body form")

;;; if-let

(Assert (eql (if-let ((x 10)) (* x 2) 'no) 20)
        "if-let evaluates the then branch when all bindings are non-nil")
(Assert (eq (if-let ((x nil)) 'yes 'no) 'no)
        "if-let evaluates the else branch for a nil binding")
(Assert (eq (if-let ((x 1) (y nil)) 'yes 'no) 'no)
        "if-let stops at the first nil binding")
(Assert (eql (if-let ((x 1) (y 2)) (+ x y) 'no) 3)
        "if-let binds all values in order")

;;; defvar-local

(defvar-local subr-compat-tests-local 'global
  "Variable defined by the `defvar-local' test.")

(Assert (eq subr-compat-tests-local 'global)
        "defvar-local keeps the global default")

(with-temp-buffer
  (setq subr-compat-tests-local 'buffer)
  (Assert (local-variable-p 'subr-compat-tests-local (current-buffer))
          "defvar-local makes a set value buffer-local")
  (Assert (eq subr-compat-tests-local 'buffer)
          "defvar-local binding is visible in a temp buffer"))

(Assert (eq subr-compat-tests-local 'global)
        "defvar-local binding does not leak out of the temp buffer")
(Assert (eq (default-value 'subr-compat-tests-local) 'global)
        "defvar-local leaves the default value alone")

;;; with-eval-after-load

(let* ((dir (make-temp-file "subr-compat-tests-" t))
       (lib "subr-compat-tests-load")
       (file (expand-file-name (concat lib ".el") dir))
       (load-path (cons dir load-path))
       (ran nil))
  (unwind-protect
      (progn
        (with-temp-file file
          (insert "(provide 'subr-compat-tests-feature)\n"))
        (with-eval-after-load lib
          (setq ran 'ran))
        (Assert (null ran)
                "with-eval-after-load defers its body until the file loads")
        (load lib)
        (Assert (eq ran 'ran)
                "with-eval-after-load evaluates its body when the file loads")
        (Assert (featurep 'subr-compat-tests-feature)
                "with-eval-after-load ran after the file was fully loaded"))
    (when (file-exists-p file)
      (delete-file file))
    (when (file-directory-p dir)
      (delete-directory dir))))

;;; Fresh child process

(defun subr-compat-tests-child-output ()
  "Run a fresh XEmacs with only the dump loaded and exercise the macros.
Return the child's standard output without a trailing newline."
  (let ((buffer (get-buffer-create " *subr-compat-child-output*"))
        (program (expand-file-name invocation-name invocation-directory))
        (form
         (concat
          "(progn"
          " (princ (format \"when=%S if=%S \""
          "               (when-let ((x 1) (y 2)) (+ x y))"
          "               (if-let ((x 1) (y nil)) 'yes 'no)))"
          " (defvar-local subr-compat-child-local 'v)"
          " (setq subr-compat-child-local 'local)"
          " (princ (format \"local=%S default=%S \""
          "               (local-variable-p 'subr-compat-child-local"
          "                                 (current-buffer))"
          "               (default-value 'subr-compat-child-local)))"
          " (let* ((dir (make-temp-file \"subr-compat-child-\" t))"
          "        (file (expand-file-name \"loaded.el\" dir))"
          "        (load-path (cons dir load-path))"
          "        (ran nil))"
          "   (with-temp-file file"
          "     (insert \"(provide 'subr-compat-child-loaded)\\n\"))"
          "   (with-eval-after-load \"loaded\" (setq ran 'ran))"
          "   (load \"loaded\")"
          "   (princ (format \"after=%S\" ran))"
          "   (delete-file file)"
          "   (delete-directory dir)))")))
    (with-current-buffer buffer
      (erase-buffer))
    (let ((status
           (call-process
            program nil (list buffer nil) nil
            "-batch" "-no-packages"
            "-eval" form)))
      (unless (and (integerp status) (zerop status))
        (let ((output (with-current-buffer buffer (buffer-string))))
          (error "Child XEmacs failed with status %S: %s" status output))))
    (let ((output (with-current-buffer buffer (buffer-string))))
      (if (string-match "[\n\r]+\\'" output)
          (substring output 0 (match-beginning 0))
        output))))

(let ((output (subr-compat-tests-child-output)))
  (Assert (string-match "when=3 if=no local=t default=v" output)
          (format "dumped macros in a fresh child: %S" output))
  (Assert (string-match "after=ran" output)
          (format "with-eval-after-load in a fresh child: %S" output)))

;;; subr-compat-tests.el ends here
