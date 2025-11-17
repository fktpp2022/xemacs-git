#!/usr/bin/env xemacs-script
;;; make-docfile.el --- Cache docstrings in external file.

;; Copyright (C) 1985, 1986, 1992-1995, 1997, 2025 Free Software Foundation,
;; Inc.
;; Copyright (C) 2002, 2003 Ben Wing.

;; Author: Initial commit was by Steve L Baur in 1997 with "unknown" documented
;; as the author. Extensively revised October 2025 by Aidan Kehoe, to avoid the
;; need to call out to the C make-docfile; nothing of that initial commit
;; remains. Other authors as documented in ChangeLog.

;; Maintainer: XEmacs Development Team

;; Keywords: internal

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

;;; Synched up with: Not in FSF

;;; Commentary:

;; In an XEmacs that is about to dump, examine the load history for file names,
;; functions and variables loaded. Write the file names for the functions and
;; variables, together with their doc strings, to ../lib-src/DOC, using the
;; escape-quoted encoding so that we can represent every XEmacs character
;; without problems. Also examine the C source files compiled for this XEmacs
;; and write file name information and doc strings for functions and variables
;; made available to Lisp with DEFUN(), DEFVAR_().

;; When called using --script, accept command line arguments to specify the
;; output file name, to specify that the output format is appropriate for
;; strings, file name information, and encoding information for XEmacs C
;; modules (see emodules.c), to append instead of truncating, to output to
;; standard output, to change directory, to include specific Lisp files. This
;; is all overkill but is compatible with the old C implementation that was in
;; ../lib-src/.

;; This file is loaded at dump time but it does not make any functions or
;; variables available, and so its program code and variables will be garbage
;; collected and not dumped. This was not practical when unexec was still
;; supported, since the garbage collected data remained in the dump file.

(symbol-macrolet
    ((output-args '("-o"    ; Specify DOC file name
                    "-a"    ; Append to DOC file
                    "-E"))  ; Append to a module source file.
     (C-whitespace " \t\n\v\f\r")
     ;; A little ridiculous, but this compiled file will be called from
     ;; loadup.el when dumping, the symbol-macrolet is here anyway, make it so
     ;; these symbols are not interned in the dumped XEmacs.  Wouldn't it be
     ;; nice to have lexical scope.
     (docfile #:docfile)    
     (C-files #:C-files)
     (docfile-out-of-date #:docfile-out-of-date)
     (source-src #:source-src)
     (build-src #:build-src)
     (build-obj #:build-obj)
     (defined-lisp-objects #:defined-lisp-objects)
     (define-lisp-object #:define-lisp-object)
     (unbound-marker '#:unbound-marker))

  (let*
      ;; We're interested in these directories and they are not provided by
      ;; loadup.el:
      ((source-src (file-name-as-directory
                    (expand-file-name "src" source-directory)))
       (build-src (file-name-as-directory
                   (expand-file-name "src" build-directory)))
       (build-obj (if (eq system-type 'windows-nt)
                      (expand-file-name "obj"
                                        (expand-file-name "nt"
                                                          build-directory))
                    build-src))

       (source-directory (file-name-as-directory source-directory))

       ;; We modify these, avoid corruption in our callers.
       (load-history (copy-alist load-history))
       (command-line-args (copy-list command-line-args))

       ;; Give a reasonable default for this local variable:
       (docfile (expand-file-name "DOC" (expand-file-name "lib-src"
                                                          build-directory)))

       ;; List of objects created with DEFINE.*LISP_OBJECT() encountered in C
       ;; files, used when processing module source files:
       defined-lisp-objects

       ;; Other local variables to this file:
       C-files docfile-out-of-date append modules
       ;; The garbage collection of #'with-temp-buffer was impacting the
       ;; performance of this file significantly, create one buffer and reuse
       ;; it with #'erase-buffer, #'kill-all-local-variables instead:
       (buffer (get-buffer-create " *make-docfile-temp*")))

    (labels
        ((usage ()
           (format-into t "Usage: %s [OPTION]... [FILE]...

Generate documentation strings and file name information for XEmacs functions
and variables, from the provided source files and from the internal load
history. Options as follows:


  -o FILENAME           Write output to FILENAME using the format understood
                        by XEmacs' internal documentation (see the
                        `documentation' function within XEmacs). If FILENAME
                        is the string \"-\", write to standard output. Delete
                        FILENAME before writing if it already exists.  Usually
                        only useful if you are dumping XEmacs as part of the
                        build process.
  -a FILENAME           Write output in the same way, but do not delete the 
                        existing file, rather, append to it.
  -E FILENAME           Write output using the format appropriate for XEmacs
                        modules (see `load-module' and the `modules'
                        subdirectory of the XEmacs source). Do not delete any
                        existing file, append to it in the same way as -a.  If
                        FILENAME is the string \"-\", write output to standard
                        output, and do not attempt to truncate.
  -d DIRECTORY          Set the working directory used to interpret relative
                        path names for following arguments. In contrast to the
                        other options, -d may be passed multiple times,
                        interspersed with source files to process.
  -i EXTRA-SOURCE       Load EXTRA-SOURCE, a Lisp file (byte-compiled or not), 
                        and add information from it to the load history to be
                        written to FILENAME. This may also be passed multiple
                        times.
                        
  -?, --help            Print this help message and exit.

Other arguments are interpreted as C or Lisp files depending on their
extension, and scanned or loaded as appropriate. If an argument starts with
`@', it is interpreted to specify a file name containing a list of further
file names used to pass more command line arguments than certain platforms
support.

If an argument ends in the extension `.obj' or `.o', %1$s examines
the corresponding C source file, checking `source-directory' rather
than `build-directory' if appropriate. "
                        (file-name-nondirectory load-file-name))
           (kill-emacs 0))

         (fatal (&rest args)
	   (if noninteractive
	       (progn
		 (format-into 'external-debugging-output
			      "%s: error: "
			      (file-name-nondirectory load-file-name))
		 (apply #'format-into 'external-debugging-output args)
		 (terpri 'external-debugging-output)
		 (kill-emacs 1))
	     (apply #'error args)))

         (sanitize-C-command-line-arg (arg)
           ;; If ARG is a C object file name, transform it to the
           ;; corresponding source file name for this XEmacs. Otherwise, it
           ;; should already be a C source file name.
           ;;
           ;; Expand it to a full path, then add it to `C-files' if
           ;; it is not already present.
           (if (string-match-p "\\.\\(?:obj\\|o\\)\\'" arg)
               (setq arg (expand-file-name
                          (file-name-nondirectory
                           (format "%.*s.c"
                                   (position ?. arg :from-end t) arg))
                          source-src))
             (setq arg (expand-file-name arg)))
           (unless (member arg C-files)
	     (if (and purify-flag (member "dump" command-line-args))
		 (when (file-exists-p arg)
		   (if (and (not docfile-out-of-date)
			    (file-newer-than-file-p arg docfile))
		       (let ((stream (make-string-output-stream)) used-codesys)
			 ;; Ignore files like dump-id.c or extw-Xlib.c when
			 ;; making a decision to rebuild DOC, by checking if
			 ;; they have any DEFUN(), DEFVAR_*(),
			 ;; DEFINE_LISP_OBJECT(). If they don't there's also no
			 ;; need to add them to C-files.
			 (declare (special used-codesys))
			 (scan-C-file stream arg nil 'used-codesys)
			 (when (> (length (get-output-stream-string stream)) 0)
			   (setq docfile-out-of-date t
				 C-files (cons arg C-files))))
		     (setq C-files (cons arg C-files))))
	       (setq C-files (cons arg C-files)))))

         (canonicalize-file-name-for-output (filename)
           ;; #### Revise this to make everything either relative to
           ;; source-directory, or absolute, after the initial commit is done,
           ;; and with support from #'symbol-file.
           (cond
             ((eql (search source-src filename) 0)
              (subseq filename (length source-src)))
             ((eql (search source-lisp filename) 0)
              (subseq filename (length source-lisp)))
             ((eql (search source-directory filename) 0)
              (subseq filename (length source-directory)))
             (t filename)))
           
         (response-file-as-list (arg)
           ;; Return contents of ARG as list of strings.
           ;;
           ;; ARG is assumed to start with `@', to be ignored. Strings
           ;; separated by space are return as a Lisp list.
           (with-current-buffer buffer
	     (kill-all-local-variables)
	     (erase-buffer)
             (insert-file-contents-internal (subseq arg 1))
             ;; Majorly grind up the response file.  Backslashes get doubled,
             ;; quotes around strings, get rid of pesky CR's and NL's, and put
             ;; parens around the whole thing so we have a valid list of
             ;; strings.
             (goto-char (point-max))
             (insert "\")")
             (goto-char (point-min))
             (insert "(\"")
             (while (search-forward "\\" nil t)
               (replace-match "\\\\" nil t))
             (goto-char (point-min))
             (while (search-forward "\n" nil t)
               (replace-match "" nil t))
             (goto-char (point-min))
             (while (search-forward "\r" nil t)
               (replace-match "" nil t))
             (goto-char (point-min))
             (while (search-forward " " nil t)
               (replace-match "\" \"" nil t))
             (goto-char (point-min))
             (read (current-buffer))))

	 (write-escaping-as-C-string (string stream)
	   (let (special (last 0))
	     (while (setq special
			  (position-if #'(lambda (character)
					   (memq character '(?\n ?\\ ?\")))
				       string :start last))
	       (write-sequence string stream
			       :start last :end special)
	       (case (aref string special)
		 (?\n
		  ;; Write an escaped C newline, an escaped C
		  ;; backslash, and then a literal newline for the
		  ;; sake of readability of the generated file.
		  (write-sequence "\\n\\\n" stream))
		 (?\"
		  (write-sequence "\\\"" stream))
		 (?\\
		  (write-sequence "\\\\" stream)))
	       (setf last (1+ special)))
	     (write-sequence string stream :start last)))

	 (needs-quoting-for-DOC (character)
	   (memq character '(?\037 ?\001)))

	 (quote-for-DOC (string)
	   ;; ?\037 within DOC needs to be quoted, otherwise it is interpreted
	   ;; as the end of the docstring (or file name). It is quoted by
	   ;; ?\001; which means that ?\001 needs itself to be quoted if
	   ;; encountered.
	   (let ((position
		  ;; Messier than (position-if #'needs-quoting-for-DOC
		  ;; ...) but much cheaper given no need to call Lisp
		  ;; for each character in the string, and the vast
		  ;; majority of strings do not need this quoting:
		  (or (position ?\037 string) (position ?\001 string))))
	     (if position
		 (let ((stream (make-string-output-stream))
		       (last 0))
		   (while (setq position
				(position-if #'needs-quoting-for-DOC string
					     :start last))
		     (write-sequence string stream
				     :start last :end position)
		     (case (aref string position)
		       (?\037
			(write-sequence "\001_" stream))
		       (?\001
			(write-sequence "\001\001" stream)))
		     (setf last (1+ position)))
		   (write-sequence string stream :start last)
		   (get-output-stream-string stream))
	       string)))

         (scan-C-file (output-stream filename modules coding-system)
           ;; Parse documentation strings and arglist information in FILENAME
           ;; and write it to OUTPUT-STREAM.
           ;;
           ;; If MODULES is non-nil, do this in a way that is compatible with
           ;; the XEmacs modules implementation (see `load-module') otherwise
           ;; do it in a way appropriate for the internal doc file. See
           ;; `internal-doc-file-name' and `documentation.'
           ;;
           ;; CODING-SYSTEM must be a non-nil symbol; its value is set to the
           ;; value of buffer-file-coding-system after loading FILENAME into a
           ;; buffer.
           (let (char-after name type commas min-args max-args print-gensym)
             (labels
                 ((next-char ()
                    (prog1 (setq char-after (char-after))
                      (or char-after (return-from scan-C-file))))
		  (whitespacep (character)
		    (memq character '(?\x20 ?\t ?\n)))
		  (write-C-docstring (string stream &optional quote)
		    (let* ((string (if quote (quote-for-DOC string) string))
			   (last (or (position-if-not #'whitespacep string) 0))
			   backslash)
                      (while (setq backslash (position ?\\ string
                                                       :start last))
                        (write-sequence string stream
                                        :start last :end backslash)
			(case (elt string (1+ backslash))
			  (?\n
			   ;; Following newline, to be ignored.
			   (incf backslash))
			  (?t
			   ;; Tab escape.
			   (write-char ?\t stream)
			   (incf backslash))
			  (?\\
			   ;; Really, write a backslash.
			   (write-char ?\\ stream)
			   (incf backslash)))
                        (setf last (1+ backslash)))
                      (write-sequence string stream :start last
				      :end (let ((last
						  (position-if-not
						   #'whitespacep
						   string :start last
						   :from-end t)))
					     (if last (1+ last))))))

                  (read-C-string ()
                    ;; Handle the feature of C that literal strings
                    ;; separated by whitespace are concatenated.
                    (let (string)
                      (while (eql (next-char) ?\")
                        (setq string (concat string (read (current-buffer))))
                        (skip-chars-forward C-whitespace))
                      string))
                  (read-C-arglist (min-args max-args)
                    (let ((list
                           (mapcan
                            #'(lambda (elt)
                                (unless (or
					 (equal "UNUSED" elt)
					 (not
					  (mismatch elt "USED_IF"
						    :end1 (length "USED_IF"))))
				  (list
				   (make-symbol
				    (nsubstitute
				     ?- ?_
				     (if (eql (aref elt (1- (length elt))) ?_)
					 (subseq elt 0 -1)
				       elt))))))
                            (split-string
                             (substitute-if
			      ?\x20
			      #'(lambda (character) (memq character
							  '(?\, ?\( ?\))))
			      (upcase
			       (buffer-substring (prog1 (point) (forward-sexp))
						 (point)
						 (current-buffer))))))))
                      (when (> max-args min-args)
                        (if (> min-args 0)
                            (setcdr (nthcdr (1- min-args) list)
                                    (cons '&optional
                                          (nthcdr min-args list)))
                          (setq list (cons '&optional list))))
                      list)))
               (declare (inline next-char))
               (with-current-buffer buffer
		 (kill-all-local-variables)
		 (erase-buffer)
                 (insert-file-contents filename)
                 (set coding-system buffer-file-coding-system)
                 (setq filename (canonicalize-file-name-for-output filename))
                 (while
                     (re-search-forward
                      "\n\\(?:DEFUN\\|\
 +\\(?:DEFVAR\\|DEFINE[A-Z_]+LISP_OBJECT\\)\\)"
                      nil t)
                   (case (char-before (point))
                     (?N
                      (setq type 'defun
                            commas 4))
                     (?R
                      (setq type 'defvar
                            commas 1))
                     (?T
                      (setq type 'define-lisp-object
                            commas 1))
                     (otherwise
                      (fatal "regular expression matched, not understood: %s"
                             (match-string 0))))
                   (skip-chars-forward "^(")
                   (forward-char) ;; Discard the ?\(.
                   (next-char)
                   (when (eql char-after ?\")
                     ;; Ignore matches that don't have a string as the first
                     ;; arg to the macro.
                     (setq name (read-C-string))
                     (while (> commas 0)
                       (skip-chars-forward "^,")
                       (next-char)
                       (decf commas)
                       (forward-char)
                       (when (and (eq type 'defun) (<= 1 commas 2))
                         (skip-chars-forward C-whitespace)
                         (if (eql commas 2)
                             (progn
                               (looking-at "\\([0-9]+\\)")
                               (setq min-args (parse-integer
                                               (match-string 1))))
                           (if (looking-at "\\([0-9]+\\)")
                               (setq max-args (parse-integer
                                               (match-string 1)))
                             ;; MANY, KEYWORDS, etc.
                             (setq max-args nil))))
                       (forward-char))
                     (skip-chars-forward C-whitespace)
                     (next-char)
                     (when (eql char-after ?\")
                       ;; Discard the interactive spec.
                       (read-C-string))
                     (case type
                       (define-lisp-object
			(push (buffer-substring
			       (point) (progn
					 (skip-chars-forward "A-Za-z0-9_")
					 (point)))
			      defined-lisp-objects))
                       ((defun defvar)
                        (skip-chars-forward "^/()")
                        (next-char)
                        (unless (eql char-after ?/)
			  (fatal "missing doc string for C %s: %s" type name))
                        (if modules
                            (format-into output-stream
                                         "  CDOC%s (\"%s\", \"\\\n"
                                         (if (eq type 'defvar) "SYM" "SUBR")
                                         name)
                          (format-into output-stream "\037S%s\n\037%c%s\n"
                                       filename (if (eq type 'defvar) ?V ?F)
                                       name))
                        (forward-char)
                        (skip-chars-forward "*")
                        (if modules
                            (write-escaping-as-C-string
			     (let ((stream (make-string-output-stream)))
			       (write-C-docstring
				(buffer-substring
				 (point)
				 (progn (search-forward "*/" nil t)
					(backward-char (length "*/"))
					(point)))
				stream)
			       (get-output-stream-string stream))
                             output-stream)
                          (write-C-docstring
                           (buffer-substring
                            (point)
                            (progn (search-forward "*/" nil t)
                                   (backward-char (length "*/"))
                                   (point)))
                           output-stream
			   t))
                        (when (and (eq type 'defun) max-args)
                          (if modules
                              (write-sequence "\\n\\\n\\n\\\n" output-stream)
                            (write-sequence "\n\n" output-stream))
                          (write-sequence "arguments: " output-stream)
                          (skip-chars-forward "^(")
			  (if (zerop max-args)
			      (progn
				(write-sequence "()" output-stream)
				(forward-sexp))
			    (prin1 (read-C-arglist min-args max-args)
				   output-stream))
			  (if modules
			      (write-sequence "\\n" output-stream)
			    (write-sequence "\n" output-stream)))
                        (when modules
                          (write-sequence "\", \"" output-stream)
                          (write-escaping-as-C-string filename output-stream)
                          (write-sequence "\");\n\n" output-stream))))))))))

         (weird-doc (symbol details type offset)
           (unless (equal details "duplicate")
             (format-into
              'external-debugging-output
              "Note: Strange doc (%s) for %s %S @ %d\n"
              details type symbol offset)))

         (Snarf-documentation (filename)
           ;; Find offsets of doc strings stored in FILENAME and record them
           ;; in function definitions, and store them as the fixnum value for
           ;; the `variable-documentation' property of variables.
           (with-current-buffer buffer
	     (kill-all-local-variables)
	     (erase-buffer)
             (let ((coding-system-for-read
                    ;; We are interested in the byte offset on disk, so load
                    ;; it as no-conversion-unix; the file is escape-quoted, so
                    ;; we use decode-coding-string when working out the name
                    ;; of the symbol.
                    'no-conversion-unix)
                   symbol offset user-variable-p old function cddr)
               (insert-file-contents filename)
               (setq internal-doc-file-name (file-name-nondirectory filename))
               (goto-char 1)
               (while (re-search-forward "\037[FV].*\n" nil t)
                 (setq symbol (intern-soft
                               (decode-coding-string
                                (buffer-substring (+ (match-beginning 0)
                                                     (length "\037F"))
                                                  (1- (point)))
                                'escape-quoted)
                               nil 0)
                       offset (1- (point))
                       user-variable-p (eql ?* (char-after (point))))
                 (when (symbolp symbol)
                   (case (char-after (+ (match-beginning 0) (length "\037")))
                     (?V
                      (setq old (get symbol 'variable-documentation 0))
                      (when (fixnump old)
                        (weird-doc symbol "duplicate" "variable" offset))
                      ;; In the case of duplicate doc file entries, always
                      ;; take the later one.
                      (put symbol 'variable-documentation
                           (if user-variable-p (- offset) offset)))
                     (?F
                      (setq function
                            (condition-case nil (indirect-function symbol)
                              (void-function unbound-marker)))
                      (if (eq (car-safe function) 'macro)
                          (setq function (cdr function)))
                      (unless (eq function unbound-marker)
                        (typecase function
                          (subr
                           (if (subr-documentation function)
                               (weird-doc symbol "duplicate" "subr" offset)
                             (set-subr-documentation function offset)))
                          (cons
                           (if (or (eq (car function) 'lambda)
                                   (eq (car function) 'autoload))
                               (progn
                                 ;; If it's a Lisp form, stick it in the form.
                                 (setq cddr (cddr function))
                                 (typecase (car-safe cddr)
                                   (string (setf (car cddr) offset))
                                   (fixnum
                                    (weird-doc symbol "duplicate" "function"
                                               offset)
                                    (setf (car cddr) offset))
                                   (otherwise
                                    (unless (consp cddr)
                                      (weird-doc symbol "not a cons"
                                                 "function" offset)))))
                             (weird-doc symbol "not lambda or autoload"
                                        "function" offset)))
                          (compiled-function
                           (setq old (compiled-function-documentation
                                      function))
                           (when (fixnump old)
                             (weird-doc symbol "duplicate" "bytecode"
                                        offset))
                           (unless (and (fboundp 'compiled-function-annotation)
                                        (not (stringp
                                              (compiled-function-annotation
                                               function)))
                                        ;; We don't want to set the
                                        ;; information for the alias as the
                                        ;; compiled function's details.
                                        (not (eq symbol
                                                 (compiled-function-annotation
                                                  function))))
                             (set-compiled-function-documentation
                              function offset))))))))))))

         (kludgily-ignore-lost-doc-p (symbol)
           ;; Don't warn about functions whose doc was lost because they were
	   ;; wrapped by advice-freeze.el.
           (and (> (length (symbol-name symbol)) (length "ad-Orig-"))
                (not (mismatch (symbol-name symbol) "ad-Orig-"
                               :end1 (length "ad-Orig-")))))

         (Verify-documentation ()
           ;; Make sure everything went well with Snarf-documentation.  Write
           ;; to stderr if not.
           (mapatoms
            #'(lambda (symbol)
                (let (offset symbol-function)
                  (when (fboundp symbol)
                    (if (eq 'macro (car-safe (symbol-function symbol)))
                        (setq symbol-function (cdr (symbol-function symbol)))
                      (setq symbol-function (symbol-function symbol)))
                    (case (type-of symbol-function)
                      (subr
                       (setq offset (subr-documentation symbol-function)))
                      ((keymap symbol)
                       (setq offset most-negative-fixnum))
                      (cons
                       (when (member* (car symbol-function)
                                      '(autoload lambda))
                         (setq offset (or (caddr symbol-function)
                                          most-negative-fixnum))))
                      (compiled-function
                       (setq offset (compiled-function-documentation
                                     symbol-function))))
                    (unless (or (fixnump offset)
                                (kludgily-ignore-lost-doc-p symbol))
                      (format-into 'external-debugging-output
                                   "Warning: doc lost for function %S.\n"
                                   symbol)))
                  (when (boundp symbol)
                    (unless (or (fixnump (get symbol 'variable-documentation))
                                (keywordp symbol)
                                (eq symbol nil) (eq symbol t)
                                (get symbol 'byte-obsolete-variable)
                                (get symbol 'byte-compatible-variable)
                                (variable-alias symbol)
                                (not (globally-boundp symbol)))
                      (format-into 'external-debugging-output
                                   "Warning: doc lost for variable %S.\n"
                                   symbol))))))))

      ;; Expand filenames in load-history to absolute pathnames with
      ;; extensions now; we check load-history for the expanded name for
      ;; handling the -i arg and we need the full absolute names when
      ;; determining if the doc file is out of date. And processing the load
      ;; history when generating the DOC file is simpler when expanded names
      ;; are present.
      (dolist (elt load-history)
        (let* ((filename (if (equal '(module) (cadr elt))
                             (locate-file (car elt) module-load-path
                                          module-extensions)
                           (locate-file (car elt) load-path
                                        '(".elc" ".el")))))
          (when filename (setcar elt filename))))

      (if (and purify-flag (member "dump" command-line-args))
          ;; Invocation directly from loadup.el when dumping, ignore the
          ;; command line args and just process the .c files corresponding to
          ;; the object files compiled, together with the Lisp load history:
          (progn
	    (when (member 'quick-build internal-error-checking)
	      ;; Reset below, avoid the stat(2) for all the relevant files.
	      (setq docfile-out-of-date t))
	    (dolist (arg (directory-files build-obj nil
					  "\\.\\(?:obj\\|o\\)\\'" nil t))
	      (sanitize-C-command-line-arg arg)))

        ;; Not being loaded from directly from loadup.el when dumping, examine
        ;; the command line args:
        (setq command-line-args
              (cddr (or (member "--script" command-line-args)
                        (fatal "invoke this program with --script"))))

        (unless command-line-args (usage))

        (while command-line-args
          (let ((arg (car command-line-args)))
            (cond
              ((member arg output-args)
               (when (intersection output-args (cdr command-line-args)
                                   :test #'equal)
                 (fatal "only one of -o, -a, -E allowed"))
               (setq modules (equal arg "-E")
                     append (or modules (equal arg "-a"))
                     docfile (if (equal "-" (cadr command-line-args))
                                 (progn 
                                   (when (and append (not modules))
                                     (fatal "-a - is nonsensical"))
                                   "-")
                               (expand-file-name (cadr command-line-args)))
                     docfile-out-of-date (or (equal docfile "-")
                                             (not (file-exists-p docfile))
                                             modules
                                             (file-exists-p
                                              (expand-file-name "NEEDTODUMP"
                                                                build-src)))
                     command-line-args (cddr command-line-args)))

              ((equal arg "-d")
               (cd (cadr command-line-args))
               ;; Passing "-d" multiple times is legitimate.
               (setq command-line-args (cddr command-line-args)))

              ((equal arg "-i")
               (let ((filename (or (locate-file (cadr command-line-args)
                                                load-path '(".elc" ".el"))
                                   (cadr command-line-args))))
                 (or (assoc filename load-history) (load filename)))
               (setq command-line-args (cddr command-line-args)))

              ((or (equal arg "-?") (equal arg "--help"))
               (usage))

              ((eql (elt arg 0) ?@)
               (setq command-line-args (nconc (response-file-as-list arg)
                                              (cdr command-line-args))))

              ((equal "NEEDTODUMP" arg)
               (setq command-line-args (cdr command-line-args)))

              ((string-match-p "\\.exe\\'" arg)
               (setq command-line-args (cdr command-line-args)))

              ((string-match-p arg "\\.elc?\\'")
               (let ((filename (or (locate-file arg load-path
                                                '(".elc" ".el" ""))
                                   arg)))
                 (or (assoc filename load-history) (load filename)))
               (setq command-line-args (cdr command-line-args)))

              (t
               (sanitize-C-command-line-arg arg)
               (setq command-line-args (cdr command-line-args)))))))

      (unless docfile-out-of-date
        (dolist (elt load-history) 
          (when (file-newer-than-file-p (car elt) docfile)
            (setq docfile-out-of-date t)
            ;; Break out of the dolist:
            (return))))

      (unless docfile-out-of-date
	(when (file-newer-than-file-p load-file-name docfile)
	  (setq docfile-out-of-date t)))

      (when (and purify-flag (member 'quick-build internal-error-checking)
		 (member "dump" command-line-args))
	(setq docfile-out-of-date (not (file-exists-p docfile))))

      (when docfile-out-of-date
	;; Not necessary, but makes comparing the output to that from
	;; make-docfile.c easier:
	(setq load-history (reverse load-history)
	      C-files (reverse C-files))
	(when (and purify-flag (member "dump" command-line-args)
		   (not C-files))
	  (fatal
	   "could not find any C files at all, check build-obj correct"))
        (let ((output-stream (if (equal docfile "-")
                                 t
                               (generate-new-buffer docfile)))
              documentation symbol used-codesys coding-systems)

          (unless (eq output-stream t)
            (if append
                (with-current-buffer output-stream
                  (let ((coding-system-for-read
                         (if modules
                             ;; This file has been generated by ellcc and that
                             ;; program does not generate any coding system
                             ;; information. There is no reason to think its
                             ;; output will have non-ASCII. We check for this
                             ;; further down, if this check fails ellcc needs
                             ;; to produce a coding system cookie in its
                             ;; output, and we need to respect that.
                             'no-conversion-unix
                           'escape-quoted)))
                    (insert-file-contents docfile)
                    (when modules
                      (goto-char (point-min))
                      (when (re-search-forward "[^\x00-\x7f]" nil t)
                        (fatal "generated module source `%s' has non-ASCII"
                               docfile)))
                    (goto-char (point-max))))
              (ignore-errors (delete-file docfile))))

          (if modules (write-sequence "{\n" output-stream))

          (dolist (file C-files)
            (scan-C-file output-stream file modules 'used-codesys)
            (push used-codesys coding-systems)
            (when (and modules (cdr (remove-duplicates coding-systems)))
              (fatal "conflicting coding systems in source files: %S"
                     coding-systems)))

          (if modules
              (progn
                (write-sequence "}\n\nconst char *emodule_coding = \""
				output-stream)
		(write-escaping-as-C-string (symbol-name (car coding-systems))
					    output-stream)
		(write-sequence "\";\n" output-stream)
                (dolist (object defined-lisp-objects)
		  (write-sequence "int lrecord_type_" output-stream)
		  (write-sequence object output-stream)
		  (write-sequence ";\n" output-stream)))
            (mapc
             (function*
              (lambda ((filename . rest))
               (setq filename (canonicalize-file-name-for-output filename))
               (dolist (elt rest)
                 (case (car-safe elt)
                   ((provide require module)) ;; Do nothing for now.
                   (defun
                    (setq symbol (cdr elt)
			  documentation
			  (and (fboundp symbol)
			       (not (symbolp (symbol-function symbol)))
			       (not (eq 'keymap
                                         (type-of
                                          (symbol-function symbol))))
			       (documentation symbol t)))
		    (write-sequence "\037S" output-stream)
		    (write-line (quote-for-DOC filename) output-stream)
		    (write-sequence "\037F" output-stream)
		    (write-line (quote-for-DOC (prin1-to-string symbol))
				output-stream)
		    (when documentation
		      (write-sequence (quote-for-DOC documentation)
				      output-stream)))
                   (t
                    (setq symbol elt
                        documentation
                        (and (boundp symbol)
                             (documentation-property
                              symbol 'variable-documentation t)))
		    (write-sequence "\037S" output-stream)
		    (write-line (quote-for-DOC filename) output-stream)
		    (write-sequence "\037V" output-stream)
		    (write-line (quote-for-DOC (prin1-to-string symbol))
				output-stream)
                    (when documentation
                      (write-sequence (quote-for-DOC documentation)
				      output-stream)))))))
             load-history))
	  
          (unless (eq t output-stream)
            (with-current-buffer output-stream
              (setq buffer-file-coding-system (if modules
                                                  (car coding-systems) 
                                                'escape-quoted)
                    buffer-file-name docfile)
              (save-buffer)
              (kill-buffer (current-buffer))))))

         (when (and purify-flag (member "dump" command-line-args))
           (message "Finding pointers to doc strings...")
           (Snarf-documentation docfile)
           (message "Finding pointers to doc strings...done")
           (Verify-documentation))
	 (kill-buffer buffer))))

;;; make-docfile.el ends here
