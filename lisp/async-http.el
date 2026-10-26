;;; async-http.el --- async HTTP with SSE streaming for XEmacs

(require 'async-core)
(require 'json nil t)

(defun http-get (url &rest keys)
  "Async GET URL.  Pass to `await' for result (STATUS HEADERS BODY).
:headers ALIST, :timeout MS, :on-chunk FUNC, :file PATH.

With :file PATH the response body is written directly to PATH by C and is
never materialized as a Lisp string; `await' then returns
\(STATUS HEADERS PATH) with the expanded file name in the body slot.  If
the transfer fails, the partial file is removed and `async-http-error' is
signaled.  The TCP connect phase is bounded even when :timeout is nil."
  (let ((headers  (plist-get keys :headers))
        (timeout  (plist-get keys :timeout))
        (on-chunk (plist-get keys :on-chunk))
        (file     (plist-get keys :file)))
    (when file
      (setq file (expand-file-name file)))
    (async-http-request-internal url "GET" headers nil on-chunk timeout file)))

(defun http-download (url file &rest keys)
  "Async download of URL into FILE.  Pass to `await' for (STATUS HEADERS FILE).
FILE is expanded with `expand-file-name'.  Response bytes are written
directly to FILE and never pass through a Lisp string; the third result
element is the expanded file name.  On failure nothing is left behind and
`async-http-error' is signaled.  KEYS are as for `http-get'."
  (apply #'http-get url (append (list :file file) keys)))

(defun http-post (url &rest keys)
  "Async POST to URL. Pass to `await' for result (STATUS HEADERS BODY).
:headers ALIST :json OBJECT :body STRING :on-chunk FUNC :timeout MS"
  (let* ((headers  (plist-get keys :headers))
         (json-obj (plist-get keys :json))
         (body     (plist-get keys :body))
         (on-chunk (plist-get keys :on-chunk))
         (timeout  (plist-get keys :timeout))
         (body-str (cond
                    (json-obj
                     (setq headers
                           (cons '("Content-Type" . "application/json") headers))
                     (json-encode json-obj))
                    (body body)
                    (t nil))))
    (async-http-request-internal url "POST" headers body-str on-chunk timeout)))

(defun http-stream (url &rest keys)
  "Async streaming request to URL. :on-chunk FUNC required."
  (apply #'http-post url keys))

;;; SSE parsing

(defun sse-parse-data (line)
  "Extract payload from SSE LINE (\"data: {...}\"). Returns string or nil."
  (when (and (>= (length line) 6)
             (string= (substring line 0 6) "data: "))
    (substring line 6)))

(defun sse-extract-text (chunk)
  "Parse SSE CHUNK, extract text from OpenAI/Anthropic streaming format."
  (let ((result '()))
    (dolist (line (split-string chunk "\n"))
      (let ((data (sse-parse-data line)))
        (when (and data (not (equal data "[DONE]")))
          (condition-case nil
              (let* ((obj     (json-read-from-string data))
                     (choices (cdr (assq 'choices obj)))
                     (delta   (when (and choices (> (length choices) 0))
                                (cdr (assq 'delta (aref choices 0)))))
                     (text    (when delta (cdr (assq 'content delta)))))
                (when text (push text result)))
            (error nil)))))
    (when result
      (apply #'concat (nreverse result)))))

(provide 'async-http)
