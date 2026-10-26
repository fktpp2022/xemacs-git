;;; m2-untrusted.el --- synthetic package held outside the allowlist

;; Copyright (C) 2026 XEmacs Development Team

;; This file is part of the MELPA milestone-2 offline fixture.

;;; Code:

(defun m2-untrusted-p ()
  "Return t; a loaded witness for this synthetic package."
  t)

(provide 'm2-untrusted)

;;; m2-untrusted.el ends here
