;;; cl-lib-tests.el --- tests for the vendored GNU cl-lib layer

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

;; Per-definition tests for lisp/cl-lib.el, the XEmacs-adapted GNU ELPA
;; cl-lib 0.7.1.  Each assertion names one definition from the milestone-2
;; support contract and compares against a known-good literal.  The
;; `cl-position' section is the dedicated regression test for the one
;; behavioural patch; its last case is the exact shape `checkbox' calls.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; The test file is byte-compiled as well as interpreted by the harness, so
;; the macros must be available to both passes.
(eval-when-compile (require 'cl-lib))
(require 'cl-lib)

(Assert (featurep 'cl-lib) "cl-lib is provided")

;;; cl-defun

(cl-defun cl-lib-tests-add (a &optional (b 10))
  "Add A and optional B, for the `cl-defun' test."
  (+ a b))

(Assert (eql (cl-lib-tests-add 5) 15) "cl-defun with default argument")
(Assert (eql (cl-lib-tests-add 5 3) 8) "cl-defun with explicit argument")

;;; cl-defmacro

(cl-defmacro cl-lib-tests-double (x)
  "Return a form doubling X, for the `cl-defmacro' test."
  `(* 2 ,x))

(Assert (eql (cl-lib-tests-double 21) 42) "cl-defmacro expands")

;;; cl-destructuring-bind

(Assert (equal (cl-destructuring-bind (a (b c)) '(1 (2 3))
                 (list a b c))
               '(1 2 3))
        "cl-destructuring-bind nested list")

;;; cl-loop

(Assert (equal (cl-loop for i from 1 to 3 collect (* i i))
               '(1 4 9))
        "cl-loop collect")
(Assert (eql (cl-loop for i from 1 to 5 when (= i 3) return i)
             3)
        "cl-loop return (cl-block/cl-return-from)")

;;; cl-letf

(Assert (eql (let ((x 1))
               (cl-letf (((symbol-value 'x) 2))
                 x))
             2)
        "cl-letf rebinds a symbol-value place")

;;; cl-case

(Assert (eq (cl-case 'b
              ((a b) 'ab)
              (t 'other))
            'ab)
        "cl-case matches a key list")
(Assert (eq (cl-case 3
              (1 'one)
              (2 'two)
              (t 'other))
            'other)
        "cl-case falls through to t")

;;; cl-incf

(Assert (eql (let ((n 1))
               (cl-incf n)
               n)
             2)
        "cl-incf default delta")
(Assert (eql (let ((n 1))
               (cl-incf n 4)
               n)
             5)
        "cl-incf explicit delta")

;;; cl-dotimes

(Assert (equal (let (result)
                 (cl-dotimes (i 3)
                   (push i result))
                 (nreverse result))
               '(0 1 2))
        "cl-dotimes runs the body three times")

;;; cl-macrolet

(Assert (eql (cl-macrolet ((square (x) `(* ,x ,x)))
               (square 5))
             25)
        "cl-macrolet expands a local macro")

;;; cl-assert

(Assert (eq (progn (cl-assert (= 2 2)) 'ok) 'ok)
        "cl-assert accepts a true form")
(Check-Error (error) (cl-assert nil))

;;; cl-position (the behavioural patch; `checkbox' is the witness)

(Assert (eql (cl-position "b" '("a" "b" "c") :test #'string=) 1)
        "cl-position finds a string with :test")
(Assert (null (cl-position "z" '("a" "b") :test #'string=))
        "cl-position returns nil when absent")
(Assert (eql (cl-position 2 '(1 2 3)) 1)
        "cl-position uses eql by default on a list")
(Assert (eql (cl-position 2 [1 2 3]) 1)
        "cl-position accepts a vector")
(Assert (eql (cl-position 2 '(1 2 3 2) :from-end t) 3)
        "cl-position honours :from-end")
(Assert (eql (cl-position 2 '(1 2 3 2) :start 2) 3)
        "cl-position honours :start")
(Assert (eql (cl-position 3 '((1 a) (2 b) (3 c)) :key #'car) 2)
        "cl-position honours :key")
(Assert (eql (cl-position 2 '(1 2 3) :test #'= :start 1 :end 2) 1)
        "cl-position honours a bounded range")
(Assert (equal (list (cl-position nil '("[ ]" "[x]") :test #'string=)
                     (cl-position "[x]" '("[ ]" "[x]") :test #'string=))
               '(nil 1))
        "cl-position on checkbox's state list")

;;; cl-rest, cl-member, cl-find-if

(Assert (equal (cl-rest '(1 2 3)) '(2 3)) "cl-rest")
(Assert (equal (cl-member 2 '(1 2 3)) '(2 3)) "cl-member")
(Assert (eql (cl-find-if #'evenp '(1 2 3)) 2) "cl-find-if")

;;; Provisioning does not override an existing definition

(defun cl-lib-tests-no-override-child ()
  "Require cl-lib in a fresh XEmacs that already defines `cl-position'.
Return the child's standard output without a trailing newline."
  (let ((buffer (get-buffer-create " *cl-lib-no-override-output*"))
        (program (expand-file-name invocation-name invocation-directory)))
    (with-current-buffer buffer
      (erase-buffer))
    (let ((status
           (call-process
            program nil (list buffer nil) nil
            "-batch" "-no-packages"
            "-eval"
            (concat
             "(progn"
             " (defun cl-position (&rest _args) 'preserved)"
             " (require 'cl-lib)"
             " (princ (format \"position=%S feature=%S\""
             "                (cl-position 'x '(x))"
             "                (featurep 'cl-lib))))"))))
      (unless (and (integerp status) (zerop status))
        (let ((output (with-current-buffer buffer (buffer-string))))
          (error "Child XEmacs failed with status %S: %s" status output))))
    (let ((output (with-current-buffer buffer (buffer-string))))
      (if (string-match "[\n\r]+\\'" output)
          (substring output 0 (match-beginning 0))
        output))))

(Assert (string-match "position=preserved feature=t"
                      (cl-lib-tests-no-override-child))
        "cl-lib does not override a pre-existing cl-position")

;;; An unmodified consumer byte-compiles and runs in a fresh XEmacs

(defconst cl-lib-tests-consumer-source
  "(require 'cl-lib)

\(cl-defun cl-lib-consumer-add (a &optional (b 10))
  (+ a b))

\(cl-defmacro cl-lib-consumer-double (x)
  `(* 2 ,x))

\(defun cl-lib-consumer-probe ()
  (let ((n 1) result)
    (cl-incf n)
    (cl-dotimes (i 2)
      (push i result))
    (cl-macrolet ((square (x) `(* ,x ,x)))
      (push (square 3) result))
    (cl-letf (((symbol-value 'n) 5))
      (push n result))
    (list n
          (cl-destructuring-bind (a b) '(1 2) (+ a b))
          (cl-case n (5 'five) (t 'other))
          (cl-loop for i from 1 to 3 collect (* i i))
          (cl-position \"b\" '(\"a\" \"b\") :test #'string=)
          (cl-rest '(1 2 3))
          (cl-member 2 '(1 2 3))
          (cl-find-if #'evenp '(1 2 3))
          (nreverse result)
          (progn (cl-assert t) 'assert-ok))))
"
  "A cl-lib consumer used by the fresh-child compile test.")

(defun cl-lib-tests-compiled-consumer-child ()
  "Compile and run the consumer in a fresh XEmacs.
Return the child's standard output."
  (let* ((dir (make-temp-file "cl-lib-consumer-" t))
         (file (expand-file-name "consumer.el" dir))
         (buffer (get-buffer-create " *cl-lib-consumer-output*"))
         (program (expand-file-name invocation-name invocation-directory))
         (form
          (concat
           "(progn"
           " (push " (prin1-to-string dir) " load-path)"
           " (condition-case e"
           "     (progn"
           "       (byte-compile-file " (prin1-to-string file) ")"
           "       (load \"consumer.elc\")"
           "       (princ (format \"add=%S double=%S probe=%S\""
           "                      (cl-lib-consumer-add 5)"
           "                      (cl-lib-consumer-double 21)"
           "                      (cl-lib-consumer-probe))))"
           "   (error (princ (format \"ERROR %S\" e)) (kill-emacs 1))))")))
    (unwind-protect
        (progn
          (with-temp-file file
            (insert cl-lib-tests-consumer-source))
          (with-current-buffer buffer
            (erase-buffer))
          (let ((status
                 (call-process
                  program nil (list buffer nil) nil
                  "-batch" "-no-packages"
                  "-eval" form)))
            (unless (and (integerp status) (zerop status))
              (let ((output (with-current-buffer buffer (buffer-string))))
                (error "Child XEmacs failed with status %S: %s"
                       status output))))
          (with-current-buffer buffer (buffer-string)))
      (mapc (lambda (generated)
              (when (file-regular-p generated)
                (delete-file generated)))
            (directory-files dir t "\\`[^.]"))
      (when (file-directory-p dir)
        (delete-directory dir)))))

(let ((output (cl-lib-tests-compiled-consumer-child)))
  (Assert (string-match
           (regexp-quote
            "add=15 double=42 probe=(2 3 other (1 4 9) 1 (2 3) (2 3) 2 (0 1 9 5) assert-ok)")
           output)
          (format "compiled cl-lib consumer: %S" output)))

;;; cl-lib-tests.el ends here
