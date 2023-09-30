;; Copyright (C) 2023 Free Software Foundation, Inc. -*- coding: utf-8 -*-

;; Author: Aidan Kehoe <kehoea@parhasard.net>
;; Maintainers: Aidan Kehoe <kehoea@parhasard.net>
;; Created: 2023
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

(Assert (equal (event-modifiers (make-event 'motion '(modifiers (button1))))
	       '(button1)))

(Assert (equal (event-modifiers (make-event 'misc-user
					     `(modifiers (button3 button1 shift)
					       function identity object ,pi)))
	       '(shift button1 button3)))

;;; end of event-tests.el