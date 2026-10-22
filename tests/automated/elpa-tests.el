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
    (Check-Error-Message 'error "not available"
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

;;; elpa-tests.el ends here
