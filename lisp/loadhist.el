;;; loadhist.el --- lisp functions for working with feature groups

;; Copyright (C) 1995 Free Software Foundation, Inc.

;; Author: Eric S. Raymond <esr@snark.thyrsus.com>
;; Version: 1.0
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

;;; Synched up with: FSF 20.2.

;; #### Sync this file! 

;;; Commentary:

;; This file is dumped with XEmacs.

;; These functions exploit the load-history system variable.
;; Entry points include `unload-feature', `symbol-file', and `feature-file'.

;;; Code:

;; load-history is a list of entries that look like this:
;; ("outline" outline-regexp ... (require . wid-edit) ... (provide . outline) ...)

;; XEmacs; this function is in subr.el in GNU, and does not deal with
;; built-in symbols.
(defun* symbol-file (sym &optional type)
  "Return the input source from which SYM was loaded.
This is a file name, or nil if the source was a buffer with no associated file.

If TYPE is nil or omitted, any kind of definition is acceptable.
If TYPE is `defun', then function, subr, special operator or macro definitions
are acceptable.
If TYPE is `defvar', then variable definitions are acceptable.

`defface' specifies a face definition only, and for the moment, it won't
return faces created with `make-face' or `copy-face', just those created
with `defface' and `custom-declare-face'."
  (interactive "SFind source file for symbol: ") ; XEmacs
  (let (built-in-file autoload-cons symbol-details)
    (labels
        ((handle-built-in ()
           (when (setq built-in-file (built-in-symbol-file sym type))
             (if (equal built-in-file (file-truename built-in-file))
                 ;; Probably a full path name:
                 built-in-file
               ;; This is a bit heuristic, but shouldn't realistically be a
               ;; problem:
               (if (string-match-p #r"\.elc?$" built-in-file)
                   (concat (if (file-readable-p source-lisp)
                               source-lisp
                             lisp-directory)
                           built-in-file)
                 (concat source-directory "/src/" built-in-file)))))
         (handle-module-file (entry)
           (when (equal (cadr entry) '(module))
             (return-from symbol-file (handle-built-in)))))
      (cond ((and (eq 'autoload
                      (car-safe
                       (setq autoload-cons
                             (and (fboundp sym) (symbol-function sym)))))
                  (or (and (or (null type) (eq 'defvar type))
                           (eq (fifth autoload-cons) 'keymap))
                      (and (or (null type) (eq 'defun type))
                           (memq (fifth autoload-cons) '(nil macro)))))
             (return-from symbol-file (locate-library (second autoload-cons))))
            ((eq 'defvar type)
             ;; Load history entries corresponding to variables are just
             ;; symbols.
             (dolist (entry load-history)
               (when (memq sym (cdr entry))
                 (handle-module-file entry)
                 (return-from symbol-file (car entry)))))
            ((not (null type))
             ;; Non-variables have the type stored as the car of the entry. 
             (dolist (entry load-history)
               (when (and (setq symbol-details (rassq sym (cdr entry)))
                          (eq type (car symbol-details)))
                 (if (equal (cadr entry) '(module))
                     (handle-module-file entry))
                 (return-from symbol-file (car entry)))))
            (t
             ;; If TYPE hasn't been specified, we need to check both for
             ;; variables and other symbols.
             (dolist (entry load-history)
               (when (or (memq sym (cdr entry))
                         (rassq sym (cdr entry)))
                 (if (equal (cadr entry) '(module))
                     (handle-module-file entry))
                 (return-from symbol-file (car entry))))))
      (handle-built-in))))

(defun feature-symbols (feature)
  "Return the file and list of symbols associated with a given FEATURE."
  (let ((pair `(provide . ,feature)))
    (dolist (entry load-history)
      (when (member pair (cdr entry))
	(return entry)))))

(defun feature-file (feature)
  "Return the file name from which a given FEATURE was loaded.
Actually, return the load argument, if any; this is sometimes the name of a
Lisp file without an extension.  If the feature came from an eval-buffer on
a buffer with no associated file, or an eval-region, return nil."
  (unless (featurep feature)
    (error 'invalid-argument "Not a currently loaded feature" feature))
  (car (feature-symbols feature)))

(defun file-symbols (file)
  "Return the file and list of symbols associated with FILE.
The file name in the returned list is the string used to load the file,
and may not be the same string as FILE, but it will be equivalent."
  (let ((test (if (file-system-ignore-case-p file) #'equalp #'equal)))
    (or (assoc* file load-history :test test)
        (assoc* (file-name-sans-extension file) load-history :test test)
        (assoc* (concat file ".el") load-history :test test)
        (assoc* (concat file ".elc") load-history :test test))))

(defun file-provides (file)
  "Return the list of features provided by FILE."
  (let ((provides nil))
    (dolist (x (cdr (file-symbols file)))
      (when (eq (car-safe x) 'provide)
	(push (cdr x) provides)))
    provides))

(defun file-requires (file)
  "Return the list of features required by FILE."
  (let ((requires nil))
    (dolist (x (cdr (file-symbols file)))
      (when (eq (car-safe x) 'require)
	(push (cdr x) requires)))
    requires))

(defun file-dependents (file)
  "Return the list of loaded libraries that depend on FILE.
This can include FILE itself."
  (let ((provides (file-provides file))
	(dependents nil))
    (dolist (entry load-history)
      (dolist (x (cdr entry))
	(when (and (eq (car-safe x) 'require)
		   (memq (cdr-safe x) provides))
	  (push (car entry) dependents))))
    dependents))

;; FSFmacs
;(defun read-feature (prompt)
;  "Read a feature name \(string\) from the minibuffer,
;prompting with PROMPT and completing from `features', and
;return the feature \(symbol\)."
;  (intern (completing-read prompt
;			   (mapcar #'(lambda (feature)
;			             (list (symbol-name feature)))
;				   features)
;			   nil t)))

;; ;;;###autoload
(defun unload-feature (feature &optional force)
  "Unload the library that provided FEATURE, restoring all its autoloads.
If the feature is required by any other loaded code, and optional FORCE
is nil, raise an error."
  (interactive "SFeature: ")
  (unless (featurep feature)
    (error 'invalid-argument "Not a currently loaded feature" feature))
  (when (not force)
    (let* ((file (feature-file feature))
	   (dependents (remove* file (file-dependents file)
				:test (if default-file-system-ignore-case
					  #'equalp
					#'equal))))
      (when dependents
	(error 'invalid-state "Loaded libraries depend on feature"
	       dependents feature))))
  (let* ((flist (feature-symbols feature))
	 (file (car flist))
	 (unloading-module nil))
    (labels ((reset-aload (x)
               (let ((aload (get x 'autoload)))
                 (if aload (fset x (cons 'autoload aload))))))
      (mapc
       #'(lambda (x)
           (cond ((stringp x) nil)
                 ((consp x)
                  (case (car x)
                    (provide
                     ;; Remove any feature names that this file provided.
                     (setq features (delete* (cdr x) features)))
                    (module
                     (setq unloading-module t))
                    (defun
                     (when (fboundp (cdr x))
                       (fmakunbound (cdr x))
                       (reset-aload (cdr x))))))
                 ((boundp x)
                  (makunbound x))))
       (cdr flist)))
    ;; Delete the load-history element for this file.
    (setq load-history (delete* file load-history :key #'car :test 
                                (if (file-system-ignore-case-p file)
                                    #'equalp
                                  #'equal)))
    ;; If it is a module, really (attempt to) unload it.
    (if unloading-module (unload-module file))))

(provide 'loadhist)

;;; loadhist.el ends here
