;;; config.el --- access configuration parameters

;; Copyright (C) 1997 Sun Microsystems, Inc.
;; Copyright (C) 2026 Free Software Foundation.

;; Author:   Martin Buchholz, Aidan Kehoe
;; Keywords: configure

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

;;; Synched up with: not in FSF.

;;; Commentary:

;;; Code:


(defvar config-value-file (expand-file-name "config.values" doc-directory)
  "File containing configuration parameters and their values.")

(defvar config-value-hash-table nil
  "Hash table to store configuration parameters and their values.")

;;;###autoload
(defun config-value-hash-table ()
  "Return hash table of configuration parameters and their values."
  (unless config-value-hash-table
    (setq config-value-hash-table (make-hash-table :size 300))
    (with-temp-buffer
      (insert-file-contents config-value-file)
      (goto-char (point-min))
      (while (progn
	       (while (progn (skip-chars-forward " \t\n\^L")
			     (eql (char-after) ?\;))
		 (forward-line 1))
	       (not (eobp)))
	(let* ((key (intern
		     (buffer-substring
		      (point)
		      (progn (skip-chars-forward "^ ") (point))
		      (current-buffer))))
	       (value (read (current-buffer))))
	  (unless (and (eql (and (> (length value) 0)
				 (aref value 0)) ?@)
		       (equal (symbol-name key)
			      (subseq value 1 -1)))
	    (puthash key value config-value-hash-table)))))
    (maphash
     (labels ((expand-configuration-parameters (value)
		(let (offset stream lookup (last 0))
		  (while (setq offset
			       (string-match-p "\\$[{(]" value last))
		    (or stream (setq stream (make-string-output-stream)))
		    (write-sequence value stream :start last :end offset)
		    (setq last (+ offset (length "${"))
			  offset (position
				  (if (eql (aref value
						 (+ offset (length "$")))
					   ?{)
				      ?}
				    ?\))
				  value :start last))
		    (if (and offset
			     (setq
			      lookup
			      (gethash (intern-soft
					(subseq value last offset))
				       config-value-hash-table)))
			(progn 
			  (setq lookup
				(expand-configuration-parameters lookup))
			  (write-sequence lookup stream)
			  (setq last (1+ offset)))
		      (write-sequence value stream
				      :start (- last (length "${"))
				      :end last)
		      (when offset
			(write-sequence value stream :start last
					:end (+ offset (length "}")))
			(setq last (+ offset (length "}"))))))
		  (if (not stream)
		      value
		    (write-sequence value stream :start last)
		    (get-output-stream-string stream)))))
       #'(lambda (key value)
	   (unless (equal value
		          (setq value (expand-configuration-parameters
				       value)))
	     (puthash key value config-value-hash-table))))
     config-value-hash-table))
  config-value-hash-table)

;;;###autoload
(defun config-value (config-symbol)
  "Return the value of the configuration parameter CONFIG_SYMBOL."
  (gethash config-symbol (config-value-hash-table)))

(provide 'config)

;;; config.el ends here
