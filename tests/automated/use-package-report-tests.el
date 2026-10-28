;;; use-package-report-tests.el --- tests for the tabulated-list port and the use-package report

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

;; The report-UI milestone: the minimal `tabulated-list' surface that
;; `M-x use-package-report' needs, the `time-subtract' shim the statistics
;; gatherer depends on, and the `%N' conversion in `format-time-string'.
;; Rendering is verified by inspecting the report buffer's text rather than
;; by a screenshot: the header, the aligned columns, the sort orders, and
;; the microsecond time all show up as characters a test can match.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;;; Surface

(Assert (featurep 'tabulated-list)
        "loading use-package brings in the tabulated-list feature")

(dolist (symbol '(tabulated-list-mode tabulated-list-print
                  tabulated-list-print-entry tabulated-list-print-col
                  tabulated-list-init-header tabulated-list-revert))
  (Assert (fboundp symbol) (format "%s is defined" symbol)))

(dolist (variable '(tabulated-list-format tabulated-list-entries
                    tabulated-list-sort-key))
  (Assert (boundp variable) (format "%s is defined" variable)))

(Assert (eq (get 'tabulated-list-mode 'derived-mode-parent) 'special-mode)
        "tabulated-list-mode derives from special-mode")

;;; Rendering

;; Render FORMAT/ENTRIES (with optional SORT-KEY) in a throwaway buffer and
;; return the resulting text.
(defun use-package-report-tests--render (format entries &optional sort-key)
  (with-temp-buffer
    (set (make-local-variable 'tabulated-list-format) format)
    ;; `sort' inside `tabulated-list-print' is destructive, so hand it a
    ;; fresh copy each call rather than the caller's (possibly quoted) list.
    (set (make-local-variable 'tabulated-list-entries) (copy-sequence entries))
    (set (make-local-variable 'tabulated-list-sort-key) sort-key)
    (tabulated-list-init-header)
    (tabulated-list-print)
    (buffer-string)))

(defun use-package-report-tests--lines (text)
  "Split TEXT into non-empty lines."
  (split-string text "\n" t))

(defun use-package-report-tests--order (text)
  "Return the leading character of each data row in TEXT."
  (mapcar (lambda (line) (substring line 0 1))
          (cdr (use-package-report-tests--lines text))))

(let* ((format [("Name" 8 t) ("Value" 6 nil)])
       (entries '(("alpha" ["alpha" "1"])
                  ("beta" ["beta" "22"])))
       (text (use-package-report-tests--render format entries))
       (lines (use-package-report-tests--lines text)))
  (Assert (= 3 (length lines)) "the header and both rows are present")
  (Assert (string-match "^Name" (nth 0 lines)) "the header lists the columns")
  (Assert (string-match "Value" (nth 0 lines)) "the header lists every column")
  (Assert (string-match "^alpha" (nth 1 lines)) "the first row is printed")
  (Assert (string-match "^beta" (nth 2 lines)) "the second row is printed")
  ;; The data column starts at the same offset in the header and in a row.
  (Assert (= (string-match "Value" (nth 0 lines))
             (string-match "1" (nth 1 lines)))
          "the header aligns with the first data column"))

;;; Sorting

(let* ((format [("Name" 8 t)])
       (entries '(("c" ["c"]) ("a" ["a"]) ("b" ["b"]))))
  (Assert (equal (use-package-report-tests--order
                  (use-package-report-tests--render format entries '("Name")))
                 '("a" "b" "c"))
          "the default string sorter orders rows")
  (Assert (equal (use-package-report-tests--order
                  (use-package-report-tests--render
                   format entries '("Name" . t)))
                 '("c" "b" "a"))
          "a non-nil FLIP reverses the sort order"))

;; A custom sorter is a predicate over the entries, and FLIP must reverse it
;; without breaking the dynamically scoped closure (XEmacs has no lexical
;; binding, so this exercises the named-sorter path).
(let* ((format [("Name" 6 t)
                ("Rank" 6 (lambda (a b)
                            (< (string-to-number (aref (cadr a) 1))
                               (string-to-number (aref (cadr b) 1))))) ])
       (entries '(("x" ["x" "3"]) ("y" ["y" "1"]) ("z" ["z" "2"]))))
  (Assert (equal (use-package-report-tests--order
                  (use-package-report-tests--render format entries '("Rank")))
                 '("y" "z" "x"))
          "a custom sorter orders rows by its predicate")
  (Assert (equal (use-package-report-tests--order
                  (use-package-report-tests--render
                   format entries '("Rank" . t)))
                 '("x" "z" "y"))
          "FLIP reverses a custom sorter"))

;;; `time-subtract'

(Assert (equal (time-subtract (list 100 200 300000) (list 100 100 100000))
               '(0 100 200000))
        "time-subtract subtracts the second time from the first")

(Assert (equal (time-subtract (list 0 10 0) (list 0 9 500000))
               '(0 0 500000))
        "time-subtract borrows from seconds into microseconds")

(Assert (equal (time-subtract (list 100 200 0) (list 100 200 0))
               '(0 0 0))
        "time-subtract of equal times is zero")

(let ((now (current-time)))
  (Assert (= (float-time (time-subtract now now)) 0.0)
          "float-time of a zero difference is zero"))

;;; `%N' in `format-time-string'

(Assert (equal (format-time-string "%6N" (list 100 200 123456)) "123456")
        "%6N prints six microsecond digits")

(Assert (equal (format-time-string "%N" (list 100 200 123456)) "123456000")
        "%N defaults to nine digits (microseconds scaled to nanoseconds)")

(Assert (string-suffix-p ".123456"
                         (format-time-string "%H:%M:%S.%6N"
                                             (list 100 200 123456)))
        "%6N composes with the other conversions")

(Assert (equal (format-time-string "%6N" (list 100 200 0)) "000000")
        "%6N zero-pads")

;;; End to end: `M-x use-package-report'

(require 'use-package)

(setq use-package-compute-statistics t)
(use-package cl-lib :config (setq use-package-report-tests-configured t))
(use-package-report)

(with-current-buffer "*use-package statistics*"
  (let ((text (buffer-string)))
    (Assert (eq major-mode 'use-package-statistics-mode)
            "the report buffer is in use-package-statistics-mode")
    (Assert (string-match "^Package" text) "the report has a header row")
    (Assert (string-match "cl-lib" text) "the report lists the package")
    (Assert (string-match "Configured" text)
            "the report shows the package's status")
    (Assert (string-match "[0-9][0-9]:[0-9][0-9]:[0-9][0-9]\\.[0-9][0-9][0-9][0-9][0-9][0-9]"
                          text)
            "the report shows a microsecond-precision time")))

;; The batch harness exits 0 even when Asserts fail, so the verdict is
;; computed from the harness's own failure counters, which are dynamically
;; bound while the suite body runs.
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
     "use-package report tests: passed"
   "use-package report tests: FAILED"))

;;; use-package-report-tests.el ends here
