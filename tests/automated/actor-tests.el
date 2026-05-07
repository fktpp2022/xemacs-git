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
