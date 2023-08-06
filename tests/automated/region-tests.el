;; Copyright (C) 2005 Adrian Aichner

;; Author: Adrian Aichner <adrian@xemacs.org>
;; Maintainer: XEmacs Beta List <xemacs-beta@xemacs.org>
;; Created: 2005
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

;;; Test region and zmacs-region functionality
;;; See test-harness.el

(condition-case err
    (require 'test-harness)
  (file-error
   (when (and (boundp 'load-file-name) (stringp load-file-name))
     (push (file-name-directory load-file-name) load-path)
     (require 'test-harness))))

;; Active region testing, verifying functionality of
;; http://list-archive.xemacs.org/xemacs-patches/200502/msg00194.html
;; Message-ID: <zmxwtboa.fsf@smtprelay.t-online.de>
(with-temp-buffer
  ;; Using active regions
  (let ((zmacs-regions t)
        (first-buffer (current-buffer)))
    (Silence-Message
     (insert (buffer-name)))
    (Assert (not (region-exists-p)))
    (Assert (not (region-active-p)))
    (Silence-Message
     (mark-whole-buffer))
    (Assert (region-exists-p))
    (Assert (region-active-p))
    ;; Turn off active regions
    (setq zmacs-regions nil)
    ;; Region still exists
    (Assert (region-exists-p))
    ;; Region is no longer active
    (Assert (not (region-active-p)))
    ;; Turn active regions back on
    (setq zmacs-regions t)
    ;; Region still exists
    (Assert (region-exists-p))
    ;; Region is active again
    (Assert (region-active-p))
    (with-temp-buffer
      (Silence-Message
       (insert (buffer-name)))
      ;; Region exists in first buffer, not this second one
      (Assert (not (region-exists-p)))
      ;; Region not active in this second temp buffer
      (Assert (not (region-active-p)))
      ;; Region still active in first temp buffer
      (Assert (eq (zmacs-region-buffer) first-buffer))
      ;; Activate region in second temp buffer
      (Silence-Message
       (mark-whole-buffer))
      ;; Region exists in second temp buffer
      (Assert (region-exists-p))
      ;; Region active in second temp buffer
      (Assert (region-active-p)))
    ;; Second temp buffer no longer exists
    (Assert (null (zmacs-region-buffer)))))

;; Test the shifted-motion handling.
(with-temp-buffer
  (let ((zmacs-regions t) (shifted-motion-keys-select-region t)
	(motion-keys-for-shifted-motion
         motion-keys-for-shifted-motion))
    (insert "(hello\nthere)")
    (dispatch-event (character-to-event '(home)))
    (Assert (not (region-active-p))
	    "checking [(home)] does not activate the region")
    (dispatch-event (character-to-event '(end)))
    (Assert (not (region-active-p))
	    "checking [(end)] does not activate the region")
    (mapc #'dispatch-event
	  (mapcar #'character-to-event '((home) (shift end))))
    (Assert (equal (buffer-substring (point) (mark)) "there)")
	    "checking [(shift end)] motion key selects the region")
    (dispatch-event (character-to-event '(left)))
    (Assert (not (region-active-p))
	    "checking unshifted motion deactivates region")
    (dispatch-event (character-to-event '(control shift b)))
    (Assert (not (region-active-p))
	    "checking shifted alphabetic motion key does not normally
activate the region")
    (end-of-line)
    (setq motion-keys-for-shifted-motion
	  (cons '(control b) motion-keys-for-shifted-motion))
    (dispatch-event (character-to-event '(control shift b)))
    (Assert (region-active-p)
	    "checking shifted alphabetic motion key activates region
when in `motion-keys-for-shifted-motion'")
    (Assert (equal (buffer-substring (point) (mark)) ")")
	    "checking appropriate region activated, alphabetic motion key")
    ;; Deactivate the region.
    (dispatch-event (character-to-event '(control f)))

    (switch-to-buffer
     (prog1 (current-buffer)
       (dispatch-event (character-to-event '(meta shift home)))))
    (Assert (not (region-active-p))
	    "checking switching to previous buffer{,-in-group} doesn't \
activate the region")
    (switch-to-buffer
     (prog1 (current-buffer)
       (dispatch-event (character-to-event '(meta shift end)))))
    (Assert (not (region-active-p))
	    "checking switching to next buffer{,-in-group} doesn't \
activate the region")
    ;; Test the shifted-motion paradigm when these keys are generated using
    ;; function-key-map, as is unremarkable on TTYs.
    (let ((home "\eO\000")
	  (shift-end "\e[1;2F")
	  (left "\e[D"))
      (define-key function-key-map home [home])
      (define-key function-key-map shift-end [(shift end)])
      (define-key function-key-map left [left])
      (goto-char (point-max))
      (mapc #'dispatch-event (mapcar #'character-to-event home))
      (Assert (not (region-active-p))
	      "checking [(home)] does not activate the region, \
function-key-map")
      (Assert (looking-at-p "there)")
	      "checking function-key-map handling of `home' moves \
appropriately")
      (mapc #'dispatch-event (mapcar #'character-to-event shift-end))
      (Assert (equal (buffer-substring (point) (mark)) "there)")
	      "checking [(shift end)] motion key selects the region, \
function-key-map")
      (mapc #'dispatch-event (mapcar #'character-to-event left))
      (Assert (not (region-active-p))
	      "checking unshifted motion deactivates region, \
function-key-map"))
    (setq shifted-motion-keys-select-region nil)
    (mapc #'dispatch-event
	  (mapcar #'character-to-event '((home) (shift end))))
    (Assert (not (region-active-p))
            "checking shifted motion does not active the region when \
`shifted-motion-keys-select-region' is nil")
    (setq shifted-motion-keys-select-region t)
    (mapc #'dispatch-event
	  (mapcar #'character-to-event '((home) (shift end))))
    (Assert (region-active-p)
            "checking shifted motion activates the region once more after \
`shifted-motion-keys-select-region' reset to t")
    (setq zmacs-regions nil)
    (mapc #'dispatch-event
	  (mapcar #'character-to-event '((home) (shift end))))
    (Assert (not (region-active-p))
            "checking shifted motion does not active the region when \
`zmacs-regions' is nil")
    (setq zmacs-regions t)
    (mapc #'dispatch-event
	  (mapcar #'character-to-event '((home) (shift end))))
    (Assert (region-active-p)
            "checking shifted motion activates the region once more when \
`zmacs-regions' is reset to t")))

;;; end of region-tests.el
