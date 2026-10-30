;;; font-tests.el --- tests for the font layer   -*- coding: utf-8 -*-

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

;; The device-free half of the font layer's verification plan (ADR 0003).
;; The ctest harness runs `-batch', which gives a `stream' device even in a
;; GUI build, so nothing here may need a frame.  The device-level assertions
;; live in tests/font-layer-gui.el.

;;; Code:

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; Must precede `(require 'font)': a stale autoload cookie makes the whole
;; descriptor route unreachable with no other symptom.
(Assert (fboundp 'font-set-face-font)
        "font-set-face-font is reachable through its autoload cookie")

(require 'font)

;;; The mapping table

(Assert (equal (mapcar #'car font-window-system-mappings)
               '(x gtk mswindows msprinter tty))
        "the mapping table is exactly the backends that carry fonts")

(dolist (row font-window-system-mappings)
  (Assert (fboundp (cadr row))
          (format "font name producer for %s is defined" (car row)))
  (Assert (fboundp (car (cdr (cdr row))))
          (format "font object producer for %s is defined" (car row))))

;;; The descriptor

(let ((font (make-font :family "monospace"
                       :weight :bold
                       :style font-italic-mask
                       :size "14pt"
                       :registry "iso10646"
                       :encoding "1")))
  (Assert (equal (font-family font) '("monospace")) "family round-trips")
  (Assert (eq (font-weight font) :bold) "weight round-trips")
  (Assert (= (font-style font) font-italic-mask) "style round-trips")
  (Assert (equal (font-size font) "14pt") "size round-trips")
  (Assert (equal (font-registry font) "iso10646") "registry round-trips")
  (Assert (equal (font-encoding font) "1") "encoding round-trips"))

;;; set-face-font and the descriptor

;; The batch device has no backend row, so the name producer is stubbed
;; when a test needs the descriptor to lower.  The assertions are about
;; which descriptor the face records, not about this machine's fonts.
(defmacro font-tests--with-name-producer (&rest body)
  `(flet ((font-create-name (fontobj &optional _device)
             (format "%s-device" (or (car (font-family fontobj)) "font"))))
     ,@body))

(define-specifier-tag 'font-tests-tag)

(font-tests--with-name-producer
 (let* ((face (make-face 'font-tests-face))
        (font (make-font :family "monospace" :size 12)))
   (set-face-font face font)
   (Assert (equal (face-property face 'font-specification) font)
           "set-face-font records a 12-element descriptor as the
font-specification")))

(let ((face (make-face 'font-tests-unlowered-face)))
  (set-face-font face (make-font :family "monospace" :size 12))
  (Assert (null (face-property face 'font-specification))
          "a descriptor that lowers to no name is not recorded"))

(let ((face (make-face 'font-tests-inherit-face)))
  (set-face-font face [bold])
  (Assert (null (face-property face 'font-specification))
          "a 1-element vector is the inheritance form, not a descriptor"))

(font-tests--with-name-producer
 (let* ((face (make-face 'font-tests-tagged-face))
        (latin (make-font :family "Latin"))
        (cjk (make-font :family "CJK")))
   (set-face-font face latin)
   (set-face-font face cjk 'global '(font-tests-tag) nil)
   (Assert (equal (face-property face 'font-specification) latin)
           "a tagged apply leaves the face's plain descriptor alone")
   (Assert (equal (specifier-specs (face-property face 'font) 'global
                                   '(font-tests-tag) t)
                  '((font-tests-tag) . "CJK-device"))
           "the tagged entry is still applied under its tag")))

(font-tests--with-name-producer
 (let* ((face (make-face 'font-tests-update-one-face))
        (latin (make-font :family "Latin"))
        (cjk (make-font :family "CJK")))
   (set-face-font face latin)
   (set-face-font face cjk 'global '(font-tests-tag) nil)
   (font-update-one-face face)
   (Assert (equal (specifier-specs (face-property face 'font)
                                   (selected-device) nil t)
                  "Latin-device")
           "font-update-one-face re-applies the plain font after a tagged
apply")))

(font-tests--with-name-producer
 (let* ((face (make-face 'font-tests-update-device-face))
        (latin (make-font :family "Latin"))
        (cjk (make-font :family "CJK")))
   (set-face-font face latin)
   (set-face-font face cjk 'global '(font-tests-tag) nil)
   (font-update-device-fonts (selected-device))
   (Assert (equal (specifier-specs (face-property face 'font)
                                   (selected-device) nil t)
                  "Latin-device")
           "font-update-device-fonts re-applies the plain font after a
tagged apply")))

;;; A device type with no row

(Assert (null (font-create-name (make-font :family "monospace")
                                (selected-device)))
        "font-create-name answers nil for a device type with no row")

;;; font-tests.el ends here
