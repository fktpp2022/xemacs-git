;;; compiled-unicode-tables.el --- Compiler and container for Unicode tables

;; Copyright (C) 2022 Jaakko Salomaa

;; Keywords: multilingual, Unicode

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

;;; Synched up with: Not in FSF.

;;; Commentary:

;; At compile time, harvest from the Lisp codebase all files that could
;; possibly be used in etc/unicode, dump them with
;; unicode.c:Fdump_unicode_mapping_table() and create a search table out of
;; them. unicode.c:Fload_unicode_mapping_table() will then call this file's
;; #'set-compiled-unicode-file-search-table to make the compiled data available
;; to it.

;; The format of the compiled table is a hash table, in which the keys are
;; filenames of the files under etc/unicode, and the values are vectors of
;; encoded vectors that represent columns of the Unicode mapping tables.

;; All the column vectors are delta encoded - that is, the very first value is
;; interpreted as such, and subsequent values are calculated by adding the
;; nth value to the (n - 1)th value. Thus a column of 0xA 0xF 0x13 would be
;; encoded as

;; [
;;    0: Type specifier for plain delta encoding without RLE
;;   10: First value, 0xA
;;    5: Delta value from previous, aka 0xF - 0xA
;;    4: Delta value from previous, aka 0x13 - 0xF
;; ]

;; The code will also calculate a run length encoded version of the column,
;; compare its length to save that, if it it shorter than the
;; plain delta encoded version. With both encodings, the first column for, say,
;; etc/unicode/unicode-consortium/VENDORS/MICSFT/PC/CP850.TXT, which is
;; just increasing values from 0 to 255, will be encoded as

;; [
;;    1: Type specifier, 1 for run length encoding
;;    0: First value literally
;;    1: Amount of deltas, always 1 for the first value
;;    1: Delta value from previous
;;  255: Amount of deltas, aka previous value + 1 repeated 255 times
;; ]

;; Each filename entry has 2 or 3 columns. If there are 3, columns 1 and 2 are
;; a range of codepoints from and to, that are mapped to column 3. If only 2,
;; the code will assume 1 to contain the singular from and to values.

;; The onus to creating this mechanism came from the licenses in etc/unicode
;; being very open to interpretation with regards to redistribution and me
;; not wanting to try to justify their inclusion in a compiled XEmacs binary
;; distribution.

;;; Code:

(eval-when-compile
;; Some of the files have an extra first column. Currently I know only of this
;; one, but if other ones pop up, update this. (It's used as a RE.)
(defvar unicode-ignore-first-column
  (concat
   (regexp-quote
    (mapconcat #'identity
               '("unicode" "unicode-consortium" "EASTASIA" "OBSOLETE" 
                 "JIS0208.TXT") (string directory-sep-char)))
   "$"))

(defun compile-unicode-file-search-table ()
  (let ((search-table (make-hash-table :test #'equal)))
    (dolist (unicode-file (harvest-unicode-files))
      (let ((dumped-table (dump-unicode-mapping-table
                           unicode-file (string-match-p
                                         unicode-ignore-first-column
                                         unicode-file))))
        (when (null dumped-table)
          (unicode-dump-error (concat "Parsed an invalid unicode mapping file "
                                      unicode-file)))
        (puthash unicode-file (optimize-dumped-table dumped-table)
                 search-table)))
    search-table))

(defun harvest-unicode-files ()
  (let ((lisp-path (find-if (lambda (p)
                              (file-exists-p
                               (expand-file-name
                                (format "mule%ccompiled-unicode-tables.el"
                                        directory-sep-char)
                                p)))
                            load-path)))
    (unless lisp-path (unicode-dump-error "Could not deduce base LISP path"))
    (setq lisp-path
          (subseq lisp-path 0
                  (string-match-p (concat (regexp-quote
                                           (string directory-sep-char))
                                          "+$")
                                  lisp-path)))
    ;; Go through lisp/unicode.el and lisp/mule/* - those are the files known
    ;; to have charset definitions.
    (let ((lisp-files
           (cons (expand-file-name "unicode.el" lisp-path)
                 (remove-if (apply-partially #'string-match-p
                                             "compiled-unicode-tables.el")
                            (directory-files
                             (expand-file-name "mule" lisp-path)
                             t "\\.el$" t t))))
          (buffer (generate-new-buffer " *unicode-file-concatenation*"))
          (combined-content nil)
          (all-unicode-strings nil)
          ;; This is a hash table just to uniq the entries easily.
          (file-mask-map (make-hash-table :test #'equal))
          (non-directory-sep-char-class
           (concat "[^" (regexp-quote (string directory-sep-char)) "]*")))
      (unwind-protect
          (with-current-buffer buffer
            ;; Slap all inspected files into a quoted list of
            ;; (("filename1" (file-contents1)) ("filename2" (contents2)) ...)
            (insert "(setq combined-content '(")
            (dolist (filename lisp-files)
              (insert (format "(%s (\n" (prin1-to-string filename)))
              (insert-file-contents filename)
              (goto-char (point-max buffer))
              (insert "\n))"))
            (insert "\n))")
            (eval-buffer buffer))
        (kill-buffer buffer))
      ;; Find all "unicode/..." strings.
      (dolist (file-elt combined-content)
        (destructuring-bind (filename content) file-elt
          (let (unicode-strings)
            (search-for-unicode-path-strings file-elt)
            (when unicode-strings
              (push (list (substring filename (1+ (length lisp-path)))
                          unicode-strings)
                    all-unicode-strings)))))
      ;; Construct regexps for filename matching out of the unicode strings.
      ;; Keep the filename and original string context for error handling.
      (dolist (str-elt all-unicode-strings)
        (destructuring-bind (filename unicode-strings) str-elt
          (dolist (unicode-string unicode-strings)
            (puthash (concat
                      (replace-in-string (regexp-quote unicode-string)
                                         "%[a-z]" non-directory-sep-char-class)
                     "$")
                     (list nil unicode-string filename) file-mask-map))))
      ;; Now, find all matched files...
      (let ((matched (match-all-unicode-files
                      (expand-file-name
                       (mapconcat #'identity '(".." "etc" "unicode")
                                  (string directory-sep-char))
                       lisp-path)))
            (mapping-errors nil))
        ;; ...but because this isn't a very sophisticated way of searching,
        ;; fail-fast at compile time in case somebody modified the Lisp code in
        ;; a way we don't currently handle.
        (maphash (lambda (unused error-handling-elt)
                   (unless (car error-handling-elt)
                     (push (list ", \"" (cadr error-handling-elt)
                                 "\" in " (caddr error-handling-elt))
                           mapping-errors)))
                 file-mask-map)
        (when mapping-errors
          (unicode-dump-error
           (apply #'concat (append
                            '("Could not find any matching files for the "
                              "following patterns in the following files: \"")
                            (cdr (apply #'append mapping-errors))))))
        (let ((etc-path-name-len (1+ (length (expand-file-name 
                                              (concat ".."
                                                      (list directory-sep-char)
                                                      "etc")
                                                               lisp-path)))))
          (mapcar (lambda (s) (substring s etc-path-name-len)) matched))))))

(defun search-for-unicode-path-strings (elt)
  (cond
   ((consp elt)
    (search-for-unicode-path-strings-in-possibly-malformed-list elt))
   ((and (stringp elt)
         (string-match-p "^unicode/" elt))
    (setq unicode-strings (cons (replace-in-string elt
                                                   (regexp-quote "/")
                                                   (string directory-sep-char)
                                                   t)
                                unicode-strings)))))

(defun search-for-unicode-path-strings-in-possibly-malformed-list (list)
  (while list
    (if (consp list)
        (progn
          (search-for-unicode-path-strings (car list))
          (setq list (cdr list)))
      (progn
        (search-for-unicode-path-strings list)
        (setq list nil)))))

(defun match-all-unicode-files (basedir)
  (apply #'append
         (mapcar (lambda (dir-ent)
                   (cond
                    ((string-match-p "\\(\\.\\.\\|\\.\\)$" dir-ent) nil)
                    ((file-directory-p dir-ent)
                     (match-all-unicode-files (expand-file-name dir-ent
                                                                basedir)))
                    (t (let (file-matched)
                         (maphash (lambda (matcher error-handling-elt)
                                    (when (string-match-p matcher dir-ent)
                                      (setq file-matched t)
                                      ;; Mark this matcher as having matched.
                                      (setcar error-handling-elt t)))
                                  file-mask-map)
                         (when file-matched (cons dir-ent nil))))))
                 (directory-files basedir t))))

(defun optimize-dumped-table (table)
  (let (col1 col2 col3)
    (dolist (elt table)
      (setq col1 (cons (car elt) col1))
      (setq col2 (cons (cadr elt) col2))
      (setq col3 (cons (caddr elt) col3)))
    (destructuring-bind (res1 res2 res3)
        (mapcar
         (lambda (col)
           (let* ((delta-ocol (vconcat (delta-optimize-dumped-table col)))
                  (rle-ocol
                   (range-length-optimize-dumped-table delta-ocol)))
             ;; Select the shortest between plain delta optimized and range
             ;; length encoded. The delta encoding is always shorter for all
             ;; used datasets, I checked.
             (if (< (length rle-ocol) (length delta-ocol))
                 (vconcat rle-ocol)
               delta-ocol)))
         (list col1 col2 col3))
      (if (equal res1 res2)
          (vconcat (list res1 res3))
        (vconcat (list res1 res2 res3))))))

(defun delta-optimize-dumped-table (table)
  (let ((prev (car table)))
    ;; 0 denotes non-range length encoded.
    (cons 0 (cons prev (mapcar (lambda (cur) (prog1
                                                 (- cur prev)
                                               (setq prev cur)))
                               (cdr table))))))

(defun range-length-optimize-dumped-table (table)
  (let ((idx 1) ; Skip the non-rle identifier.
        (len (length table))
        (ret nil)
        (count nil)
        (cur nil))
    (while (< idx len)
      (setq count 1
            cur (aref table idx)
            idx (1+ idx))
      (while (and (< idx len) (eq cur (aref table idx)))
        (setq idx (1+ idx)
              count (1+ count)))
      (setq ret (cons count (cons cur ret))))
    ;; 1 denotes range length encoded.
    (cons 1 (nreverse ret))))

(defun unicode-dump-error (msg)
  ;; Have an extra newline so that the message doesn't get mangled by make or
  ;; the file stream not flushed before "xemacs exiting".
  (error "compiled-unicode-tables.el: %s\n" msg))

) ; eval-when-compile

;;;###autoload
(defun set-compiled-unicode-file-search-table ()
  "Set the `compiled-unicode-file-search-table' variable used by
unicode.c:Fload_unicode_mapping_table(). The values are compiled into this
function, and after being called once, this function does nothing.
"
  (when (null compiled-unicode-file-search-table)
    (setq compiled-unicode-file-search-table
          (eval-when-compile
            (let (stack-trace-on-error) ; Disable to get sensible errors.
              (compile-unicode-file-search-table))))
    ;; Redefine the function to garbage collect the relatively massive
    ;; function (at the time of writing, 704 KB in .elc form).
    (defun set-compiled-unicode-file-search-table ()
      "Set the `compiled-unicode-file-search-table' variable used by
unicode.c:Fload_unicode_mapping_table(). The values are compiled into this
function, and after being called once, this function does nothing.
"
      nil))
  nil)

(provide 'compiled-unicode-tables)
;;; compiled-unicode-tables.el ends here
