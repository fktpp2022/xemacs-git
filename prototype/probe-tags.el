;;; probe-tags.el --- throwaway probe: the shim's charset tag recipe
;;
;; PROTOTYPE (wayfinder ticket #79).  NOT production code.
;;
;; Finding: XEmacs registers the charset-class specifier tags
;; (two-dimensional, one-dimensional, initial, final, ...) inside
;;   #if defined (HAVE_X_WINDOWS) || defined (HAVE_GTK)   (src/faces.c:2286)
;; so a TTY-only build has none of them: (valid-specifier-tag-set-p
;; '(two-dimensional final)) => nil, and ADR 0002's lowering
;;   (set-face-font FACE FONT 'global '(two-dimensional final))
;; signals "Invalid tag set".
;;
;; This probe checks the cheap fix: re-register the built-in tags in Lisp,
;; reusing XEmacs' own predicate functions, so the tag-set works in any build.
;;
;;   build/bin/xemacs --batch -l prototype/probe-tags.el

(dolist (s '(specifier-tag-two-dimensional-p specifier-tag-final-stage-p
             specifier-tag-one-dimensional-p specifier-tag-initial-stage-p))
  (message "%-34s fboundp=%S" s (fboundp s)))

(dolist (ts '((two-dimensional) (final) (two-dimensional final) (initial)))
  (message "valid-specifier-tag-set-p %-24s %S"
           ts (condition-case e (valid-specifier-tag-set-p ts)
                  (error (error-message-string e)))))

(message "--- re-register the built-in 2D charset tag in Lisp ---")
;; A stage tag (`final') cannot be defined from Lisp: the stage predicate
;; takes two arguments, and define-specifier-tag insists on exactly one.
(message "%S" (condition-case e
                   (define-specifier-tag 'cnfonts-final nil 'specifier-tag-final-stage-p)
                 (error (error-message-string e))))
(message "%S" (condition-case e
                   (define-specifier-tag 'cnfonts-2d nil 'specifier-tag-two-dimensional-p)
                 (error (error-message-string e))))
(message "valid-specifier-tag-set-p (cnfonts-2d): %S"
         (valid-specifier-tag-set-p '(cnfonts-2d)))

(message "--- set a face font with it ---")
(message "%S" (condition-case e
                   (set-face-font 'default "Noto Sans Mono CJK SC-14"
                                  'global '(cnfonts-2d) 'prepend)
                 (error (error-message-string e))))
(message "default font now: %S" (face-property 'default 'font))

(message "--- does the charset predicate actually fire? ---")
(message "2d predicate on chinese-gb2312: %S"
         (specifier-tag-two-dimensional-p 'chinese-gb2312))
(message "2d predicate on ascii:           %S"
         (specifier-tag-two-dimensional-p 'ascii))
(message "charset predicate of cnfonts-2d:  %S"
         (specifier-tag-charset-predicate 'cnfonts-2d))
