;; PROTOTYPE (wayfinder ticket #79) -- recording stubs for the GNU APIs that
;; XEmacs 21.5 does not have.  NOT production code, NOT the future shim.
;;
;; Every stub records its arguments in `cnfonts-stub-calls' and returns a
;; plausible placeholder, so the driver can push upstream cnfonts as far as it
;; goes and then report the exact call surface a real shim would have to lower.
;;
;; Three groups:
;;   1. the five GNU font APIs ADR 0002 names;
;;   2. three more GNU APIs cnfonts.el calls that ADR 0002 does NOT name
;;      (list-fonts, font-family-list, string-join);
;;   3. two non-font gaps (mouse-wheel-text-scale, touch-screen-pinch).

(defvar cnfonts-stub-calls nil
  "Recorded calls to the stubbed GNU APIs, most recent first.")

(defun cnfonts--stub-record (fn args)
  (setq cnfonts-stub-calls (cons (cons fn args) cnfonts-stub-calls))
  nil)

;;; 1. the five GNU font APIs named by ADR 0002

(defun font-spec (&rest args)
  "GNU font-spec.  Records the plist; returns an XEmacs-style font name.

  The real shim has to lower :name/:size/:weight/:slant into whatever
  XEmacs' set-face-font accepts -- a name string or a font object.  This
  stub returns the crude \"Family-Size\" form so the rest of cnfonts runs."
  (cnfonts--stub-record 'font-spec args)
  (format "%s-%s" (plist-get args :name) (plist-get args :size)))

(defun set-face-attribute (face frame &rest args)
  "GNU set-face-attribute.  Only :font is used by cnfonts."
  (cnfonts--stub-record 'set-face-attribute (list face frame args))
  (when (assq :font args)
    (set-face-font face (plist-get args :font) 'global nil 'prepend))
  nil)

(defun face-attribute (face attribute &optional frame inherit)
  "GNU face-attribute.  cnfonts.el never calls this; cnfonts-ui.el does."
  (cnfonts--stub-record 'face-attribute (list face attribute frame inherit))
  (face-property face attribute))

(defun x-list-fonts (pattern &optional wildcard-p frame max-number)
  "GNU x-list-fonts.  Returns one fake XLFD so cnfonts's parser has input."
  (cnfonts--stub-record 'x-list-fonts (list pattern wildcard-p frame max-number))
  (list (format "-*-%s-medium-r-*-*-*-12-*-*-*-*-iso8859-1" pattern)))

(defun set-fontset-font (fontset charset font &optional frame how-to-add)
  "GNU set-fontset-font.  No fontset exists on XEmacs; record only."
  (cnfonts--stub-record 'set-fontset-font
                        (list fontset charset font frame how-to-add))
  nil)

;;; 2. three more GNU APIs cnfonts.el calls that ADR 0002 does not name

(defun list-fonts (font &optional face frame)
  "GNU list-fonts: non-nil when FONT is available."
  (cnfonts--stub-record 'list-fonts (list font face frame))
  t)

(defun font-family-list (&optional frame)
  "GNU font-family-list: the installed font families."
  (cnfonts--stub-record 'font-family-list (list frame))
  '("Consolas" "DejaVu Sans Mono" "Monaco" "Courier" "Courier New"))

(defun string-join (strings &optional separator)
  "GNU string-join."
  (cnfonts--stub-record 'string-join (list strings separator))
  (mapconcat #'identity strings (or separator "")))

;;; 3. non-font gaps

(defun mouse-wheel-text-scale (amount)
  "GNU mouse-wheel-text-scale (XEmacs mwheel has a different API)."
  (cnfonts--stub-record 'mouse-wheel-text-scale (list amount))
  nil)

(defun touch-screen-pinch (event)
  "GNU touch-screen-pinch; touch-screen is soft-required by cnfonts."
  (cnfonts--stub-record 'touch-screen-pinch (list event))
  nil)

(provide 'xemacs-shim)

;;; xemacs-shim.el ends here
