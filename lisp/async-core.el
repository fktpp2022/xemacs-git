;;; async-core.el --- await, async-let, async-yield for XEmacs

;;; await

(defmacro await (expr)
  "Suspend the current coroutine until EXPR's operation completes.
EXPR must be one of:
  - A call to an HTTP primitive (e.g. `http-get', `http-post') that
    internally yields the current coroutine and returns nil.  On resume,
    the current coroutine's result/error fields hold the outcome.
  - A call to a spawning function (e.g. `async-read-file',
    `async-write-file', `async-shell-command') that spawns a new coroutine
    and returns its opaque handle.  `await' detects the handle via
    `async-coroutine-p' and joins it with `actor-join-internal'.

Returns the operation's result value, or re-signals its error."
  (let ((handle-sym (gensym "await-handle-"))
        (c-sym      (gensym "await-coro-"))
        (res-sym    (gensym "await-res-")))
    `(let* ((,handle-sym ,expr)
            (,c-sym (async-current-coroutine)))
       (if (async-coroutine-p ,handle-sym)
           ;; Spawning protocol: actor-join-internal suspends until the
           ;; spawned coroutine finishes and returns its result directly.
           ;; On error, coro_resume_with_error places the error in the
           ;; current coroutine's error field; re-signal it here.
           (let ((,res-sym (actor-join-internal ,handle-sym)))
             (if (and ,c-sym (not (null (async--coro-error ,c-sym))))
                 (apply 'signal (async--coro-error ,c-sym))
               ,res-sym))
         ;; Yield protocol: expression yielded the current coroutine
         ;; internally; result/error are in the current coro's fields.
         (if (and ,c-sym (not (null (async--coro-error ,c-sym))))
             (apply 'signal (async--coro-error ,c-sym))
           (and ,c-sym (async--coro-result ,c-sym)))))))

;;; async-let: parallel spawn + join

(defmacro async-let (bindings &rest body)
  "Spawn each binding as a parallel coroutine, join all, then bind results.
Like Promise.all() / asyncio.gather().

Each binding expression becomes the body of a freshly-spawned coroutine;
its return value is what gets bound.  If the expression calls an async
function, use `await' inside the expression to obtain the actual result:

  ;; spawn-protocol (returns handle): must await to get file contents
  (async-let ((text (await (async-read-file path)))) ...)

  ;; yield-protocol (returns nil): must await to get HTTP response
  (async-let ((resp (await (http-get url)))) ...)"
  (declare (indent 1))
  (let ((handle-syms (mapcar (lambda (_) (gensym "handle-")) bindings)))
    `(let ,(mapcar* (lambda (hsym binding)
                      `(,hsym (actor-spawn-internal nil (lambda (_) ,(cadr binding)) nil)))
                    handle-syms bindings)
       (let ,(mapcar* (lambda (vsym hsym)
                        `(,(car vsym) (actor-join-internal ,hsym)))
                      bindings handle-syms)
         ,@body))))

;;; async-yield

(defun async-yield ()
  "Voluntarily yield the current coroutine to the scheduler."
  (async-coroutine-yield))

;;; Error types
(define-error 'async-timeout "Async operation timed out")
(define-error 'async-http-error "Async HTTP error")
(define-error 'async-error "Async scheduler error")

(provide 'async-core)
