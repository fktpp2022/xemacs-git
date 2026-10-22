;;; actor.el --- actor model for XEmacs async

(require 'async-core)

(defvar async--actor-registry (make-hash-table :test 'eq :weakness 'value)
  "Map from actor name symbol to actor handle.")

(defun actor-spawn (name function &rest args)
  "Spawn a new actor running FUNCTION with ARGS.
NAME is a symbol for `actor-find', or nil. Returns an opaque handle."
  (let ((handle (actor-spawn-internal name function args)))
    (when name
      (puthash name handle async--actor-registry))
    handle))

(defun actor-send (actor-ref message)
  "Send MESSAGE to ACTOR-REF non-blocking."
  (actor-send-internal actor-ref message))

(defun actor-receive (&rest keys)
  "Receive the next message. Suspends until a message arrives.
:timeout MS — signal `async-timeout' after MS milliseconds."
  (let ((timeout (plist-get keys :timeout)))
    (actor-receive-internal timeout)))

(defun actor-self ()
  "Return the current actor's handle, or nil."
  (async-current-coroutine))

(defun actor-join (actor-ref)
  "Suspend current coroutine until ACTOR-REF's coroutine finishes. Returns its result."
  (actor-join-internal actor-ref))

(defun actor-monitor (actor-ref)
  "Monitor ACTOR-REF. If it dies, current actor receives (:exit reason).
Returns actor-ref."
  (actor-monitor-internal actor-ref (actor-self))
  actor-ref)

(defun actor-find (name)
  "Look up a named actor by symbol NAME. Returns handle or nil."
  (gethash name async--actor-registry))

(provide 'actor)
