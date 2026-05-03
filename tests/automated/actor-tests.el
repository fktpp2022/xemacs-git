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
(let ((h (actor-spawn nil (lambda (_) 42) nil)))
  (Assert (not (null h)) "actor-spawn returns non-nil handle"))

;; Test 2: actor-join returns the actor's return value
(let ((result nil))
  (async-spawn-coroutine
   (lambda (_)
     (let ((h (actor-spawn nil (lambda (_) 99) nil)))
       (setq result (await (actor-join h)))))
   nil)
  (dotimes (_ 10) (async-scheduler-tick))
  (Assert (equal result 99) "actor-join returns correct value"))

;; Test 3: actor-send wakes a waiting actor
(let ((delivered nil))
  (let ((h (actor-spawn nil
                        (lambda (_)
                          (actor-receive)
                          (setq delivered t))
                        nil)))
    (async-scheduler-tick)         ;; actor starts and blocks in actor-receive
    (actor-send h 'wake-up)        ;; send message
    (dotimes (_ 10) (async-scheduler-tick)))
  (Assert delivered "message delivered to waiting actor"))

;; Test 4: async-let parallel bind
(let ((results nil))
  (async-spawn-coroutine
   (lambda (_)
     (setq results
           (async-let ((a (progn (async-coroutine-yield) 'a))
                       (b (progn (async-coroutine-yield) 'b))
                       (c (progn (async-coroutine-yield) 'c)))
             (list a b c))))
   nil)
  (dotimes (_ 20) (async-scheduler-tick))
  (Assert (equal results '(a b c)) "async-let parallel binding works"))

;; Test 5: nested coroutines do not corrupt each other
(let ((sum 0))
  (dotimes (i 5)
    (let ((n i))
      (async-spawn-coroutine
       (lambda (_)
         (async-coroutine-yield)
         (setq sum (+ sum n)))
       nil)))
  (async-scheduler-tick)
  (async-scheduler-tick)
  (Assert (= sum 10) "5 coroutines summed correctly"))
