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

;; A small, install-only ELPA client.  It deliberately implements the
;; subset needed by the milestone target packages: a version-1 archive
;; index, an allowlist, tar and single-file artifacts, define-package
;; validation for tarballs, atomic staging, generated autoloads,
;; byte-compilation, and explicit activation.  It is not a replacement
;; for GNU package.el and it does not provide a package menu, deletion,
;; or rollback.  GNU ELPA and MELPA are supported; MELPA is a
;; rolling-release archive, so `elpa-install' refreshes the index before
;; resolving, installs the build currently advertised, and may replace
;; an older installed build, publishing the new build before deleting
;; the old one.  No package signature or checksum verification is
;; performed.
;;
;; Trust model: this client performs no signature verification. MELPA
;; publishes no signatures, by design; GNU ELPA signature checking is
;; deferred to a future core effort. The only controls are HTTPS
;; transport and the `elpa-installable-packages' allowlist: if an
;; archive is compromised or the connection is tampered with, any
;; allowlisted package can run arbitrary code. The allowlist is not a
;; cast-iron guarantee — keep it short, and install only packages you
;; would trust with your keystrokes.
;;
;; Cross-archive dependency resolution: an install searches every
;; archive in `elpa-archives' order for each required dependency, and
;; every dependency name that is not core-provided (and is not the
;; `emacs' pseudo-dependency) must appear in
;; `elpa-installable-packages' at every depth.  A dependency edge never
;; grants trust implicitly.
;;
;; Archive locations may be local absolute directories (used by the
;; offline tests) or HTTP(S) URLs (used against live archives such as
;; https://elpa.gnu.org/packages/).  HTTP(S) locations are fetched with
;; the async HTTP transport; index fetches use a 60 s timeout, artifact
;; downloads a 300 s timeout.  Artifact bytes are written directly to
;; staging by C and never round-trip through a Lisp string.
;;
;; This file does not use lexical binding.

;;; Code:

(require 'async-core)
(require 'async-http nil t)

(defgroup elpa nil
  "Minimal GNU ELPA client."
  :group 'emacs)

(defcustom elpa-archives
  '(("gnu" . "https://elpa.gnu.org/packages/"))
  "Alist of archive names to archive locations.
A location may be a local absolute directory path or an HTTP(S) URL.
Local locations are read directly; HTTP(S) locations go through the
async HTTP transport."
  :type '(repeat (cons string string))
  :group 'elpa)

(defcustom elpa-user-dir
  (expand-file-name "~/.emacs.d/elpa/")
  "Directory where installed packages and cached archive indexes live."
  :type 'directory
  :group 'elpa)

(defcustom elpa-installable-packages
  '(queue loc-changes
    dmacro checkbox es-lib buffer-sets
    loop levenshtein csv vector-utils number rings shell-split-string kv
    unidecode)
  "Allowlist of packages this client may install.
The `emacs' pseudo-dependency is not trusted; this list is the safety
boundary for the milestone."
  :type '(repeat symbol)
  :group 'elpa)

(defconst elpa-core-provided-packages '(cl cl-extra cl-lib)
  "Package names this XEmacs core provides itself.
A dependency requirement on one of these names is satisfied without a
download.  This is an internal constant, not a Customize option.")


(defcustom elpa-activate-at-startup t
  "Whether startup should activate installed packages.
When non-nil and `elpa-user-dir' exists, startup calls
`elpa-activate-all', which adds installed package directories to
`load-path' and loads their generated autoloads.  Set this to nil in
your init file to disable startup activation."
  :type 'boolean
  :group 'elpa)

(defvar elpa--archive-index-alist nil
  "Alist mapping archive name strings to parsed index entries.")

(defvar elpa--install-refreshed-archives nil
  "Archive names refreshed during the current install operation.")

(defvar elpa--install-emacs-requirements nil
  "Ignored `emacs' requirements collected during the current install.")

(defvar elpa--plan-nodes nil
  "Alist of package names to plan nodes for the current install.")

(defvar elpa--plan-order nil
  "Plan nodes in dependency-first install order.")

(defvar elpa--plan-path nil
  "Package names on the current planning path, for cycle detection.")

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

(defun elpa-version-join (version)
  "Return VERSION, a list of integers, as a dotted version string.
For example, (0 2) becomes \"0.2\".  This is the form used in ELPA
artifact and content-directory names."
  (mapconcat #'number-to-string version "."))

;;; HTTP transport

(defconst elpa--index-timeout 60000
  "Timeout in milliseconds for fetching an archive index.")

(defconst elpa--artifact-timeout 300000
  "Timeout in milliseconds for downloading a package artifact.")

(defun elpa--location-http-p (location)
  "Return non-nil if LOCATION is an HTTP or HTTPS URL."
  (string-match "\\`https?://" location))

(defun elpa--require-http-transport ()
  "Signal an error unless the async HTTP transport is available."
  (unless (fboundp 'http-get)
    (error "ELPA archive location is an HTTP(S) URL, but the async HTTP transport is not available in this XEmacs")))

(defun elpa--remote-url (base name)
  "Return the HTTP(S) URL joining BASE and NAME with exactly one slash."
  (concat (if (string-match "/\\'" base) base (concat base "/")) name))

(defun elpa--sync-await (thunk timeout-ms)
  "Evaluate THUNK in a fresh coroutine and return its value.
THUNK runs inside a coroutine and evaluates `await' expressions.  The
async scheduler is ticked until the coroutine completes; TIMEOUT-MS
bounds the wait in addition to any timeout the awaited operation
itself enforces.  An error that escapes THUNK is re-signaled.  This is
the synchronous bridge that lets the install pipeline use the async
HTTP transport from ordinary code, including in batch mode."
  (let ((max-ticks (+ 500 (/ timeout-ms 10)))
	(ticks 0)
	done result err)
    (async-spawn-coroutine
     (lambda (_arg)
       (setq result
	     (condition-case caught
		 (funcall thunk)
	       (t (setq err caught) nil))
	     done t))
     nil)
    (while (and (not done) (< ticks max-ticks))
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ticks (1+ ticks)))
    (unless done
      (error "ELPA asynchronous fetch did not complete within its timeout"))
    (when err
      (signal (car err) (cdr err)))
    result))

(defun elpa--http-check-status (status url not-found-message)
  "Signal a clear error unless STATUS is 200 for URL.
A 404 signals NOT-FOUND-MESSAGE; any other non-200 status signals a
message naming the status."
  (cond ((= status 200) nil)
	((= status 404)
	 (error "%s (HTTP 404)" not-found-message))
	(t
	 (error "ELPA fetch of %s failed with HTTP status %d" url status))))

(defun elpa--http-get (url timeout-ms not-found-message)
  "GET URL and return (STATUS HEADERS BODY).
TIMEOUT-MS bounds the transfer.  Signal unless the status is 200; a
404 signals NOT-FOUND-MESSAGE."
  (elpa--require-http-transport)
  (let ((result (elpa--sync-await
		 (lambda () (await (http-get url :timeout timeout-ms)))
		 timeout-ms)))
    (elpa--http-check-status (nth 0 result) url not-found-message)
    result))

(defun elpa--http-download (url file timeout-ms not-found-message)
  "Download URL into FILE and return (STATUS HEADERS FILE).
The response bytes are written directly to FILE by C and never pass
through a Lisp string.  TIMEOUT-MS bounds the transfer.  Signal unless
the status is 200; a 404 signals NOT-FOUND-MESSAGE."
  (elpa--require-http-transport)
  (let ((result (elpa--sync-await
		 (lambda ()
		   (await (http-download url file :timeout timeout-ms)))
		 timeout-ms)))
    (elpa--http-check-status (nth 0 result) url not-found-message)
    result))

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

(defun elpa--consult-archive (archive)
  "Refresh ARCHIVE once per install operation and return its entries.
Resolution consults an archive lazily: the first consultation in one
install operation refreshes it; later consultations reuse that index."
  (let ((name (elpa--archive-name archive)))
    (unless (member name elpa--install-refreshed-archives)
      (elpa-refresh-contents archive)
      (setq elpa--install-refreshed-archives
	    (cons name elpa--install-refreshed-archives)))
    (cdr (assoc name elpa--archive-index-alist))))

(defun elpa-package-info (name &optional archive)
  "Return a plist describing package NAME in ARCHIVE, or nil.
The plist has :name, :version, :reqs, :summary, :kind, :extras, and
:archive keys.  The default archive is the first entry in
`elpa-archives'."
  (let* ((archive (or archive (caar elpa-archives)))
	 (entry (assoc (elpa--package-symbol name) (elpa--index archive))))
    (when entry
      (elpa--entry-info entry archive))))

(defun elpa--cache-index (archive entries source &optional copy-source-p)
  "Register parsed ENTRIES for ARCHIVE and cache the raw index.
SOURCE is the raw index text to write, or, when COPY-SOURCE-P is
non-nil, a file to copy byte-for-byte.  Return the number of entries.
The index is only cached after the version-1 gate has accepted it."
  (elpa--ensure-user-dir)
  (let ((cache (elpa--index-cache-file archive)))
    (make-directory (file-name-directory cache) t)
    (if copy-source-p
	(copy-file source cache t)
      (let ((coding-system-for-write 'binary))
	(with-temp-file cache (insert source)))))
  (setq elpa--archive-index-alist
	(cons (cons (elpa--archive-name archive) entries)
	      elpa--archive-index-alist))
  (length entries))

(defun elpa-refresh-contents (&optional archive)
  "Refresh ARCHIVE's index and return the number of packages it contains.
ARCHIVE defaults to the first entry in `elpa-archives'.  A location may
be a local absolute directory or an HTTP(S) URL.  An HTTP(S) index is
fetched through the async HTTP transport with a 60 s timeout, must
answer HTTP 200, is cached under `elpa-user-dir', and is parsed with
the version-1 gate."
  (interactive)
  (let* ((archive (or archive (caar elpa-archives)))
	 (base (elpa--archive-location archive)))
    (cond
     ((elpa--location-http-p base)
      (let* ((url (elpa--remote-url base "archive-contents"))
	     (result (elpa--http-get url elpa--index-timeout
				     (format "ELPA archive not found: %s" url)))
	     (body (nth 2 result)))
	(unless (stringp body)
	  (error "ELPA archive index fetch returned no body: %s" url))
	(elpa--cache-index archive (elpa--parse-index body) body)))
     ((file-name-absolute-p base)
      (let ((source (expand-file-name "archive-contents" base)))
	(unless (file-readable-p source)
	  (error "ELPA archive not found: %s" source))
	(elpa--cache-index archive
			   (elpa--parse-index (elpa--read-file source))
			   source 'copy-source)))
     (t
      (error "ELPA archive location scheme is not supported: %s" base)))))

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

(defun elpa--installed-builds (name)
  "Return the installed builds of NAME as a list of (VERSION . DIRECTORY).
VERSION is a version list.  A build is a non-symlink `<name>-<version>'
directory directly under `elpa-user-dir'."
  (let ((prefix (concat (symbol-name name) "-"))
	(builds nil))
    (when (file-directory-p elpa-user-dir)
      (dolist (file (directory-files elpa-user-dir t "\\`[^.]"))
	(let ((base (file-name-nondirectory file)))
	  (when (and (file-directory-p file)
		     (not (file-symlink-p file))
		     (string-match
		      (concat "\\`" (regexp-quote prefix)
			      "\\([0-9]+\\(?:\\.[0-9]+\\)*\\)\\'")
		      base))
	    (push (cons (version-to-list (match-string 1 base)) file)
		  builds)))))
    (nreverse builds)))

(defun elpa--newest-installed-build (name)
  "Return the newest installed build of NAME as (VERSION . DIRECTORY), or nil.
Versions are compared as version lists, not by directory order."
  (let (newest)
    (dolist (build (elpa--installed-builds name))
      (when (or (null newest)
		(version-list-< (car newest) (car build)))
	(setq newest build)))
    newest))

(defun elpa--find-installed-dir (name)
  "Return the directory of the newest installed version of NAME, or nil."
  (cdr (elpa--newest-installed-build name)))

(defun elpa--delete-old-builds (name version)
  "Delete installed builds of NAME strictly older than VERSION.
VERSION is a version list.  Builds newer than VERSION are kept, so an
archive whose index rolls back does not delete a newer build already on
disk.  Call only after the new build is published."
  (dolist (build (elpa--installed-builds name))
    (when (version-list-< (car build) version)
      (elpa--delete-tree (cdr build)))))

(defun elpa-installed-p (name &optional version)
  "Return non-nil if package NAME is installed.
If VERSION is non-nil, require that exact version."
  (let* ((name (elpa--package-symbol name))
	 (dir (if version
		  (elpa--package-dir name (elpa--version-list version))
		(elpa--find-installed-dir name))))
    (and dir (file-directory-p dir) t)))

(defun elpa--resolve (name version archive)
  "Resolve NAME at VERSION in ARCHIVE and return its package plist.
A non-nil VERSION must match the index version exactly."
  (let ((info (elpa-package-info name archive)))
    (unless info
      (error "Package `%s' is not available in archive %s"
	     name (elpa--archive-name archive)))
    (when version
      (let ((wanted (elpa--version-list version)))
	(unless (version-list-= wanted (plist-get info :version))
	  (error "Package `%s' version %s is no longer available in archive %s (found %s)"
		 name version (elpa--archive-name archive)
		 (elpa-version-join (plist-get info :version))))))
    info))

(defun elpa-install (name &optional version)
  "Install package NAME at VERSION from the first configured archive.
VERSION may be a version string or a list of integers and must match
the freshly read index version exactly.  Without VERSION, the version
the index currently advertises is installed.  NAME must be in
`elpa-installable-packages', which is checked before the archive index
is refreshed.  An older installed build is replaced by the new one;
the new build is published before the old build is deleted.  Return
NAME."
  (interactive
   (list (intern (read-string "Package: "))))
  (let* ((name (elpa--package-symbol name))
	 (archive (or (caar elpa-archives)
		      (error "`elpa-archives' is empty")))
	 (elpa--install-refreshed-archives nil)
	 (elpa--install-emacs-requirements nil))
    (elpa--ensure-allowlisted name)
    ;; Top-level installs keep milestone 1's refresh-before-install.
    (elpa-refresh-contents archive)
    (setq elpa--install-refreshed-archives
	  (list (elpa--archive-name archive)))
    (prog1 (elpa--install-1 name version archive)
      (elpa--warn-emacs-summary))))

(defun elpa--installed-at-least (name version)
  "Return non-nil if NAME is installed at VERSION or newer.
A nil VERSION means any installed version counts.  Used for
dependency satisfaction, where a requirement names a minimum."
  (let ((build (elpa--newest-installed-build name)))
    (and build
	 (or (null version)
	     (version-list-<= (elpa--version-list version) (car build))))))

(defun elpa--dependency-error (format-string &rest args)
  "Signal the unified dependency error described by FORMAT-STRING and ARGS.
Missing packages, unsatisfiable floors, and disallowed names all use
this one error; the message names the offending package and the
requirement that introduced it."
  (error "ELPA dependency error: %s" (apply 'format format-string args)))

(defun elpa--resolve-dependency (name version dependent)
  "Return the first entry for NAME satisfying floor VERSION.
Every archive in `elpa-archives' is searched in order, refreshing each
one the first time it is consulted in this install operation.  The
dependent's own archive no longer limits the search.  DEPENDENT
introduced the requirement and is named in the unified error."
  (let ((archives elpa-archives)
	(wanted (and version (elpa--version-list version)))
	found first)
    (while (and archives (not found))
      (let* ((archive (car (car archives)))
	     (entries (elpa--consult-archive archive))
	     (entry (assoc name entries)))
	(when entry
	  (let ((info (elpa--entry-info entry archive)))
	    (unless first
	      (setq first info))
	    (when (or (null wanted)
		      (version-list-<= wanted (plist-get info :version)))
	      (setq found info)))))
      (setq archives (cdr archives)))
    (cond
     (found found)
     (first
      (elpa--dependency-error
       "package `%s' required by `%s': version %s or newer is not available in archive %s (found %s)"
       name dependent version (plist-get first :archive)
       (elpa-version-join (plist-get first :version))))
     (t
      (elpa--dependency-error
       "package `%s' required by `%s' is not available in any configured archive"
       name dependent)))))

(defun elpa--plan-emacs-requirement (version dependent)
  "Record one ignored `emacs' requirement VERSION introduced by DEPENDENT."
  (setq elpa--install-emacs-requirements
	(cons (cons dependent version) elpa--install-emacs-requirements)))

(defun elpa--plan-package (info)
  "Add INFO and its requirements to the preflight plan.
Dependencies are planned before INFO, so `elpa--plan-order' is a
dependency-first install order."
  (let ((name (plist-get info :name))
	(node (list :name (plist-get info :name)
		    :version (plist-get info :version)
		    :info info
		    :archive (plist-get info :archive))))
    (setq elpa--plan-nodes (cons (cons name node) elpa--plan-nodes))
    (let ((elpa--plan-path (cons name elpa--plan-path)))
      (dolist (dependency (plist-get info :reqs))
	(elpa--require-dependency dependency name)))
    (setq elpa--plan-order (append elpa--plan-order (list node)))
    node))

(defun elpa--replan-package (node info)
  "Raise NODE to INFO and replan its requirements before it.
Used when a second requirement names a higher floor than the version
already selected for NODE."
  (let ((name (plist-get node :name)))
    (plist-put node :version (plist-get info :version))
    (plist-put node :info info)
    (plist-put node :archive (plist-get info :archive))
    (setq elpa--plan-order (delq node elpa--plan-order))
    (let ((elpa--plan-path (cons name elpa--plan-path)))
      (dolist (dependency (plist-get info :reqs))
	(elpa--require-dependency dependency name)))
    (setq elpa--plan-order (append elpa--plan-order (list node)))
    node))

(defun elpa--require-dependency (dependency dependent)
  "Plan DEPENDENCY of DEPENDENT, enforcing the allowlist and its floor.
A dependency version is a minimum.  Core-provided names are skipped,
`emacs' is collected for one summary warning, and cycles are warned and
floor-checked instead of recursing."
  (let ((name (elpa--package-symbol (car dependency)))
	(version (nth 1 dependency)))
    (cond
     ((eq name 'emacs)
      (elpa--plan-emacs-requirement version dependent)
      nil)
     ((memq name elpa-core-provided-packages)
      nil)
     ((not (memq name elpa-installable-packages))
      (elpa--dependency-error
       "package `%s' required by `%s' is not in elpa-installable-packages"
       name dependent))
     ((elpa--installed-at-least name version)
      nil)
     ((memq name elpa--plan-path)
      (let ((node (cdr (assq name elpa--plan-nodes))))
	(cond
	 ((or (null version)
	      (version-list-<= (elpa--version-list version)
			       (plist-get node :version)))
	  (warn "ELPA: dependency cycle on `%s' (required by `%s'); using planned version %s"
		name dependent
		(elpa-version-join (plist-get node :version)))
	  node)
	 (t
	  (elpa--dependency-error
	   "package `%s' required by `%s': version %s or newer conflicts with the cycle member's version %s"
	   name dependent version
	   (elpa-version-join (plist-get node :version)))))))
     ((assq name elpa--plan-nodes)
      (let ((node (cdr (assq name elpa--plan-nodes))))
	(if (or (null version)
		(version-list-<= (elpa--version-list version)
				 (plist-get node :version)))
	    node
	  (elpa--replan-package
	   node (elpa--resolve-dependency name version dependent)))))
     (t
      (elpa--plan-package
       (elpa--resolve-dependency name version dependent))))))

(defun elpa--install-plan-node (node)
  "Install one preflighted plan NODE."
  (let ((name (plist-get node :name))
	(version (plist-get node :version)))
    (unless (elpa-installed-p name version)
      (elpa--install-artifact (plist-get node :info)
			      (plist-get node :archive)))))

(defun elpa--warn-emacs-summary ()
  "Warn once about every ignored `emacs' requirement in this operation."
  (when elpa--install-emacs-requirements
    (warn "ELPA: ignoring emacs requirement%s for %s"
	  (if (cdr elpa--install-emacs-requirements) "s" "")
	  (mapconcat
	   (lambda (requirement)
	     (format "%S (from %s)"
		     (or (cdr requirement) 'any-version)
		     (car requirement)))
	   (nreverse elpa--install-emacs-requirements)
	   ", "))))

(defun elpa--install-1 (name version archive)
  "Install NAME at VERSION from ARCHIVE after preflighting its closure.
VERSION is exact, defaulting to the version ARCHIVE's index currently
advertises.  The whole required closure is checked before anything
installs; dependencies then install in order.  When the advertised
build is already installed, the download is skipped and any older
builds are deleted."
  (let ((info (elpa--resolve name version archive)))
    (if (elpa-installed-p name (plist-get info :version))
	;; The advertised build is already on disk.  Skip the download
	;; but collapse any older builds.
	(progn
	  (elpa--delete-old-builds name (plist-get info :version))
	  name)
      (let ((elpa--plan-nodes nil)
	    (elpa--plan-order nil)
	    (elpa--plan-path nil))
	(elpa--plan-package info)
	;; Preflight is green: install dependencies in plan order.
	(dolist (node elpa--plan-order)
	  (elpa--install-plan-node node))
	name))))

(defun elpa--artifact-name (name version)
  "Return the artifact base name for package NAME at VERSION."
  (concat (symbol-name name) "-" (elpa-version-join version)))

(defun elpa--check-regular-file (artifact)
  "Signal an error unless ARTIFACT exists and is a regular file."
  (unless (file-regular-p artifact)
    (error "Package artifact is not a regular file: %s" artifact)))

(defun elpa--check-tar-exit (status artifact what)
  "Signal an error unless STATUS, tar's exit code for ARTIFACT, is zero.
WHAT names the tar operation in the error message."
  (unless (and (integerp status) (zerop status))
    (error "Failed to %s package artifact %s (tar exit %s)"
	   what artifact status)))

(defun elpa--acquire-artifact (base artifact-base extension staging)
  "Copy or download the artifact ARTIFACT-BASE EXTENSION from BASE.
STAGING is the directory the artifact lands in.  BASE is an archive
location: an HTTP(S) URL or a local absolute directory.  Return the
staged artifact's absolute file name.  Over HTTP(S) the bytes are
written straight into staging by C and never round-trip through a
Lisp string."
  (let ((artifact (expand-file-name (concat artifact-base extension)
				    staging)))
    (cond ((elpa--location-http-p base)
	   (let ((url (elpa--remote-url base
					(concat artifact-base extension))))
	     (elpa--http-download url artifact elpa--artifact-timeout
				  (format "Package artifact not found: %s"
					  url))))
	  ((file-name-absolute-p base)
	   (let ((source (expand-file-name
			  (concat artifact-base extension) base)))
	     (unless (file-readable-p source)
	       (error "Package artifact not found: %s" source))
	     (copy-file source artifact)))
	  (t
	   (error "ELPA archive location scheme is not supported: %s"
		  base)))
    (elpa--check-regular-file artifact)
    artifact))

(defun elpa--install-artifact (info archive)
  "Download, validate, stage, and publish package INFO from ARCHIVE.
An older build of the same package is deleted only after the new build
has been published."
  (let* ((name (plist-get info :name))
	 (version (plist-get info :version))
	 (kind (plist-get info :kind))
	 (base (elpa--archive-location (plist-get info :archive)))
	 (artifact-base (elpa--artifact-name name version))
	 (staging-root (expand-file-name ".staging/" elpa-user-dir))
	 (staging (expand-file-name artifact-base staging-root))
	 (published (elpa--package-dir name version)))
    (unless (memq kind '(tar single))
      (error "Unsupported package kind `%s' for `%s'" kind name))
    (when (file-exists-p published)
      (error "Package `%s' is already installed in %s" name published))
    (elpa--ensure-user-dir)
    (when (file-exists-p staging)
      (elpa--delete-tree staging))
    (make-directory staging t)
    (unwind-protect
	(let* ((extension (if (eq kind 'tar) ".tar" ".el"))
	       (artifact (elpa--acquire-artifact base artifact-base
						 extension staging)))
	  (let ((content (if (eq kind 'tar)
				     (elpa--stage-tar info artifact staging)
				   (elpa--stage-single info artifact staging))))
	    (when (eq kind 'tar)
	      (elpa--validate-pkg-file content name version))
	    (elpa--remove-shipped-autoloads content name)
	    (elpa--generate-autoloads content name version)
	    (elpa--byte-compile-content content)
	    (when (file-exists-p published)
	      (error "Package `%s' is already installed in %s"
			     name published))
	    (rename-file content published)
	    (elpa--delete-old-builds name version)))
      (ignore-errors (elpa--delete-tree staging))
      (when (and (file-directory-p staging-root)
		 (= 2 (length (directory-files staging-root))))
	(ignore-errors (delete-directory staging-root))))))

(defun elpa--stage-tar (info artifact staging)
  "Extract INFO's tar ARTIFACT, already staged, into STAGING.
Validate every member before extraction.  Return the staged content
directory; the extracted artifact remains under STAGING until the
caller removes it."
  (let* ((name (plist-get info :name))
	 (version (plist-get info :version))
	 (artifact-base (elpa--artifact-name name version)))
    (elpa--check-regular-file artifact)
    (elpa--validate-tar artifact name version)
    (let ((status (call-process "tar" nil nil nil
				"-xf" artifact "-C" staging)))
      (elpa--check-tar-exit status artifact "extract"))
    (let ((content (expand-file-name artifact-base staging)))
      (unless (file-directory-p content)
	(error "Package artifact did not create %s/" artifact-base))
      content)))

(defun elpa--stage-single (info artifact staging)
  "Stage INFO's single-file ARTIFACT, already staged, into STAGING.
The file is written as `<name>.el' inside the staged
`<name>-<version>/' content directory, so the artifact itself does not
survive as a separate download.  Return the content directory."
  (let* ((name (plist-get info :name))
	 (version (plist-get info :version))
	 (artifact-base (elpa--artifact-name name version))
	 (content (expand-file-name artifact-base staging)))
    (elpa--check-regular-file artifact)
    (make-directory content t)
    (rename-file artifact
		 (expand-file-name (concat (symbol-name name) ".el")
					   content))
    (elpa--validate-single content name)
    content))

(defun elpa--validate-single (content name)
  "Verify the staged single-file library for NAME in CONTENT is sane.
The file must be non-empty and look like the named library: it must
carry a `;;; <name>.el' header comment or provide NAME."
  (let ((file (expand-file-name (concat (symbol-name name) ".el") content))
	(library (symbol-name name)))
    (with-temp-buffer
      (insert-file-contents file)
      (when (zerop (buffer-size))
	(error "Package artifact is empty: %s" file))
      (goto-char (point-min))
      (unless (or (looking-at
		   (concat ";;;[ \t]*" (regexp-quote library) "\\.el"))
		  (re-search-forward
		   (concat "^(provide[ \t\n]+'"
			   (regexp-quote library)
			   "[ \t\n)]")
		   nil t))
	(error "Package artifact %s does not look like library `%s'"
	       file library)))))

(defun elpa--tar-listing (artifact &rest args)
  "Run tar with ARGS and ARTIFACT and return the output as a string."
  (with-temp-buffer
    (let ((coding-system-for-read 'raw-text)
	  (status (apply 'call-process "tar" nil t nil
			 (append args (list artifact)))))
      (elpa--check-tar-exit status artifact "inspect")
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

;;; Autoload generation and byte-compilation

(defconst elpa--generate-autoload-cookie ";;;###autoload"
  "Cookie marking the following top-level form for autoload generation.")

(defun elpa--unquote (object)
  "Return OBJECT without a surrounding quote form, if any."
  (if (and (consp object) (eq (car object) 'quote))
      (nth 1 object)
    object))

(defun elpa--form-contains-p (form symbol)
  "Return non-nil if FORM contains SYMBOL anywhere as a cons cell."
  (cond ((eq form symbol) t)
	((consp form)
	 (or (elpa--form-contains-p (car form) symbol)
	     (elpa--form-contains-p (cdr form) symbol)))
	(t nil)))

(defun elpa--autoload-form (form library)
  "Return the autoload form for top-level FORM in LIBRARY, or nil.
Only definitions whose names can be autoloaded are handled.  This is a
minimal fallback for `make-autoload', which does not handle `defalias'
cookies such as the one in the queue package."
  (let ((operator (car-safe form)))
    (cond
     ((memq operator '(defun defun* defmacro defmacro* defsubst))
      (let ((name (elpa--unquote (nth 1 form)))
	    (doc (nth 3 form)))
	(when (symbolp name)
	  (list 'autoload (list 'quote name) library
		(and (stringp doc) doc)
		(and (elpa--form-contains-p (nthcdr 2 form) 'interactive) t)
		(if (memq operator '(defmacro defmacro*))
		    (list 'quote 'macro)
		  nil)))))
     ((eq operator 'defalias)
      (let ((name (elpa--unquote (nth 1 form)))
	    (doc (nth 3 form)))
	(when (symbolp name)
	  (list 'autoload (list 'quote name) library
		(and (stringp doc) doc)
		(and (elpa--form-contains-p (nthcdr 2 form) 'interactive) t)
		nil))))
     ((memq operator '(defvar defconst))
      form)
     ((eq operator 'defcustom)
      (list 'defvar (nth 1 form) (nth 2 form) (nth 3 form)))
     ((memq operator '(define-minor-mode easy-mmode-define-minor-mode))
      (let ((name (elpa--unquote (nth 1 form)))
	    (doc (nth 2 form)))
	(when (symbolp name)
	  (list 'autoload (list 'quote name) library
		(and (stringp doc) doc) t nil))))
     ((eq operator 'define-derived-mode)
      (let ((name (elpa--unquote (nth 1 form)))
	    (doc (nth 4 form)))
	(when (symbolp name)
	  (list 'autoload (list 'quote name) library
		(and (stringp doc) doc) t nil))))
     ((eq operator 'define-generic-mode)
      (let ((name (elpa--unquote (nth 1 form)))
	    (doc (nth 7 form)))
	(when (symbolp name)
	  (list 'autoload (list 'quote name) library
		(and (stringp doc) doc) t nil))))
     (t nil))))

(defun elpa--scan-autoloads (file library)
  "Return a list of autoload forms for the cookies in FILE.
LIBRARY is the load name recorded in each autoload form."
  (with-temp-buffer
    (insert-file-contents file)
    (emacs-lisp-mode)
    (goto-char (point-min))
    (let (forms)
      (while (re-search-forward
	      (concat "^" (regexp-quote elpa--generate-autoload-cookie)
		      "[ \t]*$")
	      nil t)
	(let ((form (condition-case nil
			(read (current-buffer))
		      (error nil))))
	  (when form
	    (let ((autoload (elpa--autoload-form form library)))
	      (when autoload
		(push autoload forms))))))
      (nreverse forms))))

(defun elpa--remove-shipped-autoloads (content name)
  "Delete any autoloads files shipped in CONTENT for package NAME.
Only the generated autoloads file may be retained."
  (let ((base (concat (symbol-name name) "-autoloads")))
    (dolist (extension '(".el" ".elc"))
      (let ((file (expand-file-name (concat base extension) content)))
	(when (file-exists-p file)
	  (delete-file file))))))

(defun elpa--generate-autoloads (content name version)
  "Write NAME's autoloads into the staged CONTENT directory.
The load-path form records the final, published directory for NAME at
VERSION, not CONTENT, because CONTENT is still under .staging/.
Return the autoloads file name."
  (let* ((library (symbol-name name))
	 (autoloads (expand-file-name (concat library "-autoloads.el")
				      content))
	 (published (directory-file-name (elpa--package-dir name version)))
	 (sources (sort (directory-files content t "\\`[^.].*\\.el\\'")
			'string<)))
    (with-temp-file autoloads
      (insert ";;; " library "-autoloads.el --- generated package autoloads\n"
	      "\n;;; Code:\n\n"
	      (format "(add-to-list 'load-path %S)\n" published))
      (dolist (source sources)
	(unless (string= source autoloads)
	  (let ((forms (elpa--scan-autoloads source library)))
	    (when forms
	      (insert (format "\n;;; Generated autoloads from %s\n\n"
			      (file-name-nondirectory source)))
	      (dolist (form forms)
		(insert (prin1-to-string form) "\n"))))))
      (insert "\n(provide '" library "-autoloads)\n"
	      "\n;;; " library "-autoloads.el ends here\n"))
    autoloads))

(defun elpa--byte-compile-content (content)
  "Byte-compile every .el file in CONTENT, including generated autoloads.
CONTENT is put on `load-path' while compiling.  Compiler warnings are
logged and tolerated; a compilation error signals an error."
  (let ((load-path (cons (directory-file-name content) load-path))
	(byte-compile-error-on-warn nil)
	(files (sort (directory-files content t "\\`[^.].*\\.el\\'")
		     'string<)))
    (dolist (file files)
      (let ((result (condition-case err
			(byte-compile-file file)
		      (error
		       (error "Byte-compilation failed for %s: %s"
			      file (error-message-string err))))))
	(unless result
	  (error "Byte-compilation failed for %s" file))))))

;;; Activation

(defvar elpa--activated-directories nil
  "Content directories already added to `load-path' by activation.")

(defvar elpa--loaded-autoloads nil
  "Autoloads files already loaded by activation.")

(defun elpa--autoloads-file (dir)
  "Return the autoloads file for installed content directory DIR, or nil.
DIR must have a name of the form `<name>-<version>'."
  (let ((name (file-name-nondirectory (directory-file-name dir))))
    (when (string-match "\\`\\(.+\\)-[0-9]+\\(\\.[0-9]+\\)*\\'" name)
      (let ((file (expand-file-name
		   (concat (match-string 1 name) "-autoloads.el")
		   dir)))
	(and (file-readable-p file) file)))))

(defun elpa-activate-all ()
  "Add installed package content directories to `load-path' and load autoloads.
Only immediate `<name>-<version>' subdirectories of `elpa-user-dir' that
contain `<name>-autoloads.el' are activated.  Cached archive indexes,
staging directories, unrelated entries, and symlinks are ignored.
The operation is idempotent; activated directories and loaded autoloads
files are recorded in `elpa--activated-directories' and
`elpa--loaded-autoloads'."
  (interactive)
  (when (file-directory-p elpa-user-dir)
    (dolist (entry (directory-files elpa-user-dir t "\\`[^.]"))
      (when (and (file-directory-p entry)
		 (not (file-symlink-p entry)))
	(let* ((dir (directory-file-name entry))
	       (autoloads (elpa--autoloads-file dir)))
	  (when autoloads
	    (unless (member dir elpa--activated-directories)
	      (unless (member dir load-path)
		(setq load-path (append load-path (list dir))))
	      (push dir elpa--activated-directories))
	    (unless (member autoloads elpa--loaded-autoloads)
	      (load (file-name-sans-extension autoloads) nil t)
	      (push autoloads elpa--loaded-autoloads)))))))
  nil)

(provide 'elpa)

;;; elpa.el ends here
