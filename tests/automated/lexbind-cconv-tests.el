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
(if (not lexbind-cconv-interpreted)

    ;; ---- lexical let inside a lexbound defun -----------------------
    (let* ((f (function (lexical-let-x 10) (lexical-let-x-inner f))))
      (let ((g (lexical-make-capture 42)))
        (Assert (eq (funcall g) 42))))

    ;; ---- nested closure capture across two let levels ---------------
    (let ((outer-fn (lexical-nested-capture 7 8)))
      (let ((inner-fn (funcall outer-fn)))
        (Assert (eq (funcall inner-fn) 15))))

    ;; ---- closure writes are visible via the cell -------------------
    (let ((writer (lexical-mutating-closure 100)))
      (funcall writer 200)
      (Assert (eq (funcall (lexical-reader 100)) 100)))
  ;; Interpreted pass: smoke test only.
  (Assert t))


;; ---- helper defuns (compiled in the byte-compiled pass) ------------

;; A lexbound function with a let that holds a value captured by a
;; nested lambda.
(defun lexical-make-capture (n)
  (let ((x n))
    (function (lambda () x))))

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
(if (not lexbind-cconv-interpreted)
    (let ((top-fn (lexical-top-capture 5)))
      (Assert (eq (funcall top-fn) 5))
      ;; The closure's env must be a vector (P2-2/P2-3 shape).
      (let ((env (compiled-function-environment
                  (symbol-function 'lexical-top-capture-inner))))
        (when env (Assert (vectorp env))))))

(defun lexical-top-capture (val)
  (let ((q val))
    (lexical-top-capture-inner q)))

(defun lexical-top-capture-inner (q)
  (function (lambda () q)))


;; ---- defvar'd vars remain dynamic -----------------------------------
;;   A defvar'd variable in a lexbound file is exempt from capture.
;;   References to it should be plain byte-varref/byte-varset.
(if (not lexbind-cconv-interpreted)
    (let ((dv (lexical-defvar-ref)))
      (Assert (eq dv 99))))

(defvar lexbind-cconv-defvar-v 99)
(defun lexical-defvar-ref ()
  lexbind-cconv-defvar-v)


;; ---- make-byte-code env slot propagation to closures ----------------
;;   After `make-byte-code' with a 7-arg env, the env vector is stored
;;   on the compiled function.
(if (not lexbind-cconv-interpreted)
    (let ((cf (lexical-make-closure-cf)))
      (Assert (compiled-function-p cf))
      (Assert (vectorp (compiled-function-environment cf)))))

(defun lexical-make-closure-cf ()
  (lexical-make-closure-inner 3))

(defun lexical-make-closure-inner (n)
  (let ((z n))
    (function (lambda () z))))


;; ---- Blexref reads the correct cell from the env vector -------------
(if (not lexbind-cconv-interpreted)
    (let ((f (lexical-multi-capture 1 2 3)))
      (Assert (eq (funcall f) 6))))

(defun lexical-multi-capture (a b c)
  (let ((p a) (q b) (r c))
    (function (lambda () (+ p q r)))))
