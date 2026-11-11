;;; lexbind-dump-demo.el --- live interpreted closure in the dumped image -*- lexical-binding: t; -*-

;; Lexical-binding core (P1-4, ticket #150): loaded from loadup.el at dump
;; time (see the lexbind-dump-demo hook there).  Its defuns become
;; interpreted closures over the definition-time environment, so a dumped
;; and reloaded image exercises the closure tag in make-docfile's doc
;; snarfer, `documentation', `functionp'/`function-argcount' dispatch, and
;; the printer.

(defun lexbind-demo-adder (n)
  "Return a closure adding N to its argument."
  #'(lambda (x) (+ x n)))

(defun lexbind-demo-add-15 (x)
  "Add 15 to X via a dumped interpreted closure."
  (funcall (lexbind-demo-adder 15) x))

(provide 'lexbind-dump-demo)

;; Lexical-binding core (P2-2, ticket #158): hand-build a compiled
;; function carrying a non-nil `environment' slot and park it in a
;; defvar, so the dumped image exercises the new
;; { XD_LISP_OBJECT, offsetof (environment) } row in
;; compiled_function_description on both the dump and the reload side.
;; tests/automated/lexbind-cfenv-tests.el asserts the env survives.
(defvar lexbind-demo-cf-env nil)
(let ((cf (make-byte-code '(x) "\x87" [1 2 3] 1)))
  (set-compiled-function-environment cf (list t '(lexical-var . 42)))
  (setq lexbind-demo-cf-env cf))

;;; lexbind-dump-demo.el ends here
