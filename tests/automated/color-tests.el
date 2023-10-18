;;; color-tests.el --- test color parsing + lookup   -*- coding: utf-8 -*-

;; Copyright (C) 2023 Free Software Foundation, Inc.

;; Author: Richard Hopkins  <xemacs@unbit.co.uk>
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

;;; Commentary:

;; Test color parsing + lookup.

(require 'font)

(defun rgb-txt-available-p ()
  ;; Check for black fallback when rgb.txt is unavailable.
  (not (equal (font-lookup-rgb-components "white")
	      (list 0 0 0))))

(Skip-Test-Unless
 (rgb-txt-available-p)
 "rgb.txt not found"
 "Test colors read from rgb.txt"
 ;; Tests
 (Assert (equal (font-lookup-rgb-components "white")
		(list 65535 65535 65535)))
 nil)

;;; Test parsing of white variants
;; #fefefe format
(Assert (equal (font-parse-rgb-components "#ffffff")
	       (list 65535 65535 65535)))
;; 1 hex digit per component
(Assert (equal (font-parse-rgb-components "#fff")
	       (list 65535 65535 65535)))
;; 3 hex digits per component
(Assert (equal (font-parse-rgb-components "#fffffffff")
	       (list 65535 65535 65535)))

;; rgb:fe/fe/fe format
(Assert (equal (font-parse-rgb-components "rgb:ff/ff/ff")
	       (list 65535 65535 65535)))
(Assert (equal (font-parse-rgb-components "rgb:f/f/f")
	       (list 65535 65535 65535)))
(Assert (equal (font-parse-rgb-components "rgb:fff/fff/fff")
	       (list 65535 65535 65535)))

;;; Test conversion of white variants
;; Component range [0.0, 1.0]
(Assert (equal (font-color-rgb-components "1.0 1.0 1.0")
	       (list 65535 65535 65535)))

;; Component range [0, 255]
(Assert (equal (font-color-rgb-components "255 255 255")
	       (list 65535 65535 65535)))

;; #fefefe format
(Assert (equal (font-color-rgb-components "#ffffff")
	       (list 65535 65535 65535)))

;; Test example color from rgb.txt in different forms is converted to the same
;; value.

;; 100 149 237             CornflowerBlue
(let ((cornflowerblue (list 25700 38293 60909)))
  (Assert (equal (font-color-rgb-components "100 149 237")
		 cornflowerblue))
  (Assert (equal (font-color-rgb-components "#6495ed")
		 cornflowerblue))
  (Assert (equal (font-color-rgb-components "rgb:64/95/ed")
		 cornflowerblue)))

;;; end color-tests.el
