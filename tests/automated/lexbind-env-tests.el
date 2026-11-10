;;; lexbind-env-tests.el --- P1-1 env core + special-variable bit tests

;; Coverage for ticket #147 (lexical-binding P1-1): the
;; special-variable bit on Lisp_Symbol, the special-variable-p subr,
;; and the GNU-shaped Vinternal_interpreter_environment
;; ((t) + (sym . value) conses) as resolved by Feval.

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; special-variable-p: defvar / defconst / C DEFVAR / plain
(defvar lexbind-env-test-dyn 7)
(defconst lexbind-env-test-const 1)
(Assert (eq (special-variable-p 'lexbind-env-test-dyn) t))
(Assert (eq (special-variable-p 'lexbind-env-test-const) t))
;; debug-on-error is DEFVAR'd from C (eval.c)
(Assert (eq (special-variable-p 'debug-on-error) t))
(Assert (eq (special-variable-p 'lexbind-env-test-plain) nil))
(Assert (eq (special-variable-p (make-symbol "uninterned")) nil))

;; Hand-specbound env (GNU shape): lexical ref resolves; special
;; symbol ignores the env binding and stays dynamic.
(let ((internal-interpreter-environment
       (list t (cons 'lexbind-env-test-lex 42)
             (cons 'lexbind-env-test-dyn 99))))
  (Assert (eq (car internal-interpreter-environment) t))
  (Assert (eq (eval 'lexbind-env-test-lex) 42))
  (Assert (eq (eval 'lexbind-env-test-dyn) 7))
  ;; setq mutates the binding cons's cdr (XSETCDR), no wrapper cells
  (setq lexbind-env-test-lex 43)
  (Assert (eq (cdr (assq 'lexbind-env-test-lex
                         (cdr internal-interpreter-environment)))
              43))
  (Assert (eq (eval 'lexbind-env-test-lex) 43)))

;; Binding does not leak outside the specbound extent
(Assert (not (boundp 'lexbind-env-test-lex)))
;; Dynamic symbol unaffected by the env excursion
(Assert (eq lexbind-env-test-dyn 7))

;;; end of lexbind-env-tests.el
