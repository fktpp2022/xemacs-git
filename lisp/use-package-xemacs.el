;;; use-package-xemacs.el --- XEmacs glue for the vendored use-package -*- lexical-binding: t -*-

;; Copyright (C) 2026  Free Software Foundation, Inc.
;; See the file `xemacs' for copyright and disclaimer details.

;; This file is part of XEmacs.
;;
;; Everything here exists only to make the vendored, otherwise-unmodified
;; upstream `use-package' and `bind-key' load and behave on XEmacs.  Generic
;; GNU-Emacs compatibility (macroexp, subr-x, cl-flet, enable-theme,
;; emulation-mode-map-alists, user-emacs-directory, keyword levels for
;; display-warning, symbol FILE for eval-after-load) lives in the core files
;; proper; this file holds only what is use-package-specific.

;;; Code:

(require 'macroexp)
(require 'subr-x)

;; `declare-function' is a byte-compiler hint in GNU Emacs (a no-op at
;; runtime).  XEmacs has no such macro; the vendored files call it at load
;; time and emit it in macro expansions, so provide the no-op.
(unless (fboundp 'declare-function)
  (defmacro declare-function (fn file &optional arglist &rest _args)
    "No-op stand-in for GNU's `declare-function' compiler hint."
    (declare (debug t))
    nil))

;; `condition-case-unless-debug' is GNU's `condition-case' that also reports
;; the error when `debug-on-error' is set.  XEmacs lacks it; the plain
;; `condition-case' expansion is the compatibility behaviour we want.
(unless (fboundp 'condition-case-unless-debug)
  (defmacro condition-case-unless-debug (err &rest body)
    "Like `condition-case' for ERR and BODY, without the debug-time report."
    `(condition-case ,err ,@body)))

;; `:bind*' installs into `override-global-map', reached only through
;; `emulation-mode-map-alists', which XEmacs lacks (see keymap.el).  Degrade
;; the star forms to the non-star ones so the binding is reachable instead of
;; silently dead.  Done after `bind-key' loads so the aliases resolve to real
;; definitions.
(eval-after-load 'bind-key
  '(progn
     (defalias 'bind-key*  'bind-key)
     (defalias 'bind-keys* 'bind-keys)))

;; `:ensure' routes through `use-package-ensure-function', whose default
;; `use-package-ensure-elpa' (upstream) does `(require 'package)', which XEmacs
;; lacks.  Replace it with an `elpa.el' backend (ADR 0001: allowlist-only
;; trust).  Names outside `elpa-installable-packages' are refused; install
;; failures warn and continue.
(defun use-package-ensure-elpa-xemacs (name args _state)
  "Ensure NAME is installed via `elpa.el'.
ARGS is the normalized `:ensure' argument list (elements t, a package
symbol, or (PACKAGE . ARCHIVE)); STATE is ignored.  Refuses names outside
`elpa-installable-packages' (ADR 0001) and names with a :pin archive."
  (require 'elpa)
  (dolist (spec args)
    (let ((pkg (cond
                ((eq spec t) (if (symbolp name) name (intern name)))
                ((symbolp spec) spec)
                ((consp spec)
                 (when (and (stringp (cdr spec)))
                   (error "use-package :ensure pinning is not supported (ADR 0001): %S"
                          spec))
                 (car spec))
                (t (error "use-package :ensure: unsupported spec %S" spec)))))
      (when pkg
        (unless (memq pkg elpa-installable-packages)
          (error "Package `%s' is not in `elpa-installable-packages'; add it there to trust it (ADR 0001)"
                 pkg))
        (unless (elpa-installed-p pkg)
          (condition-case err
              (progn (elpa-install pkg) (elpa-activate-all))
            (error
             (display-warning 'use-package
                              (format "Failed to install `%s': %s"
                                      pkg (error-message-string err))
                              :warning))))))))

;; Wire `:ensure' through the backend.  Upstream's `use-package-ensure.el'
;; will later `defcustom' `use-package-ensure-function' with a default of
;; `use-package-ensure-elpa'; setting the default here first means the
;; defcustom (which honours an already-bound default) keeps this value, and
;; the `eval-after-load' alias below repoints that symbol at our backend
;; once upstream has had its chance to define (and so shadow) the name.
(setq-default use-package-ensure-function 'use-package-ensure-elpa)

(eval-after-load 'use-package-ensure
  '(defalias 'use-package-ensure-elpa 'use-package-ensure-elpa-xemacs))

(provide 'use-package-xemacs)

;;; use-package-xemacs.el ends here