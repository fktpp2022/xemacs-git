;;; elc-reader-reentrancy-tests.el --- #164 regression: nested load vs the reader

;; A `load' can start from inside a read: decoding a wide character whose
;; Unicode mapping is not loaded yet calls compiled_unicode_table_load()
;; (src/unicode.c), which loads lisp/mule/compiled-unicode-tables.elc.
;; Before the fix that nested load rewound the single global
;; Vread_buffer_stream under the atom the interrupted read was
;; accumulating, so Fintern() built a name out of the nested file's
;; leftovers ("compiled-unicode-tables\0p", a NUL inside a symbol name).
;; src/lread.c now gives every load its own buffer.

;; CAVEAT: this only regresses in a process where the compiled Unicode
;; tables are not in the dumped image yet.  If they are, the lazy load
;; never fires and the check passes vacuously -- it is a guard, not a
;; proof.  The reliable red/green signal for #164 is loading a built
;; lisp/bytecomp.elc (which carries an ESC % G designated 3-byte
;; character in a docstring) and checking constant 2 of
;; byte-compile-lexbind-frame-cell-p names byte-compile-lexbind-special-p.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(defconst elc-reader-reentrancy-fixture
  ;; ESC % G  <U+2192 = e2 86 92>  ESC % @, i.e. a designated UTF-8 run
  ;; inside an otherwise ASCII file -- the shape that triggers the lazy
  ;; Unicode table load while a read is in progress.
  (concat ";ELC" (string 21) "\0\0\0\n"
	  "(defvar elc-reader-reentrancy-before \"\\033%G"
	  (string ?\xe2 ?\x86 ?\x92)
	  "\\033%@\")\n"
	  "(defvar elc-reader-reentrancy-after 'elc-reader-reentrancy-value)\n"))

(defun elc-reader-reentrancy-nul-symbol ()
  "Return the first interned symbol whose name contains a NUL, or nil."
  (let (found)
    (mapatoms (lambda (s)
		(unless found
		  (when (string-match (string 0) (symbol-name s))
		    (setq found s)))))
    found))

(let ((file (make-temp-name
	     (expand-file-name "elc-reader-reentrancy-"
			       (temp-directory)))))
  (unwind-protect
      (progn
	(let ((coding-system-for-write 'binary))
	  (write-region elc-reader-reentrancy-fixture nil file nil 'quiet))
	(load file nil nil 'quiet)
	(Assert (null (elc-reader-reentrancy-nul-symbol))
		(format "no interned symbol may contain a NUL byte; got %S"
			(condition-case nil
			    (and (elc-reader-reentrancy-nul-symbol)
				 (symbol-name (elc-reader-reentrancy-nul-symbol)))
			  (error nil))))
	(Assert (boundp 'elc-reader-reentrancy-after)))
    (delete-file file)))

;;; end of elc-reader-reentrancy-tests.el
