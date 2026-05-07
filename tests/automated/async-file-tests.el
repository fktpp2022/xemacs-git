;;; async-file-tests.el --- async file IO and shell unit tests

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(unless (fboundp 'actor-spawn-internal)
  (message "Skipping async-file-tests: async scheduler not available")
  (provide 'async-file-tests))

(require 'async-core)
(require 'actor)
(require 'async-file)

;; Test 1: async-read-file returns file contents
(let ((result nil)
      (tmpfile (make-temp-file "async-file-test-")))
  (unwind-protect
      (progn
        (with-temp-file tmpfile (insert "hello async"))
        (async-spawn-coroutine
         (lambda (arg)
           (setq result (await (async-read-file tmpfile))))
         nil)
        (let ((ti 0))
          (while (< ti 10)
            (async-scheduler-tick)
            (setq ti (1+ ti))))
        (Assert (string= result "hello async") "async-read-file returns file contents"))
    (delete-file tmpfile)))

;; Test 2: async-write-file writes content, async-read-file reads it back
(let ((result nil)
      (tmpfile (make-temp-file "async-file-test-")))
  (unwind-protect
      (progn
        (async-spawn-coroutine
         (lambda (arg)
           (await (async-write-file tmpfile "written by async"))
           (setq result (await (async-read-file tmpfile))))
         nil)
        (let ((ti 0))
          (while (< ti 20)
            (async-scheduler-tick)
            (setq ti (1+ ti))))
        (Assert (string= result "written by async") "async-write-file roundtrip"))
    (delete-file tmpfile)))

;; Test 3: async-shell-command returns output string
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (async-shell-command "echo hello"))))
   nil)
  (let ((ti 0))
    (while (< ti 10)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (string-match "hello" result) "async-shell-command returns output"))

;; Test 4: async-shell-command :timeout kills a command that runs too long
;; Requires C-level WAIT_FD integration; not yet implemented.
(Known-Bug-Expect-Failure
 (let ((timed-out nil))
   (async-spawn-coroutine
    (lambda (arg)
      (condition-case err
          (await (async-shell-command "sleep 60" :timeout 200))
        (async-timeout (setq timed-out t))))
    nil)
   (let ((ti 0))
     (while (and (< ti 100) (not timed-out))
       (async-scheduler-tick)
       (sleep-for 0.01)
       (setq ti (1+ ti))))
   (Assert timed-out "async-shell-command :timeout kills long-running command")))
