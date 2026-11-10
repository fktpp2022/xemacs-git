;; Copyright (C) 2026 Free Software Foundation, Inc.

;; Maintainer: XEmacs Development Team
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

;;; Commentary:

;;; Test that byte-compile-file refuses lexical-binding: t files
;;; (lexbind P1-6; no closure conversion yet until P2), and that
;;; byte-compiling a non-lexbound file is unchanged.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(require 'bytecomp)

;; XEmacs names it `temp-directory'; honor GNU's name too when present.
(defvar byte-compile-refusal-test-dir
  (file-name-as-directory
   (make-temp-name
    (expand-file-name "byte-compile-refusal-tests-"
		      (if (boundp 'temporary-file-directory)
			  temporary-file-directory
			(temp-directory))))))

(defun byte-compile-refusal-make-file (name lexical-p)
  "Create test file NAME in `byte-compile-refusal-test-dir'.
If LEXICAL-P is non-nil, give it a lexical-binding: t header."
  (unless (file-directory-p byte-compile-refusal-test-dir)
    (make-directory byte-compile-refusal-test-dir 'parents))
  (let ((file (expand-file-name name byte-compile-refusal-test-dir)))
    (with-temp-buffer
      (when lexical-p
	(insert ";;; " name " --- lexical-binding: t -*-\n"))
      (insert ";;; " name " --- test file\n"
	      "(defun byte-compile-refusal-test-fn ()\n"
	      "  (let ((x 1)) (+ x 1)))\n")
      (write-region (point-min) (point-max) file nil 'quiet))
    file))

;; A lexical-binding: t file must be refused, naming the refusal reason
;; and pointing at the milestone spec.
(Check-Error-Message
 'error
 "no closure conversion yet; see docs/planning/lexbinding-milestone.md"
 (byte-compile-file
  (byte-compile-refusal-make-file "lexbound-test-file.el" t)))

;; A non-lexbound file compiles unchanged (the .elc is produced without error).
(Assert (byte-compile-file
	 (byte-compile-refusal-make-file "dynamic-test-file.el" nil)))
(Assert (file-exists-p
	 (byte-compile-dest-file
	  (expand-file-name "dynamic-test-file.el"
			    byte-compile-refusal-test-dir))))

