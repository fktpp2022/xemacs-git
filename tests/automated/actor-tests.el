;;; actor-tests.el --- actor model unit tests

(eval-when-compile
  (condition-case nil
      (require 'test-harness)
    (file-error
     (push "." load-path)
     (when (and (boundp 'load-file-name) (stringp load-file-name))
       (push (file-name-directory load-file-name) load-path))
     (require 'test-harness))))

(unless (fboundp 'actor-spawn-internal)
  (message "Skipping actor-tests: async scheduler not available")
  (provide 'actor-tests))

(require 'async-core)
(require 'actor)

;; Test 1: actor-spawn returns a handle
(let ((h (actor-spawn nil (lambda (a) 42) nil)))
  (Assert (not (null h)) "actor-spawn returns non-nil handle"))

;; Test 2: actor-join returns the actor's return value
(let ((result nil))
  (async-spawn-coroutine
   (lambda (arg)
     (let ((h (actor-spawn nil (lambda (a) 99) nil)))
       (setq result (actor-join h))))
   nil)
  (let ((ti 0))
    (while (< ti 10)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (equal result 99) "actor-join returns correct value"))

;; Test 3: actor-send wakes a waiting actor
(let ((delivered nil))
  (let ((h (actor-spawn nil
                        (lambda (a)
                          (actor-receive)
                          (setq delivered t))
                        nil)))
    (async-scheduler-tick)
    (actor-send h 'wake-up)
    (let ((ti 0))
      (while (< ti 10)
        (async-scheduler-tick)
        (setq ti (1+ ti)))))
  (Assert delivered "message delivered to waiting actor"))

;; Test 4: async-let parallel bind
(let ((results nil))
  (async-spawn-coroutine
   (lambda (arg)
     (setq results
           (async-let ((a (progn (async-coroutine-yield) 'a))
                       (b (progn (async-coroutine-yield) 'b))
                       (c (progn (async-coroutine-yield) 'c)))
             (list a b c))))
   nil)
  (let ((ti 0))
    (while (< ti 20)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (equal results '(a b c)) "async-let parallel binding works"))

;; Test 5: nested coroutines do not corrupt each other
(let ((sum 0))
  (dotimes (ci 5)
    (async-spawn-coroutine
     (lambda (n)
       (async-coroutine-yield)
       (setq sum (+ sum n)))
     ci))
  (async-scheduler-tick)
  (async-scheduler-tick)
  (Assert (= sum 10) "5 coroutines summed correctly"))

;; Test 6: actor-monitor does not evict an existing join waiter
;; When both actor-join and actor-monitor are set on the same target,
;; the joiner must still receive its result and the monitor must receive :exit.
;; The outer coroutine must be an actor so actor-receive works.
(let ((join-result 'unset)
      (monitor-received 'unset))
  (actor-spawn-internal
   'test6-outer
   (lambda (arg)
     (let* ((c (actor-spawn-internal nil
                                     (lambda (a)
                                       (async-coroutine-yield)
                                       42)
                                     nil))
            (joiner (actor-spawn-internal
                     nil
                     (lambda (handle)
                       (setq join-result (actor-join-internal handle)))
                     (list c))))
       ;; Monitor C from the outer actor — must not evict joiner's join slot
       (actor-monitor c)
       ;; Yield enough times for C and joiner to run to completion
       (async-coroutine-yield)
       (async-coroutine-yield)
       (async-coroutine-yield)
       ;; Collect the :exit message sent by monitor notification
       (setq monitor-received (actor-receive :timeout 500))))
   nil)
  (let ((ti 0))
    (while (< ti 40)
      (async-scheduler-tick)
      (setq ti (1+ ti))))
  (Assert (equal join-result 42)
          "join waiter receives result when monitor is also set on target")
  (Assert (and (listp monitor-received)
               (eq (car monitor-received) :exit))
          "monitoring coroutine receives :exit when target dies"))

;; Test 7: named-actor registry round-trip via actor-spawn + actor-find.
;; actor-spawn stores the handle in `async--actor-registry' when NAME is
;; non-nil; actor-find returns nil for never-registered names.
(let ((h (actor-spawn 'registry-probe (lambda (_) 'done) nil)))
  (Assert (not (null h))
          "actor-spawn with name returns non-nil handle")
  (Assert (eq (actor-find 'registry-probe) h)
          "actor-find recovers the registered handle by name")
  (Assert (null (actor-find 'never-was-registered))
          "actor-find returns nil for unknown names"))

;; Test 8: monitor-only, normal exit.  A watcher actor monitors a target
;; that returns normally; the watcher must receive (:exit nil) without a
;; separate join being set up.  Uses plain `actor-receive' (no :timeout)
;; because `actor-receive :timeout MS' yields WAIT_TIMER, which
;; `actor_send_internal' does not wake -- an `(:exit ...)' dispatched
;; while the watcher is blocked on WAIT_TIMER is queued but not delivered
;; until the timer expires (known limitation of the current API).
(let ((msg 'unset)
      (target (actor-spawn-internal nil (lambda (_) 'ok) nil)))
  (actor-spawn-internal
   'watcher-normal
   (lambda (_)
     (actor-monitor-internal target (async-current-coroutine))
     (setq msg (actor-receive)))
   nil)
  (let ((ti 0))
    (while (< ti 20) (async-scheduler-tick) (setq ti (1+ ti))))
  (Assert (and (listp msg) (eq (car msg) :exit) (null (cadr msg)))
          "monitor-only watcher sees (:exit nil) on normal target exit"))

;; Test 9: monitor-only, crash exit.  The target signals an error; the
;; watcher's :exit message must carry the (error-symbol . data) cons so
;; supervisor code can introspect the failure (as the cookbook's
;; Supervised Actor example depends on).
(let ((msg 'unset)
      (target (actor-spawn-internal nil (lambda (_) (error "boom!")) nil)))
  (actor-spawn-internal
   'watcher-crash
   (lambda (_)
     (actor-monitor-internal target (async-current-coroutine))
     (setq msg (actor-receive)))
   nil)
  (let ((ti 0))
    (while (< ti 20) (async-scheduler-tick) (setq ti (1+ ti))))
  (Assert (and (listp msg) (eq (car msg) :exit))
          "monitor-only watcher sees :exit on crash exit")
  (let ((reason (cadr msg)))
    (Assert (and (consp reason) (eq (car reason) 'error))
            "crash-exit reason carries (error . data) for introspection")))

;; Test 10: multiple monitors are all notified.  Two watcher actors each
;; monitor the same target; target exits normally; both must receive
;; (:exit nil).  Exercises the monitor_list dispatch loop in
;; coro_trampoline.
(let ((m1 'unset) (m2 'unset)
      (target (actor-spawn-internal nil (lambda (_) 'ok) nil)))
  (actor-spawn-internal
   'watcher1
   (lambda (_)
     (actor-monitor-internal target (async-current-coroutine))
     (setq m1 (actor-receive)))
   nil)
  (actor-spawn-internal
   'watcher2
   (lambda (_)
     (actor-monitor-internal target (async-current-coroutine))
     (setq m2 (actor-receive)))
   nil)
  (let ((ti 0))
    (while (< ti 20) (async-scheduler-tick) (setq ti (1+ ti))))
  (Assert (and (listp m1) (eq (car m1) :exit))
          "first monitor receives :exit")
  (Assert (and (listp m2) (eq (car m2) :exit))
          "second monitor receives :exit"))
