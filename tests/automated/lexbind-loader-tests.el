;;; lexbind-loader-tests.el --- P1-5 loader recognition + load-lexical-files tests

;; Coverage for ticket #151 (lexical-binding P1-5): the loader byte-scans
;; the first bytes of a SOURCE .el for the `lexical-binding: t' header and
;; specbinds internal-interpreter-environment to (t) for the file's
;; extent -- but only when the file's base name is on the
;; `load-lexical-files' whitelist (default nil).  .elc loads never
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

;; 1. Default (fixture base name not on the shipped whitelist): lexbound
;;    source loads fully dynamic, no behavior change.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(load file nil t)
	(Assert (null lexbind-loader-test-result))
	;; The env excursion does not leak past the load.
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 2. Whitelisted base name: the cookie is visible to the evaluator --
;;    the env is active for the file's extent.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(let ((load-lexical-files (list "lexbind-loader-cookie.el")))
	  (load file nil t))
	(Assert (eq lexbind-loader-test-result t))
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 3. Whitelisting is data: a different base name on the same whitelist
;;    does not activate the non-member file.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-other.el"
	     (concat lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(let ((load-lexical-files (list "lexbind-loader-cookie.el")))
	  (load file nil t))
	(Assert (null lexbind-loader-test-result)))
    (delete-file file)))

;; 4. .elc loads never trigger the scan, even when whitelisted: a
;;    byte-compiled file carrying the cookie stays fully dynamic.
(let ((file (lexbind-loader-test-write-fixture
	     "lexbind-loader-cookie.elc"
	     (concat ";ELC"
		     (string 20)
		     "\0\0\0\n"
		     lexbind-loader-test-fixture-header
		     "(lexbind-loader-test-body)\n"))))
  (unwind-protect
      (progn
	(let ((load-lexical-files (list "lexbind-loader-cookie.elc")))
	  (load file nil t))
	(Assert (null lexbind-loader-test-result))
	(Assert (null internal-interpreter-environment)))
    (delete-file file)))

;; 5. The option exists, and the P1-7 integration ships the full
;;    twelve-file whitelist as the runtime default (loadup installs
;;    `lexbound-lisp-files' past the dump-time preload extent; the
;;    C-level DEFVAR default stays nil for loadup-less processes).
;;    The XEmacs-only deviation sunsets when cconv lands, P2.
(Assert (boundp 'load-lexical-files))
(Assert (boundp 'lexbound-lisp-files))
(Assert (equal (default-value 'load-lexical-files) lexbound-lisp-files))

;;; end of lexbind-loader-tests.el
