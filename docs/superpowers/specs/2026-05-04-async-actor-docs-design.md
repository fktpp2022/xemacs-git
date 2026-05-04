# Design: Async/Actor Documentation for XEmacs

**Date:** 2026-05-04  
**Branch:** xemacs-async (base: xemacs-cmake)  
**Status:** Approved

---

## Overview

Two new Texinfo chapters documenting the coroutine-based async/actor system added in the `xemacs-async` branch. One chapter targets Lisp developers writing XEmacs packages; one targets core contributors working on the C scheduler.

---

## Files to Create

| File | Included from | Position |
|------|--------------|----------|
| `man/lispref/async.texi` | `man/lispref/lispref.texi` | After `@include processes.texi` |
| `man/internals/async.texi` | `man/internals/internals.texi` | New chapter at end (before indices) |

---

## `man/lispref/async.texi` — Chapter "Async Programming"

### Top-level node

```
@node Async Programming, System Interface, Processes, Top
@chapter Async Programming
```

The `@menu` lists three sections: Coroutine Concepts, Async API Reference, Async Cookbook.

---

### Section 1: Coroutine Concepts

Explains the mental model before any API. Covers:

- **What a coroutine is** — a Lisp function with its own stack and binding environment that can suspend itself and be resumed later, without blocking the editor's event loop.
- **Cooperative scheduling** — XEmacs runs one coroutine at a time. A coroutine runs until it calls `await` (or `async-yield`), at which point the scheduler picks the next runnable coroutine. Preemption never occurs.
- **The event loop integration** — the scheduler tick is called by `event-unixoid.c` after each `select()` call. Any coroutine whose wait condition is satisfied (fd ready, timer expired, message arrived, joined coroutine finished) is moved to runnable and resumed on the next tick.
- **Actors vs. plain coroutines** — every actor is a coroutine, but not every coroutine is an actor. An actor additionally has a mailbox and can be named for lookup via `actor-find`. Plain coroutines (spawned by `async-read-file` etc.) are anonymous and communicate only by joining.
- **Error propagation** — if a coroutine's body signals an error, the error is stored in the coroutine's result slot and re-signaled in the caller at the `await` point.
- **Variable bindings** — each coroutine has its own `specpdl` (binding stack). A `let` binding made inside a coroutine is private to that coroutine and does not affect other coroutines or the main thread.

---

### Section 2: Async API Reference

Four subsections. Each function/macro gets a `@defun` or `@defmac` block with: signature, description, inline example, cross-references where relevant.

#### 2.1 Core Primitives

| Form | Type | Description |
|------|------|-------------|
| `await` | macro | Suspend current coroutine until operation completes; return result or re-signal error |
| `async-let` | macro | Spawn each binding as a parallel actor, await all, bind results (like `Promise.all`) |
| `async-yield` | function | Voluntarily yield to the scheduler without waiting for any particular event |

Inline examples:

```lisp
;; await a single operation
(let ((contents (await (async-read-file "/etc/hosts"))))
  (message "File has %d chars" (length contents)))

;; async-let: two reads in parallel
(async-let ((a (async-read-file "/tmp/foo"))
            (b (async-read-file "/tmp/bar")))
  (message "got %d and %d chars" (length a) (length b)))
```

#### 2.2 Actor Model

| Function | Description |
|----------|-------------|
| `actor-spawn name function &rest args` | Spawn a new coroutine running `function`. `name` is a symbol for registry lookup, or `nil`. Returns opaque handle. |
| `actor-send actor-ref message` | Deliver `message` to actor's mailbox. Non-blocking; returns immediately. |
| `actor-receive &rest keys` | Suspend current coroutine until a message arrives. `:timeout MS` to signal `async-timeout` after MS milliseconds. |
| `actor-join actor-ref` | Suspend until `actor-ref` finishes; return its result. |
| `actor-monitor actor-ref` | If `actor-ref` dies, deliver `(:exit reason)` to current actor's mailbox. Returns `actor-ref`. |
| `actor-find name` | Look up a named actor by symbol. Returns handle or `nil`. |
| `actor-self` | Return the current actor's own handle, or `nil` if called from main thread. |

Inline example for each function (one or two lines showing the typical call).

#### 2.3 File and Shell I/O

| Function | Description |
|----------|-------------|
| `async-read-file path &rest keys` | Read file at `path` asynchronously. `:coding CODING-SYSTEM` (default `utf-8`). Pass to `await` for string contents. |
| `async-write-file path content &rest keys` | Write `content` to `path` asynchronously. `:coding CODING-SYSTEM`. Pass to `await` for `t`. |
| `async-shell-command cmd &rest keys` | Run shell command `cmd` asynchronously. `:timeout MS`. Pass to `await` for output string. |

Inline example:
```lisp
(let ((out (await (async-shell-command "uname -a"))))
  (message "%s" out))
```

#### 2.4 HTTP

| Function | Description |
|----------|-------------|
| `http-get url &rest keys` | Async GET. `:headers ALIST :timeout MS`. Pass to `await` for `(STATUS HEADERS BODY)`. |
| `http-post url &rest keys` | Async POST. `:headers ALIST :json OBJECT :body STRING :on-chunk FUNC :timeout MS`. Pass to `await`. |
| `http-stream url &rest keys` | Streaming POST. `:on-chunk FUNC` required; called with each chunk string. |
| `sse-parse-data line` | Extract payload from one SSE line (`"data: {...}"`). Returns string or `nil`. |
| `sse-extract-text chunk` | Parse SSE chunk, extract concatenated `content` text from OpenAI/Anthropic streaming format. |

#### 2.5 Error Symbols

| Symbol | Condition |
|--------|-----------|
| `async-timeout` | Signaled when `:timeout` expires before operation completes |
| `async-http-error` | Signaled on HTTP transport failure (curl error, not HTTP 4xx/5xx) |
| `async-error` | General scheduler-level error |

---

### Section 3: Async Cookbook

Five complete, runnable examples. Each has a brief prose intro, the full Lisp code in a `@lisp` block, and a short explanation of what to observe.

#### Example 1: Fetch two URLs in parallel

```lisp
(require 'async-core)
(require 'async-http)

(actor-spawn nil
  (lambda (_)
    (async-let ((r1 (http-get "https://example.com/api/a"))
                (r2 (http-get "https://example.com/api/b")))
      (let ((body1 (nth 2 r1))
            (body2 (nth 2 r2)))
        (with-current-buffer (get-buffer-create "*results*")
          (insert body1 "\n---\n" body2)))))
  nil)
```

Explanation: `async-let` spawns both GETs concurrently. Both suspend on their respective curl operations. The scheduler resumes each when its response arrives. `body1` and `body2` are both bound before the `with-current-buffer` form runs.

#### Example 2: Read, transform, write

```lisp
(actor-spawn nil
  (lambda (_)
    (let* ((text    (await (async-read-file "/tmp/input.txt")))
           (upcased (upcase text)))
      (await (async-write-file "/tmp/output.txt" upcased))
      (message "Done.")))
  nil)
```

Explanation: sequential `await` chain — the second `await` does not start until the first completes.

#### Example 3: Named worker actor with job queue

```lisp
(require 'actor)

;; Start a named worker
(actor-spawn 'my-worker
  (lambda (_)
    (let ((running t))
      (while running
        (let ((msg (actor-receive)))
          (pcase msg
            (`(:job ,payload)
             (message "Processing: %s" payload))
            (`(:stop)
             (setq running nil)))))))
  nil)

;; Send jobs from anywhere
(let ((w (actor-find 'my-worker)))
  (actor-send w '(:job "task-1"))
  (actor-send w '(:job "task-2"))
  (actor-send w '(:stop)))
```

Explanation: the worker loops on `actor-receive`, which suspends it until a message arrives. `actor-find` retrieves the handle by name from the registry. Sending `:stop` causes the loop to exit and the coroutine to finish.

#### Example 4: Streaming HTTP with SSE

```lisp
(require 'async-http)

(actor-spawn nil
  (lambda (_)
    (await
      (http-stream "https://api.anthropic.com/v1/messages"
        :headers '(("x-api-key" . "YOUR-KEY")
                   ("anthropic-version" . "2023-06-01"))
        :json '((model . "claude-opus-4-7")
                (max_tokens . 256)
                (stream . t)
                (messages . [((role . "user")
                               (content . "Say hello."))]))
        :on-chunk (lambda (chunk)
                    (let ((text (sse-extract-text chunk)))
                      (when text
                        (princ text)))))))
  nil)
```

Explanation: `:on-chunk` is called synchronously in the scheduler tick each time curl delivers a chunk. `sse-extract-text` handles the `data: {...}` framing and JSON parsing, returning the concatenated `content` delta text.

#### Example 5: Supervised actor with monitor

```lisp
(require 'actor)

(defun start-worker ()
  (actor-spawn 'fragile-worker
    (lambda (_)
      ;; Simulate work that might fail
      (actor-receive)
      (error "Something went wrong"))
    nil))

(actor-spawn 'supervisor
  (lambda (_)
    (let ((worker (start-worker)))
      (actor-monitor worker)
      (let ((running t))
        (while running
          (let ((msg (actor-receive)))
            (pcase msg
              (`(:exit ,_reason)
               (message "Worker died, restarting...")
               (setq worker (start-worker))
               (actor-monitor worker))
              (other
               ;; Forward other messages to worker
               (actor-send worker other))))))))
  nil)
```

Explanation: `actor-monitor` arranges for `(:exit reason)` to be delivered to the supervisor's mailbox when the worker coroutine dies (normally or via error). The supervisor restarts the worker and re-monitors it.

---

## `man/internals/async.texi` — Chapter "Async Scheduler Internals"

### Section 1: Relationship to the Lisp API

One paragraph: this chapter documents the C implementation underlying the `await`, `actor-spawn`, `http-get` and related Lisp forms. For the Lisp-facing API, see `@ref{Async Programming,,Async Programming,lispref}`.

---

### Section 2: Architecture Overview

- The scheduler is a cooperative, run-to-yield coroutine system. There is no preemption.
- The scheduler tick (`async_scheduler_tick`) is called by `event-unixoid.c` immediately after each `select()` returns. It iterates all coroutines, checks wait conditions, resumes any that are now runnable, and calls `async_http_tick` to drive the libcurl multi-handle.
- The main Lisp thread is never itself a coroutine — it is the "scheduler context" that all coroutines return to when they yield.
- Diagram: main thread → `async_scheduler_tick` → `coro_resume(c)` → `coro_swap(&scheduler_ctx, &c->ctx)` → coroutine runs → `coro_yield()` → `coro_swap(&c->ctx, &c->caller_ctx)` → back to scheduler.

---

### Section 3: The `xemacs_coro` Structure

Full field-by-field documentation of `struct xemacs_coro` from `async-scheduler.h`:

| Field | Type | Purpose |
|-------|------|---------|
| `ctx` | `coro_context_t` | Saved stack pointer; restored on next `coro_swap` into this coro |
| `caller_ctx` | `coro_context_t` | Scheduler's stack pointer; `coro_yield` swaps back here |
| `stack` | `unsigned char *` | `malloc`'d coroutine stack (default 256 KB, see `Vasync_coroutine_stack_size`) |
| `stack_size` | `Bytecount` | Allocated stack size |
| `state` | `coro_state_t` | One of `CORO_RUNNABLE`, `CORO_RUNNING`, `CORO_SUSPENDED`, `CORO_DEAD` |
| `wait_reason` | `wait_reason_t` | Why suspended: `WAIT_FD`, `WAIT_TIMER`, `WAIT_MAILBOX`, `WAIT_JOIN` |
| `result` | `Lisp_Object` | Return value delivered on resume (also used to pass initial arg) |
| `error` | `Lisp_Object` | Non-`nil`: error data to re-signal at next `await` |
| `lisp_fn` | `Lisp_Object` | The Lisp function to call on first entry; kept alive for GC |
| `gcpro_chain` | `struct gcpro *` | Head of this coroutine's live-object GC protection chain |
| `saved_specpdl_ptr` | `struct specbinding *` | `specpdl_ptr` saved at yield; restored on resume |
| `saved_specpdl_depth` | `int` | Corresponding depth counter |
| `saved_backtrace_list` | `struct backtrace *` | Backtrace chain saved at yield |
| `saved_catchlist` | `struct catchtag *` | `catchlist` saved at yield |
| `coro_specpdl` | `struct specbinding *` | Per-coroutine specpdl array (initial size 200 entries) |
| `coro_specpdl_ptr` | `struct specbinding *` | Current top of per-coro specpdl |
| `coro_specpdl_size` | `int` | Allocated entry count |
| `coro_inherit_depth` | `int` | Number of entries inherited (shared) from main thread at spawn time |
| `wait_fd` | `int` | File descriptor waited on (`WAIT_FD`) |
| `wait_deadline` | `EMACS_TIME` | Absolute timeout (`WAIT_TIMER`) |
| `mailbox` | `Lisp_Object` | List of pending messages (`WAIT_MAILBOX`) |
| `actor_name` | `Lisp_Object` | Symbol name or `Qnil` |
| `is_actor` | `int` | Non-zero if this coroutine has a mailbox |
| `join_waiter` | `xemacs_coro *` | Coroutine waiting on this one to finish (`WAIT_JOIN`) |
| `next`, `prev` | `xemacs_coro *` | Intrusive doubly-linked list of all live coroutines |

---

### Section 4: Coroutine Lifecycle

State machine: `CORO_RUNNABLE` → `CORO_RUNNING` → `CORO_SUSPENDED` → `CORO_RUNNABLE` (loop) → `CORO_DEAD`.

- **Spawn** (`coro_spawn`): allocates stack, copies inherited specpdl entries, sets state to `CORO_RUNNABLE`, appends to `all_coros` list.
- **First run**: scheduler calls `coro_resume(c, initial_arg)`. `coro_swap` transfers control to `coro_trampoline`, which calls `condition_case_1` wrapping the Lisp function. This catches any unhandled errors and stores them in `c->error`.
- **Suspend** (`coro_yield`): saves interpreter state (specpdl, backtrace, catchlist), sets state to `CORO_SUSPENDED`, calls `coro_swap(&c->ctx, &c->caller_ctx)` — returns to scheduler.
- **Resume** (`coro_resume`): restores interpreter state, sets state to `CORO_RUNNING`, calls `coro_swap(&scheduler_ctx, &c->ctx)` — returns to where `coro_yield` was called.
- **Death**: coroutine function returns (or `condition_case_1` catches an error). `c->result` is set. State → `CORO_DEAD`. If `c->join_waiter` is set, that coroutine is resumed with the result. Coroutine is moved to `dead_coros` list pending GC.

---

### Section 5: Context Switching — `coro_swap`

`coro_swap(from, to)` is a two-instruction assembly primitive (x86-64):

```asm
coro_swap:
    mov  %rsp, (%rdi)   ; save current stack pointer into from->sp
    mov  (%rsi), %rsp   ; load stack pointer from to->sp
    ret                 ; "return" into the other context
```

On first entry to a new coroutine, `to->sp` points just above `coro_trampoline`'s address on the coroutine's stack — the `ret` jumps into the trampoline. On subsequent resumes, `to->sp` points to the yield site inside `coro_yield`.

The saved state is minimal: only `%rsp`. All other registers are caller-saved by the C ABI and are therefore already saved/restored by normal C function call frames on each coroutine's stack.

**Stack layout at spawn**: `coro_spawn` writes the trampoline address at the top of the new stack, sets `ctx.sp` to point just below it, and pads to 16-byte alignment (required by System V AMD64 ABI before a `call`/`ret`).

---

### Section 6: Per-Coroutine specpdl

XEmacs uses a global `specpdl` array for `let` bindings and `unwind-protect`. Naively sharing this across coroutines would cause bindings from one coroutine to be visible in another.

Each coroutine therefore gets its own `coro_specpdl` array. At spawn, the initial portion of the main thread's specpdl (up to `coro_inherit_depth` entries) is copied — these are bindings already in effect that the coroutine should inherit (e.g. `default-directory`, active buffer). New bindings made inside the coroutine go into its private array.

On `coro_yield`: the global `specpdl` / `specpdl_ptr` are saved into the coroutine's `saved_specpdl_ptr` / `saved_specpdl_depth`. The scheduler's specpdl state is restored.

On `coro_resume`: the scheduler's specpdl state is saved; the coroutine's saved state is restored into the globals.

`specpdl_swap_one` swaps a single entry: it exchanges `symbol->value` with `entry->old_value`. Walking the range forward re-applies bindings; walking it backward undoes them. Entries with a non-NULL `func` pointer (unwind-protect handlers) are skipped — they are not symbol bindings and must not be swapped.

---

### Section 7: GC Integration

The XEmacs GC traces `Lisp_Object` references from known roots (the `specpdl`, the `gcpro` chain, etc.). Coroutine stacks are C stacks, not Lisp stacks — the GC does not know to scan them.

`async_scheduler_mark_gcpros` is called by the GC's mark phase. It iterates `all_coros` and marks:
- `c->lisp_fn` — the lambda kept alive until first run
- `c->result` — the current result/arg slot
- `c->error` — any stored error data
- `c->mailbox` — the pending message list
- `c->actor_name` — the name symbol
- Each live `Lisp_Object` referenced via `c->gcpro_chain` — objects explicitly protected on the coroutine's C stack

**Rule for C code that suspends a coroutine**: any `Lisp_Object` local variable that must survive a `coro_yield` call must be protected with `GCPRO` before yielding and `UNGCPRO`d after resuming. The `gcpro_chain` pointer on the coroutine struct is set to the active `gcpro` chain at yield time and restored at resume.

---

### Section 8: HTTP Integration

`async-http.c` wraps libcurl's multi-handle interface.

- `async_http_start(url, method, headers, body, on_chunk, timeout_ms)` — creates a `CURL` easy handle, adds it to the global `CURLM` multi-handle, suspends the calling coroutine with `WAIT_FD`, and returns the handle as a `Lisp_Object` opaque.
- `async_http_tick()` — called by `async_scheduler_tick` on every event-loop iteration. Calls `curl_multi_perform`, then `curl_multi_info_read`. For each completed transfer, finds the `async_http_req` and resumes its `waiting_coro` with the result.
- **Streaming**: if `on_chunk` is non-nil, the curl write callback calls it directly (still on the coroutine's stack via the scheduler context) for each chunk. The coroutine does not suspend during streaming — it's the `on_chunk` callback that processes each chunk incrementally.
- **fd registration**: `async_http_socket_ready(fd, data)` is registered with XEmacs's fd-watching infrastructure so that curl sockets are included in the main `select()` call. This means HTTP I/O never requires a separate thread.

---

### Section 9: Adding New Async Primitives

A step-by-step guide for contributors writing a new C-level async operation (e.g. async DNS lookup):

1. **Allocate a request struct** holding the current coroutine pointer (`async_scheduler_current_coro()`) and any operation state.
2. **Register the operation** with the relevant event source (add fd to `select()` watchlist, arm a timer, etc.).
3. **Suspend the coroutine**: call `coro_yield(WAIT_FD)` (or appropriate reason). The coroutine's stack is now frozen.
4. **In the event callback**: when the operation completes, call `coro_resume(c, result_lisp_object)` or `coro_resume_with_error(c, error_data)`. The coroutine will be made runnable and resumed on the next `async_scheduler_tick`.
5. **Expose to Lisp**: write a `DEFSUBR` that calls your start function and returns the handle; the Lisp wrapper passes it to `await`.
6. **GC**: if any `Lisp_Object` values in your request struct must survive GC cycles while the coroutine is suspended, either store them in a coroutine field that `async_scheduler_mark_gcpros` already marks, or add explicit marking in that function.

---

## Integration into Existing Files

### `man/lispref/lispref.texi`

Add one line after `@include processes.texi`:

```diff
 @include processes.texi
+@include async.texi
 @include os.texi
```

Update the `@node` line in `processes.texi` to point forward to `Async Programming`:

```diff
-@node Processes, System Interface, Databases, Top
+@node Processes, Async Programming, Databases, Top
```

Add the `@node` line at the top of `async.texi`:

```
@node Async Programming, System Interface, Processes, Top
```

### `man/internals/internals.texi`

Add near the end, before the index appendix:

```diff
+@include async.texi
 @node Index
```

---

## Texinfo Conventions to Follow

- `@defun`, `@defmac`, `@defvar` for all API entries (not `@deffn`)
- `@cindex` entries for: coroutine, actor, async, cooperative scheduling, specpdl (coroutine), await, actor-spawn
- `@lisp` / `@end lisp` for all code blocks (not `@example`)
- `@xref` for cross-references between chapters; `@pxref` inside parenthetical cross-references
- `@var` for metavariable names in prose; `@code` for Lisp symbols and C identifiers
- Copyright header matching existing lispref files

---

## Out of Scope

- Documentation for `async-spawn-coroutine` (low-level C-facing primitive exposed to Lisp tests; not a public API)
- Windows/MS-DOS async considerations (the scheduler is currently Unix-only)
- Performance tuning guidance (stack size, coroutine count limits)
