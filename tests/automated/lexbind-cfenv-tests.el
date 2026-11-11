;;; lexbind-cfenv-tests.el --- P2-2 environment slot on Lisp_Compiled_Function

;; Coverage for ticket #158 (P2-2): the `environment' slot on compiled
;; functions.  This is C-level plumbing only: the slot exists, is
;; initialized to nil, is GC-marked, and survives dump/reload.  The
;; make-byte-code env argument (#160) and the opcodes (#159) are out of
;; scope, so the tests hand-build a compiled function via
;; `make-byte-code' and set the env with the low-level setter
;; `set-compiled-function-environment'.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(defun lexbind-p22-make-cf ()
  "Build a plain (make-byte-code) compiled function for probing."
  (make-byte-code '(x) "\x87" [1 2 3] 1))

;; ---- slot exists, initialized to nil ------------------------------------

(let ((cf (lexbind-p22-make-cf)))
  (Assert (compiled-function-p cf))
  (Assert (null (compiled-function-environment cf))))

;; ---- setter stores, getter returns, eq identity -------------------------

(let ((cf (lexbind-p22-make-cf))
      (env (list t '(captured . cell))))
  (set-compiled-function-environment cf env)
  (Assert (eq (compiled-function-environment cf) env)))

;; ---- GC marks the env (closure env with captured cells survives) --------

(let ((cf nil))
  ;; Build env containing a mutable captured cell, set it, drop our
  ;; only other reference, then force a full outer GC cycle.
  (let ((env (list t (list 'lexical-var (list 'cell 42)))))
    (setq cf (lexbind-p22-make-cf))
    (set-compiled-function-environment cf env))
  (gc-outer)
  (let ((env (compiled-function-environment cf)))
    (Assert (and (consp env) (eq (car env) t)))
    (let ((binding (assq 'lexical-var (cdr env))))
      (Assert (consp binding))
      ;; The captured cell must still be live and mutable.
      (setcar (cdr binding) 43)
      (Assert (eq (car (cdr (cdr binding))) 43)))))

;; ---- equal distinguishes environments (updated by P2-5, #161) ----------

(let ((cf1 (lexbind-p22-make-cf))
      (cf2 (lexbind-p22-make-cf)))
  (Assert (equal cf1 cf2))
  (set-compiled-function-environment cf1 (list t))
  ;; Since P2-5 (#161) compiled_function_equal compares the env slot:
  ;; closures differing only in env are not equal.
  (Assert (not (equal cf1 cf2))))

;; ---- dump/reload: env survives the real pdump round trip ----------------
;; The build dumps the whole image from temacs through the portable
;; dumper, and loadup.el loads lisp/lexbind-dump-demo.el at dump time.
;; Its P2-2 block hand-builds a compiled function, sets a non-nil env
;; via the low-level setter, and stores it in `lexbind-demo-cf-env',
;; so the dumped image carries an env-carrying compiled function.  Here
;; (running under the dumped binary) we assert the env survived reload
;; through compiled_function_description's new XD_LISP_OBJECT row.
(when (boundp 'lexbind-demo-cf-env)
  (Assert (compiled-function-p lexbind-demo-cf-env))
  (let ((env (compiled-function-environment lexbind-demo-cf-env)))
    (Assert (and (consp env) (eq (car env) t)))
    (Assert (eq (cdr (car (cdr env))) 42))))

;; Since P2-5 (#161) the printer prints a non-nil env readably, and the
;; printed 7-element #[...] form reads back with the env intact.
(let ((cf (lexbind-p22-make-cf)))
  (set-compiled-function-environment cf (list t))
  (let ((cf2 (read (let ((print-readably t)) (prin1-to-string cf)))))
    (Assert (compiled-function-p cf2))
    (Assert (equal cf cf2))
    (Assert (equal (compiled-function-environment cf2) (list t)))))

;;; end of lexbind-cfenv-tests.el
