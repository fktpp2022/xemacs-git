;;; elc-load-gc-tests.el --- Regression tests for reader GCPRO windows (#157)

;; Regression for #157: the reader's `read_list_conser' / `read_vector' /
;; `read_compiled_function' / `make-byte-code' had unrooted GCPRO windows
;; that let GC sweep a freshly read element before it was linked into a
;; list/vector.  Under load-path timing the swept memory got reused and
;; a `#[...]' ended up with a stale prefix on its instructions string
;; (observed on byte-compile-lexbind-push-capture-frame: 66B = 23B stale
;; + 42B correct).  These tests force a GC at every allocation window by
;; driving `gc-cons-threshold' to 1 and loading a freshly byte-compiled
;; .elc, then assert the loaded compiled function survived intact.

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
  "A marker string passed to the loaded function; if its constants or
instructions were corrupted by the reader's GC windows, the call
returns a wrong value or signals.")

(defun elc-load-gc-write-fixture ()
  "Byte-compile a one-function fixture; return (elc-file el-file)."
  (let* ((base (make-temp-name "/tmp/elc-load-gc-"))
	 (el (concat base ".el")))
    (with-temp-file el
      (insert "(defun probe-function (x)\n"
	      "  (if (stringp x) (length x) -1))\n"))
    (byte-compile-file el)
    (list (concat base ".elc") el)))

(let* ((min-threshold 100)
       (saved-threshold gc-cons-threshold)
       (files (elc-load-gc-write-fixture))
       (elc-file (car files))
       (fixture-el (cadr files)))
  (unwind-protect
      (progn
	;; Drive every reader allocation (Fcons, make_vector,
	;; Fmake_byte_code windows) into a GC.
	(setq gc-cons-threshold min-threshold)
	(load elc-file nil t)
	(Assert (fboundp 'probe-function))
	;; The call exercises instructions + constants of the loaded
	;; #[...]: a stale-memory prefix on the instructions makes it
	;; misbehave; a freed constant makes it return the wrong value.
	(Assert (= (funcall 'probe-function elc-load-gc-unique-string)
		   (length elc-load-gc-unique-string)))
	(Assert (stringp (compiled-function-instructions
			  (symbol-function 'probe-function))))))
    ;; Restore threshold; clean up.
    (setq gc-cons-threshold saved-threshold)
    (delete-file elc-file)
    (delete-file fixture-el)
    (when (file-exists-p (concat fixture-el "~"))
      (delete-file (concat fixture-el "~"))))

;;; end of elc-load-gc-tests.el
