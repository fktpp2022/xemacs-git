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

;; Test 4: async-shell-command :timeout kills a command that runs too long.
;; Not runnable yet: :timeout requires C-level WAIT_FD integration so the
;; subprocess doesn't block the main thread.  The current implementation
;; runs call-process synchronously inside the worker coroutine, so spawning
;; `sleep 60' would block XEmacs for 60 seconds and defeat the timer.
;; When WAIT_FD lands, replace this with a real timeout assertion.
(Known-Bug-Expect-Failure
 (Assert nil "async-shell-command :timeout — pending WAIT_FD integration"))

;; Test 5: async-read-file on a missing path re-signals `file-error' across
;; `await'.  `insert-file-contents-literally' fails with a 4-element error
;; list (file-error "Opening input file" "No such file or directory" PATH);
;; `await' must call `signal' with (car ERR) (cdr ERR), not `apply', so the
;; original `file-error' reaches the caller's `condition-case'.
(let ((caught 'unset))
  (async-spawn-coroutine
   (lambda (arg)
     (condition-case e
         (await (async-read-file "/no/such/path/async-file-test-probe"))
       (file-error (setq caught e))))
   nil)
  (let ((ti 0))
    (while (< ti 10)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (and (consp caught) (eq (car caught) 'file-error))
          "async-read-file on missing path re-signals file-error via await")
  (Assert (and (consp caught)
               (let ((path (car (last caught))))
                 (and (stringp path)
                      (string-match "async-file-test-probe" path))))
          "file-error data preserves the failing path"))

;; Test 6: async-shell-command on a command that exits non-zero.  The
;; current implementation captures stdout regardless of exit status, so a
;; bare `false' produces an empty stdout string -- but it MUST not error
;; or hang.  This documents the current behavior; if future work surfaces
;; the exit code, update this test.
(let ((result 'unset))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (async-shell-command "false"))))
   nil)
  (let ((ti 0))
    (while (< ti 10)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (stringp result)
          "async-shell-command returns string even on non-zero exit")
  (Assert (string= result "")
          "stdout of `false' is empty"))

;; Test 7: async-shell-command with shell features -- a multi-line printf
;; round-trips through call-process `shell-file-name shell-command-switch'
;; and the buffer-capture path in `async-file.el'.
(let ((result 'unset))
  (async-spawn-coroutine
   (lambda (arg)
     (setq result (await (async-shell-command "printf 'a\nb\nc'"))))
   nil)
  (let ((ti 0))
    (while (< ti 10)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (string= result "a\nb\nc")
          "async-shell-command captures multi-line shell output verbatim"))
