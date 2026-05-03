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
    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length)
        resp = json.dumps({'received': body.decode()}).encode()
        self.send_response(200)
        self.send_header('Content-Type','application/json')
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
   (lambda (_)
     (setq result (await (http-get (format "http://127.0.0.1:%d/hello"
                                           async-http-test-port)))))
   nil)
  (dotimes (_ 50) (async-scheduler-tick) (sleep-for 0.01))
  (Assert (listp result) "GET returned a list")
  (Assert (= (car result) 200) "GET status 200")
  (Assert (string= (nth 2 result) "hello world") "GET body correct"))

;; Test 2: connection refused signals async-http-error
(let ((errored nil))
  (async-spawn-coroutine
   (lambda (_)
     (condition-case nil
         (await (http-get "http://127.0.0.1:1"))
       (async-http-error (setq errored t))))
   nil)
  (dotimes (_ 50) (async-scheduler-tick) (sleep-for 0.01))
  (Assert errored "connection refused signals async-http-error"))

(async-http-test-stop-server)
