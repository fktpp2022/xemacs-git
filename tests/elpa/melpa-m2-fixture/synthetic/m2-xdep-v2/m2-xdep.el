;;; m2-xdep.el --- synthetic cross-archive dependent, second build

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of the MELPA milestone-2 offline fixture.

;;; Commentary:

;; The rolling-upgrade build of the synthetic m2-xdep fixture.  It keeps
;; the GNU queue requirement of v1.

;;; Code:

(defconst m2-xdep-build "v2"
  "Build of the synthetic m2-xdep fixture.")

(defun m2-xdep-peek (queue)
  "Return the first element of QUEUE."
  (queue-first queue))

(provide 'm2-xdep)

;;; m2-xdep.el ends here
