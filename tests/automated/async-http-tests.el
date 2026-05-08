;;; async-http-tests.el --- async HTTP integration tests

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; Skip if async HTTP not compiled in
(unless (fboundp 'async-http-request-internal)
  (message "Skipping async-http-tests: async HTTP not available")
  (provide 'async-http-tests))

(require 'async-core)
(require 'async-http)

(defvar async-http-test-port 18742)
(defvar async-http-test-server-proc nil)

(defun async-http-test-start-server ()
  (setq async-http-test-server-proc
        (start-process "async-test-server" " *async-test-server*"
                       "python3" "-c"
                       (format
                        "
import http.server, threading, json, time
class H(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def do_GET(self):
        if self.path == '/hello':
            body = b'hello world'
            self.send_response(200)
            self.send_header('Content-Type','text/plain')
            self.send_header('X-Test','yes')
            self.end_headers()
            self.wfile.write(body)
        elif self.path == '/stream':
            self.send_response(200)
            self.send_header('Content-Type','text/event-stream')
            self.end_headers()
            for i in range(5):
                self.wfile.write(('data: chunk%%d\\n\\n' %% i).encode())
                self.wfile.flush()
                time.sleep(0.01)
        elif self.path == '/notfound':
            body = b'not here'
            self.send_response(404)
            self.send_header('Content-Type','text/plain')
            self.end_headers()
            self.wfile.write(body)
    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length)
        # Echo back the body AND the request headers so the client can
        # assert that what it sent made it across the socket intact.
        resp = json.dumps({'received': body.decode(),
                           'content_type': self.headers.get('Content-Type',''),
                           'x_test': self.headers.get('X-Test','')}).encode()
        self.send_response(200)
        self.send_header('Content-Type','application/json')
        self.send_header('X-Reply','ok')
        self.end_headers()
        self.wfile.write(resp)
server = http.server.HTTPServer(('127.0.0.1', %d), H)
server.serve_forever()
" async-http-test-port)))
  (sleep-for 0.3))

(defun async-http-test-stop-server ()
  (when async-http-test-server-proc
    (delete-process async-http-test-server-proc)
    (setq async-http-test-server-proc nil)))

(async-http-test-start-server)

;; Test 1: buffered GET returns 200 with body
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/hello"
                                           async-http-test-port)))))
   nil)
  (let ((ti 0))
    (while (< ti 50)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (Assert (listp result) "GET returned a list")
  (Assert (= (car result) 200) "GET status 200")
  (Assert (string= (nth 2 result) "hello world") "GET body correct"))

;; Test 2: connection refused signals async-http-error
(let ((errored nil))
  (async-spawn-coroutine
   (lambda (arg)
     (condition-case nil
         (await (http-get "http://127.0.0.1:1"))
       (async-http-error (setq errored t))))
   nil)
  (let ((ti 0))
    (while (< ti 50)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (Assert errored "connection refused signals async-http-error"))

;; Test 3: response headers alist is populated by the header callback.
;; The server sets X-Test: yes on /hello; verify we see it at position 1.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/hello"
                                           async-http-test-port)))))
   nil)
  (let ((ti 0))
    (while (< ti 50)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (let* ((headers (nth 1 result))
         (xtest   (cdr (assoc "X-Test" headers)))
         (ctype   (cdr (assoc "Content-Type" headers))))
    (Assert (consp headers)          "GET headers is an alist")
    (Assert (string= xtest "yes")    "X-Test server header round-trips")
    (Assert (string= ctype "text/plain")
                                     "Content-Type server header present")))

;; Test 4: POST with explicit body + custom request header.  The server
;; echoes the body and the headers it saw, so we can prove they crossed
;; the socket rather than being mocked.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await
                   (http-post (format "http://127.0.0.1:%d/echo"
                                      async-http-test-port)
                              :headers '(("X-Test" . "probe-val"))
                              :body   "payload-42"))))
   nil)
  (let ((ti 0))
    (while (< ti 50)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (let* ((status (nth 0 result))
         (hdrs   (nth 1 result))
         (body   (nth 2 result))
         (reply-hdr (cdr (assoc "X-Reply" hdrs))))
    (Assert (= status 200)                      "POST returned 200")
    (Assert (string= reply-hdr "ok")            "server X-Reply header arrived")
    ;; Body is JSON; match the two fields we care about via plain substring.
    (Assert (string-match "\"received\": *\"payload-42\"" body)
                                                "request body reached server")
    (Assert (string-match "\"x_test\": *\"probe-val\"" body)
                                                "request header reached server")))

;; Test 5: streaming response body is reassembled from multiple chunks.
;; /stream emits 5 SSE frames with small sleeps; libcurl's write callback
;; fires once per chunk and the default (no :on-chunk) path accumulates
;; them into response_buf.  We assert all 5 frames are present in order.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/stream"
                                           async-http-test-port)))))
   nil)
  (let ((ti 0))
    (while (< ti 200)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (let ((body (nth 2 result)))
    (Assert (= (nth 0 result) 200)              "stream returned 200")
    (Assert (stringp body)                      "stream body is a string")
    (Assert (string-match "data: chunk0" body)  "chunk 0 present")
    (Assert (string-match "data: chunk1" body)  "chunk 1 present")
    (Assert (string-match "data: chunk2" body)  "chunk 2 present")
    (Assert (string-match "data: chunk3" body)  "chunk 3 present")
    (Assert (string-match "data: chunk4" body)  "chunk 4 present")
    ;; Order preserved: chunk0 precedes chunk4 in the body string.
    (Assert (< (string-match "chunk0" body)
               (string-match "chunk4" body))
                                                "chunks arrive in order")))

;; Test 6: non-200 status round-trips without being treated as an error.
;; libcurl only signals CURLE errors for transport failures; HTTP errors
;; like 404 are normal completions with status != 200.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/notfound"
                                           async-http-test-port)))))
   nil)
  (let ((ti 0))
    (while (< ti 50)
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti))))
  (Assert (= (nth 0 result) 404)                "404 status round-trips")
  (Assert (string= (nth 2 result) "not here")   "404 body arrives intact"))

;; NOTE on :on-chunk streaming: the C callback (src/async-http.c) dispatches
;; each chunk via `enqueue_misc_user_event', which is drained by the command
;; loop -- but the command loop doesn't run in `-batch'.  An :on-chunk
;; integration test needs a GUI/tty interactive harness; for now, test 5
;; above exercises the same socket path and libcurl callback cadence, just
;; via the accumulator.

(async-http-test-stop-server)
