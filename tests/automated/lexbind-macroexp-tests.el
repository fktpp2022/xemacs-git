;;; lexbind-macroexp-tests.el --- P1-3 env-aware macroexpansion tests

;; Coverage for ticket #149 (lexical-binding P1-3): GNU-compatible
;; env-aware macroexpansion -- `macroexpand' honors the ENVIRONMENT
;; macro-definition alist (shadowing, including (sym) / (sym . nil)
;; shadow-out), the new env-aware `macro-function' accessor, and the
;; (macro closure ...) expander path: a macro defined by defmacro in a
;; lexical-binding: t context is applied through funcall in its captured
;; environment, without misbinding its own arguments.
;;
;; Closure creation and closure application are interpreted-only until
;; P2/cconv, so lexical-mode sections are guarded like
;; lexbind-closure-tests.el; the dynamic-path sections run in both modes.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

;; interpreted-mode probe: setq, not defvar (the harness runs this file
;; twice in one session).
(defun lexbind-p13-mode-probe () t)
(setq lexbind-p13-interpreted
      (not (compiled-function-p (symbol-function 'lexbind-p13-mode-probe))))

;; ---- dynamic-path macroexpansion unchanged (both modes) --------------

(defmacro lexbind-p13-dyn-mac (x) `(list 'dyn ,x))
(defun lexbind-p13-dyn-fun (x) x)

(Assert (equal (macroexpand '(lexbind-p13-dyn-mac (+ 1 2)))
	       '(list 'dyn (+ 1 2))))
(Assert (equal (macroexpand '(lexbind-p13-dyn-fun 1))
	       '(lexbind-p13-dyn-fun 1)))
;; funcalling the expander returns the EXPANSION FORM (GNU semantics);
;; macroexpand additionally evaluates it.
(Assert (equal (funcall (macro-function 'lexbind-p13-dyn-mac)
		       '(1 2))
	       '(list 'dyn (1 2))))

;; macro-function basics (both modes)
(Assert (functionp (macro-function 'lexbind-p13-dyn-mac)))
(Assert (null (macro-function 'lexbind-p13-dyn-fun))) ; function, not macro
(Assert (null (macro-function 'lexbind-p13-no-such-symbol)))

;; ---- ENVIRONMENT alist honored (shadowing works, both modes) ---------

(let ((env `((lexbind-p13-dyn-mac . ,(lambda (x) `(shadowed ,x))))))
  (Assert (equal (macroexpand '(lexbind-p13-dyn-mac 5) env)
		 '(shadowed 5)))
  ;; macro-function consults the same alist
  (Assert (equal (funcall (macro-function 'lexbind-p13-dyn-mac env) 5)
		 '(shadowed 5))))

;; shadow-out: entry (sym) or (sym . nil) makes the symbol expand to itself
(Assert (equal (macroexpand '(lexbind-p13-dyn-mac 5)
			    '((lexbind-p13-dyn-mac)))
	       '(lexbind-p13-dyn-mac 5)))
(Assert (equal (macroexpand '(lexbind-p13-dyn-mac 5)
			    '((lexbind-p13-dyn-mac . nil)))
	       '(lexbind-p13-dyn-mac 5)))
(Assert (null (macro-function 'lexbind-p13-dyn-mac
			      '((lexbind-p13-dyn-mac)))))
;; environment entries for other symbols leave the global definition alone
(Assert (equal (macroexpand '(lexbind-p13-dyn-mac 5)
			    '((lexbind-p13-unrelated . t)))
	       '(list 'dyn 5)))
;; no leak: global expansion still in effect without the alist
(Assert (equal (macroexpand '(lexbind-p13-dyn-mac 5)) '(list 'dyn 5)))

(when lexbind-p13-interpreted

  ;; ---- macro defined in an active env wraps as a closure ---------------

  (let ((internal-interpreter-environment (list t)))
    (eval '(defmacro lexbind-p13-lex-mac (x) `(list 'lex ,x))))
  (if (not (eq (car (symbol-function 'lexbind-p13-lex-mac))
	       'macro))
      ;; Guard against surprising representation changes: only proceed
      ;; if defmacro produced the (macro closure ...) shape.
      (Assert (eq (macroexpand '(lexbind-p13-lex-mac 1))
		  (macroexpand '(lexbind-p13-lex-mac 1))))
    (Assert (eq (car (cdr (symbol-function 'lexbind-p13-lex-mac)))
		'closure))

    ;; (macro closure ...) expander applies via funcall without
    ;; misbinding its own args
    (Assert (equal (funcall (macro-function 'lexbind-p13-lex-mac) 5)
		   '(list 'lex 5)))
    (Assert (equal (macroexpand '(lexbind-p13-lex-mac (+ 2 3)))
		   '(list 'lex (+ 2 3))))

    ;; expansion works in a DIFFERENT active env than definition-time
    (let ((internal-interpreter-environment
	   (list t (cons 'lexbind-p13-elsewhere 99))))
      (Assert (equal (macroexpand '(lexbind-p13-lex-mac 7))
		     '(list 'lex 7))))

    ;; and in NO active env (dynamic context) -- the closure expander
    ;; still applies its captured environment
    (Assert (equal (macroexpand '(lexbind-p13-lex-mac 7))
		   '(list 'lex 7))))

  ;; ---- a macro capturing a definition-time lexical variable ------------

  (let ((internal-interpreter-environment (list t))
	(captured nil))
    (let ((base 10))
      (setq captured
	    (eval '(defmacro lexbind-p13-cap-mac (x)
		     `(+ ,base ,x)))))
    ;; base is out of scope here; the closure must supply it
    (Assert (equal (macroexpand '(lexbind-p13-cap-mac 5)) `(+ 10 5)))
    (Assert (eq (eval (macroexpand '(lexbind-p13-cap-mac 5))) 15))
    ;; expander's own args (x) are not misbound by repeated application
    (Assert (eq (eval (macroexpand '(lexbind-p13-cap-mac 7))) 17)))

  ;; ---- dynamic macro expanded while env is active ----------------------

  ;; A plain-lambda expander (defined dynamically) must expand correctly
  ;; under an active environment, without its helper bindings being
  ;; misresolved into the caller's lexical environment.
  (let ((internal-interpreter-environment
	 (list t (cons 'lexbind-p13-shadower 42))))
    (Assert (equal (macroexpand '(lexbind-p13-dyn-mac 9))
		   '(list 'dyn 9))))

  ;; ENVIRONMENT shadowing still wins over closure definitions
  (let ((internal-interpreter-environment (list t))
	(env `((lexbind-p13-lex-mac . ,(lambda (x) `(env-shadow ,x))))))
    (Assert (equal (macroexpand '(lexbind-p13-lex-mac 1) env)
		   '(env-shadow 1)))))

;; ---- subr-x loads, then a lexbind file loads: dolist/defmacro ----------
;; (the spike's crash scenario; run in both modes)

;; Runtime fixture files, rewritten each pass (idempotent toplevel).
(let* ((dir (file-name-as-directory (temp-directory)))
       (fixture (concat dir "lexbind-p13-fixture.el")))
  (write-region
   ";;; lexbind-p13-fixture.el --- P1-3 fixture -*- lexical-binding: t -*-
(defvar lexbind-p13-fixture-log nil)
(defmacro lexbind-p13-fixture-twice (form) `(list ,form ,form))
(defun lexbind-p13-fixture-run ()
  (let ((acc nil))
    (dolist (i '(1 2 3))
      (setq acc (cons (lexbind-p13-fixture-twice i) acc)))
    (nreverse acc)))
" nil fixture nil 'no-conversion)
  ;; subr-x itself is a lexbind file and must load first (whitelisted)
  (let ((load-lexical-files
	 (list "subr-x.el" "lexbind-p13-fixture.el")))
    (condition-case err
	(progn
	  (require 'subr-x)
	  (load fixture))
      (error (Assert (equal (list 'fixture-load-failed err) nil))))
    ;; dolist resolved correctly inside the lexbind fixture; its own
    ;; defmacro resolved correctly too
    (Assert (equal (lexbind-p13-fixture-run)
		   '((1 1) (2 2) (3 3))))
    ;; dynamic context still sees the fixture's functions
    (Assert (boundp 'lexbind-p13-fixture-log))))

;;; end of lexbind-macroexp-tests.el
