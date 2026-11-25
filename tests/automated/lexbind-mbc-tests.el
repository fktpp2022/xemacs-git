;;; lexbind-mbc-tests.el --- P2-4 make-byte-code env arg + .elc reader form

;; Coverage for ticket #160 (P2-4): `make-byte-code' accepts an optional
;; seventh ENV argument, and the #[...] reader accepts the extended
;; seven-element form.  The no-backcompat policy (#137) applies: old
;; six-element (and shorter) #[...] forms still load with env = nil.
;; Emission-side changes (bytecomp) and the ;ELC cookie bump are out of
;; scope (#157, #163).

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(defun lexbind-p24-cf-args (&optional env)
  "Arguments for a minimal compiled function, optionally with ENV."
  (list '(x) "\x87" [1 2 3] 1 nil nil env))

;; ---- make-byte-code arity: 4..6 args unchanged, 7th arg is env ----------

(let ((cf (make-byte-code '(x) "\x87" [1 2 3] 1)))
  (Assert (compiled-function-p cf))
  (Assert (null (compiled-function-environment cf))))

(let ((cf (apply #'make-byte-code (lexbind-p24-cf-args nil))))
  (Assert (compiled-function-p cf))
  (Assert (null (compiled-function-environment cf))))

(let ((cf (make-byte-code '(x) "\x87" [1 2 3] 1 nil nil '(lexical-var x))))
  (Assert (compiled-function-p cf))
  (Assert (equal (compiled-function-environment cf) '(lexical-var x))))

;; wrong number of arguments still signals
(Check-Error wrong-number-of-arguments
	     (make-byte-code '(x) "\x87" [1 2 3]))
(Check-Error wrong-number-of-arguments
	     (apply #'make-byte-code
		    (append (lexbind-p24-cf-args nil) '(extra))))

;; ---- print/read round trip ----------------------------------------------

(let* ((env (list t '(captured . (42))))
       (cf (make-byte-code '(x) "\x87" [1 2 3] 1 nil nil env))
       (printed (let ((print-readably t))
		  (with-output-to-string (print cf))))
       (back (read printed)))
  (Assert (compiled-function-p back))
  (Assert (equal (compiled-function-environment back) env)))

;; nil env prints as the short form and reads back with env nil
(let* ((cf (make-byte-code '(x) "\x87" [1 2 3] 1))
       (printed (let ((print-readably t))
		  (with-output-to-string (print cf))))
       (back (read printed)))
  (Assert (compiled-function-p back))
  (Assert (null (compiled-function-environment back))))

;; interactive function with env keeps its interactive spec through the
;; round trip (nil interactive in a seven-element form means "absent")
(let* ((cf (make-byte-code '(x) "\x87" [1 2 3] 1 nil
			   '(interactive "p") '(lexical-var x)))
       (back (read (let ((print-readably t))
			 (with-output-to-string (print cf))))))
  (Assert (equal (compiled-function-environment back) '(lexical-var x)))
  (Assert (commandp back)))

;; ---- reading #[...] directly ---------------------------------------------

(let ((cf (read "#[(x) \"\\x87\" [1 2 3] 1 nil nil (lexical-var x)]")))
  (Assert (compiled-function-p cf))
  (Assert (equal (compiled-function-environment cf) '(lexical-var x))))

;; six-element form: env defaults to nil (no-backcompat policy #137)
(let ((cf (read "#[(x) \"\\x87\" [1 2 3] 1 nil]")))
  (Assert (compiled-function-p cf))
  (Assert (null (compiled-function-environment cf))))

;; ---- .elc load round trip --------------------------------------------------

(let* ((env (list t '(lexical-var x)))
       (cf (make-byte-code '(x) "\x87" [1 2 3] 1 nil nil env))
       (file (format "%slexbind-p24-%d"
		      (file-name-as-directory
		       (if (boundp 'temporary-file-directory)
			   temporary-file-directory "/tmp"))
		      (emacs-pid))))
  (with-temp-file (concat file ".elc")
    (insert ";;;lexical-binding:t\n")
    (insert "(setq lexbind-p24-roundtrip "
	    (let ((print-readably t))
	      (with-output-to-string (print cf))) ")\n"))
  (unwind-protect
      (progn
	(load (concat file ".elc") nil t t)
	(Assert (compiled-function-p lexbind-p24-roundtrip))
	(Assert (equal (compiled-function-environment lexbind-p24-roundtrip)
		       env)))
    (delete-file (concat file ".elc"))))

;;; end of lexbind-mbc-tests.el
