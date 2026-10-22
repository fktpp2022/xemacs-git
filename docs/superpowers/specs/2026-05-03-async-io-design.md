# XEmacs Async IO Design Spec

**Date:** 2026-05-03  
**Branch:** xemacs-async  
**Status:** Approved — ready for implementation  

---

## 1. Goals

Implement async IO in XEmacs core that:

1. Replaces blocking IO with non-blocking IO transparently where possible
2. Adds coroutine-based `await` semantics for new Elisp code
3. Adds an actor model for parallel tool execution
4. Supports streaming HTTP (SSE/chunked) for LLM API integration
5. Does **not** break any existing Elisp code

Primary use case: a mini-SWE-agent implemented in Elisp that talks to LLMs via streaming HTTP and executes tool calls in parallel.

---

## 2. Approach Selected

**Approach B — Hybrid: ucontext coroutines + libcurl + enhanced select() loop**

Direct integration into XEmacs core (not emodules). Rationale:

- libcurl multi interface is as mature as libuv for HTTP/TLS/streaming
- `ucontext_t` coroutine scheduler is a well-understood pattern (~2000 lines C)
- Zero change to existing `select()` event loop — new fds registered via two small patches
- libcurl handles TLS, HTTP/1.1, HTTP/2, SSE streaming with zero additional dependencies
- emodules rejected: `input_wait_mask` and `signal_event_pipe` are not exported, no stable ABI, GC cannot scan coroutine stacks automatically, unload under suspended coroutines is unsafe

Alternatives considered and rejected:
- **libuv**: event loop ownership conflict with X11/GTK; large dependency; high integration risk
- **setjmp/longjmp green threads**: no true coroutine stacks; `await` is CPS sugar only; not suitable for parallel tool execution at scale

---

## 3. Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Elisp Layer                            │
│   (await ...)   (async-let (...) ...)   (actor-spawn ...)   │
│   (http-get url :stream t :on-chunk #'cb)                   │
│   lisp/async-core.el · lisp/actor.el · lisp/async-http.el  │
├─────────────────────────────────────────────────────────────┤
│             Async Scheduler  src/async-scheduler.c/.h       │
│   coroutine pool (ucontext_t)  │  actor mailboxes           │
│   timer wheel                  │  fd→coroutine wakeup map   │
├──────────────┬──────────────────────────────────────────────┤
│ src/         │    src/event-unixoid.c  (2 small patches)    │
│ async-http.c │  + add_extra_fd() / remove_extra_fd()        │
│ libcurl multi│  + signal_async_wakeup() exported            │
│ HTTP·TLS·    │                                              │
│ streaming    │  event-stream.c / event-Xt.c / event-gtk.c   │
│              │  X11 · GTK · TTY · process pipes ← UNCHANGED │
└──────────────┴──────────────────────────────────────────────┘
          OS:  poll() / select()  ← existing loop, unchanged
```

### Core invariants

- Existing Elisp runs unchanged — no new keywords, no breaking changes
- The `select()` loop in `event-unixoid.c` is not replaced; libcurl sockets are registered into it via `add_extra_fd()`
- The Lisp evaluator remains single-threaded; coroutines are cooperative, scheduled only at `await` points
- GC safety: all Lisp objects on coroutine stacks are rooted via explicit `GCPRO` in the scheduler
- Zero scheduler overhead when no coroutines are active

---

## 4. File Layout

```
xemacs-async/
├── src/
│   ├── async-scheduler.c     NEW  coroutine pool, mailboxes, timer wheel
│   ├── async-scheduler.h     NEW  public C API
│   ├── async-http.c          NEW  libcurl multi integration, write_callback
│   ├── async-http.h          NEW  http request/response types, C API
│   ├── coro-asm.S            NEW  portable stack-swap (macOS ucontext replacement)
│   ├── event-unixoid.c       PATCH  +add_extra_fd/remove_extra_fd/signal_async_wakeup
│   └── CMakeLists.txt        PATCH  add new .c files, find_package(CURL)
│
├── lisp/
│   ├── async-core.el         NEW  await, async-let, deferred, condition-case wrappers
│   ├── actor.el              NEW  actor-spawn/send/receive/join/monitor
│   ├── async-http.el         NEW  http-get/post/stream, sse-parse
│   └── async-file.el         NEW  async-read-file, async-write-file
│
└── tests/
    ├── async-scheduler-test.el
    ├── async-http-test.el
    └── actor-test.el
```

---

## 5. Coroutine Scheduler

### Data structure

```c
typedef struct xemacs_coro {
  ucontext_t ctx;              /* CPU registers + stack pointer */
  ucontext_t caller_ctx;       /* scheduler context to return to */
  unsigned char *stack;        /* malloc'd stack, default 256KB */
  enum { CORO_RUNNABLE, CORO_RUNNING,
         CORO_SUSPENDED, CORO_DEAD } state;
  Lisp_Object result;          /* value passed to resume */
  Lisp_Object error;           /* if non-nil, re-signal on resume */
  struct gcpro *gcpro_chain;   /* GC roots for this coro's stack */
  int wait_fd;                 /* fd we're waiting on, or -1 */
  EMACS_TIME wait_deadline;    /* timeout, or zero */
  Lisp_Object mailbox;         /* actor message queue (list) */
  struct xemacs_coro *next;    /* intrusive linked list */
} xemacs_coro;
```

### Scheduler tick

Inserted into `event-unixoid.c` after `select()` returns, before Lisp event dispatch:

1. Call `curl_multi_socket_action()` — advance any ready transfers
2. For each completed transfer: move waiting coroutine to RUNNABLE, set `coro->result`
3. Check timer wheel — move expired coroutines to RUNNABLE
4. Check actor mailboxes — move coroutines with pending messages to RUNNABLE
5. Run all RUNNABLE coroutines via `swapcontext()` until each yields or finishes

### GC safety

- Every `await` call site pushes a `GCPRO` frame before yielding, pops on resume
- Scheduler links all suspended coroutines' `gcpro_chain` into the global GCPRO list during GC marking
- Net result: GC sees all live Lisp objects on all coroutine stacks

### macOS portability

`ucontext_t` is deprecated on macOS since 10.6. `coro-asm.S` implements `coro_make` / `coro_swap` using raw stack manipulation (~40 lines of x86-64 and ARM64 assembly), independent of `ucontext_t`. Same approach used by LuaJIT and libco.

### Stack size

Default 256KB per coroutine, configurable via `async-coroutine-stack-size` Lisp variable.

---

## 6. libcurl HTTP Integration

### Multi-handle architecture

One global `CURLM *curl_multi_handle` for the lifetime of XEmacs. Each HTTP request:

1. Creates a `CURL *easy` handle with headers, body, callbacks
2. Adds easy handle to the multi handle
3. Calls `curl_multi_fdset()` to get the fds to watch
4. Registers those fds with `add_extra_fd()`
5. Coroutine suspends with `WAIT_FD`
6. When `select()` fires on a curl fd: `curl_multi_socket_action()` advances the transfer
7. `curl_write_callback()` fires per chunk — either buffers or calls `:on-chunk`
8. On transfer complete: coroutine moved to RUNNABLE, result set

### Key data structure

```c
typedef struct async_http_req {
  CURL *easy;
  xemacs_coro *waiting_coro;
  Lisp_Object response_buffer;
  Lisp_Object on_chunk;        /* Elisp callback, nil for buffered mode */
  Lisp_Object headers;         /* response headers as alist */
  long status_code;
  CURLcode result;
} async_http_req;
```

### Streaming (SSE)

In streaming mode, `curl_write_callback()` calls `enqueue_misc_user_event(on_chunk, chunk)` per chunk and `signal_async_wakeup()` if `select()` is blocking. The Elisp `:on-chunk` callback receives raw SSE lines. SSE parsing (`data:` prefix stripping, JSON decoding) is done in `async-http.el` — C never touches the LLM protocol.

### TLS

Handled transparently by libcurl using the system's OpenSSL/GnuTLS/mbedTLS. `https://` just works.

### Error handling

libcurl errors surface as Lisp signals of type `async-http-error`, caught by standard `condition-case`.

---

## 7. Actor Model

### Primitives

```elisp
(actor-spawn NAME FUNCTION &rest ARGS)  ; spawn — returns actor ref
(actor-send ACTOR-REF MESSAGE)          ; non-blocking send
(actor-receive &key timeout)            ; suspend until message arrives
(actor-self)                            ; current actor's ref
(await (actor-join ACTOR-REF))          ; wait for actor to finish
(actor-monitor ACTOR-REF)              ; receive {:exit reason} if actor dies
```

### Actor as coroutine

Each actor is a `xemacs_coro` with a non-nil `mailbox`. `actor-receive` suspends the coroutine with `WAIT_MAILBOX`; the scheduler moves it to RUNNABLE when `actor-send` deposits a message.

### Error isolation

- Unhandled signal in actor: if monitored, sends `(:exit reason)` to monitor; if not, logs to `*async-errors*` buffer and discards silently
- Mirrors Erlang "let it crash" — a failing tool call does not affect the agent loop

### Parallelism model

Actors are **concurrent but not parallel** — cooperative scheduling, one CPU core. IO-bound actors (HTTP, shell, file) yield while suspended, giving near-true parallelism for LLM tool calls which are almost entirely IO-bound. CPU-bound Elisp in an actor blocks all others; use `(async-yield)` in long CPU loops.

### `async-let` — parallel join

```elisp
(async-let ((result-a (http-post tool-a-url :json args-a))
            (result-b (http-post tool-b-url :json args-b))
            (result-c (async-shell-command cmd)))
  (list result-a result-b result-c))
```

Equivalent to `Promise.all()` / `asyncio.gather()`. Spawns each binding as an actor, suspends until all complete, binds results.

---

## 8. Core Patches

### Patch 1: `src/event-unixoid.c` (~15 lines)

Add static arrays and three public functions:

```c
static void (*extra_fd_callbacks[FD_SETSIZE])(int, void *);
static void *extra_fd_data[FD_SETSIZE];

void add_extra_fd (int fd, void (*cb)(int, void *), void *data) {
  extra_fd_callbacks[fd] = cb;
  extra_fd_data[fd] = data;
  FD_SET (fd, &input_wait_mask);
  if (fd >= maxdesc) maxdesc = fd + 1;
}

void remove_extra_fd (int fd) {
  extra_fd_callbacks[fd] = NULL;
  FD_CLR (fd, &input_wait_mask);
}

void signal_async_wakeup (void) {
  if (signal_event_pipe_initialized) {
    char byte = 0;
    retry_write (signal_event_pipe[1], &byte, 1);
  }
}
```

Plus dispatch in the `select()` result loop: when a registered extra fd fires, call its callback.

### Patch 2: `src/CMakeLists.txt` (~8 lines)

```cmake
find_package(CURL REQUIRED)
target_sources(xemacs PRIVATE
  src/async-scheduler.c
  src/async-http.c
  src/coro-asm.S)
target_link_libraries(xemacs PRIVATE CURL::libcurl)
```

---

## 9. Elisp API Summary

```elisp
;; Core async primitives
(await EXPR)                                ; suspend coroutine until EXPR resolves
(async-let ((VAR EXPR) ...) BODY)          ; parallel binding, await all
(async-yield)                               ; voluntarily yield to scheduler

;; HTTP
(http-get URL &key headers timeout)
(http-post URL &key headers json body timeout)
(http-stream URL &key headers json on-chunk timeout)

;; File IO (async wrappers, use thread pool internally)
(async-read-file PATH &key coding)
(async-write-file PATH CONTENT &key coding)

;; Shell
(async-shell-command CMD &key timeout)

;; Actors
(actor-spawn NAME FUNCTION &rest ARGS)
(actor-send ACTOR-REF MESSAGE)
(actor-receive &key timeout)
(actor-self)
(actor-join ACTOR-REF)
(actor-monitor ACTOR-REF)

;; Error handling — standard condition-case works
(condition-case err
    (await (http-get "https://..."))
  (async-http-error (message "error: %s" (cadr err)))
  (async-timeout     (message "timed out")))
```

---

## 10. SWE-Agent Integration Pattern

```elisp
(defun swe-agent-loop (task)
  (let ((messages `[((role . "user") (content . ,task))]))
    (cl-loop
     (let ((chunks '()))
       (await (http-post swe-agent-llm-url
                         :json `((model . ,swe-agent-model)
                                 (stream . t)
                                 (messages . ,messages))
                         :on-chunk (lambda (chunk)
                                     (let ((text (sse-extract-text chunk)))
                                       (when text
                                         (push text chunks)
                                         (insert text))))))
       (let* ((response (apply #'concat (nreverse chunks)))
              (tool-calls (swe-parse-tool-calls response)))
         (if (null tool-calls)
             (cl-return response)
           (let ((results (swe-execute-tools-parallel tool-calls)))
             (setq messages
                   (swe-append-tool-results messages response results)))))))))

(defun swe-execute-tools-parallel (tool-calls)
  (let ((actors (mapcar (lambda (call)
                          (cons (plist-get call :id)
                                (actor-spawn nil #'swe-execute-one-tool call)))
                        tool-calls)))
    (mapcar (lambda (pair)
              (cons (car pair) (await (actor-join (cdr pair)))))
            actors)))
```

---

## 11. Testing Strategy

### Layer 1 — Coroutine scheduler unit tests
- Spawn 1000 coroutines, verify all complete in correct order
- GC stress: force GC while coroutines suspended, verify no segfault, Lisp objects intact
- Timer accuracy: coroutines woken within 10ms of deadline
- Error propagation: exception in coroutine surfaces as Lisp signal at `await` site

### Layer 2 — HTTP integration tests
- Local Python HTTP server in subprocess
- Buffered GET/POST, response headers, JSON body
- Streaming: 100 chunks × 10ms, verify all received in order
- Error cases: connection refused, timeout, TLS cert failure
- Parallel: 10 concurrent requests via `async-let`, verify timing < sequential

### Layer 3 — Actor system tests
- Ping-pong: two actors, 10000 rounds
- Parallel join: 20 actors random sleep duration, `async-let` all, verify completeness
- Monitor: actor dies with error, monitor receives `(:exit reason)`
- Backpressure: actor with full mailbox does not block event loop

### Regression gate
All existing XEmacs test suite tests must pass unchanged.

---

## 12. Known Limitations (v1)

| Limitation | Mitigation |
|---|---|
| CPU-bound actors block all others | Document; provide `(async-yield)` for long loops |
| Async file IO is thread-pool best-effort | `find-file` / `insert-file-contents` remain synchronous |
| No cross-process actor addressing | In-process only; future: Unix socket transport |
| Stack size fixed at 256KB | Configurable via `async-coroutine-stack-size` |
| No structured concurrency | Future: `async-with-scope` auto-cancels child actors |
| Actors share GC heap | Isolation is by convention, not enforced |

---

## 13. Reference: Async Patterns Evaluated

| Runtime | Key idea | What we borrowed |
|---|---|---|
| **Go** | M:N goroutine scheduler, channel communication | Coroutine lifecycle states, cooperative yield points |
| **Node.js / libuv** | Event loop owns main thread, io_uring/kqueue/IOCP | Rejected (event loop conflict); libcurl used instead |
| **Erlang/OTP** | Actor isolation, let-it-crash, supervisor trees | Actor model, error isolation, monitor pattern |
| **Python asyncio** | `async/await` syntax, event loop, `gather()` | `await` macro, `async-let` = `asyncio.gather()` |
| **Java NIO/virtual threads** | Non-blocking channels, selector, virtual thread scheduler | fd→coroutine wakeup map, selector pattern |
