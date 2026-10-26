;;; elpa-m2-tests.el --- offline tests for the MELPA milestone-2 fixture

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

;; Offline, network-free tests for the MELPA milestone-2 fixture in
;; tests/elpa/melpa-m2-fixture/.  The fixture spans two local archives:
;; a hand-written MELPA-style index advertising the four real,
;; unmodified target tarballs at pinned date versions, and a GNU-style
;; index advertising queue 0.2.  Each target installs through the public
;; client API into a temporary `elpa-user-dir', then runs its exercise in
;; a fresh child XEmacs whose final output line is compared by raw-byte
;; `string='.
;;
;; The four targets require `cl-lib', which `elpa-core-provided-packages'
;; satisfies without a download; no cl-lib (or cl, cl-extra) build may
;; appear under `elpa-user-dir'.  The checked-in indexes also carry the
;; synthetic entries used by elpa-m2-negative-tests.el; their artifacts
;; are tarred from tests/elpa/melpa-m2-fixture/synthetic/ at test setup,
;; so no synthetic binary is committed.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; Batch startup calls `startup-activate-elpa', which may preload a build
;; copy of elpa before this file runs.  Load the client under test from the
;; first `locate-library' hit and clear the allowlist so its defcustom
;; default is read from that file.
(makunbound 'elpa-installable-packages)
(load "elpa" nil t)

;;; Helpers

(defun elpa-m2-tests--directory ()
  "Return the directory containing this test file."
  (file-name-directory
   (or test-harness-current-file load-file-name buffer-file-name)))

(defun elpa-m2-tests-fixture (&optional subdirectory)
  "Return the absolute path of the m2 fixture, or of SUBDIRECTORY in it."
  (file-name-as-directory
   (expand-file-name (concat "../elpa/melpa-m2-fixture/"
                             (or subdirectory ""))
                     (elpa-m2-tests--directory))))

(defun elpa-m2-tests-archives ()
  "Return the fixture's archive configuration, MELPA first."
  (list (cons "melpa" (elpa-m2-tests-fixture "melpa/"))
        (cons "gnu" (elpa-m2-tests-fixture "gnu/"))))

(defun elpa-m2-tests-delete-tree (dir)
  "Delete DIR and its contents, including symlinks, without following links."
  (when (file-directory-p dir)
    (dolist (file (directory-files dir t))
      (unless (member (file-name-nondirectory file) '("." ".."))
        (if (and (file-directory-p file)
                 (not (file-symlink-p file)))
            (elpa-m2-tests-delete-tree file)
          (delete-file file))))
    (delete-directory dir)))

(defmacro elpa-m2-tests-with-temp-user-dir (&rest body)
  "Run BODY with a fresh temporary `elpa-user-dir'."
  `(let ((elpa-user-dir
          (file-name-as-directory (make-temp-file "elpa-m2-tests-" t))))
     (unwind-protect
         (progn ,@body)
       (ignore-errors (elpa-m2-tests-delete-tree elpa-user-dir)))))

(defun elpa-m2-tests-last-line (output)
  "Return the final non-empty line of OUTPUT."
  (car (last (split-string output "[\n\r]+" t))))

(defun elpa-m2-tests-run-child (user-dir package form)
  "Exercise FORM after requiring PACKAGE in a fresh XEmacs with USER-DIR.
The child loads elpa, activates USER-DIR, requires PACKAGE, and prints the
form's value with `%S'.  Return the final non-empty output line."
  (let ((buffer (get-buffer-create " *elpa-m2-child-output*"))
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
    (elpa-m2-tests-last-line
     (with-current-buffer buffer (buffer-string)))))

;;; The fixture itself

;; The spec's provenance total: the five vendored artifacts sum to
;; 143,360 bytes and must stay within the 192 KiB cap.
(let* ((melpa (elpa-m2-tests-fixture "melpa/"))
       (gnu (elpa-m2-tests-fixture "gnu/"))
       (melpa-tarballs (directory-files melpa t "\\.tar\\'"))
       (gnu-tarballs (directory-files gnu t "\\.tar\\'"))
       (tarballs (append melpa-tarballs gnu-tarballs))
       (total (apply '+ (mapcar (lambda (file)
                                  (nth 7 (file-attributes file)))
                                tarballs))))
  (Assert (= 4 (length melpa-tarballs))
          "the MELPA fixture vendors exactly four target tarballs")
  (Assert (= 1 (length gnu-tarballs))
          "the GNU fixture vendors exactly queue-0.2.tar")
  (Assert (= 143360 total)
          "the five vendored artifacts total 143,360 bytes")
  (Assert (<= total (* 192 1024))
          "the vendored artifacts stay within the 192 KiB cap"))

;;; Index shape: four MELPA targets and GNU queue

(elpa-m2-tests-with-temp-user-dir
  (let ((elpa-archives (elpa-m2-tests-archives))
        (elpa--archive-index-alist nil))
    (Assert (= 18 (elpa-refresh-contents "melpa"))
            "the MELPA fixture index carries the four targets and synthetics")
    (Assert (= 2 (elpa-refresh-contents "gnu"))
            "the GNU fixture index carries queue and the duplicate")
    (let ((queue (elpa-package-info "queue" "gnu")))
      (Assert queue "queue is in the GNU fixture index")
      (Assert (equal (plist-get queue :version) '(0 2))
              "queue is indexed at 0.2")
      (Assert (eq (plist-get queue :kind) 'tar)
              "queue is indexed as a tar artifact"))))

;;; The four targets install and run in a fresh child XEmacs

(defconst elpa-m2-tests-targets
  ;; (NAME VERSION VERSION-LIST REQS EXERCISE-FORM EXPECTED-OUTPUT)
  '((checkbox "20141117.58" (20141117 58)
              ((emacs (24)) (cl-lib (0 5)))
              "(with-temp-buffer (insert \"- Buy gin \") (goto-char (point-min)) (checkbox-toggle) (buffer-string))"
              "\"- [ ] Buy gin \"")
    (es-lib "20141111.1830" (20141111 1830)
            ((cl-lib (0 3)))
            "(progn (require 'es-lib-core-functions) (es-string-remove-properties (propertize \"abc\" 'face 'bold)))"
            "\"abc\"")
    (buffer-sets "20260817.2118" (20260817 2118)
                 ((cl-lib (0 5)))
                 "(let ((buf (get-buffer-create \"m2-buffer-sets-test\"))) (buffer-sets-associate-buffer-with-set \"work\" buf) (list (and (buffer-sets-buffer-in-set-p \"work\" buf) t) (buffer-sets-buffer-has-set-p buf)))"
                 "(t \"work\")")
    (dmacro "20241027.830" (20241027 830)
            ((emacs (24 1)) (cl-lib (0 6)))
            "(dmacro-array-search \"ab\" \"xab\" 0)"
            "1"))
  "The four MELPA milestone-2 targets with the spec's exercises.")

;; `elpa-core-provided-packages' is a private constant, not a Customize
;; knob; a defcustom would leave a `standard-value' property behind.
(Assert (equal elpa-core-provided-packages '(cl cl-extra cl-lib))
        "core-provided packages default to (cl cl-extra cl-lib)")
(Assert (null (get 'elpa-core-provided-packages 'standard-value))
        "elpa-core-provided-packages is not a Customize option")

(dolist (target elpa-m2-tests-targets)
  (let ((name (nth 0 target))
        (version (nth 1 target))
        (version-list (nth 2 target))
        (reqs (nth 3 target))
        (form (nth 4 target))
        (expected (nth 5 target)))
    (elpa-m2-tests-with-temp-user-dir
      (let ((elpa-archives (elpa-m2-tests-archives))
            (elpa--archive-index-alist nil))
        (Assert (memq name elpa-installable-packages)
                (format "%s is in the default allowlist" name))
        (Assert (equal (version-to-list version) version-list)
                (format "%s parses as a date version list" name))
        (Assert (string= (elpa-version-join version-list) version)
                (format "%s joins back to its pinned version string" name))
        (Assert (file-exists-p
                 (expand-file-name
                  (concat (symbol-name name) "-" version ".tar")
                  (elpa-m2-tests-fixture "melpa/")))
                (format "the %s artifact name is the joined version" name))
        (Assert (= 18 (elpa-refresh-contents))
                "the MELPA fixture index carries the four targets and synthetics")
        (let ((info (elpa-package-info name)))
          (Assert info (format "%s is in the MELPA fixture index" name))
          (Assert (equal (plist-get info :version) version-list)
                  (format "%s is indexed at its pinned date version" name))
          (Assert (eq (plist-get info :kind) 'tar)
                  (format "%s is indexed as a tar artifact" name))
          (Assert (equal (plist-get info :reqs) reqs)
                  (format "%s advertises its real Package-Requires" name)))
        (Assert (eq name (elpa-install name))
                (format "install resolves %s" name))
        ;; Exactly one content directory, named for the pinned version.
        (let ((dirs (directory-files
                     elpa-user-dir t
                     (concat "\\`" (regexp-quote (symbol-name name)) "-"))))
          (Assert (= 1 (length dirs))
                  (format "%s installs into exactly one content directory"
                          name))
          (Assert (string= (file-name-nondirectory (car dirs))
                           (concat (symbol-name name) "-" version))
                  (format "%s installs the pinned date version" name)))
        (let ((dir (expand-file-name
                    (concat (symbol-name name) "-" version) elpa-user-dir)))
          (Assert (file-directory-p dir)
                  (format "%s installs into %s-%s/" name name version))
          (dolist (source (directory-files dir t "\\`[^.].*\\.el\\'"))
            (Assert (file-exists-p (concat source "c"))
                    (format "%s byte-compiles to a .elc"
                            (file-name-nondirectory source))))
          (Assert (elpa-installed-p name version-list)
                  (format "%s is installed at the pinned version" name)))
        ;; The cl-lib requirement is core-provided: nothing is fetched.
        (dolist (core '(cl cl-extra cl-lib))
          (Assert (not (elpa-installed-p core))
                  (format "%s requirement for %s installs nothing"
                          core name)))
        (Assert (not (file-exists-p
                      (expand-file-name ".staging/" elpa-user-dir)))
                (format "%s leaves no staging residue" name))
        (Assert (string= (elpa-m2-tests-run-child elpa-user-dir name form)
                         expected)
                (format "%s runs in a fresh child XEmacs" name))))))

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
     "elpa-m2 tests: passed"
   "elpa-m2 tests: FAILED"))

;;; elpa-m2-tests.el ends here
