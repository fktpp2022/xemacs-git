;;; lexbind-loader-tests.el --- P1-5 loader recognition + load-lexical-files tests

;; Coverage for ticket #151 (lexical-binding P1-5): the loader byte-scans
;; the first bytes of a SOURCE .el for the `lexical-binding: t' header and
;; specbinds internal-interpreter-environment to (t) for the file's
;; extent.  Since P2 (#163) the header alone activates -- the
;; `load-lexical-files' whitelist is retired and inert.  .elc loads never
;; trigger the scan.

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(defvar lexbind-loader-test-result 'unset)

(defun lexbind-loader-test-body ()
  ;; Recorded by each fixture file at load time: t iff the lexical
  ;; environment was active while the file's forms were evaluated.
  (setq lexbind-loader-test-result
	(and (consp internal-interpreter-environment)
	     (eq (car internal-interpreter-environment) t))))

(defconst lexbind-loader-test-fixture-header
  ";;; -*- lexical-binding: t -*-\n")

(defun lexbind-loader-test-write-fixture (name content)
  (let ((file (concat (file-name-as-directory (make-temp-name
					       "/tmp/lexbind-loader-test-"))
		      name)))
    (make-directory (file-name-directory file) t)
    (let ((coding-system-for-write 'binary))
      (write-region content nil file nil 'quiet))
    file))

;; 1. The header alone activates: a file that was never whitelisted (the
;;    whitelist no longer exists) still loads with the lexical
;;    environment active.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(setq lexbind-loader-test-result 'unset)
	(load file nil t)
	(Assert (eq lexbind-loader-test-result t))
	;; The env excursion does not leak past the load.
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 2. The cookie is visible to the evaluator -- the env is active for the
;;    file's extent, and does not leak past it.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(setq lexbind-loader-test-result 'unset)
	(load file nil t)
	(Assert (eq lexbind-loader-test-result t))
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 3. `load-lexical-files' is inert: binding it to a non-matching list
;;    changes nothing (_the retired whitelist could veto activation_).
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-other.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(setq lexbind-loader-test-result 'unset)
	(let ((load-lexical-files (list "lexbind-loader-cookie.el")))
	  (load file nil t))
	(Assert (eq lexbind-loader-test-result t)))
    (delete-file file)))

;; 4. .elc loads never trigger the scan: a byte-compiled file carrying
;;    the cookie stays fully dynamic.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.elc"
	     (concat ";ELC"
		     (string 20)
		     "\0\0\0\n"
		     lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(setq lexbind-loader-test-result 'unset)
	(load file nil t)
	(Assert (null lexbind-loader-test-result))
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 5. The whitelist is retired (P2, #163): the option survives as an
;;    inert alias for diagnostics, and the cookie alone activates
;;    lexical binding.  No `lexbound-lisp-files' data list any more.
(Assert (boundp 'load-lexical-files))
(Assert (not (boundp 'lexbound-lisp-files)))

;;; end of lexbind-loader-tests.el
