;;; async-scheduler-tests.el --- coroutine scheduler unit tests

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness)))
  ;; `await' is a macro from async-core; some of the cases below use it.
  (require 'async-core))
(require 'async-core)

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

;; Regression: a suspended coroutine must not leak its condition_case_1
;; handler frame into the global Vcondition_handlers list.  If it does,
;; later top-level `signal' loops back to a `struct catchtag' that is no
;; longer in `catchlist' and Fsignal recurses until `throw_level' aborts.
;; We verify by spawning a coro that yields indefinitely (never returns),
;; then confirming a top-level `(signal ...)' is caught by an outer
;; `condition-case' as usual.
(let ((outer-caught nil))
  (async-spawn-coroutine
   (lambda (_)
     (while t (async-coroutine-yield)))
   nil)
  (async-scheduler-tick)          ;; coro enters trampoline, yields
  (condition-case err
      (signal 'error '("top-level error after coro suspended"))
    (error (setq outer-caught err)))
  (Assert (and (consp outer-caught) (eq (car outer-caught) 'error))
          "top-level signal is caught normally after coro yield"))

;; Regression: garbage-collect with a long-yielded coroutine must not
;; follow dangling references in either the saved gcprolist or the
;; detached Vcondition_handlers chain.  Triggers the same GC-mark path
;; that used to SIGSEGV during the test-harness's byte-compiled run.
(let ((_survived nil))
  (async-spawn-coroutine
   (lambda (_)
     (while t (async-coroutine-yield)))
   nil)
  (async-scheduler-tick)
  (garbage-collect)
  (async-scheduler-tick)
  (garbage-collect)
  (setq _survived t)
  (Assert _survived "GC across suspended coroutine with condition_case_1"))

;; The timer-driven wakeup callback must exist as a callable Lisp function
;; so `execute_internal_event' can dispatch the timeout_event.
(Assert (functionp 'async--scheduler-tick-from-timer)
        "async--scheduler-tick-from-timer is defined")
;; It accepts and ignores an argument; calling it directly runs a tick.
(let ((before-count 0) (after-count 0))
  (async-spawn-coroutine
   (lambda (_) (setq before-count 1))
   nil)
  (setq after-count (progn (async--scheduler-tick-from-timer nil) before-count))
  (Assert (= after-count 1)
          "async--scheduler-tick-from-timer runs a tick"))

;; Dynamic-scope inheritance: a coro spawned while a variable is let-bound
;; must see the let-bound value, not the global.  Exercises the specpdl
;; memcpy + specpdl_rebind_range path in coro_spawn / async_scheduler_tick.
(defvar async-scheduler-test-inherit-var 'global-value)
(let ((observed nil))
  (let ((async-scheduler-test-inherit-var 'let-bound-value))
    (async-spawn-coroutine
     (lambda (_)
       (setq observed async-scheduler-test-inherit-var))
     nil)
    (async-scheduler-tick))
  (Assert (eq observed 'let-bound-value)
          "coroutine inherits spawner's let-bound dynamic binding"))

;; Joining a target that is already dead must return its result immediately
;; via the graveyard fast-path in actor_join_internal.
(let ((target (actor-spawn-internal nil (lambda (_) 77) nil))
      (result 'unset))
  (async-scheduler-tick)          ;; run target to completion
  (async-spawn-coroutine
   (lambda (_)
     (setq result (actor-join-internal target)))
   nil)
  (async-scheduler-tick)          ;; run joiner; hits dead-target branch
  (Assert (= result 77)
          "actor-join-internal returns result when target already dead"))

;; An error inside an actor body must propagate across `await' as a
;; re-signal in the joining coroutine.  `await' macroexpands to
;; actor-join-internal + a check of `async--coro-error' that calls
;; (apply 'signal ...) when set.
(let ((caught nil))
  (async-spawn-coroutine
   (lambda (_)
     (let ((h (actor-spawn-internal nil
                                    (lambda (_) (error "intentional boom"))
                                    nil)))
       (condition-case e
           (await h)
         (error (setq caught e)))))
   nil)
  (let ((ti 0))
    (while (< ti 20) (async-scheduler-tick) (setq ti (1+ ti))))
  (Assert (and (consp caught) (eq (car caught) 'error))
          "error in actor body propagates across await via condition-case"))

;; async-current-coroutine reports nil at top level and distinct live
;; handles inside each running coroutine.
(let ((outer-self 'unset)
      (inner-self 'unset))
  (Assert (null (async-current-coroutine))
          "async-current-coroutine is nil at top level")
  (async-spawn-coroutine
   (lambda (_)
     (setq outer-self (async-current-coroutine))
     (let ((ih (actor-spawn-internal
                nil
                (lambda (_) (setq inner-self (async-current-coroutine)))
                nil)))
       (actor-join-internal ih)))
   nil)
  (let ((ti 0))
    (while (< ti 10) (async-scheduler-tick) (setq ti (1+ ti))))
  (Assert (async-coroutine-p outer-self)
          "async-current-coroutine returns a valid handle in outer coro")
  (Assert (async-coroutine-p inner-self)
          "async-current-coroutine returns a valid handle in inner coro"))
