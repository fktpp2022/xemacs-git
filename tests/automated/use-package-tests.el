;;; use-package-tests.el --- tests for the vendored use-package on XEmacs

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;;; Commentary:

;; Two layers:
;;
;;  1. In-process: the compatibility surface and macro expansion, `:init'
;;     execution, `:bind' key installation, the `:ensure' allowlist hard
;;     error, the pin refusal, and the `:ensure nil' no-op.
;;
;;  2. A fresh child XEmacs (spec section 13.1): a real init file with
;;     `(use-package queue :ensure t :defer t :commands make-queue :config
;;     ...)' installs from the milestone-2 offline fixture; the autoload is
;;     live after startup while the body stays deferred; first use loads the
;;     body and runs `:config'; and a second startup re-installs nothing.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(require 'use-package)

;;; Helpers

(defun use-package-tests--directory ()
  "Return the directory containing this test file."
  (file-name-directory
   (or test-harness-current-file load-file-name buffer-file-name)))

(defun use-package-tests-fixture ()
  "Return the absolute path of the milestone-2 GNU fixture archive."
  (file-name-as-directory
   (expand-file-name "../elpa/melpa-m2-fixture/gnu/"
                     (use-package-tests--directory))))

(defun use-package-tests-delete-tree (dir)
  "Delete DIR and its contents, including symlinks, without following links."
  (when (file-directory-p dir)
    (dolist (file (directory-files dir t))
      (unless (member (file-name-nondirectory file) '("." ".."))
        (if (and (file-directory-p file)
                 (not (file-symlink-p file)))
            (use-package-tests-delete-tree file)
          (delete-file file))))
    (delete-directory dir)))

(defmacro use-package-tests-with-temp-home (&rest body)
  "Run BODY with HOME and a fresh `elpa-user-dir' inside it."
  `(let* ((home (file-name-as-directory (make-temp-file "use-package-home-" t)))
          (elpa-user-dir (expand-file-name ".emacs.d/elpa/" home)))
     (unwind-protect
         (progn ,@body)
       (ignore-errors (use-package-tests-delete-tree home)))))

(defun use-package-tests-child-environment (home)
  "Return `process-environment' with HOME replaced by HOME."
  (cons (concat "HOME=" home)
        (let (result)
          (dolist (entry process-environment)
            (unless (string-match "\\`\\(HOME\\|XEMACSDEBUG\\)=" entry)
              (push entry result)))
          (nreverse result))))

(defun use-package-tests-run-child (home &rest args)
  "Run a fresh XEmacs with HOME set to HOME and ARGS.
Return the child's standard output without trailing newlines."
  (let ((buffer (get-buffer-create " *use-package-child-output*"))
        (program (expand-file-name invocation-name invocation-directory))
        (process-environment (use-package-tests-child-environment home)))
    (with-current-buffer buffer
      (erase-buffer))
    (let ((status (apply 'call-process program nil (list buffer nil) nil
                         (append '("-batch" "-no-site-file") args))))
      (unless (and (integerp status) (zerop status))
        (error "Child XEmacs failed with status %S: %s" status
               (with-current-buffer buffer (buffer-string)))))
    (let ((output (with-current-buffer buffer (buffer-string))))
      (if (string-match "[\n\r]+\\'" output)
          (substring output 0 (match-beginning 0))
        output))))

;;; Compatibility surface

(Assert (fboundp 'use-package) "use-package is available")
(Assert (fboundp 'bind-key) "bind-key is available")
(Assert (featurep 'use-package) "requiring use-package provides the feature")

(Assert (boundp 'user-emacs-directory)
        "user-emacs-directory is bound for use-package-core")
(Assert (boundp 'emulation-mode-map-alists)
        "emulation-mode-map-alists is bound for bind-key")
(Assert (fboundp 'enable-theme) "enable-theme is available for :custom")
(Assert (boundp 'custom-enabled-themes)
        "custom-enabled-themes is bound for the theme path")
(Assert (fboundp 'macroexp-progn) "macroexp-progn is available")
(Assert (fboundp 'cl-flet) "cl-flet is available")
(Assert (fboundp 'hash-table-keys) "hash-table-keys is available via subr-x")

;;; Macro expansion

(let ((expansion (macroexpand '(use-package foo :ensure t :defer t
                                 :commands foo-command))))
  (Assert (consp expansion) "a use-package form expands to a form")
  (Assert (string-match "use-package-ensure-elpa"
                        (prin1-to-string expansion))
          "the :ensure expansion routes through the backend")
  (Assert (string-match "autoload" (prin1-to-string expansion))
          "the :commands expansion emits an autoload"))

;;; :init runs at declaration time

(let ((use-package-tests-init-ran nil))
  (use-package use-package-tests-init-marker
    :init (setq use-package-tests-init-ran 'ran)
    :defer t)
  (Assert (eq use-package-tests-init-ran 'ran)
          ":init runs when the declaration is evaluated")
  (Assert (not (featurep 'use-package-tests-init-marker))
          ":defer holds the package body"))

;;; :bind installs the key and autoloads the command

(use-package use-package-tests-bind-marker
  :defer t
  :bind ("C-c C-y y" . use-package-tests-bind-command))

(Assert (fboundp 'use-package-tests-bind-command)
        ":bind autoloads the bound command")
(Assert (let ((definition (symbol-function 'use-package-tests-bind-command)))
          (and (consp definition) (eq (car definition) 'autoload)))
        "the bound command is an autoload, not a loaded function")
(Assert (let ((binding (lookup-key global-map (kbd "C-c C-y y"))))
          (and binding (not (eq binding 'undefined))))
        ":bind installs a binding in the global map")

;;; :ensure nil is a no-op

(use-package use-package-tests-ensure-nil
  :ensure nil
  :defer t)
(Assert (not (elpa-installed-p 'use-package-tests-ensure-nil))
        ":ensure nil installs nothing")

;;; :ensure allowlist hard error (ADR 0001)

(Assert (condition-case err
            (progn (use-package use-package-tests-not-allowlisted
                     :ensure t
                     :defer t)
                   nil)
          (error (string-match "elpa-installable-packages"
                               (error-message-string err))))
        ":ensure on a name outside the allowlist errors naming the allowlist")

;;; :ensure pinning is refused (ADR 0001)

(Assert (condition-case err
            (progn (use-package queue
                     :ensure (queue :pin "gnu")
                     :defer t)
                   nil)
          (error (string-match "ADR 0001" (error-message-string err))))
        ":ensure with an archive pin errors pointing at ADR 0001")

;;; :ensure success from the offline fixture (in process)

(use-package-tests-with-temp-home
  (let ((elpa-archives `(("gnu" . ,(use-package-tests-fixture)))))
    (use-package queue
      :ensure t
      :defer t
      :commands make-queue)
    (Assert (elpa-installed-p 'queue) ":ensure installs queue from the fixture")
    (Assert (fboundp 'make-queue) ":ensure leaves the command autoloaded")))

;;; :ensure warns and continues when an allowlisted package is absent

(use-package-tests-with-temp-home
  (let ((elpa-archives `(("gnu" . ,(use-package-tests-fixture)))))
    (let ((use-package-tests-init-ran nil))
      (use-package loc-changes
        :ensure t
        :init (setq use-package-tests-init-ran 'ran)
        :defer t)
      (Assert (eq use-package-tests-init-ran 'ran)
              ":ensure warns and continues when the package is absent; :init ran")
      (Assert (not (elpa-installed-p 'loc-changes))
              "the absent package was not installed"))))

;;; Offline fixture startup, a real init file (spec 13.1)

(defconst use-package-tests-init-check
  (concat
   "(progn"
   " (princ (format \"before installed=%s fbound=%s feature=%s config=%s\""
   " (elpa-installed-p 'queue) (fboundp 'make-queue)"
   " (featurep 'queue) (and (boundp 'use-package-test-config-ran)"
   " use-package-test-config-ran)))"
   " (make-queue)"
   " (princ (format \" after-feature=%s config=%s\""
   " (featurep 'queue) use-package-test-config-ran)))")
  "Command-line form checking spec 13.1 assertions in a fresh startup.")

(defun use-package-tests-write-init (file fixture)
  "Write the spec 13.1 init file to FILE, installing from FIXTURE."
  (with-temp-file file
    (insert
     (format "(setq elpa-archives '((\"gnu\" . %S)))\n" fixture)
     "(defvar use-package-test-config-ran nil)\n"
     "(use-package queue\n"
     "  :ensure t\n"
     "  :defer t\n"
     "  :commands make-queue\n"
     "  :config (setq use-package-test-config-ran t))\n")))

(use-package-tests-with-temp-home
  (let* ((init (expand-file-name "init.el" home))
         (fixture (use-package-tests-fixture))
         (content-dir (expand-file-name "queue-0.2/" elpa-user-dir)))
    (use-package-tests-write-init init fixture)

    ;; First startup: :ensure installs, the autoload is live, the body
    ;; stays deferred, and first use loads the body and runs :config.
    (let ((first (use-package-tests-run-child
                  home "-l" init "-eval" use-package-tests-init-check)))
      (Assert (string-match
               "before installed=t fbound=t feature=nil config=nil" first)
              (format "13.1: deferred autoload after init: %S" first))
      (Assert (string-match "after-feature=t config=t" first)
              (format "13.1: first use loads the body and runs :config: %S"
                      first)))
    (Assert (file-directory-p content-dir)
            "13.1#1: :ensure installed queue into elpa-user-dir")

    ;; Second startup with the same elpa-user-dir: :ensure short-circuits on
    ;; elpa-installed-p, so the install tree is not touched.
    (let ((mtime (nth 5 (file-attributes content-dir))))
      (let ((second (use-package-tests-run-child
                     home "-l" init "-eval" use-package-tests-init-check)))
        (Assert (string-match
                 "before installed=t fbound=t feature=nil config=nil" second)
                (format "13.1: second startup keeps the autoload: %S" second)))
      (Assert (equal mtime (nth 5 (file-attributes content-dir)))
              "13.1#4: a second startup does not re-install"))))

;;; use-package-tests.el ends here
