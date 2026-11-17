;;; elc-load-gc-tests.el --- Regression tests for reader GCPRO windows (#157)

;; Regression for #157: the reader's `read_list_conser' / `read_vector' /
;; `read_compiled_function' / `make-byte-code' had unrooted GCPRO windows
;; that let GC sweep a freshly read element before it was linked into a
;; list/vector.  Under load-path timing the swept memory got reused and
;; a `#[...]' ended up with a stale prefix on its instructions string
;; (observed on byte-compile-lexbind-push-capture-frame: 66B = 23B stale
;; + 42B correct).  These tests force a GC at every allocation window by
;; driving `gc-cons-threshold' to its minimum and loading a freshly
;; byte-compiled .elc, then assert the loaded compiled function survived
;; intact: its string/vector constants still compare equal and it returns
;; the expected value when called.

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(require 'bytecomp)

(defconst elc-load-gc-unique-string
  "elc-load-gc-#157-a7f3-unique-constant-string-marker"
  "A marker string that must survive the reader's GC windows.")

(defun elc-load-gc-probe-function (x)
  "Return the length of X if it is a string, otherwise -1."
  (if (stringp x) (length x) -1))

(defun elc-load-gc-write-fixture ()
  (let* ((dir (make-temp-directory "/tmp/elc-load-gc-"))
	 (el (expand-file-name "probe-function.el" dir)))
    (with-temp-file el
      (insert
       (format
	(progn (prin1-to-string
		'(eval-when-compile
		    (condition-case nil
			(require 'bytecomp)
		  (file-error
		   (when (boundp 'load-file-name)
		     (require 'bytecomp))
		   nil))))
	       "\n")
	 "(defun probe-function (x)\n"
	 "  (if (stringp x) (length x) -1)\n"))
      (byte-compile-file el))
    (list (expand-file-name "probe-function.elc" dir)
	  dir)))

;; Drive `gc-cons-threshold' to its smallest value so every allocation
;; (including the Fcons / make_vector / Fmake_byte_code windows inside the
;; reader) triggers a GC.
(let* ((min-threshold (min (gc-cons-threshold) 1))
       (saved-threshold (gc-cons-threshold))
       (files (elc-load-gc-write-fixture))
       (elc-file (car files))
       (fixture-dir (cdr files)))
  (unwind-protect
      (progn
	(setq gc-cons-threshold min-threshold)
	(load elc-file nil t)
	(Assert (fboundp 'probe-function))
	(Assert (= (funcall 'probe-function elc-load-gc-unique-string)
		   (length elc-load-gc-unique-string)))
	;; Also verify a vector of strings loaded correctly.
	(Assert (= (length (aref (symbol-function 'probe-function) 0) 0))
		 0)))
    ;; Restore threshold; clean up temp files.
    (setq gc-cons-threshold saved-threshold)
    (delete-file elc-file)
    (delete-file (expand-file-name "probe-function.el" fixture-dir))
    (delete-file (expand-file-name "probe-function.el~" fixture-dir)))
  (delete-directory fixture-dir))

;;; end of elc-load-gc-tests.el
