;;; elpa-live-tests.el --- live network tests for the ELPA client

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;;; Commentary:

;; Live, network-backed tests for lisp/elpa.el against the real GNU ELPA
;; archive at https://elpa.gnu.org/packages/.  They are gated on the
;; ELPA_LIVE environment variable: without it the file self-skips in
;; seconds and ctest labels it `network'.  Nothing here may pin package
;; versions; the installed directory must match the version the live
;; index advertises.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(require 'elpa)

(if (null (getenv "ELPA_LIVE"))
    (message "Skipping elpa-live-tests: ELPA_LIVE not set")

  (unless (fboundp 'async-http-request-internal)
    (error "ELPA_LIVE is set but the async HTTP transport is not available"))
  (require 'async-core)
  (require 'async-http)

  (defun elpa-live-tests-delete-tree (dir)
    "Delete DIR and its contents, including symlinks, without following links."
    (when (file-directory-p dir)
      (dolist (file (directory-files dir t))
        (unless (member (file-name-nondirectory file) '("." ".."))
          (if (and (file-directory-p file)
                   (not (file-symlink-p file)))
              (elpa-live-tests-delete-tree file)
            (delete-file file))))
      (delete-directory dir)))

  (defun elpa-live-tests-run-child (user-dir)
    "Run a fresh XEmacs with USER-DIR, activate packages, and exercise queue.
Return the child's standard output without a trailing newline."
    (let ((buffer (get-buffer-create " *elpa-live-child-output*"))
          (program (expand-file-name invocation-name invocation-directory)))
      (with-current-buffer buffer
        (erase-buffer))
      (let ((status
             (call-process
              program nil (list buffer nil) nil
              "-batch" "-no-packages"
              "-l" "elpa"
              "-eval" (format "(setq elpa-user-dir %S)" user-dir)
              "-eval" "(elpa-activate-all)"
              "-eval" "(require 'queue)"
              "-eval" (concat
                       "(let ((q (queue-create)))"
                       " (queue-enqueue q 'a)"
                       " (queue-enqueue q 'b)"
                       " (princ (format \"deq1=%s deq2=%s empty=%s len=%s first=%s\""
                       " (queue-dequeue q) (queue-dequeue q)"
                       " (queue-empty q) (queue-length q) (queue-first q))))"))))
        (unless (and (integerp status) (zerop status))
          (error "Child XEmacs failed with status %S" status)))
      (let ((output (with-current-buffer buffer (buffer-string))))
        (if (string-match "[\n\r]+\\'" output)
            (substring output 0 (match-beginning 0))
          output))))

  (let ((elpa-user-dir (file-name-as-directory
                        (make-temp-file "elpa-live-" t)))
        (elpa-archives '(("gnu" . "https://elpa.gnu.org/packages/"))))
    (unwind-protect
        (progn
          ;; Refresh the live version-1 index over HTTPS.
          (let ((count (elpa-refresh-contents)))
            (Assert (and (integerp count) (> count 0))
                    "the live HTTPS refresh returns a package count")
            (Assert (file-readable-p
                     (expand-file-name "archives/gnu/archive-contents"
                                       elpa-user-dir))
                    "the live HTTPS index is cached"))
          (let ((queue (elpa-package-info "queue")))
            (Assert queue "queue is present in the live index")
            (Assert (and (listp (plist-get queue :version))
                         (integerp (car (plist-get queue :version))))
                    "queue's advertised version is a version list")
            (Assert (eq (plist-get queue :kind) 'tar)
                    "queue is a tar package")
            (let* ((version (plist-get queue :version))
                   (dirname (concat "queue-" (elpa-version-join version)))
                   (published (expand-file-name dirname elpa-user-dir)))
              ;; Install queue from the live archive over HTTPS.
              (Assert (eq 'queue (elpa-install "queue"))
                      "elpa-install downloads and installs queue over HTTPS")
              (Assert (file-directory-p published)
                      "the installed directory name matches the advertised version")
              (Assert (elpa-installed-p "queue" version)
                      "elpa-installed-p accepts the advertised version")
              (Assert (file-exists-p
                       (expand-file-name "queue.el" published))
                      "queue.el was installed")
              (Assert (file-exists-p
                       (expand-file-name "queue.elc" published))
                      "the live install byte-compiled queue.el")
              (Assert (not (file-exists-p
                            (expand-file-name ".staging/" elpa-user-dir)))
                      "no staging residue after a live install")
              (Assert (string=
                       (elpa-live-tests-run-child elpa-user-dir)
                       "deq1=a deq2=b empty=t len=0 first=nil")
                      "a fresh XEmacs activates the live-installed queue and runs the functional check")))

          ;; A 404 index is a clear error and caches nothing new.
          (let ((elpa-archives
                 '(("gnu" . "https://elpa.gnu.org/packages/no-such-dir/"))))
            (Check-Error-Message 'error "archive not found"
              (elpa-refresh-contents))
            (Assert (file-readable-p
                     (expand-file-name "archives/gnu/archive-contents"
                                       elpa-user-dir))
                    "a 404 index refresh leaves the good cache in place"))

          ;; A 404 artifact is a clear error and leaves no package
          ;; directory behind.
          (let ((elpa-user-dir (file-name-as-directory
                                (make-temp-file "elpa-live-404-" t)))
                version)
            (unwind-protect
                (progn
                  (let ((elpa-archives
                         '(("gnu" . "https://elpa.gnu.org/packages/"))))
                    (elpa-refresh-contents)
                    (setq version
                          (plist-get (elpa-package-info "queue") :version)))
                  (let ((elpa-archives
                         '(("gnu" . "https://elpa.gnu.org/packages/no-such-dir/"))))
                    (Check-Error-Message 'error "artifact not found"
                      (elpa-install "queue")))
                  (Assert (not (file-directory-p
                                (expand-file-name
                                 (concat "queue-"
                                         (elpa-version-join version))
                                 elpa-user-dir)))
                          "a 404 artifact leaves no package directory")
                  (Assert (not (file-exists-p
                                (expand-file-name ".staging/"
                                                  elpa-user-dir)))
                          "a 404 artifact leaves no staging residue"))
              (ignore-errors (elpa-live-tests-delete-tree elpa-user-dir)))))
      (ignore-errors (elpa-live-tests-delete-tree elpa-user-dir)))))

(provide 'elpa-live-tests)

;;; elpa-live-tests.el ends here
