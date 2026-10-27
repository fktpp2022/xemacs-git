;;; use-package-keywords.el --- throwaway keyword probe for wayfinder #67 -*- mode: emacs-lisp -*-

;; NOT production code.  Prototype for ticket #67 of map #63.
;;
;; Run with an XEmacs 21.5-b36 that has the installed package tree (the
;; in-tree build/bin/xemacs has no cl-lib on its load-path):
;;   /path/to/install/bin/xemacs -q -batch -l prototype/use-package-keywords.el
;;
;; Layout expected next to this file:
;;   xemacs-shim.el                      -- P3 compat shim
;;   vendor/use-package-2.4.6/*.el       -- upstream jwiegley/use-package @ 2f09ce19
;;
;; Exit status is non-zero if any check fails.  The one GAP line is the
;; known `:bind*' limitation tracked separately; it does not fail the run.

(defvar proto-root (file-name-directory (or load-file-name buffer-file-name)))
(add-to-list 'load-path (expand-file-name "vendor/use-package-2.4.6" proto-root))
(add-to-list 'load-path proto-root)
(require 'xemacs-shim)
(require 'use-package)
(require 'use-package-bind-key)
(require 'bind-key)

(defvar proto-dir (expand-file-name "proto67-scratch" (or (getenv "TMPDIR") "/tmp")))
(unless (file-directory-p proto-dir) (make-directory proto-dir))
(add-to-list 'load-path proto-dir)

(defvar proto-fail 0)
(defvar proto-n 0)
(defun proto-check (label expected actual)
  (setq proto-n (1+ proto-n))
  (let ((ok (equal expected actual)))
    (unless ok (setq proto-fail (1+ proto-fail)))
    (princ (format "%-42s %s\n    expected: %S\n    actual:   %S\n"
                   label (if ok "PASS" "FAIL") expected actual))))

(defun proto-gap (label expected actual note)
  "Record a known XEmacs limitation: the keyword works only partially."
  (princ (format "%-42s GAP\n    expected: %S\n    actual:   %S\n    note:     %s\n"
                 label expected actual note)))

(defun proto-pkg (name &rest forms)
  "Write a fake package NAME whose body runs FORMS when loaded."
  (with-temp-file (expand-file-name (concat (symbol-name name) ".el") proto-dir)
    (insert ";;; fake package for the #67 probe\n")
    (insert (format "(setq %s-loaded t)\n" name))
    (dolist (f forms) (insert (prin1-to-string f) "\n"))
    (insert (format "(provide '%s)\n" name))))

(defun proto-autoload-p (symbol)
  (eq 'autoload (car-safe (symbol-function symbol))))

(princ (format "XEmacs %s / use-package %s\n\n"
               emacs-version (or (ignore-errors use-package-version) "2.4.6")))

;;; :init -- runs at declaration, before the package loads
(proto-pkg 'pf-init)
(defvar proto-init-ran nil)
(use-package pf-init :defer t :init (setq proto-init-ran t))
(proto-check ":init runs at declaration" t proto-init-ran)
(proto-check ":init does not load package" nil (featurep 'pf-init))

;;; :defer / :config -- config held until the package loads
(proto-pkg 'pf-defer)
(defvar proto-defer-config nil)
(use-package pf-defer :defer t :config (setq proto-defer-config t))
(proto-check ":defer t does not load package" nil (featurep 'pf-defer))
(proto-check ":defer holds :config" nil proto-defer-config)
(require 'pf-defer)
(proto-check ":config runs on load" t proto-defer-config)

;;; :config without deferral -- package loaded, then configured
(proto-pkg 'pf-eager)
(defvar proto-eager-config nil)
(use-package pf-eager :config (setq proto-eager-config t))
(proto-check "no :defer loads package" t (featurep 'pf-eager))
(proto-check "no :defer runs :config" t proto-eager-config)

;;; :commands -- autoload, deferred
(proto-pkg 'pf-cmd)
(use-package pf-cmd :commands pf-cmd-do)
(proto-check ":commands autoloads command" t (and (fboundp 'pf-cmd-do)
                                                  (proto-autoload-p 'pf-cmd-do)))
(proto-check ":commands does not load package" nil (featurep 'pf-cmd))

;;; :hook -- add hook, autoload the hook function, deferred
(proto-pkg 'pf-hook '(defun pf-hook-mode () (interactive)))
(use-package pf-hook :hook (pf-hook-mode . proto-hook-fn))
(proto-check ":hook registers function" t
             (and (memq 'proto-hook-fn pf-hook-mode-hook) t))
(proto-check ":hook autoloads hook function" t
             (and (fboundp 'proto-hook-fn) (proto-autoload-p 'proto-hook-fn)))
(proto-check ":hook does not load package" nil (featurep 'pf-hook))

;;; :mode -- auto-mode-alist entry + autoload, deferred
(proto-pkg 'pf-mode '(defun pf-mode () (interactive)))
(use-package pf-mode :mode ("\\.pf\\'" . pf-mode))
(proto-check ":mode adds auto-mode-alist entry" t
             (equal (assoc "\\.pf\\'" auto-mode-alist) (cons "\\.pf\\'" 'pf-mode)))
(proto-check ":mode autoloads mode" t (and (fboundp 'pf-mode)
                                           (proto-autoload-p 'pf-mode)))
(proto-check ":mode does not load package" nil (featurep 'pf-mode))

;;; :bind -- global binding + autoload, deferred
(proto-pkg 'pf-bind)
(use-package pf-bind :bind ("C-c q" . pf-bind-cmd))
(proto-check ":bind binds global key" 'pf-bind-cmd (key-binding (kbd "C-c q")))
(proto-check ":bind autoloads command" t (and (fboundp 'pf-bind-cmd)
                                              (proto-autoload-p 'pf-bind-cmd)))
(proto-check ":bind does not load package" nil (featurep 'pf-bind))

;;; :bind* -- override map exists, but XEmacs has no lookup support
(proto-pkg 'pf-bindstar)
(override-global-mode 1)
(use-package pf-bindstar :bind* ("C-c w" . pf-bindstar-cmd))
(proto-check ":bind* binds override map" 'pf-bindstar-cmd
             (and (boundp 'override-global-map)
                  (lookup-key override-global-map (kbd "C-c w"))))
(proto-gap ":bind* reaches key lookup" 'pf-bindstar-cmd
           (key-binding (kbd "C-c w"))
           "XEmacs has no `emulation-mode-map-alists'; the map is never consulted")
(proto-check ":bind* does not load package" nil (featurep 'pf-bindstar))

;;; :after -- deferred until the named feature loads
(proto-pkg 'pf-after)
(proto-pkg 'pf-trigger)
(defvar proto-after-ran nil)
(use-package pf-after :after pf-trigger :config (setq proto-after-ran t))
(proto-check ":after holds config" nil proto-after-ran)
(proto-check ":after holds load" nil (featurep 'pf-after))
(require 'pf-trigger)
(proto-check ":after releases on trigger load" t (featurep 'pf-after))
(proto-check ":after runs :config" t proto-after-ran)

;;; :if / :when / :unless -- expansion is conditional
(proto-pkg 'pf-if)
(defvar proto-if-init nil)
(defvar proto-if-config nil)
(use-package pf-if :if nil
  :init (setq proto-if-init t) :config (setq proto-if-config t))
(proto-check ":if nil skips :init" nil proto-if-init)
(proto-check ":if nil skips load" nil (featurep 'pf-if))

(proto-pkg 'pf-when)
(defvar proto-when-ran nil)
(use-package pf-when :when t :config (setq proto-when-ran t))
(proto-check ":when t loads and configures" t (and (featurep 'pf-when)
                                                   proto-when-ran))

(proto-pkg 'pf-unless)
(defvar proto-unless-ran nil)
(use-package pf-unless :unless nil :config (setq proto-unless-ran t))
(proto-check ":unless nil loads and configures" t (and (featurep 'pf-unless)
                                                       proto-unless-ran))

;;; :custom -- applied at declaration to an already-bound variable
(defvar pf-custom-var 'default)
(proto-pkg 'pf-custom)
(use-package pf-custom :defer t :custom (pf-custom-var "customized"))
(proto-check ":custom sets bound var (theme path)" "customized" pf-custom-var)

(setq pf-custom-var 'default)
(setq use-package-use-theme nil)
(proto-pkg 'pf-custom2)
(use-package pf-custom2 :defer t :custom (pf-custom-var "customized-2"))
(proto-check ":custom sets bound var (plain path)" "customized-2" pf-custom-var)

;;; :custom -- the realistic case: the variable is only defined by the package
(setq use-package-use-theme t)
(proto-pkg 'pf-custom-unbound '(defvar pf-custom-unbound-var 'default))
(use-package pf-custom-unbound :defer t
  :custom (pf-custom-unbound-var "themed"))
(require 'pf-custom-unbound)
(proto-check ":custom reaches unbound var (theme path)" "themed"
             pf-custom-unbound-var)

(setq use-package-use-theme nil)
(proto-pkg 'pf-custom-unbound2 '(defvar pf-custom-unbound2-var 'default))
(use-package pf-custom-unbound2 :defer t
  :custom (pf-custom-unbound2-var "plain"))
(require 'pf-custom-unbound2)
(proto-check ":custom reaches unbound var (plain path)" "plain"
             pf-custom-unbound2-var)

;;; summary
(princ (format "\n%d checks, %d failed\n" proto-n proto-fail))
(kill-emacs (if (> proto-fail 0) 1 0))
