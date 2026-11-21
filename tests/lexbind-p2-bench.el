;;; lexbind-p2-bench.el --- P2 (#162) performance gate

;; Runs under both the stock dynamic build and this tree.  Byte-compile
;; it with whatever compiler is at hand, then:
;;   xemacs -batch -l lexbind-p2-bench.elc
;; Every case but `closure-create' is deliberately env-free, so it
;; exercises the *dynamic* fast path and measures what the P2 varref
;; guard costs code that never opts in.

(defconst b-iter 1000000)
(defconst b-runs 5)

(defvar b-dyn 7 "Special var: reading it compiles to a dynamic varref.")

(defvar b-sink nil)

(defun b-secs ()
  "Current time in float seconds, portable to stock 21.5."
  (let ((t0 (current-time)))
    (+ (* (car t0) 65536.0) (cadr t0) (/ (car (cddr t0)) 1000000.0))))

(defun b-run (label thunk)
  "Run THUNK B-RUNS times, report the minimum elapsed seconds."
  (let ((best most-positive-fixnum))
    (dotimes (_ b-runs)
      (garbage-collect)
      (let ((t0 (b-secs)))
        (funcall thunk)
        (setq best (min best (- (b-secs) t0)))))
    (princ (format "%-18s %8.4f\n" label best))
    best))

(defun b-arith-loop ()
  "Control: same loop body, no variable access to optimize."
  (let ((i 0))
    (while (< i b-iter) (setq i (1+ i)))))

(defun b-varref-loop ()
  (let ((i 0))
    (while (< i b-iter) (setq i (1+ (if (= b-dyn 7) i i))))))

(defun b-varset-loop ()
  (let ((i 0))
    (while (< i b-iter) (setq b-dyn i) (setq i (1+ i)))))

(defun b-funcall-loop (f)
  (let ((i 0))
    (while (< i b-iter) (setq i (funcall f i)))))

(defun b-closure-create-loop ()
  (let ((i 0))
    (while (< i 100000) (setq b-sink (let ((x i)) (lambda () x))) (setq i (1+ i)))))

(princ (format "iter=%d runs=%d\n" b-iter b-runs))
(let ((arith (b-run "arith-loop" #'b-arith-loop))
      (vref  (b-run "varref-loop" #'b-varref-loop))
      (vset  (b-run "varset-loop" #'b-varset-loop)))
  (b-run "funcall-compiled" (lambda () (b-funcall-loop #'1+)))
  (b-run "closure-create" #'b-closure-create-loop)
  ;; Ratios cancel general build/version differences between the two
  ;; binaries; the delta is what the varref guard itself costs.
  (princ (format "varref/arith     %8.3f\n" (/ vref arith)))
  (princ (format "varset/arith     %8.3f\n" (/ vset arith))))
