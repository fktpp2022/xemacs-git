;;; async-core.el --- await, async-let, async-yield for XEmacs

;;; await

(defmacro await (expr)
  "Suspend the current coroutine until EXPR's operation completes.
EXPR must initiate an async operation that registers with the scheduler
and yields internally — on resume, the coro's result/error fields hold
the outcome.  Returns the result or re-signals the error."
  `(progn
     ,expr
     (let ((c (async-current-coroutine)))
       (if (and c (not (null (async--coro-error c))))
           (apply 'signal (async--coro-error c))
         (and c (async--coro-result c))))))

;;; async-let: parallel spawn + join

(defmacro async-let (bindings &rest body)
  "Spawn each binding as a parallel actor, await all, then bind results.
Like Promise.all() / asyncio.gather()."
  (declare (indent 1))
  (let ((handle-syms (mapcar (lambda (_) (gensym "handle-")) bindings)))
    `(let ,(mapcar* (lambda (hsym binding)
                      `(,hsym (actor-spawn nil (lambda (_) ,(cadr binding)) nil)))
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
