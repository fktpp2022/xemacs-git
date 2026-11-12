;;; lexbind-ops-tests.el --- P2-3 Blexref/Blexset opcodes

;; Coverage for ticket #159 (P2-3): the Blexref/Blexset byte opcodes,
;; reading and writing captured cells through the compiled function's
;; environment slot (P2-2).  The compiler's emission side (#157) does
;; not exist yet, so all bytecode here is hand-emitted, following the
;; prototype/lexbind-capture-spike operand shapes: each opcode takes a
;; one-byte operand, the index of the captured cell in the environment
;; VECTOR.  The environment is attached with the P2-2 low-level setter
;; `set-compiled-function-environment' (the make-byte-code env argument
;; is #160, out of scope here).

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

;; byte-lexref = 184 (0270), byte-lexset = 185 (0271), byte-return = 135.
(defconst lexbind-p23-lexref "\330")
(defconst lexbind-p23-lexset "\331")
(defconst lexbind-p23-return "\207")

(defun lexbind-p23-make-closure (instructions env)
  "Hand-build a compiled function with INSTRUCTIONS and environment ENV."
  (let ((cf (make-byte-code '() instructions [] (length instructions))))
    (set-compiled-function-environment cf env)
    cf))

;; ---- Blexref: read a captured cell --------------------------------------

;; env vector is one captured cell (a cons); lexref 0 pushes its car.
(let* ((cell (cons 42 'unused))
       (env (vector cell))
       (cf (lexbind-p23-make-closure
	    (concat lexbind-p23-lexref "\000" lexbind-p23-return) env)))
  (Assert (eq (funcall cf) 42))
  ;; If the cell changes, the same bytecode reads the new value: it is
  ;; a genuine shared-cell read, not a copied constant.
  (setcar cell 43)
  (Assert (eq (funcall cf) 43)))

;; Two cells, second index.
(let* ((cells (list (cons 'a 'tag-a) (cons 'b 'tag-b)))
       (env (apply 'vector cells))
       (cf (lexbind-p23-make-closure
	    (concat lexbind-p23-lexref "\001" lexbind-p23-return) env)))
  (Assert (eq (funcall cf) 'b)))

;; ---- Blexset: write a captured cell -------------------------------------

;; constant 99, lexset 0 (pops into the cell), then lexref 0 to return it.
(let* ((cell (cons 'old 'tag))
       (env (vector cell))
       (cf (lexbind-p23-make-closure
	    (concat "\300" lexbind-p23-lexset "\000"
		    lexbind-p23-lexref "\000" lexbind-p23-return)
	    env)))
  ;; constants vector: index 0 is the literal 99.
  (setq cf (make-byte-code '() (compiled-function-instructions cf)
			   (vector 99) 2))
  (set-compiled-function-environment cf env)
  (Assert (eq (funcall cf) 99))
  ;; The write went through the shared cell.
  (Assert (eq (car cell) 99)))

;; Two different closures over the SAME cell see each other's writes
;; (the essence of mutable lexical capture).
(let* ((cell (cons 0 'tag))
       (env (vector cell))
       (writer (lexbind-p23-make-closure
		(concat "\301" lexbind-p23-lexset "\000" lexbind-p23-return)
		env))
       (reader (lexbind-p23-make-closure
		(concat lexbind-p23-lexref "\000" lexbind-p23-return) env)))
  ;; writer's constants: (vector nil 7)
  (setq writer (make-byte-code '() (compiled-function-instructions writer)
			       (vector nil 7) 2))
  (set-compiled-function-environment writer env)
  (funcall writer)
  (Assert (eq (funcall reader) 7))
  (Assert (eq (car cell) 7)))

;; ---- GC: the env and captured cells survive collection ------------------

(let ((cf nil))
  (let ((env (vector (cons 42 'tag))))
    (setq cf (lexbind-p23-make-closure
	      (concat lexbind-p23-lexref "\000" lexbind-p23-return) env)))
  (gc-outer)
  (Assert (eq (funcall cf) 42)))

;; ---- optimize_byte_code: lexref/lexset survive optimization -------------
;; #136 finding 4: the optimizer must consume the one-byte operand of
;; each lexref/lexset, or the instruction stream mis-aligns and later
;; opcodes decode wrong.  Hand-emit a stream where mis-alignment would
;; be visible (a lexref operand byte that looks like an opcode), then
;; optimize explicitly and execute.

(let* ((cell (cons 42 'tag))
       (env (vector cell))
       ;; lexref 0, lexset 1, lexref 1, return -- three operands in a row;
       ;; any operand-size slip moves `return' or an index byte.
       (instructions (concat lexbind-p23-lexref "\000"
			     lexbind-p23-lexset "\001"
			     lexbind-p23-lexref "\001"
			     lexbind-p23-return))
       (cf (make-byte-code '() instructions [] 4)))
  (set-compiled-function-environment cf (vector cell (cons 99 'tag2)))
  ;; Force the optimizer over the hand-emitted stream.
  (optimize-compiled-function cf)
  (Assert (eq (funcall cf) 99))
  (Assert (eq (car cell) 42))
  (Assert (eq (car (aref (compiled-function-environment cf) 1)) 99)))

;; ---- dynamic byte-code (no environment): lexref must signal -------------

;; The `byte-code' subr has no environment (Qnil); lexref there is a
;; malformed stream and must signal invalid-byte-code, not crash.  The
;; optimizer has already consumed the operand correctly if the error
;; comes from the lexref dispatch (not from a mis-aligned opcode).
(let ((err (condition-case e
	       (byte-code (concat lexbind-p23-lexref "\000"
				  lexbind-p23-return) [] 1)
	     (invalid-byte-code (list 'ok (cdr e)))
	     (error (list 'wrong-error (car e))))))
  (Assert (and (consp err) (eq (car err) 'ok))))

;; ---- load from .elc: #[...] carrying lexref bytecode --------------------
;; The compiler's emission side is #157, so simulate what it will
;; write: a .elc file whose defalias is a #[...] compiled function with
;; lexref instructions, loaded back, env attached, then executed.

(let ((file (make-temp-name
	     (expand-file-name "lexbind-p23" temporary-file-directory))))
  (with-temp-buffer
    (insert ";;; lexbind P2-3 hand-emitted test file\n"
	    "(defalias 'lexbind-p23-elc-closure\n"
	    "  #[(n) \"\\300\\331\\000\\330\\000\\207\" [n cell] 2])\n")
    (write-region (point-min) (point-max) file nil 'quiet))
  (unwind-protect
      (progn
	(load file nil nil 'quiet)
	(Assert (compiled-function-p (symbol-function 'lexbind-p23-elc-closure)))
	(let ((cell (cons 0 'tag)))
	  (set-compiled-function-environment
	   (symbol-function 'lexbind-p23-elc-closure) (vector cell))
	  (Assert (eq (funcall 'lexbind-p23-elc-closure 5) 5))
	  (Assert (eq (car cell) 5))))
    (delete-file file)))

;; ---- disassembler prints the new opcodes --------------------------------

(require 'disass)
(let* ((env (vector (cons 42 'tag)))
       (cf (lexbind-p23-make-closure
	    (concat lexbind-p23-lexref "\000"
		    lexbind-p23-lexset "\000"
		    lexbind-p23-return)
	    env))
       (buffer (get-buffer-create " *lexbind-p23-disassemble*")))
  (unwind-protect
      (progn
	(with-current-buffer buffer (erase-buffer))
	(disassemble cf buffer)
	(with-current-buffer buffer
	  (goto-char (point-min))
	  ;; disass.el strips the "byte-" prefix when printing opnames.
	  (Assert (search-forward "lexref" nil t))
	  (goto-char (point-min))
	  (Assert (search-forward "lexset" nil t))))
    (kill-buffer buffer)))

;;; end of lexbind-ops-tests.el
