;; Copyright (C) 2000 Free Software Foundation, Inc.

;; Author: Yoshiki Hayashi  <yoshiki@xemacs.org>
;; Maintainer: Yoshiki Hayashi  <yoshiki@xemacs.org>
;; Created: 2000
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

;; Redisplay related tests.

(require 'test-harness)

;; This used to crash XEmacs.
(when (featurep 'mule)
  (let ((buffer (generate-new-buffer "*split test*")))
    (set-window-buffer (selected-window) buffer)
    (split-window-vertically)
    (insert (make-char 'japanese-jisx0208 36 44))
    (backward-char)
    (redraw-frame)
    (delete-other-windows)
    (split-window)
    (kill-buffer buffer)
    (delete-other-windows)))

(let ((buffer (generate-new-buffer " hello")))
  (set-window-buffer (selected-window) buffer)
  (write-sequence "hello\nhello" buffer)
  (set-buffer buffer)
  (setq modeline-format '(line-number-mode "%l")
        line-number-mode t)
  (goto-char 1)
  (sit-for 0.0)
  (Assert (equal generated-modeline-string "1")
          "checking a bug fixed with line numbers and the modeline")
  (kill-buffer buffer))
