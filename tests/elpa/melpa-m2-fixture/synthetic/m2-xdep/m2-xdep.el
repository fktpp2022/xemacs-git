;;; m2-xdep.el --- synthetic cross-archive dependent

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of the MELPA milestone-2 offline fixture.

;;; Commentary:

;; Synthetic fixture package.  Its v1 and v2 builds require GNU queue.
;; The requirement is deliberately not `require'd here: the client
;; byte-compiles a package before activating its dependencies, so the
;; requirement is exercised at load time through queue's autoloads.

;;; Code:

(defconst m2-xdep-build "v1"
  "Build of the synthetic m2-xdep fixture.")

(defun m2-xdep-peek (queue)
  "Return the first element of QUEUE."
  (queue-first queue))

(provide 'm2-xdep)

;;; m2-xdep.el ends here
