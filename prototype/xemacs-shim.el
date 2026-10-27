;; Minimal XEmacs compat shim to test use-package P3 viability.
;; Provides the symbols use-package-core.el / bind-key.el reference
;; that XEmacs 21.5-b36 lacks. NOT for production — just a probe.

;; --- subr-x shim (only symbols actually used: hash-table-keys, string-suffix-p) ---
(unless (fboundp 'hash-table-keys)
  (defun hash-table-keys (hash-table)
    "Return a list of keys in HASH-TABLE."
    (let (keys)
      (maphash (lambda (k _v) (push k keys)) hash-table)
      (nreverse keys))))
(unless (fboundp 'string-suffix-p)
  (defun string-suffix-p (suffix string &optional ignore-case)
    (let ((start-pos (- (length string) (length suffix))))
      (and (>= start-pos 0)
           (eq t (compare-strings suffix nil nil
                                  string start-pos nil ignore-case))))))
(provide 'subr-x)

;;; subr-x.el ends here

;; --- macroexp shim (XEmacs 21.5 has NO macroexp feature) ---
(unless (fboundp 'macroexp-progn)
  (defun macroexp-progn (exps)
    "Return a form that evaluates EXPS sequentially."
    (if (cdr exps) (cons 'progn exps) (car exps))))
(unless (fboundp 'macroexp-unquote)
  (defun macroexp-unquote (exp) exp))

;; --- eval-after-load accepts a feature symbol (GNU) ---
;; XEmacs matches the literal library name passed to `load'; use-package
;; emits `(eval-after-load 'feature ...)`, which XEmacs ignores.
(unless (get 'eval-after-load 'proto-symbol-compat)
  (defvar proto-orig-eval-after-load (symbol-function 'eval-after-load))
  (fset 'eval-after-load
        (lambda (file form)
          (if (and (symbolp file) (featurep file))
              (eval form)
            (funcall proto-orig-eval-after-load
                     (if (symbolp file) (symbol-name file) file)
                     form))))
  (put 'eval-after-load 'proto-symbol-compat t))

;; --- display-warning accepts GNU's keyword levels ---
;; XEmacs rejects :error/:warning/:debug; use-package's error handler
;; passes :error, so any failing :config would crash instead of warn.
(unless (get 'display-warning 'proto-level-compat)
  (defvar proto-orig-display-warning (symbol-function 'display-warning))
  (fset 'display-warning
        (lambda (type message &optional level)
          (funcall proto-orig-display-warning type message
                   (if (memq level '(:debug :warning :error))
                       (if (eq level :debug) 'debug 'warning)
                     level))))
  (put 'display-warning 'proto-level-compat t))

;; --- theme stubs ---
;; XEmacs has deftheme/custom-theme-set-variables but no enable-theme.
;; Marking the theme immediate makes recorded :custom values apply now,
;; so they survive the package's later defcustom.
(unless (fboundp 'enable-theme)
  (defun enable-theme (theme)
    (put theme 'theme-immediate t)
    theme))
(unless (boundp 'custom-enabled-themes)
  (defvar custom-enabled-themes nil))

;; --- tabulated-list stub (display-only feature; statistics mode will be inert) ---
(unless (featurep 'tabulated-list)
  (defvar tabulated-list-format nil)
  (defvar tabulated-list-entries nil)
  (defvar tabulated-list-sort-key nil)
  (defun tabulated-list-init-header () nil)
  (defun tabulated-list-print (&optional _update) nil)
  (define-derived-mode tabulated-list-mode nil "Tabulated"
    "Stub `tabulated-list-mode' for XEmacs."
    (kill-all-local-variables))
  (provide 'tabulated-list))

;; --- cl-flet (cl-lib 0.7.1 lacks it; use dynamic flet as a stand-in) ---
(unless (fboundp 'cl-flet)
  (defmacro cl-flet (bindings &rest body)
    "(Approximate) `cl-flet' implemented on top of dynamic `flet'."
    `(flet ,bindings ,@body)))

;; --- font-lock / user-emacs-directory stubs (runtime enhancement, not core) ---
(unless (fboundp 'font-lock-add-keywords)
  (defun font-lock-add-keywords (feature keywords) (list feature keywords)))
(unless (boundp 'user-emacs-directory)
  (defvar user-emacs-directory (expand-file-name "~/.xemacs/")
    "XEmacs equivalent of `user-emacs-directory'."))
(unless (boundp 'emulation-mode-map-alists)
  (defvar emulation-mode-map-alists nil))
(provide 'xemacs-shim)
