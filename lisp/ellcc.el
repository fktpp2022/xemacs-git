#!/usr/bin/env xemacs-script
;;; ellcc.el - front-end for compiling Emacs modules
;; 
;; Copyright (C) 1998, 1999 J. Kean Johnston.
;; Copyright (C) 2002 Jerry James.
;; Copyright (C) 2026 Free Software Foundation
;;
;; Author: J. Kean Johnston (jkj@sco.com) (development of the file in C),
;; Aidan Kehoe (port to Lisp).  Please mail bugs and suggestions to the XEmacs
;; maintainer.
;;
;; Maintainer: XEmacs Development Team

;; Keywords: internal

;; This file is part of XEmacs.
;;
;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.
;;
;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Synched up with: Not in FSF

;;; Commentary:

;; This program is used to invoke the compiler, the linker and to generate the
;; module specific documentation and initialization code.  We assume we are in
;; 'compile' mode unless we encounter an argument which tells us that we're
;; not.  We take all arguments and pass them on directly to the compiler,
;; except for a few which are specific to this program:
;;
;;   --mode=VALUE      This sets the program mode. VALUE can be one of
;;                     compile, link, init or verbose.
;;   --mod-name=NAME   Sets the module name to the string NAME.
;;   --mod-title=TITLE Sets the module title to the string TITLE.
;;   --mod-version=VER Sets the module version to the string VER.
;;
;; The idea is that Makefiles will use ellcc as the compiler for making
;; dynamic Emacs modules, and life should be as simple as:
;;
;;   make CC=ellcc LD='ellcc --mode=link'
;;
;; The only additional requirement is an entry in the Makefile to produce the
;; module initialization file, which will usually be something along the lines
;; of:
;;
;;      modinit.c: $(SRCS)
;;              ellcc --mode=init --mod-name=\"$(MODNAME)\" \
;;              --mod-title=\"$(MODTITLE)\" --mod-version=\"$(MODVERSION)\" \
;;              -o $@ $(SRCS)
;;
;; See the samples for more details.

(require 'config)

(let ((ellcc-cc            (config-value 'XEMACS_CC))
      (ellcc-cflags        (config-value 'XE_CFLAGS))
      (ellcc-ldflags       (config-value 'LDFLAGS))
      (ellcc-cf-all        (config-value 'c_switch_all))
      (ellcc-dll-cflags    (config-value 'dll_cflags))
      (ellcc-dll-ldflags   (config-value 'dll_ldflags))
      (ellcc-dll-post      (config-value 'dll_post))
      (ellcc-dll-ld        (config-value 'dll_ld))
      (ellcc-dll-ldo       (config-value 'dll_ldo))
      (ellcc-config        (config-value 'configuration))
      (ellcc-emacs-ver     (config-value 'version))
      (ellcc-archdir       (config-value 'ARCHLIBDIR))
      (ellcc-moddir        (config-value 'MODULEDIR))
      (ellcc-sitemods      (config-value 'SITEMODULEDIR))

      (verbose 0)
      (progname (file-name-sans-extension
		 (cadr (member "--script" command-line-args))))
      module-name module-title module-output mode ellcc-module-version)

  (labels
      ((prefixp (needle haystack)
	 (not (mismatch haystack needle :end1 (length needle))))

       (space-or-tab-p (character)
	 (or (eql character ?\x20) (eql character ?\t)))

       (split-into-command-line-args (string)
	 ;; Split STRING into a list of individual words separated by
	 ;; whitespace, taking quoting appropriate for the shell into
	 ;; account. Always return a list.  If STRING is entirely whitespace,
	 ;; return nil.
	 (let ((start 0) (length (length string)) list offset)
	   (when (> length 0)
	     (while (and (< start length)
			 (space-or-tab-p (aref string start)))
	       (incf start))
	     (while (< start length)
	       (setq offset start)
	       (while (and (< offset length)
			   (not (space-or-tab-p (aref string offset))))
		 (cond
		   ((and (eql (aref string offset) ?\\) (< (1+ offset) length))
		    (incf offset 2))
		   ((eql (aref string offset) ?')
		    (incf offset)
		    (while (and (< offset length)
				(not (eql (aref string offset) ?')))
		      (if (and (< (1+ offset) length)
			       (eql (aref string offset) ?\\))
			  (incf offset 2)
			(incf offset)))
		    (if (and (< offset length)
			     (eql (aref string offset) ?'))
			(incf offset)))
		   ((eql (aref string offset) ?\")
		    (incf offset)
		    (while (and (< offset length)
				(not (eql (aref string offset) ?\")))
		      (if (and (< (1+ offset) length)
			       (eql (aref string offset) ?\\))
			  (incf offset 2)
			(incf offset)))
		    (when (and (< offset length)
			       (eql (aref string offset) ?\"))
		      (incf offset)))
		   (t
		    (incf offset))))
	       (push (subseq string start offset) list)
	       (setq start offset)
	       ;; Omit trailing whitespace.
	       (while (and (< start length)
			   (space-or-tab-p (aref string start)))
		 (incf start)))
	     (nreverse list))))

       (standard-error-process-filter (process string)
	 (write-sequence string 'external-debugging-output))

       (standard-output-process-filter (process string)
	 (write-sequence string t))

       (call-process-and-exit (program &rest arguments)
	 ;; Call PROGRAM synchronously, passing ARGUMENTS as its command-line
	 ;; args.
	 ;; Print its standard error to standard error, print its standard
	 ;; output to standard output.
	 ;; Then exit XEmacs with the same exit value that the underlying
	 ;; program gave.
	 (when (> verbose 0)
	   (write-line (mapconcat #'identity arguments " ") t))
	 (let* ((stderr-buffer (generate-new-buffer " *exec-stderr*"))
		(stdout-buffer (generate-new-buffer " *exec-stdout*"))
		status process)
	   (setq status
		 (catch 'process
		   (setf process (apply #'start-process
					(concat "exec " program)
					(list stdout-buffer stderr-buffer)
					program arguments)
			 (process-filter process)
			 #'standard-output-process-filter
			 (process-stderr-filter process)
			 #'standard-error-process-filter
			 (process-sentinel process)
			 (lexical-let ((stderr-buffer stderr-buffer)
				       (stdout-buffer stdout-buffer))
			   #'(lambda (process message)
			       (when (member (process-status process)
					     '(signal exit))
				 (kill-buffer stderr-buffer)
				 (kill-buffer stdout-buffer)
				 (setf (process-sentinel process) nil)
				 (throw 'process
				   (if (eq (process-status process) 'exit)
				       (process-exit-status process)
				     message))))))
		   (while t (accept-process-output process))))
	   (if (fixnump status)
	       (kill-emacs status)
	     (format-into 'external-debugging-output "%s: %s\n"
			  (file-name-nondirectory load-file-name)
			  status)
	     (kill-emacs 1))))

       (fatal (&rest args)
	 (if noninteractive
	     (progn
	       (format-into 'external-debugging-output
			    "%s: fatal error: "
			      (file-name-nondirectory load-file-name))
	       (apply #'format-into 'external-debugging-output args)
	       (terpri 'external-debugging-output)
	       (kill-emacs 1))
	   (apply #'error args))))

    (declare (inline space-or-tab-p prefixp))

    (unless (featurep 'modules)
      (fatal "no support for modules in this XEmacs"))

    (dolist (elt (prog1
		     (cdr (member "--" command-line-args))
		   (setq command-line-args nil)))
      (cond
	((prefixp "--mode=" elt)
	 (when (and mode (not (equal elt "--mode=verbose")))
	   (fatal "more than one mode specified: %s, %s"
		  (subseq (symbol-name mode) 1)
		  (subseq elt (length "--mode="))))
	 (cond
	   ((not (mismatch elt "link" :start1 (length "--mode=")))
	    (setf mode :link))
	   ((not (mismatch elt "compile" :start1 (length "--mode=")))
	    (setf mode :compile))
	   ((not (mismatch elt "verbose" :start1 (length "--mode=")))
	    (incf verbose))
	   ((not (mismatch elt "init" :start1 (length "--mode=")))
	    (setf mode :init))
	   (t
	    (fatal "mode must be link, compile, init, or verbose: %s"
		   (subseq elt (length "--mode="))))))
	
	((equal elt "--mod-location")
	 (format-into t "%s\n" ellcc-moddir)
	 (kill-emacs 0))

	((equal elt "--mod-site-location")
	 (format-into t "%s\n" ellcc-sitemods)
	 (kill-emacs 0))
	
	((equal elt "--mod-archdir")
	 (format-into t "%s\n" ellcc-archdir)
	 (kill-emacs 0))

	((equal elt "--mod-config")
	 (format-into t "%s\n" ellcc-config)
	 (kill-emacs 0))

	((prefixp "--mod-name=" elt)
	 (setf module-name (subseq elt (length "--mod-name="))))

	((prefixp "--mod-title=" elt)
	 (setf module-title (subseq elt (length "--mod-title="))))

	((prefixp "--mod-version=" elt)
	 (setf ellcc-module-version (subseq elt (length "--mod-version="))))

	((prefixp "--mod-output=" elt)
	 (setf module-output (subseq elt (length "--mod-output="))))

	(t
	 (setq command-line-args (cons elt command-line-args)))))

    (setq command-line-args (nreverse command-line-args))

    (unless mode
      (setq mode (or (and (> (length progname) 2)
			  (cdr (assoc (subseq progname -2)
				      '(("cc" . :compile)
					("ld" . :link)
					("it" . :init)))))
		   :compile)))

    (when (> verbose 0)
      (format-into
       t "ellcc driver version %s for EMODULES version %s (%ld)\n"
       ellcc-emacs-ver module-version module-revision))

    (macrolet ((override (&rest arguments)
		 (cons 'setq
		       (loop for (var symbol) on arguments by #'cddr
			 nconc `(,symbol (or (getenv ,var) ,symbol))))))
      (override "ELLCC" ellcc-cc	  
		"ELLLD" ellcc-dll-ld	  
		"ELLCFLAGS" ellcc-cflags
		"ELLLDFLAGS" ellcc-ldflags
		"ELLDLLFLAGS" ellcc-dll-ldflags
		"ELLPICFLAGS" ellcc-dll-cflags))

    (when (>= verbose 2)
      (format-into t "              mode = %s\n" (subseq (symbol-name mode) 1))
      (format-into t "       module_name = \"%s\"\n" module-name)
      (format-into t "      module_title = \"%s\"\n" module-title)
      (format-into t "    module_version = \"%s\"\n" ellcc-module-version)
      (format-into t "                CC = %s\n" ellcc-cc)
      (format-into t "            CFLAGS = %s\n" ellcc-cflags)
      (format-into t "      CC PIC flags = %s\n" ellcc-dll-cflags)
      (format-into t "                LD = %s\n" ellcc-dll-ld)
      (format-into t "           LDFLAGS = %s\n" ellcc-dll-ldflags)
      (format-into t "      architecture = %s\n" ellcc-config)
      (format-into t " Include directory = %s/include\n" ellcc-archdir)
      (format-into t "\n"))
		
    (case mode
      (:compile
       ;; For compile mode, things are pretty straightforward. All we need to
       ;; do is build up the argument list and exec() it (or rather, use our
       ;; milquetoast equivalent, #'call-process-and-exit).
       (apply #'call-process-and-exit
	      (append
	       (mapcan #'split-into-command-line-args
		       (list ellcc-cc ellcc-cf-all ellcc-cflags
			     ellcc-dll-cflags))
	       (list* "-DPIC" "-DEMACS_MODULE" "-DXEMACS_MODULE" "-Dxemacs"
		     "-Demacs" command-line-args))))
      (:link
       ;; For link mode, things are a little bit more complicated. We need to
       ;; insert the linker commands first, replace any occurrence of
       ;; ELLSONAME with the desired output file name, insert the output
       ;; arguments, then all of the provided arguments, then the final post
       ;; arguments. Once all of this has been done, pass the command line arg
       ;; list to #'call-process-and-exit.

       (unless module-output
	 (fatal "must specify --mod-output when linking"))

       (apply #'call-process-and-exit
	      (append
	       (mapcan #'split-into-command-line-args
		       (list ellcc-dll-ld ellcc-ldflags
			     ellcc-dll-ldflags ellcc-dll-ldo module-output))
	       (reduce #'cons command-line-args :from-end t
		       :initial-value (split-into-command-line-args
				       ellcc-dll-post)
		       :key #'(lambda (string)
				(replace-in-string string "ELLSONAME"
						   module-output t))))))

      (:init
       ;; In init mode, things are a bit easier. We assume that the only things
       ;; passed on the command line are the names of source files which the
       ;; make-docfile.elc will be processing.  We prepare the output file with
       ;; the header information first, as make-docfile.elc will append to the
       ;; file by special dispensation.

       (unless module-output
	 (fatal "must specify --mod-output when creating init file"))
       (unless module-name
	 (fatal "must specify --mod-name when creating init file"))

       (let ((command-line-args
	      (list* invocation-name "--script" "make-docfile.elc"
		     "-E" module-output command-line-args))
	     (executing-kbd-macro t) ;; Silence the "Writing ..." message.
	     (load-never-display-messages t)
	     auto-mode-alist) ;; Don't load c-mode.

	 (when (> verbose 0)
	   (write-line (mapconcat #'identity command-line-args " ") t))

	 (with-temp-buffer
	   (format-into (current-buffer)
			"/* DO NOT EDIT - AUTOMATICALLY GENERATED */
#include <emodules.h>

#ifdef __cplusplus
extern \"C\" {
#endif

extern const long emodule_compiler;
extern const char *emodule_name, *emodule_version, *emodule_title, *emodule_coding;
extern void docs_of_%s (void);
#ifdef __cplusplus
}
#endif

const long emodule_compiler = %ld;
const char *emodule_name = \"%s\";
const char *emodule_version = \"%s\";
const char *emodule_title = \"%s\";

void docs_of_%s ()\n"   module-name module-revision module-name
			ellcc-module-version module-title module-name)
	   (write-file (expand-file-name module-output)))

	 (load "make-docfile.elc"))))))

;; ellcc.el ends here
