;;; lexbind-spike.el --- PROTOTYPE (lexbind spike).  Throwaway; never ships.
;;; -*- lexical-binding: t; -*-
;;
;; Exercises the capture path end to end while the loader's lexical
;; extent is active (top level): interpreted closures (plain + mutable),
;; let*/let lexical binding, the special-variable bit, and a
;; hand-emitted compiled closure through make-byte-code's new env slot.
;; Results are stashed in `lexbind-spike-results' so a plain
;; (dynamic-context) caller can inspect them after the file's extent.

;;; Code:

;; Whitelist this file for real activation (see load-lexical-files doc).
;; Must run BEFORE the cookie line is reached? No: the loader scans the
;; header before evaluating any form, so this only matters for nested
;; (load "lexbind-spike") calls; the priming is done by the caller.
;; The dump's loadup.el primes the whitelist before loading us.
(print (list :cookie-active internal-interpreter-environment :wl load-lexical-files))

;; --- special-variable bit -------------------------------------------------
(defvar lexbind-spike-special nil)      ; must get the special bit

;; --- interpreted: plain capture -------------------------------------------
;; Built while the file's lexical extent is active, so #'lambda closes
;; over the let-bound `a'.
(setq lexbind-spike-results
      (let* ((a 10)
	     (adder #'(lambda (b) (+ a b)))
	     (r-adder (funcall adder 5))
	     ;; --- interpreted: mutable capture ------------------------
	     (n 0)
	     (counter #'(lambda () (setq n (1+ n)) n))
	     (r-counter (list (funcall counter) (funcall counter)
			      (funcall counter)))
	     ;; --- let* chain seeing earlier lexical bindings ----------
	     (r-let* (let* ((x 1) (y (1+ x)) (z (1+ y))) (list x y z)))
	     ;; --- let on a special var stays dynamic ------------------
	     (r-special
	      (progn
		(let ((lexbind-spike-special 'lexical?))
		  lexbind-spike-special))))
	(list :adder r-adder              ; expect 15
	      :counter r-counter          ; expect (1 2 3) shared cell
	      :let* r-let*                ; expect (1 2 3)
	      :special r-special          ; expect lexical?
	      :global-after (default-value 'lexbind-spike-special) ; nil
	      :bit (list (special-variable-p 'lexbind-spike-special)
			 (special-variable-p 'lexbind-spike-never)))))

;; --- compiled: hand-emitted closure (make-byte-code with env slot) --------
;; The compiler has no cconv yet, so the spike hand-emits byte code:
;; (lambda (a) captured x=40) => a + x = 42. Opcodes: Blexref = 0270,
;; Bplus = 0134, Breturn = 0207. frame[0] = outer cell (x = 40);
;; frame[1] = the arg a, appended by funcall_compiled_function.
(defconst lexbind-spike-compiled-adder
  (make-byte-code
   '(a)                                     ; arglist
   "\270\0\270\1\134\207"                   ; lexref 0; lexref 1; plus; return
   []                                       ; constants: none needed
   3                                        ; stack depth
   nil                                      ; doc
   nil                                      ; interactive
   (vector (cons 40 'cell))                 ; ENV slot (frame[0] = x = 40)
   ))

(setq lexbind-spike-results
      (plist-put lexbind-spike-results
		 :compiled (funcall lexbind-spike-compiled-adder 2)))
;; expect :compiled => 42

(provide 'lexbind-spike)
;;; lexbind-spike.el ends here
