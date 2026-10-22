;;; elpa-tests.el --- offline tests for the ELPA client

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;;; Commentary:

;; Offline, network-free tests for lisp/elpa.el and the version helpers it
;; relies on.  The fixture archive lives in tests/elpa/fixture/ and is
;; used as a local absolute-path ELPA archive.

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

;;; Helpers

(defun elpa-tests--directory ()
  "Return the directory containing this test file."
  (file-name-directory
   (or test-harness-current-file load-file-name buffer-file-name)))

(defun elpa-tests-fixture ()
  "Return the absolute path of the vendored fixture archive."
  (file-name-as-directory
   (expand-file-name "../elpa/fixture/" (elpa-tests--directory))))

(defun elpa-tests-delete-tree (dir)
  "Delete DIR and its contents, including symlinks, without following links."
  (when (file-directory-p dir)
    (dolist (file (directory-files dir t))
      (unless (member (file-name-nondirectory file) '("." ".."))
        (if (and (file-directory-p file)
                 (not (file-symlink-p file)))
            (elpa-tests-delete-tree file)
          (delete-file file))))
    (delete-directory dir)))

(defmacro elpa-tests-with-temp-user-dir (&rest body)
  "Run BODY with a fresh temporary `elpa-user-dir'."
  `(let ((elpa-user-dir
          (file-name-as-directory (make-temp-file "elpa-tests-" t))))
     (unwind-protect
         (progn ,@body)
       (ignore-errors (elpa-tests-delete-tree elpa-user-dir)))))

(defun elpa-tests-make-archive (contents)
  "Create a temporary local archive whose index is CONTENTS.
Return its absolute directory name."
  (let ((dir (file-name-as-directory (make-temp-file "elpa-archive-" t))))
    (with-temp-file (expand-file-name "archive-contents" dir)
      (insert contents))
    dir))

(defun elpa-tests-build-tar-package (archive work name version)
  "Build a minimal tar package NAME at VERSION under WORK.
ARCHIVE is where the artifact is written.  Return the artifact path."
  (let ((content (expand-file-name (concat name "-" version "/") work)))
    (make-directory content t)
    (with-temp-file (expand-file-name (concat name ".el") content)
      (insert (format "(provide '%s)\n" name)))
    (with-temp-file (expand-file-name (concat name "-pkg.el") content)
      (insert (format "(define-package \"%s\" \"%s\" \"test package\")\n"
                      name version)))
    (let ((artifact (expand-file-name (concat name "-" version ".tar")
                                      archive)))
      (Assert (= 0 (call-process "tar" nil nil nil "-cf" artifact
                                 "-C" work (concat name "-" version)))
              "built a test tar package")
      artifact)))

(defun elpa-tests-run-child (user-dir)
  "Run a fresh XEmacs with USER-DIR, activate packages, and exercise queue.
Return the child's standard output without a trailing newline."
  (let ((buffer (get-buffer-create " *elpa-child-output*"))
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

(defun elpa-tests-install-collecting-messages (name)
  "Install NAME and return all messages emitted during the install."
  (let ((messages "")
        (original (symbol-function 'message)))
    (unwind-protect
        (progn
          (fset 'message
                (lambda (format-string &rest args)
                  (setq messages
                        (concat messages (apply 'format format-string args)))
                  nil))
          (elpa-install name))
      (fset 'message original))
    messages))

;;; Version helpers

(Assert (equal (version-to-list "0.2") '(0 2)))
(Assert (equal (version-to-list "1.0pre2") '(1 0 -1 2))
        "version-to-list parses a pre release")
(Assert (equal (version-to-list "1.0beta3") '(1 0 -2 3))
        "version-to-list parses a beta marker")
(Assert (equal (version-to-list "0.9alpha1") '(0 9 -3 1))
        "version-to-list parses an alpha marker")
(Assert (equal (version-to-list "2.4.snapshot") '(2 4 -4))
        "version-to-list parses a snapshot marker")
(Assert (equal (version-to-list "22.8 Beta3") '(22 8 -2 3))
        "version-to-list accepts a space before a marker")
(Assert (equal (elpa-version-join '(0 2)) "0.2"))
(Assert (version-list-< '(1 0 -1 2) '(1 0))
        "a pre release sorts before the release")
(Assert (version-list-< '(0 9 -2 3) '(0 9 -1))
        "beta sorts before pre")
(Assert (version-list-< '(1 0) '(1 0 1)))
(Assert (version-list-= '(1 0) '(1 0 0))
        "trailing zeros are insignificant")
(Assert (version-list-<= '(1 0) '(1 0)))
(Assert (not (version-list-< '(1 0 0) '(1 0))))
(Assert (version-list-= (version-to-list "1.0pre2") '(1 0 -1 2)))

;;; Archive refresh

(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "fixture" (elpa-tests-fixture)))))
    (Assert (= 2 (elpa-refresh-contents))
            "refresh returns the number of packages in the index")
    (let ((queue (elpa-package-info "queue")))
      (Assert queue "queue is present in the refreshed index")
      (Assert (equal (plist-get queue :version) '(0 2)))
      (Assert (eq (plist-get queue :kind) 'tar))
      (Assert (null (plist-get queue :reqs))))
    (let ((loc (elpa-package-info 'loc-changes)))
      (Assert loc "loc-changes is present in the refreshed index")
      (Assert (equal (plist-get loc :version) '(1 2)))
      (Assert (eq (plist-get loc :kind) 'single)))
    (Assert (file-readable-p
             (expand-file-name "archives/fixture/archive-contents"
                               elpa-user-dir))
            "the index is cached under archives/<archive>/")))

;; An archive-contents whose first element is not 1 is rejected.
(elpa-tests-with-temp-user-dir
  (let ((dir (elpa-tests-make-archive
              "(2\n (queue . [(0 2) nil \"Queue data structure\" tar nil]))\n")))
    (unwind-protect
        (let ((elpa-archives (list (cons "bad" dir))))
          (Check-Error-Message 'error "version"
            (elpa-refresh-contents)))
      (elpa-tests-delete-tree dir))))

;;; Installing queue

(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "fixture-install" (elpa-tests-fixture)))))
    (Assert (= 2 (elpa-refresh-contents)))
    (Assert (eq 'queue (elpa-install "queue"))
            "install resolves queue by name")
    (let ((dir (expand-file-name "queue-0.2/" elpa-user-dir)))
      (Assert (file-exists-p (expand-file-name "queue.el" dir)))
      (Assert (file-exists-p (expand-file-name "queue-pkg.el" dir))))
    (Assert (not (file-exists-p (expand-file-name ".staging/" elpa-user-dir)))
            "staging is removed after a successful install")
    (Assert (not (file-exists-p (expand-file-name "queue-0.2.tar"
                                                  elpa-user-dir)))
            "the downloaded artifact is removed after a successful install")
    (Assert (elpa-installed-p "queue"))
    (Assert (elpa-installed-p "queue" '(0 2)))
    (Assert (not (elpa-installed-p "queue" '(0 1))))
    (Assert (eq 'queue (elpa-install "queue" "0.2"))
            "a second install returns without overwriting")
    (Check-Error-Message 'error "not available"
      (elpa-install "queue" '(9 9)))
    (Assert (string=
             (elpa-tests-run-child elpa-user-dir)
             "deq1=a deq2=b empty=t len=0 first=nil")
            "a fresh XEmacs activates the installed package and runs queue")))

;;; Negative cases

;; Packages outside the allowlist are rejected before resolution.
(elpa-tests-with-temp-user-dir
  (Check-Error-Message 'error "elpa-installable-packages"
    (elpa-install "spinner")))

;; A tarball member that escapes the package directory is rejected before
;; extraction and leaves no file outside staging.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   "(1\n (evilpkg . [(1 0) nil \"Evil\" tar nil]))\n"))
         (work (file-name-as-directory (make-temp-file "elpa-evil-" t)))
         (content (expand-file-name "evilpkg-1.0/" work))
         (artifact (expand-file-name "evilpkg-1.0.tar" archive)))
    (unwind-protect
        (progn
          (make-directory content t)
          (with-temp-file (expand-file-name "evilpkg.el" content)
            (insert "(provide 'evilpkg)\n"))
          (with-temp-file (expand-file-name "evilpkg-pkg.el" content)
            (insert "(define-package \"evilpkg\" \"1.0\")\n"))
          (with-temp-file (expand-file-name "evil.el" work)
            (insert "evil\n"))
          (Assert (= 0 (call-process "tar" nil nil nil "-cf" artifact
                                     "-C" work "evilpkg-1.0")))
          (Assert (= 0 (call-process "tar" nil nil nil "-rf" artifact
                                     "-C" work "--transform=s|^|../|"
                                     "evil.el")))
          (let ((elpa-archives (list (cons "evil" archive)))
                (elpa-installable-packages '(evilpkg)))
            (elpa-refresh-contents)
            (Check-Error-Message 'error "artifact"
              (elpa-install "evilpkg"))
            (Assert (not (file-exists-p
                          (expand-file-name "evilpkg-1.0" elpa-user-dir)))
                    "no package directory after a rejected tarball")
            (Assert (not (file-exists-p
                          (expand-file-name "evil.el" elpa-user-dir)))
                    "no file outside staging after a rejected tarball")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A tarball with a link entry is rejected before extraction.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   "(1\n (linkpkg . [(1 0) nil \"Link\" tar nil]))\n"))
         (work (file-name-as-directory (make-temp-file "elpa-link-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "linkpkg" "1.0")
          (make-symbolic-link "/etc/passwd"
                              (expand-file-name "link"
                                                (expand-file-name "linkpkg-1.0/"
                                                                  work)))
          (Assert (= 0 (call-process "tar" nil nil nil "-rf"
                                     (expand-file-name "linkpkg-1.0.tar"
                                                       archive)
                                     "-C" work "linkpkg-1.0/link")))
          (let ((elpa-archives (list (cons "link" archive)))
                (elpa-installable-packages '(linkpkg)))
            (elpa-refresh-contents)
            (Check-Error-Message 'error "link entry"
              (elpa-install "linkpkg"))
            (Assert (not (file-exists-p
                          (expand-file-name "linkpkg-1.0" elpa-user-dir))))))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A missing artifact errors cleanly and leaves no package directory.
(elpa-tests-with-temp-user-dir
  (let ((archive (elpa-tests-make-archive
                  "(1\n (queue . [(0 2) nil \"Queue data structure\" tar nil]))\n")))
    (unwind-protect
        (let ((elpa-archives (list (cons "missing" archive))))
          (elpa-refresh-contents)
          (Check-Error-Message 'error "artifact not found"
            (elpa-install "queue"))
          (Assert (not (file-exists-p
                        (expand-file-name "queue-0.2" elpa-user-dir))))
          (Assert (not (file-exists-p
                        (expand-file-name ".staging/" elpa-user-dir)))))
      (elpa-tests-delete-tree archive))))

;; Requirements are installed before the dependent; the emacs pseudo-requirement
;; is ignored with a warning.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n"
                           " (dep-a . [(1 0) ((dep-b (1 0)) (emacs (24 3)))"
                           " \"Dependent\" tar nil])\n"
                           " (dep-b . [(1 0) nil \"Dependency\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-deps-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "dep-a" "1.0")
          (elpa-tests-build-tar-package archive work "dep-b" "1.0")
          (let ((elpa-archives (list (cons "deps" archive)))
                (elpa-installable-packages '(dep-a dep-b)))
            (elpa-refresh-contents)
            (let ((messages (elpa-tests-install-collecting-messages "dep-a")))
              (Assert (string-match "ignoring emacs requirement" messages)
                      "the emacs pseudo-requirement is ignored with a warning"))
            (Assert (elpa-installed-p "dep-a" '(1 0)))
            (Assert (elpa-installed-p "dep-b" '(1 0))
                    "requirements are installed recursively")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; elpa-tests.el ends here
