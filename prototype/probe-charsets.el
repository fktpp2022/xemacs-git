;;; probe-charsets.el --- throwaway probe: XEmacs' charset inventory for CJK
;;
;; PROTOTYPE (wayfinder ticket #82).  NOT production code.
;;
;;   build/bin/xemacs --batch -l prototype/probe-charsets.el

(dolist (c (sort (copy-sequence (charset-list)) #'string<))
  (let ((dim (condition-case e (charset-dimension c) (error :err))))
    (when (memq dim '(1 2))
      (message "%-28s dimension=%S" c dim))))

(message "--- JIT charsets ---")
(dolist (c (sort (copy-sequence (charset-list)) #'string<))
  (when (string-match-p "jit" (symbol-name c))
    (message "%-28s dimension=%S" c (charset-dimension c))))

(message "--- grouping lists in lisp/mule ---")
(dolist (sym '(chinese-gb/list chinese-big5/list chinese-cns/list
               chinese-gb-list chinese-big5-list chinese-cns-list))
  (message "%-24s bound=%S value=%S" sym (boundp sym) (and (boundp sym) (symbol-value sym))))

(message "--- 2D tag predicate on some charsets ---")
(dolist (c '(chinese-gb2312 japanese-euc-jp korean-euc-kr ascii))
  (message "%-20s 2d-p=%S" c
           (condition-case e (specifier-tag-two-dimensional-p c) (error (error-message-string e)))))
