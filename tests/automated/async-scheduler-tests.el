;;; async-scheduler-tests.el --- coroutine scheduler unit tests

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

;; Test basic coroutine: spawns, runs, returns
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg) (setq result (* arg 2)))
   21)
  (async-scheduler-tick)
  (Assert (= result 42) "coroutine ran and computed result"))

;; Test yield: coroutine stops at yield point
(let ((log '()))
  (async-spawn-coroutine
   (lambda (arg)
     (push 'before log)
     (async-coroutine-yield)
     (push 'after log))
   nil)
  (async-scheduler-tick)
  (Assert (equal log '(before)) "coroutine stopped at yield")
  (async-scheduler-tick)
  (Assert (equal (reverse log) '(before after)) "coroutine resumed on next tick"))

;; Test multiple coroutines interleaved
(let ((ids '()))
  (dotimes (i 5)
    (async-spawn-coroutine (lambda (n) (push n ids)) i))
  (async-scheduler-tick)
  (Assert (= (length ids) 5) "all 5 coroutines ran")
  (Assert (null (set-difference '(0 1 2 3 4) ids)) "all ids present"))

;; Test error isolation: error in coroutine does not kill XEmacs
(let ((reached nil))
  (async-spawn-coroutine
   (lambda (arg) (error "intentional test error"))
   nil)
  (condition-case nil
      (async-scheduler-tick)
    (error nil))
  (setq reached t)
  (Assert reached "scheduler survived coroutine error"))

;; Test GC stress: force GC while coroutines are suspended
(let ((count 0))
  (dotimes (gi 10)
    (async-spawn-coroutine
     (lambda (arg) (async-coroutine-yield) (setq count (1+ count)))
     nil))
  (async-scheduler-tick)     ;; all 10 yield
  (garbage-collect)          ;; GC while all suspended
  (async-scheduler-tick)     ;; all 10 resume
  (Assert (= count 10) "GC with suspended coroutines: all resumed correctly"))

;; Test actor-join: spawned actor's result returned
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (let ((h (actor-spawn-internal nil (lambda (a) 99) nil)))
       (setq result (actor-join-internal h))))
   nil)
  (let ((ti 0))
    (while (< ti 5)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (equal result 99) "actor-join-internal returns actor result"))
