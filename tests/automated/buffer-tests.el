;; Copyright (C) 2025 Free Software Foundation, Inc. -*- coding: utf-8 -*-

;; Author: Aidan Kehoe <kehoea@parhasard.net>
;; Maintainers: Aidan Kehoe <kehoea@parhasard.net>
;; Created: 2025
;; Keywords: tests

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

(Check-Error wrong-type-argument (decode-buffer '*scratch*))

;; These used to succeed and return the current buffer, unclear why (not even
;; GNU-compatible).
(Check-Error wrong-type-argument (decode-buffer ?h))
(Check-Error wrong-type-argument (decode-buffer #xabcdef))

(Check-Error wrong-type-argument (buffer-base-buffer #xabcdef))
(Check-Error wrong-type-argument (buffer-indirect-children #xabcdef))
(Check-Error wrong-type-argument (buffer-local-variables #xabcdef))
(Check-Error wrong-type-argument (buffer-modified-p #xabcdef))
(Check-Error wrong-type-argument (set-buffer-modified-p t #xabcdef))

(Check-Error wrong-type-argument (buffer-modified-tick #xabcdef))
(Check-Error wrong-type-argument (buffer-disable-undo #xabcdef))
(Check-Error wrong-type-argument (buffer-enable-undo #xabcdef))
(Check-Error wrong-type-argument (kill-buffer #xabcdef))
(Check-Error wrong-type-argument (barf-if-buffer-read-only #xabcdef))

(Check-Error invalid-argument
	     (decode-buffer
	      (generate-new-buffer-name (symbol-name (gensym)))))

(Assert (eq (current-buffer) (decode-buffer nil)))
(Assert (eq (current-buffer) (decode-buffer (buffer-name (current-buffer)))))
(Assert (equal " *Echo Area*"
	       (buffer-name (decode-buffer " *Echo Area*"))))
(Assert (equal " *Echo Area*"
	       (buffer-name (decode-buffer (get-buffer " *Echo Area*")))))

;;; end of buffer-tests.el
