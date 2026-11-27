;;; lexbind-hoist-tests.el --- env-guard hoist refresh regression

;; The byte-code VM caches LEXBIND_ENV_ACTIVE_P() once per
;; execute_optimized_program call (P2 gate #173) instead of re-reading
;; Vinternal_interpreter_environment on every varref/varset.  That hoist
;; is only sound if the cold sites that can move the env mid-frame
;; refresh it.  These cases turn the env on inside one compiled function
;; and then read a non-special symbol; the read must see the new env.
;; A stale hoist skips the env and signals void-variable.

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; The read symbol is deliberately NOT defvar'd: it must stay non-special
;; so a varref consults the ambient lexical environment.
(defun lexbind-hoist-env-setq-fn ()
  "A dynamically-compiled fn that turns the env on with setq, then reads."
  (let ((byte-compile-lexical-binding nil))
    (byte-compile
     '(lambda ()
        (setq internal-interpreter-environment
              (list t (cons 'lexbind-hoist-fresh 1)))
        lexbind-hoist-fresh))))

(defun lexbind-hoist-env-let-fn ()
  "A dynamically-compiled fn that turns the env on with let, then reads."
  (let ((byte-compile-lexical-binding nil))
    (byte-compile
     '(lambda ()
        (let ((internal-interpreter-environment
               (list t (cons 'lexbind-hoist-fresh 2))))
          lexbind-hoist-fresh)))))

;; Bvarset refresh: env turned on by setq of the env symbol.
(let ((internal-interpreter-environment nil))
  (Assert (condition-case nil
              (eq 1 (funcall (lexbind-hoist-env-setq-fn)))
            (error nil))
          "varref after setq of the env symbol must consult the new env"))

;; Bvarbind refresh: env turned on by let-binding the env symbol.
(let ((internal-interpreter-environment nil))
  (Assert (condition-case nil
              (eq 2 (funcall (lexbind-hoist-env-let-fn)))
            (error nil))
          "varref after let of the env symbol must consult the new env"))

;;; end of lexbind-hoist-tests.el
