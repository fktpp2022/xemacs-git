;; loadup.el --- load up standardly loaded Lisp files for XEmacs.

;; Copyright (C) 1985, 1986, 1992, 1994, 1997 Free Software Foundation, Inc.
;; Copyright (C) 1996 Richard Mlynarik.
;; Copyright (C) 1995, 1996, 2003, 2005 Ben Wing.

;; Maintainer: XEmacs Development Team
;; Keywords: internal, dumped

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

;;; Synched up with: Last synched with FSF 19.30, with wild divergence since.

;;; Commentary:

;; If you are wanting to add files to be dumped into your local version of
;; XEmacs, DO NOT add them here.  Use site-init.el or site-load.el instead.

;; This is loaded into a bare XEmacs to make a dumpable one.

;;; Code:

;; Help debug problems.
(setq stack-trace-on-error t
      load-always-display-messages t)
(if (featurep 'debug-xemacs)
    ;; Immediately dump core upon an unhandled error, rather than just
    ;; quitting the program.  This can also be achieved by setting an
    ;; environment variable XEMACSDEBUG to contain '(setq
    ;; debug-on-error t)', e.g.  export XEMACSDEBUG='(setq
    ;; debug-on-error t)'
    (setq debug-on-error t))

(if (and purify-flag (member "run-temacs" command-line-args))
    (setq purify-flag nil))

;(format-into 'standard-output "command-line-args: %s\n" command-line-args)
;(format-into 'standard-output "configure-lisp-directory: %S\n" configure-lisp-directory)
;(format-into 'standard-output "configure-data-directory: %S\n" configure-data-directory)
;(format-into 'standard-output "lisp-directory: %S\n" lisp-directory)

(if (fboundp 'error) (error "loadup.el already loaded!"))

(defconst running-xemacs t "\
Non-nil when the current emacs is XEmacs.")

;; Can't make this constant for now because it causes an error in
;; update-elc.el. 
(defvar source-lisp (file-name-directory (expand-file-name (car (nthcdr 2 command-line-args)))) "\
Root of tree containing the Lisp source code for the current build. 
Differs from `lisp-directory' if this XEmacs has been installed. ")

(defconst build-directory (expand-file-name ".." invocation-directory) "\
Root of tree containing object files and executables produced by build. 
Differs from `source-directory' if configured with --srcdir option, a practice 
recommended for developers.")

(defconst source-directory (expand-file-name ".." (file-truename source-lisp))
  "Root of tree containing source code for the current build. 
Used during loadup and for documenting source of symbols defined in C.")

(defvar preloaded-file-list nil "\
List of Lisp files preloaded into the XEmacs binary image,
with the exception of `loadup.el'.")

;(start-profiling)

;; really-early-error-handler outputs a stack trace so let's not do it
;; twice.
(let ((stack-trace-on-error nil))
  ;; This is awfully damn early to be getting an error, right?
  (call-with-condition-handler #'really-early-error-handler
      #'(lambda ()
	  (setq load-path (nconc
			   (mapcar
			    #'file-name-as-directory
			    (directory-files source-lisp t "^[^-.]"
					     nil 'dirs-only))
			   (list
			    (file-name-as-directory
			     (expand-file-name "." source-lisp))))

		module-load-path (list (expand-file-name "modules"
							 build-directory))
		load-warn-when-source-only t) ; Set to nil at the end

	  ;; message not defined yet ...
	  (format-into 'external-debugging-output "\nUsing load-path %s"
		       load-path)
	  (format-into 'external-debugging-output "\nUsing module-load-path %s"
		       module-load-path)

	  (load (expand-file-name "dumped-lisp.el" source-lisp))

	  ;; No point attempting to detect a non-nil return value, that just
	  ;; errors in any event and #'really-early-error-handler is invoked.
	  (mapc #'load preloaded-file-list)

	  (packages-load-package-dumped-lisps late-package-load-path))))

;; Fix up the preloaded file list
(setq preloaded-file-list (mapcar #'file-name-sans-extension
				  preloaded-file-list)
      load-warn-when-source-only nil
      debugger 'debug)

(if (member "no-site-file" command-line-args) (setq site-start-file nil))

;; If you want additional libraries to be preloaded and their
;; doc strings kept in the DOC file rather than in core,
;; you may load them with a "site-load.el" file.
;; But you must also cause them to be scanned when the DOC file
;; is generated.  For MS Windows, you must edit ../nt/xemacs.mak.
;; For other systems, you must edit ../src/Makefile.in.in.
(load "site-load" t)

;; Note: You can cause additional libraries to be preloaded by writing a
;; site-init.el that loads them.  See also "site-load" above.
(when (stringp site-start-file)
  (load "site-init" t))

;; Add information from this file to the load history.  Clear
;; current-load-list; this (and adding information to load-history) is
;; normally done in lread.c after reading the entirety of a file, something
;; which never happens for loadup.el.
(setq load-history (cons (nreverse current-load-list) load-history)
      current-load-list nil)

;(stop-profiling)

(when (member "dump" command-line-args)
  ;; Generate DOC if it is out of date.
  (load "make-docfile")
  (message "Finding pointers to doc strings...")
  (Snarf-documentation "DOC")
  (message "Finding pointers to doc strings...done")
  (Verify-documentation)

  ;; String extent info is not dumpable, clear it from the match data now.
  (store-match-data
   (list (let ((extent (make-extent 0 6 "string")))
           ;; This property is a signal to the search code, at dump time and
           ;; only at dump time, to discard all string extent info that the
           ;; search code knows about:
           (set-extent-property extent 'search 'discard)
           extent)))

  ;; Delete information that is available from DOC for those files in
  ;; preloaded-file-list; in practice, this boils down to #'provide and
  ;; #'require calls, and variables without documentation. Yes, this is a bit
  ;; ugly.
  (setq load-history (delete*
                      nil
                      (mapc #'(lambda (element)
                                (delete* 'defun element :key #'car-safe)
                                (delete-if
                                 #'(lambda (elt)
                                     (and
                                      (symbolp elt)
                                      (get elt 'variable-documentation)))
                                 element))
                            load-history)
                      :key #'cdr))

  (message "Dumping under the name xemacs")
  (dump-emacs
   "xemacs"
   "temacs")
  (kill-emacs))

;; Avoid error if user loads some more libraries now.
(setq purify-flag nil)

(when (member "run-temacs" command-line-args)
  (message "\nBootstrapping from temacs...")
  ;; Remove all args up to and including "run-temacs"
  (apply #'run-emacs-from-temacs (cdr (member "run-temacs" command-line-args)))
  ;; run-emacs-from-temacs doesn't actually return anyway.
  (kill-emacs))

;; XEmacs change
;; If you are using 'recompile', then you should have used -l loadup-el.el
;; so that the .el files always get loaded (the .elc files may be out-of-
;; date or bad).
(when (member "recompile" command-line-args)
  (setq command-line-args-left (cdr (member "recompile" command-line-args)))
  (batch-byte-recompile-directory)
  (kill-emacs))

;; For machines with CANNOT_DUMP defined in config.h,
;; this file must be loaded each time Emacs is run.
;; So run the startup code now.

(unless (fboundp 'dump-emacs)
  ;; Avoid loading loadup.el a second time!
  (setq command-line-args (cdr (cdr command-line-args)))
  (eval top-level))

;;; loadup.el ends here
