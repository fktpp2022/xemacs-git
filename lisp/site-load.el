;;; site-load.el --- Template file for site-wide XEmacs customization
;; Copyright (C) 1997 Free Software Foundation, Inc.

;; Author: Steven L. Baur <steve@xemacs.org>
;; Keywords: internal

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

;; This is a prototype site-load.el file.
;; The site-load.el mechanism is provided so XEmacs installers can easily
;; dump lisp packages with XEmacs that do not get dumped standardly.

;; The file `site-packages' if it exists should look something like:
;; (setq site-load-packages '(
;; "../lisp/modes/cc-mode.elc"
;; "../lisp/utils/redo.elc"
;; "../lisp/packages/scroll-in-place.elc"
;; )
;; )

;; Documentation will be picked up by the load history and written to DOC in
;; the normal way at dump time, so there is no specific restriction on the
;; syntax.

;; Also note that site-packages belongs in the top level directory not the
;; lisp directory for use with --srcdir configurations.

;;; Code:
(defvar site-load-package-file "../site-packages"
  "File name containing the list of extra packages to dump with XEmacs.")
(defvar site-load-packages nil
  "A list of .elc files that should be dumped with XEmacs.
This variable should be set by `site-load-package-file'.")

;; Load site specific packages for dumping with the XEmacs binary.
(when (file-exists-p site-load-package-file) 
  (load site-load-package-file t t t)
  (message "Loading site-wide packages for dumping...")
  (mapc #'load site-load-packages)
  (message "Loading site-wide packages for dumping...done"))

;; This file is intended for end user additions.
;; Put other initialization here, like setting of language-environment, etc.
;; Perhaps this should really be in the site-init.el.

;;; site-load.el ends here
