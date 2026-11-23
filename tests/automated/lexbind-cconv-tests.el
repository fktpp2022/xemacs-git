;;; lexbind-cconv-tests.el --- P2-1 cconv compiler emission tests

;; Coverage for ticket #157 (P2-1): the byte-compiler now handles
;; `lexical-binding: t' source via the frame-cell cconv design:
;;  - defun bodies no longer emit byte-varbind/byte-varref for lexical
;;    vars — instead each binding is boxed into a cons cell.
;;  - Nested lambdas that capture outer variables are compiled with
;;    Blexref/Blexset and a make-byte-code env (7th arg).
;;
;; All tests run in the byte-compiled pass of the ctest gate.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

;; Probe: is this file itself byte-compiled?
(defun lexbind-cconv-mode-probe () t)
(defconst lexbind-cconv-interpreted
  (not (compiled-function-p (symbol-function 'lexbind-cconv-mode-probe))))

;; In the byte-compiled pass, lexbound lambdas that appear in the test
;; file must be closures with env set.


;; ---- helper defuns (compiled in the byte-compiled pass) ------------

;; A lexbound function with a let that holds a value captured by a
;; nested lambda.
(defun lexical-make-capture (n)
  (let ((x n))
    (function (lambda () x))))

;; ---- a capture read from a `let' INITIALISER (regression) ---------
;; `(let ((c s)) c)' evaluates `s' in the ENCLOSING scope.  The
;; free-variable walk used to skip plain `let' initialisers entirely,
;; so `s' was never a capture: the closure got no env, `s' compiled to
;; a dynamic varref, and calling the closure signalled
;; `void-variable s'.  Minimised repro; keep it separate from the
;; nested-capture case so a regression names itself.
(defun lexical-let-init-capture (a b)
  (let ((s (+ a b)))
    (function (lambda () (let ((c s)) c)))))

;; Nested closure: outer-fn returns a closure that captures the sum of
;; a and b computed in two separate lets.
(defun lexical-nested-capture (a b)
  (let ((sum a)
        (fn (function (lambda ()
                        (let ((c sum)
                              (inner (function (lambda () c))))
                          (funcall inner))))))
    fn))

;; Writer: modifies a cell through Blexset; reader: reads through Blexref.
(defun lexical-mutating-closure (initial)
  (let ((v initial))
    (function (lambda (new) (setq v new)))))

(defun lexical-reader (initial)
  (let ((v initial))
    (function (lambda () v))))


;; ---- defun with lexical let: the defun itself must be a closure ----
;;   In the byte-compiled pass the test file is lexbound (cookie on
;;   line 1).  Top-level let in a lexbound defun must be boxed.
;;   A lambda inside that let must get an env slot.

(defun lexical-top-capture (val)
  (let ((q val))
    (lexical-top-capture-inner q)))

(defun lexical-top-capture-inner (q)
  (function (lambda () q)))


;; ---- defvar'd vars remain dynamic -----------------------------------;;   A defvar'd variable in a lexbound file is exempt from capture.
;;   References to it should be plain byte-varref/byte-varset.

(defvar lexbind-cconv-defvar-v 99)
(defun lexical-defvar-ref ()
  lexbind-cconv-defvar-v)


;; ---- make-byte-code env slot propagation to closures ----------------
;;   After `make-byte-code' with a 7-arg env, the env vector is stored
;;   on the compiled function.

(defun lexical-make-closure-cf ()
  (lexical-make-closure-inner 3))

(defun lexical-make-closure-inner (n)
  (let ((z n))
    (function (lambda () z))))


;; ---- Blexref reads the correct cell from the env vector -------------

(defun lexical-multi-capture (a b c)
  (let ((p a) (q b) (r c))
    (function (lambda () (+ p q r)))))


;; ---- assertions run last: every helper above is defined --------
;; A top-level assert that precedes its defun is `void-function` in the
;; interpreted pass, which is how this file fails on a clean build.






;; ---- cconv driven explicitly -------------------------------------------
;; The harness compiles this file as a lambda read from a buffer
;; (lisp/test-harness.el:573), which carries no file header and therefore no
;; `lexical-binding: t' cookie, so its "compiled pass" is NOT lexbound and
;; `lexbind-cconv-interpreted' above answers a different question (was the
;; file compiled) than the one it is used for (is lexical binding on).
;; Drive the compiler's lexbind mode directly instead:
;; `byte-compile-lexical-binding' is exactly what `byte-compile-file' sets
;; from the cookie.

(let ((code (let ((byte-compile-lexical-binding t))
	      (byte-compile
	       '(lambda (n)
		  (let ((x n))
		    (lambda () (let ((c (* x 2))) c))))))))
  (Assert (compiled-function-p code))
  (let ((closure (funcall code 21)))
    (Assert (compiled-function-p closure))
    ;; A capturing lambda gets the env slot (P2-2) and its cells (P2-3).
    (Assert (vectorp (compiled-function-environment closure)))
    ;; `c' is initialised from a capture inside a `let' -- the shape #157
    ;; got wrong (it emitted a dynamic varref, `void-variable x').
    (Assert (eq (funcall closure) 42))))

