;;; swe-agent.el --- minimal SWE-agent using XEmacs async IO

(require 'async-core)
(require 'actor)
(require 'async-http)
(require 'async-file)
(require 'json nil t)

(defvar swe-agent-api-url "https://api.openai.com/v1/chat/completions"
  "LLM API endpoint URL.")

(defvar swe-agent-model "gpt-4o"
  "LLM model name.")

(defvar swe-agent-api-key nil
  "API key. Set via (setq swe-agent-api-key \"sk-...\").")

(defun swe-agent--headers ()
  (list (cons "Authorization" (concat "Bearer " swe-agent-api-key))
        (cons "Content-Type" "application/json")))

(defun swe-agent--extract-text (chunk)
  "Extract text delta from OpenAI SSE chunk."
  (let ((data (sse-parse-data (car (split-string chunk "\n")))))
    (when (and data (not (equal data "[DONE]")))
      (condition-case nil
          (let* ((obj (json-read-from-string data))
                 (choices (cdr (assq 'choices obj)))
                 (delta (when (and choices (> (length choices) 0))
                          (cdr (assq 'delta (aref choices 0)))))
                 (text (when delta (cdr (assq 'content delta)))))
            text)
        (error nil)))))

(defun swe-agent--parse-tools (response)
  "Extract tool_calls from JSON response, or nil."
  (condition-case nil
      (let* ((obj (json-read-from-string response))
             (choices (cdr (assq 'choices obj)))
             (msg (when (and choices (> (length choices) 0))
                    (cdr (assq 'message (aref choices 0)))))
             (tools (when msg (cdr (assq 'tool_calls msg)))))
        (when (and tools (> (length tools) 0))
          (mapcar (lambda (tc)
                    (list :id   (cdr (assq 'id tc))
                          :name (cdr (assq 'name (cdr (assq 'function tc))))
                          :args (condition-case nil
                                    (json-read-from-string
                                     (cdr (assq 'arguments
                                                (cdr (assq 'function tc)))))
                                  (error nil))))
                  tools)))
    (error nil)))

(defun swe-agent--execute-tool (call)
  "Execute tool CALL plist. Returns result string."
  (let ((name (plist-get call :name))
        (args (plist-get call :args)))
    (cond
     ((equal name "read_file")
      (await (async-read-file (cdr (assq 'path args)))))
     ((equal name "run_shell")
      (await (async-shell-command (cdr (assq 'command args))
                                  :timeout 30000)))
     ((equal name "write_file")
      (await (async-write-file (cdr (assq 'path args))
                               (cdr (assq 'content args))))
      "ok")
     (t (format "unknown tool: %s" name)))))

(defun swe-agent--run-tools-parallel (tool-calls)
  "Execute all TOOL-CALLS in parallel. Returns alist of id -> result."
  (let ((actors (mapcar (lambda (call)
                          (cons (plist-get call :id)
                                (actor-spawn nil 'swe-agent--execute-tool call)))
                        tool-calls)))
    (mapcar (lambda (pair)
              (cons (car pair)
                    (await (actor-join (cdr pair)))))
            actors)))

(defun swe-agent-loop (task output-buffer)
  "Run SWE-agent loop for TASK, streaming to OUTPUT-BUFFER."
  (unless swe-agent-api-key
    (error "swe-agent-api-key is not set"))
  (actor-spawn
   'swe-agent
   (lambda (_)
     (let ((messages (vector (list (cons 'role "user") (cons 'content task))))
           (done nil))
       (while (not done)
         (let ((chunks '()))
           (with-current-buffer output-buffer (goto-char (point-max)))
           (await (http-post swe-agent-api-url
                             :headers (swe-agent--headers)
                             :json (list (cons 'model swe-agent-model)
                                         (cons 'stream t)
                                         (cons 'messages messages))
                             :on-chunk
                             (lambda (chunk)
                               (let ((text (swe-agent--extract-text chunk)))
                                 (when text
                                   (push text chunks)
                                   (with-current-buffer output-buffer
                                     (goto-char (point-max))
                                     (insert text)))))))
           (let* ((response (apply 'concat (nreverse chunks)))
                  (tool-calls (swe-agent--parse-tools response)))
             (if (null tool-calls)
                 (setq done t)
               (let ((results (swe-agent--run-tools-parallel tool-calls)))
                 (setq messages
                       (vconcat messages
                                (vector
                                 (list (cons 'role "assistant")
                                       (cons 'content response))
                                 (list (cons 'role "tool")
                                       (cons 'tool_results
                                             (mapcar (lambda (r)
                                                       (list (cons 'tool_call_id (car r))
                                                             (cons 'content (cdr r))))
                                                     results)))))))))))))
   nil))

(defun swe-agent (task)
  "Start a SWE-agent session for TASK."
  (interactive "sTask: ")
  (let ((buf (get-buffer-create
              (format "*swe-agent: %s*"
                      (substring task 0 (min 40 (length task)))))))
    (switch-to-buffer buf)
    (erase-buffer)
    (swe-agent-loop task buf)
    (message "SWE-agent started in %s" (buffer-name buf))))

(provide 'swe-agent)
