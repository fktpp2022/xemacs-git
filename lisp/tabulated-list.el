;;; tabulated-list.el --- generic tabulated data major mode  -*- lexical-binding: t; -*-

;; This file is a deliberately minimal XEmacs port of GNU Emacs's
;; tabulated-list.el.  It provides exactly the symbols that the
;; use-package statistics reporter (`use-package-report') needs; it is
;; NOT a drop-in replacement for the full GNU library.
;;
;; ponytail: ceiling -- this port omits the header line (XEmacs has no
;; `header-line-format'), clickable sort buttons, the mode's keymap,
;; tags, groups, :right-align/:pad-right properties, truncation, and
;; incremental reprint.  Add them when a consumer actually needs them.

;;; Code:

;; `special-mode', the parent, is preloaded by dumped-lisp.el.
(require 'derived)

(defgroup tabulated-list nil
  "Tabulated list editing."
  :group 'editing)

(defcustom tabulated-list-padding 0
  "Number of spaces to prepend to each row."
  :type 'integer
  :group 'tabulated-list)

(defcustom tabulated-list-printer 'tabulated-list-print-entry
  "Function called to insert a tabulated list entry at point.
Called with two arguments, ID and COLS (see `tabulated-list-entries')."
  :type 'function
  :group 'tabulated-list)

(defcustom tabulated-list-revert-hook nil
  "Hook run before reverting a Tabulated List buffer."
  :type 'hook
  :group 'tabulated-list)

(defvar-local tabulated-list-format nil
  "Vector describing the columns of a Tabulated List buffer.
Each element has the form (NAME WIDTH SORT), where NAME is the
column header, WIDTH is the column width in characters, and SORT
is non-nil if the column may be sorted: either `t' for the default
string comparison, or a two-argument predicate (A B) called with
two entries as returned by `tabulated-list-entries'.")

(defvar-local tabulated-list-entries nil
  "Entries displayed by the Tabulated List buffer.
A list of (ID . COLS) conses, or a function returning such a list.
COLS is a vector of one column descriptor per element of
`tabulated-list-format'.")

(defvar-local tabulated-list-sort-key nil
  "Sort key for `tabulated-list-print'.
If nil, don't sort.  Otherwise a cons (COLUMN-NAME . FLIP); a
non-nil FLIP reverses the sort order.")

(defvar-local tabulated-list--header-string nil
  "Header string computed by `tabulated-list-init-header'.")

;; The data variables are permanent-local so they survive a major-mode
;; change: callers (e.g. `use-package-report') set them BEFORE invoking
;; `tabulated-list-mode', which runs `kill-all-local-variables'.
(put 'tabulated-list-format 'permanent-local t)
(put 'tabulated-list-entries 'permanent-local t)
(put 'tabulated-list-sort-key 'permanent-local t)

(defun tabulated-list--column-number (name)
  "Return the index of the column named NAME."
  (let ((len (length tabulated-list-format))
        (n 0)
        found)
    (while (and (< n len) (null found))
      (when (equal (car (aref tabulated-list-format n)) name)
        (setq found n))
      (setq n (1+ n)))
    (or found
        (error "No column named %s" name))))

;; XEmacs's `lambda' is dynamically scoped, so a sorting predicate cannot
;; close over a local variable; the column index and any custom sorter are
;; passed through these dynamic variables instead.  They are set by
;; `tabulated-list--get-sorter' and read only during the `sort' that
;; immediately follows, so there is no reentrancy hazard.
(defvar tabulated-list--sort-column-index 0)
(defvar tabulated-list--sorter nil)

(defun tabulated-list--default-sorter (a b)
  "Compare entries A and B on the column named by the current sort key."
  (let ((x (aref (cadr a) tabulated-list--sort-column-index))
        (y (aref (cadr b) tabulated-list--sort-column-index)))
    (string< (if (stringp x) x (car x))
             (if (stringp y) y (car y)))))

(defun tabulated-list--selected-sorter (a b)
  "Compare entries A and B with the sorter selected by `tabulated-list--get-sorter'."
  (funcall (or tabulated-list--sorter #'tabulated-list--default-sorter) a b))

(defun tabulated-list--sort-reverse (a b)
  "Like `tabulated-list--selected-sorter', but with the arguments reversed."
  (tabulated-list--selected-sorter b a))

(defun tabulated-list--get-sorter ()
  "Return a sorting predicate for `tabulated-list-entries', or nil.
See `tabulated-list-sort-key' for the meaning of the sort key; a
non-nil FLIP there reverses the order."
  (when (and tabulated-list-sort-key
             (car tabulated-list-sort-key))
    (let* ((sort-column (car tabulated-list-sort-key))
           (n (tabulated-list--column-number sort-column))
           (sorter (nth 2 (aref tabulated-list-format n))))
      (setq tabulated-list--sort-column-index n
            tabulated-list--sorter (if (eq sorter t) nil sorter))
      (if (cdr tabulated-list-sort-key)
          #'tabulated-list--sort-reverse
        #'tabulated-list--selected-sorter))))

(defun tabulated-list-print-col (n col-desc x)
  "Insert column N of an entry at point.
COL-DESC is the column descriptor from the entry's COLS vector; X
is the current column.  Return the column number after insertion.
ponytail: plain text only -- no help-echo, buttons, or align-to
display properties; labels longer than the column are not truncated."
  (let* ((fmt (aref tabulated-list-format n))
         (width (nth 1 fmt))
         (label (if (stringp col-desc) col-desc (car col-desc)))
         (pad-right 1)
         (not-last-col (< n (1- (length tabulated-list-format)))))
    (insert label)
    (when not-last-col
      ;; At least one space, widened to the declared column width.
      (insert (make-string (max 1 (- width (length label))) 32)))
    (+ x pad-right width)))

(defun tabulated-list-print-entry (id cols)
  "Insert a tabulated list entry at point.
ID identifies the entry; COLS is a vector of column descriptors.
This is the default value of `tabulated-list-printer'."
  (let ((x (max tabulated-list-padding 0))
        (ncols (length tabulated-list-format))
        (inhibit-read-only t))
    (when (> x 0)
      (insert (make-string x 32)))
    (dotimes (n ncols)
      (setq x (tabulated-list-print-col n (aref cols n) x)))
    (insert ?\n)))

(defun tabulated-list-init-header ()
  "Compute the header string for the current Tabulated List buffer.
XEmacs has no header line, so the string is stored in
`tabulated-list--header-string' and printed at the top of the
buffer by `tabulated-list-print'."
  (let* ((x (max tabulated-list-padding 0))
         (len (length tabulated-list-format))
         (parts nil))
    (dotimes (n len)
      (let* ((col (aref tabulated-list-format n))
             (label (nth 0 col))
             (width (nth 1 col)))
        (push label parts)
        (when (< n (1- len))
          (push (make-string (max 1 (- width (length label))) 32)
                parts))))
    (setq tabulated-list--header-string
          (concat (make-string x 32)
                  (apply #'concat (nreverse parts))))))

(defun tabulated-list-print (&optional _remember-pos _update)
  "Populate the current Tabulated List buffer from its entries.
Sort `tabulated-list-entries' according to `tabulated-list-sort-key',
erase the buffer, print the header, then print each entry with
`tabulated-list-printer'.
ponytail: no remember-pos, no incremental update, no groups."
  (let ((inhibit-read-only t)
        (entries (if (functionp tabulated-list-entries)
                     (funcall tabulated-list-entries)
                   tabulated-list-entries))
        (sorter (tabulated-list--get-sorter)))
    (when sorter
      (setq entries (sort entries sorter)))
    (unless (functionp tabulated-list-entries)
      (setq tabulated-list-entries entries))
    (erase-buffer)
    (when tabulated-list--header-string
      (insert tabulated-list--header-string ?\n))
    (dolist (entry entries)
      (funcall tabulated-list-printer (car entry) (cadr entry)))
    (goto-char (point-min))
    (set-buffer-modified-p nil)))

(defun tabulated-list-revert (&rest _ignored)
  "Recompute and reprint the current Tabulated List buffer."
  (run-hooks 'tabulated-list-revert-hook)
  (tabulated-list-print)
  (goto-char (point-min)))

(define-derived-mode tabulated-list-mode special-mode "Tabulated"
  "Generic major mode for browsing a list of items in columns.
This is a minimal XEmacs port; see the commentary at the top of the
file for what is intentionally omitted.")

(put 'tabulated-list-mode 'mode-class 'special)

(provide 'tabulated-list)

;;; tabulated-list.el ends here
