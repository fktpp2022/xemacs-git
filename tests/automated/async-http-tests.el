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

(defvar async-http-test-directory
  (file-name-directory (or test-harness-current-file
                           load-file-name
                           buffer-file-name)))
(defvar async-http-test-fixture
  (expand-file-name "../elpa/fixture/queue-0.2.tar"
                    async-http-test-directory))

;; 1056 bytes: every byte value four times, then bytes 0x80, 0x81, 0xfe,
;; 0xff eight more times.  The 0x80-0xFF bytes are invalid as standalone
;; internal-Mule bytes, so this payload crashes a make_string() round-trip.
(defconst async-http-test-bin-payload-length 1056)
(defconst async-http-test-bin-payload-md5
  "487448015349532eb4abd9b49d314726")
(defconst async-http-test-tarball-size 10240)
(defconst async-http-test-tarball-md5
  "c1f635568dce3ab01b5adad023833ff2")

(defvar async-http-test-server-script
  "
import http.server, sys, time, json

PORT = int(sys.argv[1])
FIXTURE = sys.argv[2]

BINARY_PAYLOAD = bytes(range(256)) * 4 + bytes([0x80, 0x81, 0xfe, 0xff]) * 8

with open(FIXTURE, 'rb') as f:
    TARBALL = f.read()

class H(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def do_GET(self):
        path = self.path.split('?', 1)[0]
        if path == '/hello':
            body = b'hello world'
            self.send_response(200)
            self.send_header('Content-Type','text/plain')
            self.send_header('X-Test','yes')
            self.end_headers()
            self.wfile.write(body)
        elif path == '/binary':
            self.send_response(200)
            self.send_header('Content-Type','application/octet-stream')
            self.send_header('X-Test','binary')
            self.send_header('Content-Length', str(len(BINARY_PAYLOAD)))
            self.end_headers()
            self.wfile.write(BINARY_PAYLOAD)
        elif path == '/tarball':
            self.send_response(200)
            self.send_header('Content-Type','application/x-tar')
            self.send_header('X-Test','tarball')
            self.send_header('Content-Length', str(len(TARBALL)))
            self.end_headers()
            self.wfile.write(TARBALL)
        elif path.startswith('/redirect/'):
            n = int(path.rsplit('/', 1)[1])
            if n > 0:
                self.send_response(302)
                self.send_header('Location', '/redirect/%d' % (n - 1))
                self.end_headers()
            else:
                self.send_response(200)
                self.send_header('Content-Type','text/plain')
                self.end_headers()
                self.wfile.write(b'hello world')
        elif path == '/slow':
            time.sleep(1.0)
            self.send_response(200)
            self.send_header('Content-Type','text/plain')
            self.end_headers()
            self.wfile.write(b'slow')
        elif path == '/stream':
            self.send_response(200)
            self.send_header('Content-Type','text/event-stream')
            self.end_headers()
            for i in range(5):
                self.wfile.write(('data: chunk%d\\n\\n' % i).encode())
                self.wfile.flush()
                time.sleep(0.01)
        elif path == '/notfound':
            body = b'not here'
            self.send_response(404)
            self.send_header('Content-Type','text/plain')
            self.end_headers()
            self.wfile.write(body)
        else:
            self.send_response(404)
            self.end_headers()
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
server = http.server.HTTPServer(('127.0.0.1', PORT), H)
server.serve_forever()
")

(defun async-http-test-start-server ()
  (setq async-http-test-server-proc
        (start-process "async-test-server" " *async-test-server*"
                       "python3" "-c" async-http-test-server-script
                       (number-to-string async-http-test-port)
                       async-http-test-fixture))
  (sleep-for 0.5))

(defun async-http-test-wait-until (predicate)
  "Tick the scheduler until PREDICATE is non-nil or about six seconds pass."
  (let ((ti 0))
    (while (and (not (funcall predicate)) (< ti 600))
      (async-scheduler-tick)
      (sleep-for 0.01)
      (setq ti (1+ ti)))))

(defun async-http-test-file-md5 (file)
  "Return the MD5 of FILE's exact bytes, read without coding conversion."
  (with-temp-buffer
    (insert-file-contents-literally file)
    (md5 (current-buffer) nil nil 'binary)))

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

;; Test 7: binary payload with bytes that are invalid in the internal Mule
;; encoding downloads to memory byte-exactly.  The body must be a binary
;; external string built with make_extstring/Qbinary, not make_string.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/binary"
                                           async-http-test-port)))))
   nil)
  (async-http-test-wait-until (lambda () result))
  (Assert (= (nth 0 result) 200)                "binary GET status 200")
  (let ((body (nth 2 result)))
    (Assert (stringp body)                      "binary body is a string")
    (Assert (= (length body) async-http-test-bin-payload-length)
                                                "binary body length exact")
    (Assert (string= (md5 body nil nil 'binary)
                     async-http-test-bin-payload-md5)
                                                "binary body md5 exact")
    (Assert (= (aref body 130) 130)             "byte 0x82 survives")
    (Assert (= (aref body 254) 254)             "byte 0xfe survives")
    (Assert (= (aref body 255) 255)             "byte 0xff survives")))

;; Test 8: the same binary payload downloaded to a file is byte-identical
;; and the file path round-trips in the third result slot.  This path writes
;; response bytes directly from C with fwrite and never builds a body string.
(let ((dest (make-temp-name "/tmp/xemacs-async-http-25-"))
      (result nil))
  (unwind-protect
      (progn
        (async-spawn-coroutine
         (lambda (arg)
           (setq result (await (http-get
                                (format "http://127.0.0.1:%d/binary"
                                        async-http-test-port)
                                :file dest))))
         nil)
        (async-http-test-wait-until (lambda () result))
        (Assert (= (nth 0 result) 200)          "binary file GET status 200")
        (Assert (string= (cdr (assoc "X-Test" (nth 1 result))) "binary")
                                                "binary file headers collected")
        (Assert (string= (nth 2 result) dest)   "file destination returned")
        (Assert (file-exists-p dest)            "destination file created")
        (Assert (string= (async-http-test-file-md5 dest)
                         async-http-test-bin-payload-md5)
                                                "binary file md5 exact"))
    (when (file-exists-p dest) (delete-file dest))))

;; Test 9: the vendored queue-0.2.tar downloads to memory byte-exactly.
;; This is the acceptance payload from the milestone spec.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/tarball"
                                           async-http-test-port)))))
   nil)
  (async-http-test-wait-until (lambda () result))
  (Assert (= (nth 0 result) 200)                "tarball GET status 200")
  (let ((body (nth 2 result)))
    (Assert (= (length body) async-http-test-tarball-size)
                                                "tarball body length exact")
    (Assert (string= (md5 body nil nil 'binary) async-http-test-tarball-md5)
                                                "tarball body md5 exact")))

;; Test 10: the queue-0.2.tar file destination is byte-identical.
(let ((dest (make-temp-name "/tmp/xemacs-async-http-25-"))
      (result nil))
  (unwind-protect
      (progn
        (async-spawn-coroutine
         (lambda (arg)
           (setq result (await (http-download
                                (format "http://127.0.0.1:%d/tarball"
                                        async-http-test-port)
                                dest))))
         nil)
        (async-http-test-wait-until (lambda () result))
        (Assert (= (nth 0 result) 200)          "tarball file GET status 200")
        (Assert (string= (nth 2 result) dest)   "http-download returns FILE")
        (Assert (string= (async-http-test-file-md5 dest)
                         async-http-test-tarball-md5)
                                                "tarball file md5 exact"))
    (when (file-exists-p dest) (delete-file dest))))

;; Test 11: a redirect chain within the cap (5 hops) succeeds.
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (http-get (format "http://127.0.0.1:%d/redirect/5"
                                           async-http-test-port)))))
   nil)
  (async-http-test-wait-until (lambda () result))
  (Assert (= (nth 0 result) 200)                "5-hop redirect succeeds")
  (Assert (string= (nth 2 result) "hello world")
                                                "5-hop redirect body correct"))

;; Test 12: a redirect chain beyond CURLOPT_MAXREDIRS (10) fails with
;; async-http-error instead of following forever.
(let ((errored nil)
      (err-msg nil))
  (async-spawn-coroutine
   (lambda (arg)
     (condition-case err
         (await (http-get (format "http://127.0.0.1:%d/redirect/11"
                                  async-http-test-port)))
       (async-http-error
        (setq errored t
              err-msg (nth 1 err)))))
   nil)
  (async-http-test-wait-until (lambda () errored))
  (Assert errored                                "11-hop redirect signals error")
  (Assert (and (stringp err-msg) (> (length err-msg) 0))
                                                "redirect error has a message")
  (Assert (string-match "redirect" (downcase err-msg))
                                                "redirect error names redirects"))

;; Test 13: a total timeout surfaces as async-http-error with a message.
;; /slow sleeps for one second; :timeout 150 aborts the transfer first.
(let ((errored nil)
      (err-msg nil))
  (async-spawn-coroutine
   (lambda (arg)
     (condition-case err
         (await (http-get (format "http://127.0.0.1:%d/slow"
                                  async-http-test-port)
                          :timeout 150))
       (async-http-error
        (setq errored t
              err-msg (nth 1 err)))))
   nil)
  (async-http-test-wait-until (lambda () errored))
  (Assert errored                                "timeout signals error")
  (Assert (and (stringp err-msg) (> (length err-msg) 0))
                                                "timeout error has a message")
  (Assert (string-match "timeout" (downcase err-msg))
                                                "timeout error names timeout"))

;; NOTE on :on-chunk streaming: the C callback (src/async-http.c) dispatches
;; each chunk via `enqueue_misc_user_event', which is drained by the command
;; loop -- but the command loop doesn't run in `-batch'.  An :on-chunk
;; integration test needs a GUI/tty interactive harness; for now, test 5
;; above exercises the same socket path and libcurl callback cadence, just
;; via the accumulator.

(async-http-test-stop-server)
