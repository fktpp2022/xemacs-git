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

;;; lexbind-dump-demo.el ends here
