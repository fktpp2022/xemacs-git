;;; elpa-live-tests.el --- live network tests for the ELPA client

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;;; Commentary:

;; Live, network-backed tests for lisp/elpa.el against the real GNU ELPA
;; archive at https://elpa.gnu.org/packages/ and the real MELPA archive at
;; https://melpa.org/packages/.  They are gated on the ELPA_LIVE environment
;; variable: without it the file self-skips in seconds and ctest labels it
;; `network'.  Nothing here may pin package versions; the installed directory
;; must match the version the live index advertises.  MELPA's full index is
;; also the full-scale regression for the large in-memory body fix.

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
    (progn
      (message "Skipping elpa-live-tests: ELPA_LIVE not set")
      ;; CTest SKIP_RETURN_CODE: report this run as skipped, not passed.
      (kill-emacs 125))

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

  (defun elpa-live-tests-last-line (output)
    "Return the final non-empty line of OUTPUT."
    (car (last (split-string output "[\n\r]+" t))))

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

  (defun elpa-live-tests-run-package-form (user-dir package form)
    "Exercise FORM after requiring PACKAGE in a fresh XEmacs with USER-DIR.
The child loads elpa, activates USER-DIR, requires PACKAGE, and prints
FORM's value.  Return the final non-empty output line."
    (let ((buffer (get-buffer-create " *elpa-live-form-output*"))
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
              "-eval" (format "(progn (require '%s) (princ (format \"%%S\" %s)))"
                              package form))))
        (unless (and (integerp status) (zerop status))
          (error "Child XEmacs failed with status %S: %s"
                 status (with-current-buffer buffer (buffer-string)))))
      (elpa-live-tests-last-line
       (with-current-buffer buffer (buffer-string)))))

  ;; A local HTTP fixture for the artifact-404 case only.  The live index
  ;; refresh itself stays pointed at gnu.org and melpa.org.
  (defconst elpa-live-tests-http-port 18745
    "Port for the live suite's local artifact-404 fixture server.
Must not collide with async-http-tests (18742) or elpa-tests
\(18743 and 18744).")

  (defvar elpa-live-tests-http-server-script
    "
import http.server, sys
PORT = int(sys.argv[1])
ROOT = sys.argv[2]
class H(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        http.server.SimpleHTTPRequestHandler.__init__(self, *a, directory=ROOT, **k)
    def log_message(self, *a): pass
http.server.HTTPServer(('127.0.0.1', PORT), H).serve_forever()
"
    "Python http.server source serving a directory as an ELPA archive.")

  (defun elpa-live-tests-start-http-server (root)
    "Serve ROOT on 127.0.0.1:`elpa-live-tests-http-port'; return the process."
    (let ((proc (start-process "elpa-live-test-server"
                               " *elpa-live-test-server*"
                               "python3" "-c"
                               elpa-live-tests-http-server-script
                               (number-to-string elpa-live-tests-http-port)
                               root)))
      (sleep-for 0.5)
      proc))

  (defun elpa-live-tests-stop-http-server (proc)
    "Stop the fixture server PROC."
    (when (processp proc)
      (delete-process proc)))

  ;; GNU ELPA: refresh the live index, install queue, exercise it, and
  ;; check the 404 index negative.
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
              ;; Install queue from the live archive over HTTPS.  Install
              ;; refreshes the index again, so the fetch above is repeated.
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

          ;; A 404 index is a clear error and caches nothing new.  Install
          ;; refreshes before resolving, so a no-such-dir base now fails at
          ;; the index, not later at the artifact.
          (let ((elpa-archives
                 '(("gnu" . "https://elpa.gnu.org/packages/no-such-dir/"))))
            (Check-Error-Message 'error "archive not found"
              (elpa-refresh-contents))
            (Check-Error-Message 'error "archive not found"
              (elpa-install "queue"))
            (Assert (file-readable-p
                     (expand-file-name "archives/gnu/archive-contents"
                                       elpa-user-dir))
                    "a 404 index leaves the good cache in place")))
      (ignore-errors (elpa-live-tests-delete-tree elpa-user-dir))))

  ;; MELPA: refresh the full rolling index in memory (about 2.6 MB, the
  ;; full-scale regression for the large-body fix), install checkbox
  ;; with no version pinned, and exercise it in a fresh child.
  (let ((elpa-user-dir (file-name-as-directory
                        (make-temp-file "elpa-live-melpa-" t)))
        (elpa-archives '(("melpa" . "https://melpa.org/packages/")))
        (elpa--archive-index-alist nil))
    (unwind-protect
        (progn
          (let ((count (elpa-refresh-contents)))
            (Assert (and (integerp count) (> count 6000))
                    "the live MELPA refresh parses the full index in memory")
            (Assert (file-readable-p
                     (expand-file-name "archives/melpa/archive-contents"
                                       elpa-user-dir))
                    "the full MELPA index is cached"))
          (let* ((info (elpa-package-info "checkbox"))
                 (version (plist-get info :version)))
            (Assert info "checkbox is present in the live MELPA index")
            (Assert (and (listp version) (integerp (car version)))
                    "checkbox's advertised version is a version list")
            (Assert (eq (plist-get info :kind) 'tar)
                    "checkbox is a tar package")
            (let* ((dirname (concat "checkbox-"
                                    (elpa-version-join version)))
                   (dir (expand-file-name dirname elpa-user-dir)))
              ;; Install with no version pinned: install refreshes again
              ;; and resolves whatever the rolling index advertises.
              (Assert (eq 'checkbox (elpa-install "checkbox"))
                      "elpa-install downloads and installs checkbox over HTTPS")
              (Assert (equal (plist-get (elpa-package-info "checkbox")
                                        :version)
                             version)
                      "the install refresh advertised the same version")
              (Assert (file-directory-p dir)
                      "the installed directory name matches the advertised date version")
              (Assert (equal (directory-files elpa-user-dir nil
                                              "\\`checkbox-")
                             (list dirname))
                      "the only content directory is checkbox-<advertised version>")
              (Assert (equal (elpa--find-installed-dir 'checkbox) dir)
                      "elpa--find-installed-dir names the installed build")
              (Assert (elpa-installed-p "checkbox")
                      "elpa-installed-p accepts the advertised version")
              (Assert (file-exists-p
                       (expand-file-name "checkbox.elc" dir))
                      "the live install byte-compiled checkbox.el")
              (Assert (not (file-exists-p
                            (expand-file-name ".staging/" elpa-user-dir)))
                      "no staging residue after a live MELPA install")
              (Assert (string=
                       (elpa-live-tests-run-package-form
                        elpa-user-dir 'checkbox
                        "(with-temp-buffer (insert \"- Buy gin \") (goto-char (point-min)) (checkbox-toggle) (buffer-string))")
                       "\"- [ ] Buy gin \"")
                      "a fresh XEmacs activates the live-installed checkbox and toggles the spec's bytes"))))
      (ignore-errors (elpa-live-tests-delete-tree elpa-user-dir))))

  ;; A 404 artifact: a local server serves a valid index whose tarball is
  ;; missing.  Install refreshes that index successfully, then fails at
  ;; the download with no package directory or staging residue.  The live
  ;; no-such-dir index negative above cannot reach this point any more
  ;; because install refreshes before resolving.
  (let ((root (file-name-as-directory
               (make-temp-file "elpa-live-http-" t)))
        server)
    (unwind-protect
        (progn
          (with-temp-file (expand-file-name "archive-contents" root)
            (insert "(1\n (queue . [(0 2) nil \"Queue data structure\" tar nil]))\n"))
          (setq server (elpa-live-tests-start-http-server root))
          (let ((elpa-user-dir
                 (file-name-as-directory
                  (make-temp-file "elpa-live-404-" t)))
                (elpa-archives
                 (list (cons "local"
                             (format "http://127.0.0.1:%d/"
                                     elpa-live-tests-http-port))))
                (elpa--archive-index-alist nil))
            (unwind-protect
                (progn
                  (Assert (= 1 (elpa-refresh-contents))
                          "the local HTTP index refresh succeeds before the artifact fetch")
                  (Check-Error-Message
                   'error
                   (concat
                    (regexp-quote
                     (format "Package artifact not found: http://127.0.0.1:%d/queue-0.2.tar (HTTP 404)"
                             elpa-live-tests-http-port))
                    "\\'")
                    (elpa-install "queue"))
                  (Assert (not (file-directory-p
                                (expand-file-name "queue-0.2"
                                                  elpa-user-dir)))
                          "a 404 artifact leaves no package directory")
                  (Assert (not (file-exists-p
                                (expand-file-name ".staging/"
                                                  elpa-user-dir)))
                          "a 404 artifact leaves no staging residue"))
              (ignore-errors (elpa-live-tests-delete-tree elpa-user-dir)))))
      (elpa-live-tests-stop-http-server server)
      (ignore-errors (elpa-live-tests-delete-tree root)))))

(provide 'elpa-live-tests)

;;; elpa-live-tests.el ends here
