;;; probe-charset-tags.el --- throwaway probe: charset-tag grouping API
;;
;; PROTOTYPE (wayfinder ticket #82).  NOT production code.
;;
;;   build/bin/xemacs --batch -l prototype/probe-charset-tags.el

(require 'mule-charset)
(require 'chinese)

(message "--- charset-tag-to-charset-list ---")
(dolist (tag '(chinese/language japanese/language korean/language
               kanji/script kana/script katakana/script hirigana/script
               hangul/script bopomofo/script
               chinese-gb/list chinese-big5/list chinese-cns/list
               japanese-kanji/list japanese/list))
  (message "%-24s %S" tag
           (condition-case e (charset-tag-to-charset-list tag)
             (error (error-message-string e)))))

(message "--- charset tags of a charset (symbol vs object) ---")
(message "tags of 'chinese-gb2312:   %S" (condition-case e (charset-tags 'chinese-gb2312) (error (error-message-string e))))
(message "tags of 'chinese-sisheng:  %S" (condition-case e (charset-tags 'chinese-sisheng) (error (error-message-string e))))
(message "tags of 'katakana-jisx0201: %S" (condition-case e (charset-tags 'katakana-jisx0201) (error (error-message-string e))))
(message "tags of 'korean-ksc5601:    %S" (condition-case e (charset-tags 'korean-ksc5601) (error (error-message-string e))))
(message "tags of 'jit-ucs-charset-0: %S" (condition-case e (charset-tags 'jit-ucs-charset-0) (error (error-message-string e))))

(message "--- what does a specifier-tag predicate receive? ---")
;; The C code calls the charset predicate with the charset OBJECT.
(define-specifier-tag 'probe-tag nil
  (lambda (charset)
    (message "   predicate got: %S  (symbolp=%S)  name=%S"
             charset (symbolp charset)
             (condition-case e (charset-name charset) (error (error-message-string e))))
    nil))
(message "--- so: how to test membership from a predicate ---")
(message "charset-name of object: %S"
         (condition-case e (charset-name (charset-list))
           (error (error-message-string e))))
