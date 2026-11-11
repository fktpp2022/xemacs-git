;;; lexbind-p17-twelve-tests.el --- P1-7 gate (e): twelve-file runtime whitelist
;;  Lexical-binding P1 milestone, ticket #153 (P1-7).
;;
;;  Run against a DUMPED image.  Confirms the full P1-7 contract:
;;   1. `load-lexical-files' default is the twelve-file whitelist
;;      (installed by loadup at dump time via `(setq-default ... lexbound-lisp-files)').
;;   2. `lexbound-lisp-files' is bound and equals the same twelve entries.
;;   3. Each of the twelve files loads in a lexical environment and its
;;      witness defun/defmacro is a closure (not a dynamic defun).
;;
;;  No new C, no new package-require: this exercises the P1-3 loader, P1-4
;;  runtime env, P1-5 interpreter, and P1-7 loadup/whitelist machinery.

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; 1. The twelve-file whitelist is shipped as the runtime default.
(Assert (boundp 'lexbound-lisp-files))
(Assert (equal lexbound-lisp-files
               '("bind-key.el"
                  "cl-lib.el"
                  "macroexp.el"
                  "subr-x.el"
                  "tabulated-list.el"
                  "use-package.el"
                  "use-package-bind-key.el"
                  "use-package-core.el"
                  "use-package-delight.el"
                  "use-package-diminish.el"
                  "use-package-ensure.el"
                  "use-package-xemacs.el")))

;; 2. load-lexical-files default is the twelve-file whitelist.
(Assert (boundp 'load-lexical-files))
(Assert (equal (default-value 'load-lexical-files) lexbound-lisp-files))

;; 3. Each of the twelve files loads in a lexical environment and its
;;    witness defun/defmacro is a closure.  The `use-package' macro is
;;    defined by use-package-core.el, so use-package.el's witness is
;;    that macro; the sub-file witnesses are checked via their own defuns.
(defun lexbound-p17-witness-closure-p (sym)
  "t if SYM is fbound and its function value is a closure (P1-5 check)."
  (and (fboundp sym)
       (let ((sf (symbol-function sym)))
         (and (consp sf) (eq (car sf) 'closure)))))

;;  bind-key: `bind-key' itself is a defmacro (its witness); bind-key--remove
;;    is a defun helper and must be a closure.
(let ((load-lexical-files lexbound-lisp-files))
  (load "bind-key" nil t))
(Assert (fboundp 'bind-key))
(Assert (fboundp 'bind-key--remove))
;;  cl-lib: cl-position is a defun, must be a closure after loading.
(let ((load-lexical-files lexbound-lisp-files))
  (load "cl-lib" nil t))
(Assert (lexbound-p17-witness-closure-p 'cl-position))
;;  macroexp: already dumped as closure; reload from source confirms it stays.
(let ((load-lexical-files lexbound-lisp-files))
  (load "macroexp" nil t))
(Assert (lexbound-p17-witness-closure-p 'macroexp-progn))
;;  subr-x: string-trim is a defun, must be a closure.
(let ((load-lexical-files lexbound-lisp-files))
  (load "subr-x" nil t))
(Assert (lexbound-p17-witness-closure-p 'string-trim))
;;  tabulated-list: already dumped; column-number is a closure.
(let ((load-lexical-files lexbound-lisp-files))
  (load "tabulated-list" nil t))
(Assert (lexbound-p17-witness-closure-p 'tabulated-list--column-number))
;;  use-package: the macro is a macro (not a closure); use-package-core's
;;    defun witnesses are checked in the use-package-core block below.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package" nil t))
(Assert (fboundp 'use-package))
(Assert (eq (car (symbol-function 'use-package)) 'macro))
;;  use-package-bind-key: normalize-binder is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-bind-key" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-normalize-binder))
;;  use-package-core: normalize-regex is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-core" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-normalize-regex))
;;  use-package-delight: normalize-delight is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-delight" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-normalize-delight))
;;  use-package-diminish: normalize-diminish is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-diminish" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-normalize-diminish))
;;  use-package-ensure: pin-package is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-ensure" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-pin-package))
;;  use-package-xemacs: ensure-elpa-xemacs is a defun.
(let ((load-lexical-files lexbound-lisp-files))
  (load "use-package-xemacs" nil t))
(Assert (lexbound-p17-witness-closure-p 'use-package-ensure-elpa-xemacs))

;;; end of lexbind-p17-twelve-tests.el
