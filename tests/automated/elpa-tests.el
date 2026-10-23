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

;; Batch startup calls `startup-activate-elpa', which may preload a build copy
;; of elpa before this file runs.  Load the client under test from the first
;; `locate-library' hit (the worktree's private .elc when it is prepended) and
;; clear the allowlist so its defcustom default is read from that file.
(makunbound 'elpa-installable-packages)
(load "elpa" nil t)

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

(defun elpa-tests-build-tar-package (archive work name version
                                            &optional extra-files)
  "Build a minimal tar package NAME at VERSION under WORK.
ARCHIVE is where the artifact is written.  EXTRA-FILES, when non-nil,
is an alist of additional content-relative file names to contents.
Return the artifact path."
  (let ((content (expand-file-name (concat name "-" version "/") work)))
    (make-directory content t)
    (with-temp-file (expand-file-name (concat name ".el") content)
      (insert (format "(provide '%s)\n" name)))
    (with-temp-file (expand-file-name (concat name "-pkg.el") content)
      (insert (format "(define-package \"%s\" \"%s\" \"test package\")\n"
                      name version)))
    (dolist (extra extra-files)
      (with-temp-file (expand-file-name (car extra) content)
        (insert (cdr extra))))
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

(defun elpa-tests-run-loc-changes-child (user-dir)
  "Run a fresh XEmacs with USER-DIR, activate packages, and exercise loc-changes.
Before any require, print whether `loc-changes-add-and-goto' is
autoloaded; then require the library and run the marker-tracking check
in a temp buffer whose contents are \"alpha\\nbeta\\ngamma\\n\".  Return
the child's standard output without a trailing newline."
  (let ((buffer (get-buffer-create " *elpa-loc-child-output*"))
        (program (expand-file-name invocation-name invocation-directory))
        (form
         (concat
          "(progn"
          "(princ (format \"autoloaded=%s\""
          "       (fboundp 'loc-changes-add-and-goto)))"
          "(require 'loc-changes)"
          "(with-current-buffer (get-buffer-create \" *loc-child*\")"
          "  (erase-buffer)"
          "  (insert \"alpha\\nbeta\\ngamma\\n\")"
          "  (goto-char (point-min))"
          "  (loc-changes-add-and-goto 2)"
          "  (let ((before (point)))"
          "    (goto-char (point-min))"
          "    (insert \"0000\")"
          "    (loc-changes-goto 2)"
          "    (let ((after (point)))"
          "      (princ (format \" before=%s after=%s line=%s text=%S\""
          "                     before after"
          "                     (line-number-at-pos after)"
          "                     (buffer-substring after (+ after 4)))))))))")))
    (with-current-buffer buffer
      (erase-buffer))
    (let ((status
           (call-process
            program nil (list buffer nil) nil
            "-batch" "-no-packages"
            "-l" "elpa"
            "-eval" (format "(setq elpa-user-dir %S)" user-dir)
            "-eval" "(elpa-activate-all)"
            "-eval" form)))
      (unless (and (integerp status) (zerop status))
        (let ((output (with-current-buffer buffer (buffer-string))))
          (error "Child XEmacs failed with status %S: %s" status output))))
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

(defun elpa-tests-read-forms (file)
  "Return the top-level forms read from FILE."
  (with-temp-buffer
    (insert-file-contents file)
    (goto-char (point-min))
    (let (forms)
      (condition-case nil
          (while t
            (push (read (current-buffer)) forms))
        (end-of-file nil))
      (nreverse forms))))

(defun elpa-tests-unquote (object)
  "Return OBJECT without a surrounding quote form, if any."
  (if (and (consp object) (eq (car object) 'quote))
      (nth 1 object)
    object))

(defun elpa-tests-autoload-for-p (name forms)
  "Return non-nil if FORMS contain an autoload form for NAME."
  (catch 'found
    (dolist (form forms)
      (when (and (consp form)
                 (eq (car form) 'autoload)
                 (eq (elpa-tests-unquote (nth 1 form)) name))
        (throw 'found form)))
    nil))

(defun elpa-tests-file-contents (file)
  "Return the contents of FILE as a string."
  (with-temp-buffer
    (insert-file-contents file)
    (buffer-string)))

(defun elpa-tests-count-loads (file)
  "Return how many times FILE has been loaded in this process."
  (let ((base (file-name-sans-extension file))
        (count 0))
    (dolist (entry load-history)
      (when (and (consp entry)
                 (stringp (car entry))
                 (string= (file-name-sans-extension (car entry)) base))
        (setq count (1+ count))))
    count))

(defun elpa-tests-count-member (item list)
  "Return how many times ITEM occurs in LIST under `equal'."
  (let ((count 0))
    (dolist (element list)
      (when (equal element item)
        (setq count (1+ count))))
    count))

(defun elpa-tests-installed-builds (name)
  "Return the installed `NAME-VERSION' content directories under `elpa-user-dir'."
  (directory-files elpa-user-dir nil
                   (concat "\\`" (regexp-quote name) "-[0-9]")))

(defmacro elpa-tests-with-temp-home (&rest body)
  "Run BODY with HOME and a fresh `elpa-user-dir' inside it."
  `(let* ((home (file-name-as-directory (make-temp-file "elpa-home-" t)))
          (elpa-user-dir (expand-file-name ".emacs.d/elpa/" home)))
     (unwind-protect
         (progn ,@body)
       (ignore-errors (elpa-tests-delete-tree home)))))

(defconst elpa-tests-startup-queue-check
  (concat
   "(progn"
   "(princ (format \"fbound=%s\" (fboundp 'make-queue)))"
   "(require 'queue)"
   "(let ((q (queue-create)))"
   " (queue-enqueue q 'a)"
   " (queue-enqueue q 'b)"
   "(princ (format \" deq1=%s deq2=%s empty=%s len=%s first=%s\""
   " (queue-dequeue q) (queue-dequeue q)"
   " (queue-empty q) (queue-length q) (queue-first q)))))")
  "Command-line form checking queue in a fresh startup.")

(defvar elpa-tests-extra-environment nil
  "Additional environment entries for `elpa-tests-run-startup-child'.")

(defun elpa-tests-run-startup-child (home &rest args)
  "Run a fresh XEmacs with HOME set to HOME and ARGS.
Return the child's standard output without trailing newlines."
  (let ((buffer (get-buffer-create " *elpa-startup-output*"))
        (program (expand-file-name invocation-name invocation-directory))
        (process-environment
         (append elpa-tests-extra-environment
                 (cons (concat "HOME=" home)
                       (let (result)
                         (dolist (entry process-environment)
                           (unless (string-match "\\`\\(HOME\\|XEMACSDEBUG\\)="
                                                 entry)
                             (push entry result)))
                         (nreverse result))))))
    (with-current-buffer buffer
      (erase-buffer))
    (let ((status (apply 'call-process program nil (list buffer nil) nil
                         (append '("-batch" "-no-site-file") args))))
      (unless (and (integerp status) (zerop status))
        (error "Child XEmacs failed with status %S" status)))
    (let ((output (with-current-buffer buffer (buffer-string))))
      (if (string-match "[\n\r]+\\'" output)
          (substring output 0 (match-beginning 0))
        output))))

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
    (let* ((dir (expand-file-name "queue-0.2/" elpa-user-dir))
           (autoloads (expand-file-name "queue-autoloads.el" dir)))
      (Assert (file-exists-p (expand-file-name "queue.el" dir)))
      (Assert (file-exists-p (expand-file-name "queue-pkg.el" dir)))
      (Assert (file-exists-p (expand-file-name "queue.elc" dir))
              "install byte-compiles the package source")
      (Assert (file-exists-p autoloads)
              "install generates queue-autoloads.el")
      (Assert (file-exists-p (expand-file-name "queue-autoloads.elc" dir))
              "install byte-compiles the generated autoloads")
      (let ((forms (elpa-tests-read-forms autoloads)))
        (Assert (elpa-tests-autoload-for-p 'make-queue forms)
                "the generated autoloads define make-queue")
        (let ((load-path nil))
          (eval (car forms))
          (Assert (member (directory-file-name dir) load-path)
                  "the first autoloads form adds the published content directory")))
      (Assert (not (file-exists-p (expand-file-name ".staging/"
                                                    elpa-user-dir)))
              "the generated autoloads leave no staging residue"))
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
    (Check-Error-Message 'error
        "version (9 9) is no longer available in archive fixture-install (found 0.2)"
      (elpa-install "queue" '(9 9)))
    (Assert (string=
             (elpa-tests-run-child elpa-user-dir)
             "deq1=a deq2=b empty=t len=0 first=nil")
            "a fresh XEmacs activates the installed package and runs queue")))

;;; Compatibility shims

;; `wholenump' is an alias of the existing core `natnump'; `redisplay' is
;; only defined when core does not provide one, and `line-number-at-pos'
;; must remain the existing core definition from simple.el.
(Assert (fboundp 'wholenump) "wholenump exists")
(Assert (eq (symbol-function 'wholenump) 'natnump)
        "wholenump is an alias of natnump")
(Assert (wholenump 0))
(Assert (wholenump 3))
(Assert (null (wholenump -1)))
(Assert (null (wholenump "1")))
(Assert (fboundp 'redisplay) "redisplay exists")
(Assert (progn (redisplay) (redisplay 1 2 3) t)
        "redisplay accepts and ignores arguments")
(Assert (fboundp 'line-number-at-pos)
        "line-number-at-pos remains defined")
(with-temp-buffer
  (insert "alpha\nbeta\ngamma\n")
  (goto-char (point-min))
  (forward-line 1)
  (Assert (= (line-number-at-pos (point)) 2)
          "line-number-at-pos still reports core line numbers"))

;;; Installing loc-changes (single kind)

(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "fixture-single" (elpa-tests-fixture)))))
    (Assert (= 2 (elpa-refresh-contents)))
    (Assert (eq 'loc-changes (elpa-install "loc-changes"))
            "install resolves loc-changes (single kind)")
    (let* ((dir (expand-file-name "loc-changes-1.2/" elpa-user-dir))
           (autoloads (expand-file-name "loc-changes-autoloads.el" dir)))
      (Assert (file-exists-p (expand-file-name "loc-changes.el" dir))
              "the single-file artifact is published as loc-changes.el")
      (Assert (file-exists-p (expand-file-name "loc-changes.elc" dir))
              "install byte-compiles the single-file package source")
      (Assert (file-exists-p autoloads)
              "install generates loc-changes-autoloads.el")
      (Assert (file-exists-p
               (expand-file-name "loc-changes-autoloads.elc" dir))
              "install byte-compiles the generated autoloads")
      (let ((forms (elpa-tests-read-forms autoloads)))
        (Assert (elpa-tests-autoload-for-p 'loc-changes-goto-line forms)
                "the generated autoloads define loc-changes-goto-line")
        (Assert (elpa-tests-autoload-for-p 'loc-changes-add-and-goto forms)
                "the generated autoloads define loc-changes-add-and-goto")
        (Assert (elpa-tests-autoload-for-p 'loc-changes-clear-buffer forms)
                "the generated autoloads define loc-changes-clear-buffer")
        (Assert (elpa-tests-autoload-for-p 'loc-changes-reset-position forms)
                "the generated autoloads define loc-changes-reset-position")
        (let ((load-path nil))
          (eval (car forms))
          (Assert (member (directory-file-name dir) load-path)
                  "the first autoloads form adds the published directory")))
      (Assert (not (file-exists-p (expand-file-name ".staging/"
                                                   elpa-user-dir)))
              "a single-file install leaves no staging residue")
      (Assert (not (file-exists-p (expand-file-name "loc-changes-1.2.el"
                                                    elpa-user-dir)))
              "the downloaded single-file artifact is cleaned")
      (Assert (elpa-installed-p "loc-changes"))
      (Assert (elpa-installed-p "loc-changes" '(1 2)))
      (Assert (string=
               (elpa-tests-run-loc-changes-child elpa-user-dir)
               (concat "autoloaded=t"
                       " before=7 after=11 line=2 text=\"beta\""))
              "a fresh XEmacs activates loc-changes and tracks markers"))))

;; A single-file artifact that is missing errors cleanly and leaves no
;; package directory behind.
(elpa-tests-with-temp-user-dir
  (let ((archive (elpa-tests-make-archive
                  "(1\n (loc-changes . [(1 2) nil \"markers\" single nil]))\n")))
    (unwind-protect
        (let ((elpa-archives (list (cons "missing-single" archive))))
          (elpa-refresh-contents)
          (Check-Error-Message 'error "artifact not found"
            (elpa-install "loc-changes"))
          (Assert (not (file-exists-p
                        (expand-file-name "loc-changes-1.2"
                                          elpa-user-dir))))
          (Assert (not (file-exists-p
                        (expand-file-name ".staging/" elpa-user-dir)))))
      (elpa-tests-delete-tree archive))))

;; A single-file artifact that does not look like the named library is
;; rejected before publication.
(elpa-tests-with-temp-user-dir
  (let ((archive (elpa-tests-make-archive
                  "(1\n (impostor . [(1 0) nil \"Impostor\" single nil]))\n")))
    (unwind-protect
        (let ((elpa-archives (list (cons "impostor" archive)))
              (elpa-installable-packages '(impostor)))
          (with-temp-file (expand-file-name "impostor-1.0.el" archive)
            (insert ";;; other.el --- not the named library\n\n"
                    "(provide 'something-else)\n"))
          (elpa-refresh-contents)
          (Check-Error-Message 'error "library"
            (elpa-install "impostor"))
          (Assert (not (file-exists-p
                        (expand-file-name "impostor-1.0"
                                          elpa-user-dir)))
                  "no package directory after a rejected single file")
          (Assert (not (file-exists-p
                        (expand-file-name ".staging/" elpa-user-dir)))))
      (elpa-tests-delete-tree archive))))

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

;; A non-HTTP(S) URL scheme is rejected with a clear error when
;; refreshing the index.
(elpa-tests-with-temp-user-dir
  (let ((elpa-archives
         '(("ftp-archive" . "ftp://elpa.example.org/packages/"))))
    (Check-Error-Message 'error "not supported"
      (elpa-refresh-contents))
    (Assert (not (file-exists-p
                  (expand-file-name
                   "archives/ftp-archive/archive-contents"
                   elpa-user-dir)))
            "an unsupported scheme caches nothing")))

;; The same clear error through `elpa-install', which refreshes the
;; switched-to unsupported scheme before resolving anything.
(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "mixed" (elpa-tests-fixture)))))
    (elpa-refresh-contents)
    (let ((elpa-archives
           '(("mixed" . "ftp://elpa.example.org/packages/"))))
      (Check-Error-Message 'error "not supported"
        (elpa-install "queue"))
      (Assert (not (elpa-installed-p "queue"))
              "no package directory after an unsupported-scheme fetch"))))

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

;; Dependency requirements are minimums, not exact matches (GNU
;; semantics): a (1 0) requirement is satisfied by an index version
;; (1 1), and by an already-installed (1 1), but not by (0 9).
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n"
                           " (dep-min . [(1 0) ((dep-new (1 0)))"
                           " \"Minimum dependent\" tar nil])\n"
                           " (dep-min2 . [(1 0) ((dep-new (1 0)))"
                           " \"Second dependent\" tar nil])\n"
                           " (dep-old . [(1 0) ((dep-new (2 0)))"
                           " \"Unsatisfiable dependent\" tar nil])\n"
                           " (dep-new . [(1 1) nil \"Dependency\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-depmin-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "dep-min" "1.0")
          (elpa-tests-build-tar-package archive work "dep-min2" "1.0")
          (elpa-tests-build-tar-package archive work "dep-old" "1.0")
          (elpa-tests-build-tar-package archive work "dep-new" "1.1")
          (let ((elpa-archives (list (cons "depmin" archive)))
                (elpa-installable-packages
                 '(dep-min dep-min2 dep-old dep-new)))
            (elpa-refresh-contents)
            (Assert (eq 'dep-min (elpa-install "dep-min"))
                    "a (1 0) requirement installs an index version (1 1)")
            (Assert (elpa-installed-p "dep-new" '(1 1))
                    "the dependency installed at the index version")
            (Assert (not (elpa-installed-p "dep-new" '(1 0)))
                    "the dependency was not pinned to the required version")
            (Assert (eq 'dep-min2 (elpa-install "dep-min2"))
                    "an installed newer dependency satisfies a (1 0) requirement")
            (Check-Error-Message 'error
                "version (2 0) or newer is not available in archive depmin (found 1.1)"
              (elpa-install "dep-old"))
            (Assert (not (elpa-installed-p "dep-old"))
                    "nothing published for the unsatisfiable requirement")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; Refresh before install

;; `elpa-install' always refreshes first, so it works from a cold user
;; directory and caches the index it used.
(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "cold" (elpa-tests-fixture)))))
    (Assert (not (file-exists-p
                  (expand-file-name "archives/cold/archive-contents"
                                    elpa-user-dir)))
            "no cached index before the install")
    (Assert (eq 'queue (elpa-install "queue"))
            "elpa-install refreshes when no cached index exists")
    (Assert (file-readable-p
             (expand-file-name "archives/cold/archive-contents"
                               elpa-user-dir))
            "the install left the refreshed index cached")
    (Assert (elpa-installed-p "queue" '(0 2)))))

;; A changed local index is seen by the next install without an explicit
;; refresh: `elpa-install' refreshes before resolving, so the advertised
;; build replaces the older one and the old build is deleted afterwards.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (rollpkg . [(20260101 1200) nil"
                           " \"Rolling package\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-rolling-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "rollpkg"
                                        "20260101.1200")
          (let ((elpa-archives (list (cons "rolling" archive)))
                (elpa-installable-packages '(rollpkg)))
            (elpa-install "rollpkg")
            (Assert (elpa-installed-p "rollpkg" '(20260101 1200)))
            (elpa-tests-build-tar-package archive work "rollpkg"
                                          "20260202.1300")
            (with-temp-file (expand-file-name "archive-contents" archive)
              (insert "(1\n (rollpkg . [(20260202 1300) nil"
                      " \"Rolling package\" tar nil]))\n"))
            (elpa-install "rollpkg")
            (Assert (elpa-installed-p "rollpkg" '(20260202 1300))
                    "the install sees the changed index without an explicit refresh")
            (Assert (not (file-exists-p
                          (expand-file-name "rollpkg-20260101.1200"
                                            elpa-user-dir)))
                    "the replaced build is deleted")
            (Assert (equal (elpa-tests-installed-builds "rollpkg")
                           '("rollpkg-20260202.1300"))
                    "exactly one content directory remains")
            (Assert (not (file-exists-p
                          (expand-file-name ".staging/" elpa-user-dir)))
                    "staging is clean after a replacement")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; Explicit versions and failed replacements

;; An explicit version is honored only while the freshly read index still
;; advertises it: a pruned version errors clearly, and an explicit current
;; version installs or replaces.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (pinned . [(20260101 1200) nil"
                           " \"Pinned package\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-pinned-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "pinned"
                                        "20260101.1200")
          (let ((elpa-archives (list (cons "pinned-archive" archive)))
                (elpa-installable-packages '(pinned)))
            (elpa-install "pinned" "20260101.1200")
            (Assert (elpa-installed-p "pinned" '(20260101 1200)))
            (elpa-tests-build-tar-package archive work "pinned"
                                          "20260202.1300")
            (with-temp-file (expand-file-name "archive-contents" archive)
              (insert "(1\n (pinned . [(20260202 1300) nil"
                      " \"Pinned package\" tar nil]))\n"))
            (Check-Error-Message 'error
                "version 20260101.1200 is no longer available in archive pinned-archive (found 20260202.1300)"
              (elpa-install "pinned" "20260101.1200"))
            (Assert (elpa-installed-p "pinned" '(20260101 1200))
                    "a pruned explicit version leaves the old build intact")
            (elpa-install "pinned" "20260202.1300")
            (Assert (elpa-installed-p "pinned" '(20260202 1300))
                    "an explicit current version installs the new build")
            (Assert (equal (elpa-tests-installed-builds "pinned")
                           '("pinned-20260202.1300"))
                    "an explicit current version replaces the older build")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A failed replacement leaves the old build intact: nothing new is
;; published and staging is removed.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (replacefail . [(20260101 1200) nil"
                           " \"Replace fail\" tar nil]))\n")))
         (work (file-name-as-directory
                (make-temp-file "elpa-replacefail-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "replacefail"
                                        "20260101.1200")
          (let ((elpa-archives (list (cons "replacefail" archive)))
                (elpa-installable-packages '(replacefail)))
            (elpa-install "replacefail")
            (Assert (elpa-installed-p "replacefail" '(20260101 1200)))
            (with-temp-file (expand-file-name "archive-contents" archive)
              (insert "(1\n (replacefail . [(20260202 1300) nil"
                      " \"Replace fail\" tar nil]))\n"))
            (Check-Error-Message 'error "artifact not found"
              (elpa-install "replacefail"))
            (Assert (elpa-installed-p "replacefail" '(20260101 1200))
                    "a failed replacement keeps the old build")
            (Assert (not (file-exists-p
                          (expand-file-name "replacefail-20260202.1300"
                                            elpa-user-dir)))
                    "a failed replacement publishes no new directory")
            (Assert (equal (elpa-tests-installed-builds "replacefail")
                           '("replacefail-20260101.1200"))
                    "a failed replacement leaves exactly the old build")
            (Assert (not (file-exists-p
                          (expand-file-name ".staging/" elpa-user-dir)))
                    "a failed replacement removes staging")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A replacement whose new build fails to compile leaves the old build
;; intact: nothing new is published and staging is removed.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (replacebad . [(20260101 1200) nil"
                           " \"Replace bad\" tar nil]))\n")))
         (work (file-name-as-directory
                (make-temp-file "elpa-replacebad-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "replacebad"
                                        "20260101.1200")
          (let ((elpa-archives (list (cons "replacebad" archive)))
                (elpa-installable-packages '(replacebad)))
            (elpa-install "replacebad")
            (Assert (elpa-installed-p "replacebad" '(20260101 1200)))
            (elpa-tests-build-tar-package
             archive work "replacebad" "20260202.1300"
             '(("replacebad-broken.el" . "(defun replacebad-broken (\n")))
            (with-temp-file (expand-file-name "archive-contents" archive)
              (insert "(1\n (replacebad . [(20260202 1300) nil"
                      " \"Replace bad\" tar nil]))\n"))
            (Check-Error-Message 'error "ompil"
              (elpa-install "replacebad"))
            (Assert (elpa-installed-p "replacebad" '(20260101 1200))
                    "a compile failure keeps the old build")
            (Assert (not (file-exists-p
                          (expand-file-name "replacebad-20260202.1300"
                                            elpa-user-dir)))
                    "a compile failure publishes no new directory")
            (Assert (equal (elpa-tests-installed-builds "replacebad")
                           '("replacebad-20260101.1200"))
                    "a compile failure leaves exactly the old build")
            (Assert (not (file-exists-p
                          (expand-file-name ".staging/" elpa-user-dir)))
                    "a compile failure removes staging")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; Newest-keyed install queries

;; Installed queries key off the newest build, not directory order, and a
;; re-install collapses duplicate builds to exactly the newest one.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (newestpkg . [(20260202 1300) nil"
                           " \"Newest\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-newest-" t))))
    (unwind-protect
        (progn
          (make-directory
           (expand-file-name "newestpkg-20260101.1200/" elpa-user-dir) t)
          (make-directory
           (expand-file-name "newestpkg-20260202.1300/" elpa-user-dir) t)
          (Assert (equal
                   (file-name-nondirectory
                    (directory-file-name
                     (elpa--find-installed-dir 'newestpkg)))
                   "newestpkg-20260202.1300")
                  "the newest installed version is found, not the first directory")
          (Assert (elpa-installed-p "newestpkg")
                  "a versionless installed check keys off the newest build")
          (Assert (elpa--installed-at-least 'newestpkg '(20260202 1300))
                  "the newest build satisfies an at-least check at its version")
          (Assert (not (elpa--installed-at-least 'newestpkg '(20260203 0)))
                  "an older build does not shadow the newest build")
          (elpa-tests-build-tar-package archive work "newestpkg"
                                        "20260202.1300")
          (let ((elpa-archives (list (cons "newest" archive)))
                (elpa-installable-packages '(newestpkg)))
            (elpa-install "newestpkg"))
          (Assert (equal (elpa-tests-installed-builds "newestpkg")
                         '("newestpkg-20260202.1300"))
                  "a re-install collapses duplicates to the newest build")
          (Assert (elpa-installed-p "newestpkg" '(20260202 1300)))
          (Assert (not (elpa-installed-p "newestpkg" '(20260101 1200)))))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A rolling archive's index can move backwards.  Installing the older
;; advertised build must not delete the newer build already on disk: only
;; builds strictly older than the installed version are pruned.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   (concat "(1\n (rollbackpkg . [(20260303 1400) nil"
                           " \"Rollback\" tar nil]))\n")))
         (work (file-name-as-directory (make-temp-file "elpa-rollback-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package archive work "rollbackpkg"
                                        "20260303.1400")
          (let ((elpa-archives (list (cons "rollback" archive)))
                (elpa-installable-packages '(rollbackpkg)))
            (elpa-install "rollbackpkg")
            (Assert (elpa-installed-p "rollbackpkg" '(20260303 1400)))
            (elpa-tests-build-tar-package archive work "rollbackpkg"
                                          "20260202.1300")
            (with-temp-file (expand-file-name "archive-contents" archive)
              (insert "(1\n (rollbackpkg . [(20260202 1300) nil"
                      " \"Rollback\" tar nil]))\n"))
            (elpa-install "rollbackpkg")
            (Assert (elpa-installed-p "rollbackpkg" '(20260202 1300))
                    "the rolled-back version installs")
            (Assert (equal (elpa-tests-installed-builds "rollbackpkg")
                           '("rollbackpkg-20260202.1300"
                             "rollbackpkg-20260303.1400"))
                    "a rollback keeps the newer build")
            (Assert (equal
                     (file-name-nondirectory
                      (directory-file-name
                       (elpa--find-installed-dir 'rollbackpkg)))
                     "rollbackpkg-20260303.1400")
                    "the newer build remains the installed build")
            (Assert (elpa-installed-p "rollbackpkg")
                    "the versionless installed check stays true")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; Activation

;; Activation loads the generated autoloads once, adds the content directory
;; once, and ignores cached indexes, staging, symlinks, and unrelated
;; entries even when the operation is repeated.
(elpa-tests-with-temp-user-dir
  (let ((elpa-archives (list (cons "fixture-activate" (elpa-tests-fixture)))))
    (elpa-refresh-contents)
    (elpa-install "queue")
    (make-directory (expand-file-name "archives/stray-1.0/" elpa-user-dir) t)
    (with-temp-file (expand-file-name "archives/stray-1.0/stray-autoloads.el"
                                      elpa-user-dir)
      (insert "(provide 'stray-archives-autoloads)\n"))
    (make-directory (expand-file-name "notes-1.0/" elpa-user-dir) t)
    (make-directory (expand-file-name ".staging/target-1.0/" elpa-user-dir) t)
    (with-temp-file
        (expand-file-name ".staging/target-1.0/linked-autoloads.el"
                          elpa-user-dir)
      (insert "(provide 'linked-autoloads)\n"))
    (make-symbolic-link (expand-file-name ".staging/target-1.0" elpa-user-dir)
                        (expand-file-name "linked-1.0" elpa-user-dir))
    (let* ((dir (directory-file-name
                 (expand-file-name "queue-0.2" elpa-user-dir)))
           (autoloads (expand-file-name "queue-0.2/queue-autoloads.el"
                                        elpa-user-dir))
           (stray (directory-file-name
                   (expand-file-name "archives/stray-1.0" elpa-user-dir)))
           (linked (directory-file-name
                    (expand-file-name ".staging/target-1.0" elpa-user-dir))))
      (let ((load-path load-path))
        (elpa-activate-all)
        (Assert (member dir load-path)
                "activation adds the content directory to load-path")
        (Assert (fboundp 'make-queue)
                "activation loads the generated autoloads")
        (Assert (= 1 (elpa-tests-count-loads autoloads))
                "activation loads the autoloads file once")
        (Assert (not (member stray load-path))
                "activation ignores autoloads under archives/")
        (Assert (not (member linked load-path))
                "activation skips symlinked directories")
        (elpa-activate-all)
        (elpa-activate-all)
        (Assert (= 1 (elpa-tests-count-loads autoloads))
                "repeated activation does not reload the autoloads")
        (Assert (= 1 (elpa-tests-count-member dir load-path))
                "repeated activation does not duplicate the load-path entry")
        (Assert (not (featurep 'stray-archives-autoloads))
                "autoloads under archives/ are never loaded")))))

;;; Shipped autoloads and compilation failures

;; An autoloads file supplied by the tarball is replaced by the generated
;; one; only the generated file is retained.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   "(1\n (shippkg . [(1 0) nil \"Ship\" tar nil]))\n"))
         (work (file-name-as-directory (make-temp-file "elpa-ship-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package
           archive work "shippkg" "1.0"
           '(("shippkg-autoloads.el"
              . "(provide 'shipped-autoloads)\n")))
          (let ((elpa-archives (list (cons "ship" archive)))
                (elpa-installable-packages '(shippkg)))
            (elpa-refresh-contents)
            (elpa-install "shippkg")
            (let* ((dir (expand-file-name "shippkg-1.0/" elpa-user-dir))
                   (autoloads (expand-file-name "shippkg-autoloads.el" dir)))
              (Assert (file-exists-p autoloads))
              (Assert (not (string-match
                            "shipped-autoloads"
                            (elpa-tests-file-contents autoloads)))
                      "a tarball-supplied autoloads file is not retained")
              (let ((load-path nil))
                (eval (car (elpa-tests-read-forms autoloads)))
                (Assert (member (directory-file-name dir) load-path)))
              (Assert (file-exists-p
                       (expand-file-name "shippkg-autoloads.elc" dir))))))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;; A compilation error fails the install and leaves no staging or published
;; directory behind.
(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   "(1\n (brokenpkg . [(1 0) nil \"Broken\" tar nil]))\n"))
         (work (file-name-as-directory (make-temp-file "elpa-broken-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package
           archive work "brokenpkg" "1.0"
           '(("brokenpkg-broken.el" . "(defun brokenpkg-broken (\n")))
          (let ((elpa-archives (list (cons "broken" archive)))
                (elpa-installable-packages '(brokenpkg)))
            (elpa-refresh-contents)
            (Check-Error-Message 'error "ompil"
              (elpa-install "brokenpkg"))
            (Assert (not (file-exists-p
                          (expand-file-name "brokenpkg-1.0" elpa-user-dir)))
                    "a failed compile publishes nothing")
            (Assert (not (file-exists-p
                          (expand-file-name ".staging/" elpa-user-dir)))
                    "a failed compile removes staging")))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; HTTP(S) transport over a local server (no external network)

(defconst elpa-tests-http-port 18743
  "Port for the local HTTP fixture server.
Must not collide with async-http-tests' port.")

(defvar elpa-tests-http-server-script
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

(defun elpa-tests-start-http-server (port root)
  "Serve ROOT on 127.0.0.1:PORT; return the server process."
  (let ((proc (start-process "elpa-test-server" " *elpa-test-server*"
                             "python3" "-c" elpa-tests-http-server-script
                             (number-to-string port) root)))
    (sleep-for 0.5)
    proc))

(defun elpa-tests-stop-http-server (proc)
  (when (processp proc)
    (delete-process proc)))

;; The fixture directory doubles as an HTTP archive: /archive-contents and
;; /queue-0.2.tar are the same bytes the local-path tests read.  A second
;; server serves an index whose artifacts are missing, for the artifact-404
;; case.  Everything here is loopback-only, so the default offline suite
;; needs no external network.
(elpa-tests-with-temp-user-dir
  (let ((fixture (elpa-tests-fixture))
        (broken (file-name-as-directory
                 (make-temp-file "elpa-tests-http-" t)))
        server server-404)
    (unwind-protect
        (progn
          (with-temp-file (expand-file-name "archive-contents" broken)
            (insert "(1\n (queue . [(0 2) nil \"Queue data structure\" tar nil]))\n"))
          (setq server (elpa-tests-start-http-server
                        elpa-tests-http-port fixture)
                server-404 (elpa-tests-start-http-server
                            (1+ elpa-tests-http-port) broken))

          ;; A 404 index: clear error, nothing cached.  Both an explicit
          ;; refresh and an install (which refreshes first) take this path.
          (let ((elpa-archives
                 (list (cons "http"
                             (format "http://127.0.0.1:%d/no-such-dir/"
                                     elpa-tests-http-port)))))
            (Check-Error-Message 'error "archive not found"
              (elpa-refresh-contents))
            (Check-Error-Message 'error "archive not found"
              (elpa-install "queue"))
            (Assert (not (file-exists-p
                          (expand-file-name
                           "archives/http/archive-contents"
                           elpa-user-dir)))
                    "a 404 index leaves no cached archive-contents"))

          ;; A good refresh and install over HTTP.
          (let ((elpa-archives
                 (list (cons "http"
                             (format "http://127.0.0.1:%d/"
                                     elpa-tests-http-port)))))
            (Assert (= 2 (elpa-refresh-contents))
                    "refresh over HTTP returns the package count")
            (let ((queue (elpa-package-info "queue")))
              (Assert queue "queue is present after an HTTP refresh")
              (Assert (equal (plist-get queue :version) '(0 2)))
              (Assert (eq (plist-get queue :kind) 'tar)))
            (Assert (file-readable-p
                     (expand-file-name "archives/http/archive-contents"
                                       elpa-user-dir))
                    "an HTTP refresh caches the index under archives/<archive>/")
            (Assert (eq 'queue (elpa-install "queue"))
                    "install downloads the artifact over HTTP")
            (Assert (file-exists-p
                     (expand-file-name "queue-0.2/queue.elc" elpa-user-dir))
                    "the HTTP-installed package is byte-compiled")
            (Assert (not (file-exists-p
                          (expand-file-name ".staging/" elpa-user-dir)))
                    "no staging residue after an HTTP install")
            (Assert (not (file-exists-p
                          (expand-file-name "queue-0.2.tar" elpa-user-dir)))
                    "the downloaded artifact is removed after an HTTP install")
            (Assert (string=
                     (elpa-tests-run-child elpa-user-dir)
                     "deq1=a deq2=b empty=t len=0 first=nil")
                    "a fresh XEmacs runs a package installed over HTTP"))

          ;; A 404 artifact: the second server serves a valid index whose
          ;; tarballs are missing.  The install refreshes that index, then
          ;; fails cleanly at the download with no staging or package dir.
          (let ((elpa-user-dir
                 (file-name-as-directory
                  (make-temp-file "elpa-tests-404-" t))))
            (unwind-protect
                (let ((elpa-archives
                       (list (cons "http"
                                   (format "http://127.0.0.1:%d/"
                                           (1+ elpa-tests-http-port))))))
                  (Check-Error-Message 'error "artifact not found"
                    (elpa-install "queue"))
                  (Assert (not (file-directory-p
                                (expand-file-name "queue-0.2"
                                                  elpa-user-dir)))
                          "a 404 artifact leaves no package directory")
                  (Assert (not (file-exists-p
                                (expand-file-name ".staging/"
                                                  elpa-user-dir)))
                          "a 404 artifact leaves no staging residue"))
              (ignore-errors (elpa-tests-delete-tree elpa-user-dir)))))
      (elpa-tests-stop-http-server server)
      (elpa-tests-stop-http-server server-404)
      (ignore-errors (elpa-tests-delete-tree broken)))))

;;; Startup activation

;; Startup activation runs after site-start and the user's init file but
;; before command-line -eval forms, so either init source can disable it by
;; setting `elpa-activate-at-startup' to nil, and -eval can observe the
;; activated state.  Batch mode never loads init files, so the disabled case
;; sets the variable through XEMACSDEBUG, which XEmacs evaluates at the very
;; start of `normal-top-level', before elpa is loaded.  The child uses
;; -no-site-file rather than -no-packages because startup activation is
;; deliberately suppressed when packages are inhibited.
(elpa-tests-with-temp-home
  (let ((elpa-archives (list (cons "fixture-startup" (elpa-tests-fixture)))))
    (elpa-refresh-contents)
    (elpa-install "queue")
    (Assert (string=
             (elpa-tests-run-startup-child
              home "-eval" elpa-tests-startup-queue-check)
             "fbound=t deq1=a deq2=b empty=t len=0 first=nil")
            "startup activation exposes queue without an explicit require")
    (let ((elpa-tests-extra-environment
           '("XEMACSDEBUG=(setq elpa-activate-at-startup nil)")))
      (Assert (string=
               (elpa-tests-run-startup-child
                home
                "-eval" "(princ (format \"fbound=%s\" (fboundp 'make-queue)))")
               "fbound=nil")
              "startup activation can be disabled"))))

;;; MELPA fixture: the nine targets install and run offline

;; The fixture archive lives beside the GNU one in tests/elpa/melpa-fixture/.
;; It vendors the nine unmodified upstream MELPA tarballs at pinned date
;; versions plus a hand-built version-1 index.  The exercise forms and
;; expected outputs are frozen by the no-kit audit (spec section 6.1); every
;; child runs in a fresh XEmacs with no compatibility shims.

(defun elpa-tests-melpa-fixture ()
  "Return the absolute path of the vendored MELPA fixture archive."
  (file-name-as-directory
   (expand-file-name "../elpa/melpa-fixture/" (elpa-tests--directory))))

;; Pin the provenance total recorded in the fixture README: the nine
;; vendored tarballs sum to 593,920 bytes, the sizes MELPA served.
(let ((tarballs (directory-files (elpa-tests-melpa-fixture) t "\\.tar\\'")))
  (Assert (= 9 (length tarballs))
          "the MELPA fixture vendors exactly nine tarballs")
  (Assert (= 593920
             (apply '+ (mapcar (lambda (file)
                                 (nth 7 (file-attributes file)))
                               tarballs)))
          "the nine vendored tarballs total 593,920 bytes"))

(defconst elpa-tests-melpa-targets
  ;; (NAME VERSION VERSION-LIST EXERCISE-FORM EXPECTED-OUTPUT)
  '((loop "20160813.1407" (20160813 1407)
          "(let ((i 0) (acc nil)) (loop-while t (setq i (1+ i)) (if (= i 3) (loop-continue)) (if (> i 5) (loop-break)) (push i acc)) (nreverse acc))"
          "(1 2 4 5)")
    (levenshtein "20090830.1040" (20090830 1040)
                 "(levenshtein-distance \"kitten\" \"sitting\")"
                 "3")
    (csv "20231216.1243" (20231216 1243)
         "(with-temp-buffer (insert \"a,b,c\\n1,2,3\\n\") (csv-parse-buffer nil (current-buffer)))"
         "((\"a\" \"b\" \"c\") (\"1\" \"2\" \"3\"))")
    (vector-utils "20140508.2041" (20140508 2041)
                  "(vector-utils-flatten '[1 2 [3 4 [5 6 7]]])"
                  "[1 2 3 4 5 6 7]")
    (number "20170901.1312" (20170901 1312)
            "(number-format (number-modify :number (lambda (x) (+ x 1)) (number-read \"6.30\")))"
            "\"7.30\"")
    (rings "20160531.2027" (20160531 2027)
           "(with-temp-buffer (rings-add-buffer \"demo\") (let ((before (length (rings-buffers \"demo\")))) (rings-remove-buffer \"demo\") (list before (length (rings-buffers \"demo\")))))"
           "(1 0)")
    (shell-split-string "20151224.1008" (20151224 1008)
                        "(shell-split-string \"abc \\\"def ghi\\\"\")"
                        "(\"abc\" \"def ghi\")")
    (kv "20140108.1534" (20140108 1534)
        "(list (kva 'b '((a . 1) (b . 2))) (kvalist->keys '((a . 1) (b . 2))))"
        "(2 (a b))")
    (unidecode "20201213.1449" (20201213 1449)
               "(unidecode \"Crème Brûlée\")"
               "\"Creme Brulee\""))
  "The nine MELPA milestone targets with their audit-frozen exercises.")

(defun elpa-tests-melpa-last-line (output)
  "Return the final non-empty line of OUTPUT."
  (car (last (split-string output "[\n\r]+" t))))

(defun elpa-tests-melpa-run-child (user-dir package form)
  "Exercise FORM after requiring PACKAGE in a fresh XEmacs with USER-DIR.
The child loads elpa, activates USER-DIR, requires PACKAGE, and prints the
form's value.  Return the final non-empty output line; `rings-add-buffer'
emits a message line before the value."
  (let ((buffer (get-buffer-create " *elpa-melpa-child-output*"))
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
    (elpa-tests-melpa-last-line
     (with-current-buffer buffer (buffer-string)))))

(dolist (target elpa-tests-melpa-targets)
  (let ((name (nth 0 target))
        (version (nth 1 target))
        (version-list (nth 2 target))
        (form (nth 3 target))
        (expected (nth 4 target)))
    (elpa-tests-with-temp-user-dir
      (let ((elpa-archives
             (list (cons "melpa-fixture" (elpa-tests-melpa-fixture))))
            (elpa--archive-index-alist nil))
        (Assert (equal (version-to-list version) version-list)
                (format "%s parses as a date version list" name))
        (Assert (string= (elpa-version-join version-list) version)
                (format "%s joins back to its pinned version string" name))
        (Assert (file-exists-p
                 (expand-file-name
                  (concat (symbol-name name) "-" version ".tar")
                  (elpa-tests-melpa-fixture)))
                (format "the %s artifact name is the joined version" name))
        (Assert (= 9 (elpa-refresh-contents))
                "the MELPA fixture index carries exactly nine packages")
        (let ((info (elpa-package-info name)))
          (Assert info (format "%s is in the MELPA fixture index" name))
          (Assert (equal (plist-get info :version) version-list)
                  (format "%s is indexed at its pinned date version" name))
          (Assert (eq (plist-get info :kind) 'tar)
                  (format "%s is indexed as a tar artifact" name)))
        (Assert (eq name (elpa-install name))
                (format "install resolves %s" name))
        (let ((dir (expand-file-name (concat (symbol-name name) "-" version)
                                     elpa-user-dir)))
          (Assert (file-directory-p dir)
                  (format "%s installs into exactly %s-%s/"
                          name name version))
          (Assert (file-exists-p
                   (expand-file-name (concat (symbol-name name) ".elc") dir))
                  (format "the downloaded %s.el is byte-compiled" name))
          (Assert (elpa-installed-p name version-list)
                  (format "%s is installed at the pinned version" name)))
        (Assert (string= (elpa-tests-melpa-run-child elpa-user-dir name form)
                         expected)
                (format "%s runs in a fresh child XEmacs" name))))))

;; A MELPA name outside the allowlist is rejected before resolution and
;; before any download, leaving no package directory or staging behind.
(elpa-tests-with-temp-user-dir
  (let ((archive (elpa-tests-make-archive
                  "(1\n (memoize . [(20220718 1430) nil \"Memoize\" tar nil]))\n")))
    (unwind-protect
        (let ((elpa-archives (list (cons "melpa-negative" archive)))
              (elpa--archive-index-alist nil))
          (Check-Error-Message 'error "elpa-installable-packages"
            (elpa-install "memoize"))
          (Assert (null elpa--archive-index-alist)
                  "the allowlist is checked before resolution")
          (Assert (not (file-exists-p
                        (expand-file-name
                         "archives/melpa-negative/archive-contents"
                         elpa-user-dir)))
                  "the refused install never refreshes the index")
          (Assert (not (file-exists-p
                        (expand-file-name "memoize-20220718.1430"
                                          elpa-user-dir)))
                  "the refused install creates no package directory")
          (Assert (not (file-exists-p
                        (expand-file-name ".staging/" elpa-user-dir)))
                  "the refused install leaves no staging residue")
          (Assert (null (directory-files elpa-user-dir nil "\\`[^.]"))
                  "the refused install creates nothing under elpa-user-dir"))
      (elpa-tests-delete-tree archive))))

;;; Autoload docstrings for docstring-less cookie definitions

;; `elpa' is dumped into the build's XEmacs via startup.el, so a `require'
;; here would keep the dumped snapshot even when the freshly compiled
;; worktree copy is first on `load-path'.  Reload it so this section
;; exercises the source under test; loading the same version again in a
;; build without a dump is harmless.
(load "elpa" nil t)

;; A cookie'd definition with no docstring must not leak its first body
;; form into the autoloads file: loading the file evaluates that form.  A
;; backquote body used to abort activation with "Symbol's value as
;; variable is void: key".  A documented defun pins the other direction.
(defun elpa-tests-run-cookie-child (user-dir form)
  "Run a fresh XEmacs with USER-DIR, activate packages, then eval FORM.
The child loads the same `elpa' library as this test run.  Return a list
of the child's exit status, standard output, and standard error."
  (let ((buffer (get-buffer-create " *elpa-cookie-child-output*"))
        (stderr (make-temp-file "elpa-cookie-child-err-"))
        (program (expand-file-name invocation-name invocation-directory))
        (elpa-dir (file-name-directory (locate-library "elpa"))))
    (unwind-protect
        (progn
          (with-current-buffer buffer
            (erase-buffer))
          (let ((status
                 (call-process
                  program nil (list buffer stderr) nil
                  "-batch" "-no-packages"
                  "-eval" (format "(setq load-path (cons %S load-path))"
                                  elpa-dir)
                  "-l" "elpa"
                  "-eval" (format "(setq elpa-user-dir %S)" user-dir)
                  "-eval" "(elpa-activate-all)"
                  "-eval" form)))
            (list status
                  (let ((output (with-current-buffer buffer (buffer-string))))
                    (if (string-match "[\n\r]+\\'" output)
                        (substring output 0 (match-beginning 0))
                      output))
                  (with-temp-buffer
                    (insert-file-contents stderr)
                    (buffer-string)))))
      (ignore-errors (delete-file stderr)))))

(elpa-tests-with-temp-user-dir
  (let* ((archive (elpa-tests-make-archive
                   "(1\n (cookiepkg . [(1 0) nil \"Cookie\" tar nil]))\n"))
         (work (file-name-as-directory (make-temp-file "elpa-cookie-" t))))
    (unwind-protect
        (progn
          (elpa-tests-build-tar-package
           archive work "cookiepkg" "1.0"
           (list
            (cons "cookiepkg.el"
                  (concat
                   ";;;###autoload\n"
                   "(defmacro cookiepkg-undocumented (key)\n"
                   "  `(list 'saw ,key))\n"
                   "\n"
                   ";;;###autoload\n"
                   "(defun cookiepkg-documented ()\n"
                   "  \"Return the documented value.\"\n"
                   "  'documented-value)\n"
                   "\n"
                   ";;;###autoload\n"
                   ;; A real XEmacs shape: (CHILD PARENT NAME DOCSTRING),
                   ;; so the docstring lives in the fifth slot.
                   "(define-derived-mode cookiepkg-mode"
                   " fundamental-mode \"Cookie Mode\""
                   " \"Derived cookie mode.\")\n"
                   "\n"
                   ";;;###autoload\n"
                   "(define-derived-mode cookiepkg-bare"
                   " fundamental-mode \"Bare Mode\")\n"
                   "\n"
                   "(provide 'cookiepkg)\n"))))
          (let ((elpa-archives (list (cons "cookie" archive)))
                (elpa-installable-packages '(cookiepkg)))
            (elpa-refresh-contents)
            (elpa-install "cookiepkg")
            (let* ((dir (expand-file-name "cookiepkg-1.0/" elpa-user-dir))
                   (autoloads (expand-file-name "cookiepkg-autoloads.el" dir))
                   (contents (elpa-tests-file-contents autoloads))
                   (forms (elpa-tests-read-forms autoloads))
                   (documented (elpa-tests-autoload-for-p
                                'cookiepkg-documented forms))
                   (undocumented (elpa-tests-autoload-for-p
                                  'cookiepkg-undocumented forms))
                   (derived (elpa-tests-autoload-for-p
                             'cookiepkg-mode forms))
                   (bare (elpa-tests-autoload-for-p
                          'cookiepkg-bare forms)))
              (Assert documented "the documented defun is autoloaded")
              (Assert (equal (nth 3 documented) "Return the documented value.")
                      "a documented defun keeps its real docstring")
              (Assert undocumented "the docstring-less macro is autoloaded")
              (Assert (null (nth 3 undocumented))
                      "a docstring-less macro gets no body form as docstring")
              (Assert derived "the derived mode is autoloaded")
              (Assert (equal (nth 3 derived) "Derived cookie mode.")
                      "a derived mode keeps its real docstring")
              (Assert bare "the docstring-less derived mode is autoloaded")
              (Assert (null (nth 3 bare))
                      "a docstring-less derived mode gets no docstring")
              (Assert (not (string-match "(backquote" contents))
                      "the macro body is not emitted as an autoload docstring")
              (let ((result (elpa-tests-run-cookie-child
                             elpa-user-dir
                             (concat "(princ (format \"result=%S\""
                                     " (cookiepkg-undocumented 7)))"))))
                (Assert (zerop (nth 0 result))
                        (format "a fresh activation succeeds (stderr: %s)"
                                (nth 2 result)))
                (Assert (equal (nth 1 result) "result=(saw 7)")
                        "the autoloaded macro runs after activation")))))
      (elpa-tests-delete-tree work)
      (elpa-tests-delete-tree archive))))

;;; Terminal line

;; Spec section 9 ends the offline suite with an expected terminal
;; line.  The batch harness exits 0 even when Asserts fail, so the
;; verdict is computed from the harness's own failure counters, which
;; are dynamically bound while the suite body runs.
(Assert (and (boundp 'passes)
             (boundp 'assertion-failures)
             (boundp 'no-error-failures)
             (boundp 'wrong-error-failures)
             (boundp 'missing-message-failures)
             (boundp 'other-failures))
        "the harness failure counters are visible for the terminal line")
(message
 (if (and (zerop assertion-failures)
          (zerop no-error-failures)
          (zerop wrong-error-failures)
          (zerop missing-message-failures)
          (zerop other-failures))
     "elpa tests: passed"
   "elpa tests: FAILED"))

;;; elpa-tests.el ends here
