;;; async-http.el --- async HTTP with SSE streaming for XEmacs

(require 'async-core)
(require 'json nil t)

(defun http-get (url &rest keys)
  "Async GET URL. Pass to `await' for result (STATUS HEADERS BODY).
:headers ALIST :timeout MS"
  (let ((headers (plist-get keys :headers))
        (timeout (plist-get keys :timeout)))
    (async-http-request-internal url "GET" headers nil nil timeout)))

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
