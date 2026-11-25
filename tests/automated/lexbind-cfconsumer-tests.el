;;; lexbind-cfconsumer-tests.el --- P2-5 closure consumer updates

;; Coverage for ticket #161 (P2-5): compiled-function consumers that
;; must know about the `environment' slot:
;;   - `equal' distinguishes closures over different environments.
;;   - the printer prints #[...] readably including a non-nil env, and
;;     the printed form reads back with equivalent behavior.
;;   - `define-function' annotation stays orthogonal to the env slot.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(defun lexbind-p25-make-cf (&optional env)
  "Build a compiled function for probing, optionally carrying ENV."
  (let ((cf (make-byte-code '(x) "\x87" [1 2 3] 1)))
    (when env
      (set-compiled-function-environment cf env))
    cf))

;; ---- equal: same env equal, different env not equal ---------------------

;; Baseline: env-free compiled functions compare as before.
(let ((cf1 (lexbind-p25-make-cf))
      (cf2 (lexbind-p25-make-cf)))
  (Assert (equal cf1 cf2)))

;; Both carrying the *same* (eq) env: equal.
(let ((env (list t '(x . cell)))
      (cf1 (lexbind-p25-make-cf))
      (cf2 (lexbind-p25-make-cf)))
  (set-compiled-function-environment cf1 env)
  (set-compiled-function-environment cf2 env)
  (Assert (equal cf1 cf2)))

;; Environments that are merely equal (not eq): still equal.
(let ((cf1 (lexbind-p25-make-cf '(a b)))
      (cf2 (lexbind-p25-make-cf '(a b))))
  (Assert (equal cf1 cf2)))

;; Differing only in env: NOT equal.
(let ((cf1 (lexbind-p25-make-cf '(a)))
      (cf2 (lexbind-p25-make-cf '(b))))
  (Assert (not (equal cf1 cf2))))

;; One nil env vs one non-nil env: NOT equal.
(let ((cf1 (lexbind-p25-make-cf))
      (cf2 (lexbind-p25-make-cf '(a))))
  (Assert (not (equal cf1 cf2))))

;; ---- print/read: #[...] carries a non-nil env, reads back ---------------

(defun lexbind-p25-print-readable (object)
  "Print OBJECT to a string in the readable #[...] form."
  (let ((print-readably t))
    (prin1-to-string object)))

;; Env-free functions keep the 6-element printed form and round-trip.
(let ((cf (lexbind-p25-make-cf)))
  (let* ((form (read (lexbind-p25-print-readable cf))))
    (Assert (compiled-function-p form))
    (Assert (equal cf form))
    (Assert (null (compiled-function-environment form)))))

;; A non-nil env is printed (readable form has 7 elements) and reads
;; back with equivalent behavior: same code, same env.
(let* ((env (list t '(lexical-var (cell . 42))))
       (cf (lexbind-p25-make-cf env))
       (string (lexbind-p25-print-readable cf))
       (form (read string)))
  (Assert (compiled-function-p form))
  ;; print == read == original behavior
  (Assert (equal cf form))
  (Assert (equal (compiled-function-environment form) env))
  (Assert (string-match "\\`#\\[" string))
  ;; the env really made it into the printed form: the env-free twin
  ;; prints differently and reads back with a nil env
  (Assert (not (equal string (lexbind-p25-print-readable
			      (lexbind-p25-make-cf)))))
  (Assert (null (compiled-function-environment
		 (read (lexbind-p25-print-readable
			(lexbind-p25-make-cf)))))))

;; ---- equal-objects hash consistency --------------------------------------

;; equal compiled functions must hash equal (hash includes the env).
(let ((cf1 (lexbind-p25-make-cf '(a b)))
      (cf2 (lexbind-p25-make-cf '(a b))))
  (Assert (= (sxhash cf1) (sxhash cf2))))

;; ---- define-function annotation path is orthogonal -----------------------

;; An env-carrying compiled function passed through define-function
;; gets annotated like any other; the env slot is untouched.
(let* ((env (list t '(y . cell)))
       (cf (lexbind-p25-make-cf env)))
  (define-function 'lexbind-p25-annotated cf)
  (Assert (eq (symbol-function 'lexbind-p25-annotated) cf))
  (Assert (equal (compiled-function-annotation cf)
                 'lexbind-p25-annotated))
  (Assert (equal (compiled-function-environment cf) env)))

;;; end of lexbind-cfconsumer-tests.el
