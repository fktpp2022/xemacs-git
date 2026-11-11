;;; lexbind-closure-tests.el --- P1-2 closures + lexical let tests

;; Coverage for ticket #148 (lexical-binding P1-2): interpreted closures
;; ((closure ENV ARGLIST . BODY)), lexical let/let* binding into the
;; GNU-shaped environment, defun/defmacro wrapping, funcall dispatch,
;; setq mutation of binding conses, and special-symbol dynamic fallback.
;;
;; P1-2 is interpreted-only by design (byte-compiled lexical binding is
;; P2/cconv territory), so the lexical-mode sections are guarded to run
;; only under interpreted Lisp.  The dynamic-path sections run in both
;; modes.

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

(defvar lexbind-p12-special 7)

;; Interpreted-code probe: a defun is a compiled function only in a
;; byte-compiled test run.
(defun lexbind-p12-mode-probe () t)
;; setq (not just defvar): the harness runs this file twice in one
;; session (interpreted, then byte-compiled); defvar would keep the
;; interpreted pass's value in the compiled pass.
(setq lexbind-p12-interpreted
      (not (compiled-function-p (symbol-function 'lexbind-p12-mode-probe))))

;; ---- dynamic path unchanged when env is inactive (both modes) ---------

(Assert (eq (let ((x 5) (y 6)) (+ x y)) 11))
(Assert (eq (let* ((x 5) (y (* x 2))) y) 10))
(Assert (eq (let ((lexbind-p12-special 8)) lexbind-p12-special) 8))
(Assert (eq lexbind-p12-special 7))
(Assert (not internal-interpreter-environment))

;; defun outside lexical context stays a plain lambda (interpreted;
;; byte-compiled runs legitimately produce compiled functions)
(defun lexbind-p12-dynamic-fun (x) (1+ x))
(if lexbind-p12-interpreted
    (Assert (eq (car (symbol-function 'lexbind-p12-dynamic-fun)) 'lambda))
  (Assert (compiled-function-p (symbol-function 'lexbind-p12-dynamic-fun))))
(fmakunbound 'lexbind-p12-dynamic-fun)

(when lexbind-p12-interpreted

  ;; ---- #'lambda under an active env yields a closure ------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((add3 #'(lambda (x) (+ x 3))))
      (Assert (eq (car add3) 'closure))
      (Assert (consp (car (cdr add3))))          ; ENV is the captured list
      (Assert (eq (car (car (cdr add3))) t))     ; GNU shape: leading t
      (Assert (eq (funcall add3 4) 7))))

  ;; Called from dynamic context (env inactive) it still captures correctly
  (let ((f nil))
    (let ((internal-interpreter-environment (list t)))
      (let ((n 10))
        (setq f #'(lambda (x) (+ n x)))))
    ;; env is nil again here
    (Assert (eq (funcall f 5) 15)))

  ;; ---- plain capture: the spike's adder -------------------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((adder (let ((n 10)) #'(lambda (x) (+ n x)))))
      (Assert (eq (funcall adder 5) 15))))

  ;; ---- mutable shared cell: two closures over one let share the cell --

  (let ((internal-interpreter-environment (list t)))
    (let ((count 0))
      (let ((inc #'(lambda () (setq count (1+ count))))
            (get #'(lambda () count)))
        (funcall inc)
        (funcall inc)
        (funcall inc)
        (Assert (eq (funcall get) 3)))))

  ;; ---- separate lets stay independent ---------------------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((f nil) (g nil))
      (let ((count 0))
        (setq f (cons #'(lambda () (setq count (1+ count)))
                      #'(lambda () count))))
      (let ((count 0))
        (setq g (cons #'(lambda () (setq count (1+ count)))
                      #'(lambda () count))))
      (funcall (car f))
      (funcall (car f))
      (funcall (car g))
      (Assert (eq (funcall (cdr f)) 2))
      (Assert (eq (funcall (cdr g)) 1))))

  ;; ---- setq mutates the binding cons's cdr (GNU shape) ----------------

  (let ((internal-interpreter-environment (list t)))
    (let ((x 1))
      (setq x 2)
      (Assert (eq x 2))
      (Assert (eq (cdr (assq 'x (cdr internal-interpreter-environment))) 2))))

  ;; ---- defun / defmacro wrap in lexical context ----------------------

  (let ((internal-interpreter-environment (list t)))
    (defun lexbind-p12-make-adder (n)
      #'(lambda (x) (+ n x)))
    (Assert (eq (car (symbol-function 'lexbind-p12-make-adder)) 'closure))
    ;; plain capture through a named function, called from dynamic context
    (let ((add5 (lexbind-p12-make-adder 5)))
      (Assert (eq (car add5) 'closure))
      (Assert (eq (funcall add5 3) 8)))
    (defmacro lexbind-p12-inc (x)
      (list '1+ x))
    (Assert (eq (car (cdr (symbol-function 'lexbind-p12-inc))) 'closure))
    (Assert (eq (lexbind-p12-inc 4) 5))
    (fmakunbound 'lexbind-p12-make-adder)
    (fmakunbound 'lexbind-p12-inc))

  ;; ---- arg binding: required / &optional / &rest ----------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((f #'(lambda (a &optional b &rest c) (list a b c))))
      (Assert (equal (funcall f 1) '(1 nil nil)))
      (Assert (equal (funcall f 1 2) '(1 2 nil)))
      (Assert (equal (funcall f 1 2 3 4) '(1 2 (3 4))))))

  ;; ---- let* sees earlier bindings; let evaluates before binding -------

  (let ((internal-interpreter-environment (list t)))
    (let* ((a 1)
           (b (1+ a))
           (c (* b 2)))
      (Assert (and (eq a 1) (eq b 2) (eq c 4))))
    (let ((w 42))
      (let ((w 1) (v w))                 ; v sees the OUTER w
        (Assert (eq w 1))
        (Assert (eq v 42)))
      (Assert (eq w 42)))
    (let ((w 42))
      (let* ((w 1) (v w))                ; let*: v sees the NEW w
        (Assert (eq w 1))
        (Assert (eq v 1)))
      (Assert (eq w 42)))
    ;; bindings do not leak
    (Assert (not (boundp 'a))))

  ;; ---- let on a special symbol specbinds dynamically ------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((lexbind-p12-special 99))
      (Assert (eq lexbind-p12-special 99)))
    (Assert (eq lexbind-p12-special 7)))
  (Assert (eq lexbind-p12-special 7))

  ;; special parameters in closures also stay dynamic
  (let ((internal-interpreter-environment (list t)))
    (let ((f #'(lambda (lexbind-p12-special) lexbind-p12-special)))
      (Assert (eq (funcall f 21) 21)))
    (Assert (eq lexbind-p12-special 7)))

  ;; ---- GC stress: closures survive collection -------------------------

  (let ((internal-interpreter-environment (list t)))
    (let ((adders nil))
      (let ((i 0))
        (while (< i 100)
          (setq adders (cons #'(lambda (x) (+ i x)) adders))
          (setq i (1+ i))))
      (garbage-collect)
      ;; P1-2 semantics: all closures capture the same binding cons of i
      ;; (shared mutable cell), and setq left i at 100 after the loop.
      ;; The point of the stress is that closures and their cell survive
      ;; the explicit GC above.
      (Assert (eq (funcall (car adders) 1) 101))
      (Assert (eq (funcall (car (last adders)) 1) 101))
      ;; shared mutable cell survives GC
      (let ((count 0))
        (let ((inc #'(lambda () (setq count (1+ count)))))
          (garbage-collect)
          (funcall inc)
          (funcall inc)
          (Assert (eq count 2))))))

  ) ; end interpreted-only section

;;; end of lexbind-closure-tests.el
