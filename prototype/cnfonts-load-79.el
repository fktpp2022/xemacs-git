;;; cnfonts-load-79.el --- throwaway breakage catalogue for wayfinder #79 -*- mode: emacs-lisp -*-

;; NOT production code.  Prototype for ticket #79 of map #77:
;; "Does unmodified upstream cnfonts.el load and run on XEmacs at all, and
;; what breaks?"
;;
;; Run with the cnfonts package installed by the repo's elpa client:
;;   CNFONTS_PKG_DIR=/path/to/elpa-user-dir/cnfonts-20251223.636 \
;;     build/bin/xemacs --batch -l prototype/cnfonts-load-79.el
;;
;; Layout expected next to this file:
;;   xemacs-shim.el   -- recording stubs for the GNU APIs XEmacs lacks
;;
;; The run prints a phase-by-phase catalogue.  It never fails the batch: the
;; catalogue IS the output.

(defvar proto-root (file-name-directory (or load-file-name buffer-file-name)))
(add-to-list 'load-path proto-root)

(defvar pkg-dir (or (getenv "CNFONTS_PKG_DIR") ""))
(unless (file-directory-p pkg-dir)
  (error "CNFONTS_PKG_DIR is not set to an installed cnfonts package dir"))
(add-to-list 'load-path pkg-dir)

;; Keep cnfonts's profile/config writes inside a scratch dir.
(defvar scratch (file-name-as-directory (make-temp-file "cnfonts79-" t)))
(setq cnfonts-directory (expand-file-name "cnfonts/" scratch))

(defun proto-line (fmt &rest args) (princ (apply #'format fmt args)))
(defun proto-head (title) (princ (format "\n=== %s ===\n" title)))

(defun proto-try (label thunk)
  "Call THUNK, print the result or the error.  Returns (ok . value-or-error)."
  (condition-case err
      (let ((value (funcall thunk)))
        (proto-line "  %-34s OK    %S\n" label value)
        (cons t value))
    (error
     (proto-line "  %-34s ERROR %s\n" label (error-message-string err))
     (cons nil err))))

;;; ---------------------------------------------------------------- phase 1
(proto-head "1. GNU API availability in this XEmacs (before any shim)")
(dolist (s '(font-spec set-face-attribute face-attribute x-list-fonts set-fontset-font
               list-fonts font-family-list string-join
               mouse-wheel-text-scale touch-screen-pinch
               display-graphic-p display-multi-font-p set-face-font face-property
               font-list make-font))
  (proto-line "  %-24s %s\n" s (if (fboundp s) "present" "ABSENT")))
(dolist (s '(face-font-rescale-alist frame-inhibit-implied-resize))
  (proto-line "  %-24s %s\n" s (if (boundp s) "bound" "unbound")))

;;; ---------------------------------------------------------------- phase 2
(proto-head "2. (require 'cnfonts) with NO shim")
(proto-try "require cnfonts" (lambda () (require 'cnfonts) 'required))

;;; ---------------------------------------------------------------- phase 3
(proto-head "3. (cnfonts-enable) with NO shim")
(proto-try "cnfonts-enable" (lambda () (cnfonts-enable) 'enabled))
(proto-line "  after-make-frame-functions: %S\n" after-make-frame-functions)
(proto-line "  window-setup-hook:           %S\n" window-setup-hook)
(proto-line "  cnfonts-mode:                %S\n" (and (boundp 'cnfonts-mode) cnfonts-mode))

;;; ---------------------------------------------------------------- phase 4
(proto-head "4. the font path with the recording shim loaded")
(require 'xemacs-shim)

(proto-try "cnfonts-set-font (public)" (lambda () (cnfonts-set-font) 'done))
(proto-line "  display-graphic-p:      %S\n" (display-graphic-p))
(proto-line "  display-multi-font-p:   %S\n" (display-multi-font-p))

(proto-try "cnfonts--get-valid-fonts"
           (lambda () (cnfonts--get-valid-fonts)))
(proto-try "cnfonts--font-exists-p \"Consolas\""
           (lambda () (cnfonts--font-exists-p "Consolas")))
(proto-try "cnfonts--set-font-1 (internal)"
           (lambda () (cnfonts--set-font-1 (list 12 12 12 12 12))))

;;; ---------------------------------------------------------------- phase 5
(proto-head "5. the shim surface cnfonts actually asked for")
(dolist (call (reverse cnfonts-stub-calls))
  (proto-line "  %-18s %S\n" (car call) (cdr call)))
(proto-line "  total calls: %d\n" (length cnfonts-stub-calls))

;;; ---------------------------------------------------------------- phase 6
(proto-head "6. dynamic-scope (no lexical binding) check")
;; cnfonts.el is `-*- lexical-binding: t -*-'; XEmacs ignores that and binds
;; dynamically.  The only closure in cnfonts.el that captures a variable is the
;; lambda inside cnfonts--font-exists-p, which reads `font'.  Compare the
;; byte-compiled package (loaded above) against the interpreted source.
(proto-try "compiled: cnfonts--font-exists-p \"Consolas\""
           (lambda () (cnfonts--font-exists-p "Consolas")))
(proto-try "compiled: cnfonts--font-exists-p \"DejaVu Sans Mono\""
           (lambda () (cnfonts--font-exists-p "DejaVu Sans Mono")))
(proto-try "interpreted: reload cnfonts.el as source"
           (lambda ()
             (load (expand-file-name "cnfonts.el" pkg-dir) t t)
             (cnfonts--font-exists-p "Consolas")))
(proto-try "interpreted: cnfonts--font-exists-p \"DejaVu Sans Mono\""
           (lambda () (cnfonts--font-exists-p "DejaVu Sans Mono")))

;;; ---------------------------------------------------------------- phase 7
(proto-head "7. faces after the run")
(dolist (face '(default bold italic bold-italic))
  (proto-line "  %-12s font=%S\n" face (face-property face 'font)))

(princ "\nCATALOGUE COMPLETE\n")
