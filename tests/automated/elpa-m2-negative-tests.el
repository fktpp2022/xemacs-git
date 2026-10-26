;;; elpa-m2-negative-tests.el --- cross-archive resolution and atomicity

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.

;; You should have received a copy of the GNU General Public License
;; along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; Network-free tests for cross-archive dependency resolution.  The
;; checked-in MELPA and GNU indexes carry the synthetic names; their
;; sources live under tests/elpa/melpa-m2-fixture/synthetic/, and this
;; file tars them into temporary copies of both archives at setup.  No
;; synthetic binary is checked in.
;;
;; The witness is m2-xdep: a MELPA-style, date-versioned package whose
;; requirement on GNU queue 0.2 must be found in the second archive.
;; The negatives cover a disallowed dependency, a name in no index, an
;; unsatisfiable floor, a corrupt dependency artifact, a package present
;; in both archives, and a rolling upgrade that keeps its GNU
;; dependency.  Cycles are floor-checked and warned, and all `emacs'
;; requirements produce one summary warning per install operation.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(makunbound 'elpa-installable-packages)
(load "elpa" nil t)

;;; Helpers

(defun elpa-m2-negative-tests--directory ()
  "Return the directory containing this test file."
  (file-name-directory
   (or test-harness-current-file load-file-name buffer-file-name)))

(defun elpa-m2-negative-tests-fixture (&optional subdirectory)
  "Return the absolute path of the m2 fixture, or of SUBDIRECTORY in it."
  (file-name-as-directory
   (expand-file-name (concat "../elpa/melpa-m2-fixture/"
                             (or subdirectory ""))
                     (elpa-m2-negative-tests--directory))))

(defun elpa-m2-negative-tests-delete-tree (dir)
  "Delete DIR and its contents without following symlinks."
  (when (file-directory-p dir)
    (dolist (file (directory-files dir t))
      (unless (member (file-name-nondirectory file) '("." ".."))
        (if (and (file-directory-p file)
                 (not (file-symlink-p file)))
            (elpa-m2-negative-tests-delete-tree file)
          (delete-file file))))
    (delete-directory dir)))

(defmacro elpa-m2-negative-tests-with-user-dir (&rest body)
  "Run BODY with a fresh temporary `elpa-user-dir'."
  `(let ((elpa-user-dir
          (file-name-as-directory
           (make-temp-file "elpa-m2-negative-tests-" t))))
     (unwind-protect
         (progn ,@body)
       (ignore-errors
         (elpa-m2-negative-tests-delete-tree elpa-user-dir)))))

(defun elpa-m2-negative-tests-staging-p ()
  "Return non-nil if the current user directory still has staging."
  (file-exists-p (expand-file-name ".staging/" elpa-user-dir)))

(defun elpa-m2-negative-tests-builds (name)
  "Return the installed content directories whose name starts with NAME."
  (directory-files elpa-user-dir t
                   (concat "\\`" (regexp-quote (symbol-name name)) "-")))

(defun elpa-m2-negative-tests-copy-archive (from to)
  "Copy every regular file in directory FROM into TO."
  (make-directory to t)
  (dolist (file (directory-files from t "\\`[^.]"))
    (when (file-regular-p file)
      (copy-file file (expand-file-name (file-name-nondirectory file) to) t))))

(defun elpa-m2-negative-tests-read-index (file)
  "Return the version-1 index form stored in FILE."
  (with-temp-buffer
    (insert-file-contents file)
    (goto-char (point-min))
    (read (current-buffer))))

(defun elpa-m2-negative-tests-write-index (file form)
  "Write FORM to FILE as a readable archive index."
  (let ((coding-system-for-write 'binary))
    (with-temp-file file
      (prin1 form (current-buffer))
      (insert "\n"))))

(defun elpa-m2-negative-tests-drop-entry (archive name)
  "Remove NAME from ARCHIVE's temporary index."
  (let* ((file (expand-file-name "archive-contents" archive))
         (form (elpa-m2-negative-tests-read-index file))
         (cell (assoc (intern name) (cdr form))))
    (when cell
      (setcdr form (delq cell (cdr form))))
    (elpa-m2-negative-tests-write-index file form)))

(defun elpa-m2-negative-tests-set-entry (archive name version reqs summary)
  "Set NAME's index entry in ARCHIVE to VERSION, REQS, and SUMMARY."
  (let* ((file (expand-file-name "archive-contents" archive))
         (form (elpa-m2-negative-tests-read-index file))
         (name (intern name))
         (cell (assoc name (cdr form)))
         (entry (cons name
                      (vector version reqs summary 'tar
                              (list (cons :url
                                          "https://example.invalid/synthetic"))))))
    (if cell
        (setcdr cell (cdr entry))
      (setcdr form (cons entry (cdr form))))
    (elpa-m2-negative-tests-write-index file form)))

(defun elpa-m2-negative-tests-package-form (source-dir)
  "Return the define-package form of the package in SOURCE-DIR."
  (let ((pkg (car (directory-files source-dir t "-pkg\\.el\\'"))))
    (unless pkg
      (error "No package description in %s" source-dir))
    (with-temp-buffer
      (insert-file-contents pkg)
      (goto-char (point-min))
      (read (current-buffer)))))

(defun elpa-m2-negative-tests-truncate (file bytes)
  "Truncate FILE to BYTES, leaving a partial tar artifact."
  (let ((partial (concat file ".partial")))
    (Assert (= 0 (call-process "dd" nil nil nil
                               (concat "if=" file)
                               (concat "of=" partial)
                               "bs=1" (format "count=%d" bytes)))
            (format "truncated %s to %d bytes"
                    (file-name-nondirectory file) bytes))
    (rename-file partial file t)))

(defun elpa-m2-negative-tests-build-source (archive-dir source-dir truncate)
  "Tar the source tree SOURCE-DIR into ARCHIVE-DIR.
When TRUNCATE is non-nil, truncate the artifact to an invalid tar.
Return the artifact file name."
  (let* ((form (elpa-m2-negative-tests-package-form source-dir))
         (name (nth 1 form))
         (version (nth 2 form))
         (base (concat name "-" version))
         (work (file-name-as-directory (make-temp-file "elpa-m2-synth-" t)))
         (content (expand-file-name (concat base "/") work))
         (pkg (car (directory-files source-dir t "-pkg\\.el\\'")))
         (artifact (expand-file-name (concat base ".tar") archive-dir)))
    (unwind-protect
        (progn
          (make-directory content t)
          (copy-file (expand-file-name (concat name ".el") source-dir)
                     (expand-file-name (concat name ".el") content) t)
          (copy-file pkg (expand-file-name (concat name "-pkg.el") content) t)
          (Assert (= 0 (call-process "tar" nil nil nil "-cf" artifact
                                     "-C" work base))
                  (format "built synthetic artifact %s" base))
          (when truncate
            ;; Fewer than one tar header block: even `tar -tf' fails.
            (elpa-m2-negative-tests-truncate artifact 100))
          artifact)
      (elpa-m2-negative-tests-delete-tree work))))

(defun elpa-m2-negative-tests-make-archives ()
  "Create temporary MELPA and GNU archives with synthetic artifacts.
Return the directory that holds `melpa/' and `gnu/'."
  (let* ((root (file-name-as-directory (make-temp-file "elpa-m2-archives-" t)))
         (melpa (expand-file-name "melpa/" root))
         (gnu (expand-file-name "gnu/" root)))
    (elpa-m2-negative-tests-copy-archive
     (elpa-m2-negative-tests-fixture "melpa/") melpa)
    (elpa-m2-negative-tests-copy-archive
     (elpa-m2-negative-tests-fixture "gnu/") gnu)
    ;; (SOURCE-DIRECTORY . TRUNCATE)
    (dolist (spec '(("m2-xdep" . nil)
                    ("m2-xdep-v2" . nil)
                    ("m2-xdep-notallowed" . nil)
                    ("m2-xdep-missing" . nil)
                    ("m2-xdep-floor" . nil)
                    ("m2-xdep-broken" . nil)
                    ("m2-xdep-dup" . nil)
                    ("m2-dup-melpa" . nil)
                    ("m2-untrusted" . nil)
                    ("m2-corrupt" . t)
                    ("m2-cycle-a" . nil)
                    ("m2-cycle-b" . nil)
                    ("m2-cycle-floor-a" . nil)
                    ("m2-cycle-floor-b" . nil)))
      (elpa-m2-negative-tests-build-source
       melpa
       (elpa-m2-negative-tests-fixture
        (concat "synthetic/" (car spec) "/"))
       (cdr spec)))
    (elpa-m2-negative-tests-build-source
     gnu
     (elpa-m2-negative-tests-fixture "synthetic/m2-dup-gnu/")
     nil)
    ;; "Missing everywhere" needs a name no consulted index advertises.
    ;; The checked-in single index records it; the temporary copy used by
    ;; these tests removes it so resolution, not artifact fetching, is
    ;; what fails.
    (elpa-m2-negative-tests-drop-entry melpa "m2-nowhere")
    root))

(defun elpa-m2-negative-tests-last-line (output)
  "Return the final non-empty line of OUTPUT."
  (car (last (split-string output "[\n\r]+" t))))

(defun elpa-m2-negative-tests-run-child (user-dir package form)
  "Exercise FORM after requiring PACKAGE in a fresh XEmacs with USER-DIR."
  (let ((buffer (get-buffer-create " *elpa-m2-negative-child-output*"))
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
    (elpa-m2-negative-tests-last-line
     (with-current-buffer buffer (buffer-string)))))

(defun elpa-m2-negative-tests-collect-messages (thunk)
  "Call THUNK and return the messages it emitted as one string."
  (let ((messages "")
        (original (symbol-function 'message)))
    (unwind-protect
        (progn
          (fset 'message
                (lambda (format-string &rest args)
                  (setq messages
                        (concat messages (apply 'format format-string args)
                                "\n"))
                  nil))
          (funcall thunk))
      (fset 'message original))
    messages))

(defun elpa-m2-negative-tests-count-matches (regexp string)
  "Return the number of non-overlapping matches of REGEXP in STRING."
  (let ((start 0)
        (count 0))
    (while (string-match regexp string start)
      (setq count (1+ count))
      (setq start (match-end 0)))
    count))

;;; The witness and the negatives

(let* ((root (elpa-m2-negative-tests-make-archives))
       (melpa (expand-file-name "melpa/" root))
       (gnu (expand-file-name "gnu/" root))
       (archives (list (cons "melpa" melpa) (cons "gnu" gnu)))
       (default-allowlist elpa-installable-packages))
  (unwind-protect
      (progn
        ;;; m2-xdep pulls GNU queue 0.2 across archives

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep) default-allowlist))
                (elpa--archive-index-alist nil)
                messages)
            (setq messages
                  (elpa-m2-negative-tests-collect-messages
                   (lambda () (elpa-install 'm2-xdep))))
            (Assert (elpa-installed-p 'm2-xdep '(20261026 1200))
                    "m2-xdep installs its MELPA v1 build")
            (Assert (= 1 (length (elpa-m2-negative-tests-builds 'm2-xdep)))
                    "m2-xdep installs into exactly one directory")
            (Assert (file-directory-p
                     (expand-file-name "m2-xdep-20261026.1200" elpa-user-dir))
                    "the m2-xdep content directory is the v1 build")
            (Assert (elpa-installed-p 'queue '(0 2))
                    "the queue requirement installs 0.2")
            (Assert (file-directory-p
                     (expand-file-name "queue-0.2" elpa-user-dir))
                    "queue installs into queue-0.2/")
            (Assert (= 1 (length (elpa-m2-negative-tests-builds 'queue)))
                    "queue installs exactly once")
            (let ((queue-info (elpa-package-info "queue" "gnu")))
              (Assert (equal "gnu" (plist-get queue-info :archive))
                      "queue's install record carries GNU as its provenance")
              (Assert (null (elpa-package-info "queue" "melpa"))
                      "MELPA does not advertise queue"))
            (Assert (file-readable-p
                     (expand-file-name "archives/gnu/archive-contents"
                                       elpa-user-dir))
                    "the consulted GNU archive was refreshed lazily")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "the cross-archive install leaves no staging residue")
            (Assert (= 1 (elpa-m2-negative-tests-count-matches
                          "ignoring emacs requirement" messages))
                    "two emacs requirements produce one summary warning")
            (Assert (string=
                     (elpa-m2-negative-tests-run-child
                      elpa-user-dir 'm2-xdep
                      "(let ((q (make-queue))) (queue-enqueue q 7) (m2-xdep-peek q))")
                     "7")
                    "m2-xdep and GNU queue run in a fresh child XEmacs")))

        ;; 1. A dependency outside the allowlist fails before download.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep-notallowed) default-allowlist))
                (elpa--archive-index-alist nil))
            (Check-Error-Message
                'error
                "m2-untrusted.*m2-xdep-notallowed.*elpa-installable-packages"
              (elpa-install 'm2-xdep-notallowed))
            (Assert (not (elpa-installed-p 'm2-xdep-notallowed))
                    "the dependent is not installed")
            (Assert (not (elpa-installed-p 'm2-untrusted))
                    "the non-allowlisted dependency is not installed")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "no staging residue after a disallowed dependency")))

        ;; 2. A dependency advertised by no configured index.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep-missing m2-nowhere) default-allowlist))
                (elpa--archive-index-alist nil))
            (Check-Error-Message
                'error
                "m2-nowhere.*m2-xdep-missing.*not available"
              (elpa-install 'm2-xdep-missing))
            (Assert (not (elpa-installed-p 'm2-xdep-missing))
                    "the dependent is not installed")
            (Assert (not (elpa-installed-p 'm2-nowhere))
                    "the missing dependency is not installed")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "no staging residue after a missing dependency")))

        ;; 3. A dependency floor no configured archive can satisfy.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep-floor) default-allowlist))
                (elpa--archive-index-alist nil))
            (Check-Error-Message
                'error
                "queue.*m2-xdep-floor.*9 9"
              (elpa-install 'm2-xdep-floor))
            (Assert (not (elpa-installed-p 'm2-xdep-floor))
                    "the dependent is not installed")
            (Assert (not (elpa-installed-p 'queue))
                    "the unsatisfiable dependency is not installed")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "no staging residue after an unsatisfiable floor")))

        ;; 4. A failed dependency keeps completed dependencies only.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep-broken m2-corrupt) default-allowlist))
                (elpa--archive-index-alist nil))
            (Check-Error-Message 'error "artifact"
              (elpa-install 'm2-xdep-broken))
            (Assert (not (elpa-installed-p 'm2-xdep-broken))
                    "the dependent aborts after a failed dependency")
            (Assert (not (elpa-installed-p 'm2-corrupt))
                    "the corrupt dependency is not published")
            (Assert (elpa-installed-p 'queue '(0 2))
                    "the dependency completed before the failure remains")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "the failed dependency install leaves no staging residue")))

        ;; 5a. A top-level install takes the first archive's build.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-dup) default-allowlist))
                (elpa--archive-index-alist nil))
            (Assert (eq 'm2-dup (elpa-install 'm2-dup))
                    "the duplicate installs from the first archive")
            (Assert (elpa-installed-p 'm2-dup '(20261026 1200))
                    "the MELPA build wins the top-level install")
            (Assert (not (elpa-installed-p 'm2-dup '(20261026 1100)))
                    "the GNU build is not installed")
            (Assert (= 1 (length (elpa-m2-negative-tests-builds 'm2-dup)))
                    "the duplicate leaves exactly one content directory")
            (Assert (string=
                     (elpa-m2-negative-tests-run-child
                      elpa-user-dir 'm2-dup "(m2-dup-source)")
                     "\"melpa-20261026.1200\"")
                    "the installed duplicate is the MELPA build")))

        ;; 5b. Dependency resolution also takes the first archive's build.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep-dup m2-dup) default-allowlist))
                (elpa--archive-index-alist nil))
            (Assert (eq 'm2-xdep-dup (elpa-install 'm2-xdep-dup))
                    "the duplicate's dependent installs")
            (Assert (elpa-installed-p 'm2-dup '(20261026 1200))
                    "dependency resolution takes the MELPA build")
            (Assert (not (elpa-installed-p 'm2-dup '(20261026 1100)))
                    "dependency resolution ignores the later GNU build")
            (Assert (= 1 (length (elpa-m2-negative-tests-builds 'm2-dup)))
                    "the dependency leaves exactly one content directory")))

        ;; 6. A rolling upgrade keeps its GNU dependency intact.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-xdep) default-allowlist))
                (elpa--archive-index-alist nil))
            (Assert (eq 'm2-xdep (elpa-install 'm2-xdep))
                    "the v1 build installs")
            (Assert (string=
                     (elpa-m2-negative-tests-run-child
                      elpa-user-dir 'm2-xdep "m2-xdep-build")
                     "\"v1\"")
                    "v1 is the active build")
            (elpa-m2-negative-tests-set-entry
             melpa "m2-xdep" '(20261026 1300)
             '((emacs (24)) (emacs (25)) (queue (0 2)))
             "Synthetic cross-archive dependent.")
            (Assert (eq 'm2-xdep (elpa-install 'm2-xdep))
                    "the rolling upgrade installs v2")
            (Assert (elpa-installed-p 'm2-xdep '(20261026 1300))
                    "v2 replaces v1")
            (Assert (not (elpa-installed-p 'm2-xdep '(20261026 1200)))
                    "the v1 content directory is gone")
            (Assert (equal '("m2-xdep-20261026.1300")
                           (mapcar 'file-name-nondirectory
                                   (elpa-m2-negative-tests-builds 'm2-xdep)))
                    "the upgrade leaves exactly one content directory")
            (Assert (string=
                     (elpa-m2-negative-tests-run-child
                      elpa-user-dir 'm2-xdep "m2-xdep-build")
                     "\"v2\"")
                    "v2 is the active build")
            (Assert (elpa-installed-p 'queue '(0 2))
                    "queue 0.2 survives the rolling upgrade")
            (Assert (= 1 (length (elpa-m2-negative-tests-builds 'queue)))
                    "the upgrade leaves exactly one queue directory")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "the rolling upgrade leaves no staging residue")))

        ;; 7. A dependency cycle is warned, not silently recursed.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-cycle-a m2-cycle-b) default-allowlist))
                (elpa--archive-index-alist nil)
                messages)
            (setq messages
                  (elpa-m2-negative-tests-collect-messages
                   (lambda () (elpa-install 'm2-cycle-a))))
            (Assert (elpa-installed-p 'm2-cycle-a '(20261026 1200))
                    "the first cycle member installs")
            (Assert (elpa-installed-p 'm2-cycle-b '(20261026 1200))
                    "the second cycle member installs")
            (Assert (string-match "cycle" messages)
                    "the dependency cycle is warned")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "the cycle leaves no staging residue")))

        ;; 8. A cycle back edge is floor-checked.

        (elpa-m2-negative-tests-with-user-dir
          (let ((elpa-archives archives)
                (elpa-installable-packages
                 (append '(m2-cycle-floor-a m2-cycle-floor-b)
                         default-allowlist))
                (elpa--archive-index-alist nil))
            (Check-Error-Message
                'error
                "m2-cycle-floor-a.*m2-cycle-floor-b.*99999999"
              (elpa-install 'm2-cycle-floor-a))
            (Assert (not (elpa-installed-p 'm2-cycle-floor-a))
                    "the unsatisfiable cycle installs nothing")
            (Assert (not (elpa-installed-p 'm2-cycle-floor-b))
                    "no cycle member is published")
            (Assert (not (elpa-m2-negative-tests-staging-p))
                    "the floor-checked cycle leaves no staging residue"))))
    (elpa-m2-negative-tests-delete-tree root)))

;;; Terminal line

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
     "elpa-m2-negative tests: passed"
   "elpa-m2-negative tests: FAILED"))

;;; elpa-m2-negative-tests.el ends here
