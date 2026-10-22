;;; elpa.el --- minimal ELPA package client for XEmacs

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

;; A small, install-only GNU ELPA client.  It deliberately implements the
;; subset needed by the milestone target packages: a version-1 archive
;; index, an allowlist, tar artifacts, define-package validation, atomic
;; staging, and explicit activation.  It is not a replacement for GNU
;; package.el and it does not support upgrades, deletion, a package menu,
;; or signature verification.
;;
;; Archive locations may be local absolute directories (used by the
;; offline tests).  HTTP(S) locations are recognised but not yet
;; implemented; binary-safe transport is a separate milestone ticket.
;;
;; This file does not use lexical binding.

;;; Code:

(defgroup elpa nil
  "Minimal GNU ELPA client."
  :group 'emacs)

(defcustom elpa-archives
  '(("gnu" . "https://elpa.gnu.org/packages/"))
  "Alist of archive names to archive locations.
A location may be a local absolute directory path.  HTTP(S) locations
are reserved for the transport milestone and are not implemented yet."
  :type '(repeat (cons string string))
  :group 'elpa)

(defcustom elpa-user-dir
  (expand-file-name "~/.emacs.d/elpa/")
  "Directory where installed packages and cached archive indexes live."
  :type 'directory
  :group 'elpa)

(defcustom elpa-installable-packages
  '(queue loc-changes)
  "Allowlist of packages this client may install.
The `emacs' pseudo-dependency is not trusted; this list is the safety
boundary for the milestone."
  :type '(repeat symbol)
  :group 'elpa)

(defcustom elpa-check-signature nil
  "Whether to verify package signatures.
Milestone 1 installs unsigned archives; .sig files are ignored."
  :type 'boolean
  :group 'elpa)

(defcustom elpa-activate-at-startup t
  "Whether startup should activate installed packages.
Startup activation itself is implemented by a later milestone ticket."
  :type 'boolean
  :group 'elpa)

(defvar elpa--archive-index-alist nil
  "Alist mapping archive name strings to parsed index entries.")

(defvar elpa--install-in-progress nil
  "Names currently being installed, used to break dependency cycles.")

;;; Basic helpers

(defun elpa--package-symbol (name)
  "Return NAME as a package symbol."
  (cond ((symbolp name) name)
	((stringp name) (intern name))
	(t (error "Package name must be a symbol or a string: %S" name))))

(defun elpa--archive-name (archive)
  "Return ARCHIVE's name as a string."
  (cond ((symbolp archive) (symbol-name archive))
	((stringp archive) archive)
	(t (error "Archive name must be a symbol or a string: %S" archive))))

(defun elpa--archive-location (archive)
  "Return the location configured for ARCHIVE."
  (let ((name (elpa--archive-name archive)))
    (let ((cell (or (assoc archive elpa-archives)
		    (assoc name elpa-archives)
		    (assoc (intern name) elpa-archives))))
      (unless cell
	(error "Unknown ELPA archive: %s" name))
      (unless (stringp (cdr cell))
	(error "ELPA archive location must be a string: %S" (cdr cell)))
      (cdr cell))))

(defun elpa--ensure-user-dir ()
  "Create `elpa-user-dir' and its parents if needed."
  (make-directory elpa-user-dir t))

(defun elpa--index-cache-file (archive)
  "Return the cached index file name for ARCHIVE."
  (expand-file-name (concat "archives/" (elpa--archive-name archive)
			    "/archive-contents")
		    elpa-user-dir))

(defun elpa--read-file (file)
  "Return the contents of FILE as a string."
  (with-temp-buffer
    (insert-file-contents file)
    (buffer-string)))

(defun elpa--delete-tree (dir)
  "Delete DIR and everything below it without following symlinks."
  (when (file-directory-p dir)
    (dolist (file (directory-files dir t))
      (unless (member (file-name-nondirectory file) '("." ".."))
	(if (and (file-directory-p file)
		 (not (file-symlink-p file)))
	    (elpa--delete-tree file)
	  (delete-file file))))
    (delete-directory dir)))

(defun elpa--version-list (version)
  "Normalise VERSION, a string or a list of integers, to a version list."
  (cond ((stringp version) (version-to-list version))
	((listp version) version)
	(t (error "Version must be a string or a list: %S" version))))

;;; Archive index

(defun elpa--parse-index (raw)
  "Parse RAW archive-contents text and enforce the version-1 gate.
Return the list of package entries."
  (let ((form (condition-case err
		  (car (read-from-string raw))
		(error
		 (error "Malformed ELPA archive index: %s"
			(error-message-string err))))))
    (unless (and (consp form) (integerp (car form)))
      (error "Malformed ELPA archive index: missing version"))
    (unless (= (car form) 1)
      (error "Unsupported ELPA archive-contents version: %s" (car form)))
    (let ((entries (cdr form)))
      (dolist (entry entries)
	(unless (and (consp entry)
		     (symbolp (car entry))
		     (or (consp (cdr entry)) (vectorp (cdr entry))))
	  (error "Malformed ELPA archive entry: %S" entry)))
      entries)))

(defun elpa--entry-info (entry archive)
  "Return a plist describing package ENTRY from ARCHIVE."
  (let* ((fields (if (vectorp (cdr entry))
		     (append (cdr entry) nil)
		   (cdr entry)))
	 (info (list :name (car entry)
		     :version (nth 0 fields)
		     :reqs (nth 1 fields)
		     :summary (nth 2 fields)
		     :kind (nth 3 fields)
		     :extras (nth 4 fields)
		     :archive (elpa--archive-name archive))))
    (unless (listp (plist-get info :version))
      (error "Malformed version for package `%s': %S"
	     (car entry) (plist-get info :version)))
    info))

(defun elpa--index (archive)
  "Return the parsed index entries for ARCHIVE, refreshing if needed."
  (let* ((key (elpa--archive-name archive))
	 (cell (assoc key elpa--archive-index-alist)))
    (if cell
	(cdr cell)
      (let ((cache (elpa--index-cache-file archive)))
	(if (file-readable-p cache)
	    (let ((entries (elpa--parse-index (elpa--read-file cache))))
	      (setq elpa--archive-index-alist
		    (cons (cons key entries) elpa--archive-index-alist))
	      entries)
	  (elpa-refresh-contents archive)
	  (cdr (assoc key elpa--archive-index-alist)))))))

(defun elpa-package-info (name &optional archive)
  "Return a plist describing package NAME in ARCHIVE, or nil.
The plist has :name, :version, :reqs, :summary, :kind, :extras, and
:archive keys.  The default archive is the first entry in
`elpa-archives'."
  (let* ((archive (or archive (caar elpa-archives)))
	 (entry (assoc (elpa--package-symbol name) (elpa--index archive))))
    (when entry
      (elpa--entry-info entry archive))))

(defun elpa-refresh-contents (&optional archive)
  "Refresh ARCHIVE's index and return the number of packages it contains.
ARCHIVE defaults to the first entry in `elpa-archives'.  Only local
absolute-path archives are supported in this milestone."
  (interactive)
  (let* ((archive (or archive (caar elpa-archives)))
	 (base (elpa--archive-location archive)))
    (when (string-match "\\`\\(https?\\|ftp\\)://" base)
      (error "ELPA archive %s uses a network location; HTTP transport is not implemented yet"
	     base))
    (unless (file-name-absolute-p base)
      (error "ELPA archive location must be an absolute directory: %s" base))
    (let ((source (expand-file-name "archive-contents" base)))
      (unless (file-readable-p source)
	(error "ELPA archive not found: %s" source))
      (let* ((raw (elpa--read-file source))
	     (entries (elpa--parse-index raw))
	     (cache (elpa--index-cache-file archive)))
	(elpa--ensure-user-dir)
	(make-directory (file-name-directory cache) t)
	(copy-file source cache t)
	(setq elpa--archive-index-alist
	      (cons (cons (elpa--archive-name archive) entries)
		    elpa--archive-index-alist))
	(length entries)))))

;;; Resolution and installation

(defun elpa--ensure-allowlisted (name)
  "Signal an error unless NAME is in `elpa-installable-packages'."
  (unless (memq name elpa-installable-packages)
    (error "Package `%s' is not in elpa-installable-packages" name)))

(defun elpa--package-dir (name version)
  "Return the install directory for package NAME at VERSION."
  (expand-file-name (concat (symbol-name name) "-"
			    (elpa-version-join version))
		    elpa-user-dir))

(defun elpa--find-installed-dir (name)
  "Return the directory of an installed version of NAME, or nil."
  (let ((prefix (concat (symbol-name name) "-")))
    (when (file-directory-p elpa-user-dir)
      (catch 'found
	(dolist (file (directory-files elpa-user-dir t "\\`[^.]"))
	  (when (and (file-directory-p file)
		     (not (file-symlink-p file))
		     (string-match
		      (concat "\\`" (regexp-quote prefix)
			      "[0-9]+\\(\\.[0-9]+\\)*\\'")
		      (file-name-nondirectory file)))
	    (throw 'found file)))
	nil))))

(defun elpa-installed-p (name &optional version)
  "Return non-nil if package NAME is installed.
If VERSION is non-nil, require that exact version."
  (let* ((name (elpa--package-symbol name))
	 (dir (if version
		  (elpa--package-dir name (elpa--version-list version))
		(elpa--find-installed-dir name))))
    (and dir (file-directory-p dir) t)))

(defun elpa--resolve (name version archive)
  "Resolve NAME at VERSION in ARCHIVE and return its package plist."
  (let ((info (elpa-package-info name archive)))
    (unless info
      (error "Package `%s' is not available in archive %s"
	     name (elpa--archive-name archive)))
    (when version
      (let ((wanted (elpa--version-list version)))
	(unless (version-list-= wanted (plist-get info :version))
	  (error "Package `%s' version %s is not available in archive %s (found %s)"
		 name version (elpa--archive-name archive)
		 (elpa-version-join (plist-get info :version))))))
    info))

(defun elpa-install (name &optional version)
  "Install package NAME at VERSION from the first configured archive.
VERSION may be a version string or a list of integers.  NAME must be in
`elpa-installable-packages'.  Return NAME."
  (interactive
   (list (intern (read-string "Package: "))))
  (let* ((name (elpa--package-symbol name))
	 (archive (or (caar elpa-archives)
		      (error "`elpa-archives' is empty")))
	 (elpa--install-in-progress nil))
    (elpa--install-1 name version archive)
    name))

(defun elpa--install-1 (name version archive)
  "Install NAME at VERSION from ARCHIVE, including its dependencies."
  (elpa--ensure-allowlisted name)
  (cond
   ((elpa-installed-p name version) name)
   ;; A dependency cycle: this package is already being installed by an
   ;; outer call, so do not recurse.
   ((memq name elpa--install-in-progress) name)
   (t
    (let ((info (elpa--resolve name version archive)))
      (let ((elpa--install-in-progress
	     (cons name elpa--install-in-progress)))
	(dolist (dependency (plist-get info :reqs))
	  (elpa--install-dependency dependency name archive))
	(elpa--install-artifact info archive)))
    name)))

(defun elpa--install-dependency (dependency dependent archive)
  "Install DEPENDENCY of DEPENDENT from ARCHIVE, if it is required."
  (let ((name (car dependency))
	(version (nth 1 dependency)))
    (cond
     ((eq name 'emacs)
      (warn "ignoring emacs requirement %s for %s" dependency dependent))
     ((elpa-installed-p name version) nil)
     (t
      (unless (memq name elpa-installable-packages)
	(error "Required package `%s' for `%s' is not in elpa-installable-packages"
	       name dependent))
      (elpa--install-1 name version archive)))))

(defun elpa--artifact-name (name version)
  "Return the artifact base name for package NAME at VERSION."
  (concat (symbol-name name) "-" (elpa-version-join version)))

(defun elpa--install-artifact (info archive)
  "Download, validate, stage, and publish package INFO from ARCHIVE."
  (let* ((name (plist-get info :name))
	 (version (plist-get info :version))
	 (kind (plist-get info :kind))
	 (base (elpa--archive-location (plist-get info :archive)))
	 (artifact-base (elpa--artifact-name name version))
	 (staging-root (expand-file-name ".staging/" elpa-user-dir))
	 (staging (expand-file-name artifact-base staging-root))
	 (published (elpa--package-dir name version)))
    (unless (eq kind 'tar)
      (error "Unsupported package kind `%s' for `%s'" kind name))
    (when (file-exists-p published)
      (error "Package `%s' is already installed in %s" name published))
    (elpa--ensure-user-dir)
    (when (file-exists-p staging)
      (elpa--delete-tree staging))
    (make-directory staging t)
    (unwind-protect
	(let ((artifact (expand-file-name (concat artifact-base ".tar")
					  staging))
	      (source (expand-file-name (concat artifact-base ".tar")
					base)))
	  (unless (file-readable-p source)
	    (error "Package artifact not found: %s" source))
	  (copy-file source artifact)
	  (unless (file-regular-p artifact)
	    (error "Package artifact is not a regular file: %s" artifact))
	  (elpa--validate-tar artifact name version)
	  (let ((status (call-process "tar" nil nil nil
				      "-xf" artifact "-C" staging)))
	    (unless (and (integerp status) (zerop status))
	      (error "Failed to extract package artifact %s (tar exit %s)"
		     artifact status)))
	  (let ((content (expand-file-name artifact-base staging)))
	    (unless (file-directory-p content)
	      (error "Package artifact did not create %s/" artifact-base))
	    (elpa--validate-pkg-file content name version)
	    (when (file-exists-p published)
	      (error "Package `%s' is already installed in %s"
		     name published))
	    (rename-file content published)))
      (ignore-errors (elpa--delete-tree staging))
      (when (and (file-directory-p staging-root)
		 (= 2 (length (directory-files staging-root))))
	(ignore-errors (delete-directory staging-root))))))

(defun elpa--tar-listing (artifact &rest args)
  "Run tar with ARGS and ARTIFACT and return the output as a string."
  (with-temp-buffer
    (let ((coding-system-for-read 'raw-text)
	  (status (apply 'call-process "tar" nil t nil
			 (append args (list artifact)))))
      (unless (and (integerp status) (zerop status))
	(error "Failed to inspect package artifact %s (tar exit %s)"
	       artifact status))
      (buffer-string))))

(defun elpa--validate-tar (artifact name version)
  "Validate every member of ARTIFACT for NAME at VERSION.
Reject absolute names, `..' components, members outside the single
top-level `<name>-<version>/' directory, and link entries."
  (let ((top (elpa--artifact-name name version))
	(members (split-string (elpa--tar-listing artifact "-tf")
			       "\n" t)))
    (unless members
      (error "Package artifact is empty: %s" artifact))
    (dolist (member members)
      (when (string-match "\\`/" member)
	(error "Package artifact contains an absolute member name: %s"
	       member))
      (let ((components (split-string member "/" t)))
	(when (member ".." components)
	  (error "Package artifact contains a `..' component: %s" member))
	(unless (and components (string= (car components) top))
	  (error "Package artifact member %s is not under %s/"
		 member top)))))
  (let ((verbose (elpa--tar-listing artifact "-tvf")))
    (with-temp-buffer
      (insert verbose)
      (goto-char (point-min))
      (while (not (eobp))
	(when (looking-at "\\([lh]\\)")
	  (error "Package artifact contains a link entry"))
	(forward-line 1)))))

(defun elpa--validate-pkg-file (content name version)
  "Validate CONTENT's define-package metadata for NAME at VERSION."
  (let ((file (expand-file-name (concat (symbol-name name) "-pkg.el")
				content)))
    (unless (file-readable-p file)
      (error "Package `%s' is missing %s" name
	     (file-name-nondirectory file)))
    (with-temp-buffer
      (insert-file-contents file)
      (goto-char (point-min))
      (let ((form (condition-case err
		      (read (current-buffer))
		    (error
		     (error "Malformed package description in %s: %s"
			    file (error-message-string err))))))
	(unless (and (consp form) (eq (car form) 'define-package))
	  (error "Package `%s' has no valid define-package form" name))
	(let ((declared-name (nth 1 form))
	      (declared-version (nth 2 form)))
	  (unless (equal (format "%s" declared-name) (symbol-name name))
	    (error "Package name in %s is `%s', expected `%s'"
		   file declared-name name))
	  (unless (version-list-=
		   (version-to-list (format "%s" declared-version)) version)
	    (error "Package version in %s is `%s', expected `%s'"
		   file declared-version (elpa-version-join version))))))))

;;; Activation

(defun elpa-activate-all ()
  "Add installed package content directories to `load-path'.
This is idempotent.  Loading autoloads and startup activation are
implemented by later milestone tickets."
  (interactive)
  (when (file-directory-p elpa-user-dir)
    (dolist (dir (directory-files elpa-user-dir t "\\`[^.]"))
      (when (and (file-directory-p dir)
		 (not (file-symlink-p dir))
		 (string-match "\\`[^/]+-[0-9]+\\(\\.[0-9]+\\)*\\'"
			       (file-name-nondirectory dir)))
	(let ((dir (directory-file-name dir)))
	  (unless (member dir load-path)
	    (setq load-path (append load-path (list dir))))))))
  nil)

(provide 'elpa)

;;; elpa.el ends here
