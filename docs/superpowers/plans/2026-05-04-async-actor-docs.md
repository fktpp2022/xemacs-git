# Async/Actor Documentation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Write two new Texinfo chapters — `man/lispref/async.texi` (Lisp API reference + cookbook) and `man/internals/async.texi` (C scheduler internals) — and wire them into the existing XEmacs documentation.

**Architecture:** Option C from the design spec: lispref chapter has three layers (Concepts → API Reference → Cookbook); internals chapter is pure C reference with a short orientation paragraph cross-referencing the lispref chapter. Each chapter is a standalone `.texi` file `@include`d from its parent manual.

**Tech Stack:** GNU Texinfo markup, XEmacs Lisp code examples, x86-64 assembly snippet (documentation only).

---

## File Map

| Action | Path | Purpose |
|--------|------|---------|
| Create | `man/lispref/async.texi` | Lispref chapter: Concepts + API Reference + Cookbook |
| Create | `man/internals/async.texi` | Internals chapter: full C scheduler documentation |
| Modify | `man/lispref/lispref.texi:1259` | Add `@include async.texi` after `processes.texi` |
| Modify | `man/lispref/processes.texi:5` | Update `@node` forward pointer to `Async Programming` |
| Modify | `man/lispref/os.texi:5` | Update `@node` back pointer to `Async Programming` |
| Modify | `man/internals/internals.texi:33289` | Insert `@include async.texi` before `@node Index`; update node chain |

---

## Task 1: Create `man/lispref/async.texi` — Header, Concepts, and @menu

**Files:**
- Create: `man/lispref/async.texi`

- [ ] **Step 1: Create the file with copyright header, top-level node, chapter title, @menu, and the Coroutine Concepts section**

Create `man/lispref/async.texi` with this exact content:

```texinfo
@c -*-texinfo-*-
@c This is part of the XEmacs Lisp Reference Manual.
@c Copyright (C) 1990, 1991, 1992, 1993, 1994 Free Software Foundation, Inc.
@c Copyright (C) 2026 the XEmacs Project.
@c See the file lispref.texi for copying conditions.
@node Async Programming, System Interface, Processes, Top
@chapter Async Programming
@cindex async
@cindex coroutine
@cindex cooperative scheduling
@cindex actor

  XEmacs provides a cooperative coroutine system for writing asynchronous
code that does not block the editor's event loop.  Lisp code uses
@code{await} and @code{actor-spawn} to initiate operations that run
concurrently; the scheduler resumes each coroutine when its operation
completes.

@menu
* Coroutine Concepts::          What coroutines are and how the scheduler works.
* Async API Reference::         Complete reference for all async functions and macros.
* Async Cookbook::              Complete worked examples for common patterns.
@end menu

@node Coroutine Concepts, Async API Reference, Async Programming, Async Programming
@section Coroutine Concepts
@cindex coroutine, definition

@subsection What Is a Coroutine?

  A @dfn{coroutine} is a Lisp function with its own private call stack
and variable-binding environment.  Unlike a normal function call, a
coroutine can @dfn{suspend} itself mid-execution and be @dfn{resumed}
later, without blocking the rest of the editor.  While a coroutine is
suspended, XEmacs continues processing keyboard input and redisplay
normally.

@subsection Cooperative Scheduling
@cindex cooperative scheduling

  XEmacs runs at most one coroutine at a time.  A coroutine runs until
it explicitly suspends itself by calling @code{await} or
@code{async-yield}.  At that point the scheduler selects the next
runnable coroutine and transfers control to it.  There is no preemption:
a coroutine that never yields will never be interrupted.

@subsection Event Loop Integration

  The scheduler tick is driven by @code{event-unixoid.c}, which calls
@code{async_scheduler_tick} immediately after each @code{select()} system
call returns.  The tick examines every suspended coroutine and resumes
any whose wait condition is now satisfied:

@itemize @bullet
@item
A coroutine waiting for a file descriptor becomes runnable when
@code{select()} reports that descriptor ready.
@item
A coroutine waiting for a timeout becomes runnable when the deadline
passes.
@item
A coroutine waiting for a mailbox message becomes runnable when another
coroutine or the main thread sends it a message.
@item
A coroutine waiting to join another becomes runnable when that coroutine
finishes.
@end itemize

@subsection Actors vs. Plain Coroutines

  Every @dfn{actor} is a coroutine, but not every coroutine is an actor.
An actor additionally has a @dfn{mailbox} — a queue of incoming messages
— and may be assigned a name for lookup by other code.  Plain coroutines
(such as those spawned internally by @code{async-read-file}) are
anonymous and communicate only by @code{actor-join}.

@subsection Error Propagation

  If a coroutine's body signals an error that is not caught inside the
coroutine, the error is stored in the coroutine's result slot.  When the
caller reaches the corresponding @code{await} expression, the error is
re-signaled there, as if @code{await} itself had signaled it.  This means
standard @code{condition-case} forms work normally around @code{await}.

@example
(condition-case err
    (let ((text (await (async-read-file "/nonexistent"))))
      (message "got %d chars" (length text)))
  (file-error
   (message "File error: %s" (error-message-string err))))
@end example

@subsection Variable Bindings and Isolation
@cindex specpdl, coroutine

  Each coroutine has its own @dfn{specpdl} (special binding stack).
A @code{let} binding or @code{unwind-protect} form executed inside a
coroutine is private to that coroutine and does not affect other
coroutines or the main Lisp thread.  When a coroutine is spawned it
inherits a copy of the caller's current bindings (such as
@code{default-directory} and the current buffer), but subsequent bindings
diverge independently.
```

- [ ] **Step 2: Commit**

```bash
git add man/lispref/async.texi
git commit -m "docs(lispref): async.texi — header, Coroutine Concepts section"
```

---

## Task 2: Add API Reference — Core Primitives and Actor Model

**Files:**
- Modify: `man/lispref/async.texi`

- [ ] **Step 1: Append the API Reference node, @menu, and Core Primitives subsection**

Append to `man/lispref/async.texi`:

```texinfo

@node Async API Reference, Async Cookbook, Coroutine Concepts, Async Programming
@section Async API Reference

@menu
* Core Primitives::             @code{await}, @code{async-let}, @code{async-yield}.
* Actor Model::                 Spawn, send, receive, join, monitor.
* Async File and Shell IO::     Async file read/write and shell commands.
* Async HTTP::                  HTTP GET, POST, streaming, and SSE parsing.
* Async Error Symbols::         Error conditions signaled by the async system.
@end menu

@node Core Primitives, Actor Model, Async API Reference, Async API Reference
@subsection Core Primitives

@defmac await expr
Suspend the current coroutine until the async operation initiated by
@var{expr} completes.  @var{expr} must be a call to an async function
(such as @code{async-read-file} or @code{http-get}) that internally
spawns a coroutine and yields.  Returns the operation's result value, or
re-signals the error if the operation failed.

@code{await} may only be called from within a running coroutine.  Calling
it from the main Lisp thread signals @code{async-error}.

@lisp
(let ((contents (await (async-read-file "/etc/hosts"))))
  (message "File has %d characters" (length contents)))
@end lisp
@end defmac

@defmac async-let bindings &rest body
Spawn each binding's value expression as a parallel coroutine, await all
of them, then evaluate @var{body} with each variable bound to its
coroutine's result.  Analogous to @code{Promise.all()} in JavaScript or
@code{asyncio.gather()} in Python.

@var{bindings} has the same syntax as @code{let}: a list of
@code{(@var{var} @var{expr})} pairs.  All expressions are started
concurrently; @var{body} does not begin until every expression has
completed.

@lisp
(async-let ((a (async-read-file "/tmp/foo"))
            (b (async-read-file "/tmp/bar")))
  (message "Got %d and %d characters" (length a) (length b)))
@end lisp
@end defmac

@defun async-yield
Voluntarily yield the current coroutine to the scheduler without waiting
for any particular event.  The coroutine is immediately placed back on
the runnable queue and will be resumed on the next scheduler tick.
Useful for breaking up long-running computations to keep the editor
responsive.

@lisp
(dotimes (i 1000000)
  (when (zerop (mod i 10000))
    (async-yield))   ; let the editor breathe every 10k iterations
  (do-work i))
@end lisp
@end defun

@node Actor Model, Async File and Shell IO, Core Primitives, Async API Reference
@subsection Actor Model
@cindex actor-spawn
@cindex actor model

@defun actor-spawn name function &rest args
Spawn a new coroutine that calls @var{function} with @var{args}.
@var{name} is a symbol used to register the actor for later lookup via
@code{actor-find}, or @code{nil} for an anonymous coroutine.  Returns an
opaque actor handle.

@lisp
(actor-spawn 'my-worker
  (lambda (_)
    (while t
      (process-message (actor-receive))))
  nil)
@end lisp
@end defun

@defun actor-send actor-ref message
Deliver @var{message} to the mailbox of @var{actor-ref}.  This function
is non-blocking: it returns immediately without suspending the caller.
@var{message} may be any Lisp object.  Messages are delivered in FIFO
order.

@lisp
(actor-send worker '(:job "compile-buffer"))
@end lisp
@end defun

@defun actor-receive &rest keys
Suspend the current coroutine until a message arrives in its mailbox.
Returns the message.  The coroutine must be an actor (spawned via
@code{actor-spawn}); calling this from a plain coroutine or the main
thread signals @code{async-error}.

@table @code
@item :timeout @var{ms}
If no message arrives within @var{ms} milliseconds, signal
@code{async-timeout}.
@end table

@lisp
(let ((msg (actor-receive :timeout 5000)))
  (message "Got: %s" msg))
@end lisp
@end defun

@defun actor-join actor-ref
Suspend the current coroutine until @var{actor-ref}'s coroutine finishes,
then return its result value.  If the joined coroutine terminated with an
error, that error is re-signaled here.

@lisp
(let ((handle (actor-spawn nil (lambda (_) (+ 1 2)) nil)))
  (message "Result: %s" (actor-join handle)))   ; => "Result: 3"
@end lisp
@end defun

@defun actor-monitor actor-ref
Arrange for the current actor to receive a @code{(:exit @var{reason})}
message when @var{actor-ref}'s coroutine terminates, whether normally or
due to an error.  @var{reason} is @code{nil} on normal exit, or the
error data on abnormal exit.  Returns @var{actor-ref}.

@lisp
(actor-monitor fragile-worker)
(let ((msg (actor-receive)))
  (when (eq (car msg) :exit)
    (message "Worker exited: %s" (cadr msg))))
@end lisp
@end defun

@defun actor-find name
Look up a named actor by its symbol @var{name}.  Returns the actor handle
if a live actor with that name exists, or @code{nil} otherwise.

@lisp
(let ((w (actor-find 'my-worker)))
  (when w (actor-send w '(:stop))))
@end lisp
@end defun

@defun actor-self
Return the handle of the currently executing actor, or @code{nil} if
called from the main Lisp thread.  Useful when an actor needs to pass its
own handle to another actor.

@lisp
(actor-spawn 'echo-server
  (lambda (_)
    (let ((self (actor-self)))
      (actor-send coordinator `(:ready ,self))))
  nil)
@end lisp
@end defun
```

- [ ] **Step 2: Commit**

```bash
git add man/lispref/async.texi
git commit -m "docs(lispref): async.texi — Core Primitives and Actor Model API reference"
```

---

## Task 3: Add API Reference — File/Shell I/O, HTTP, Error Symbols

**Files:**
- Modify: `man/lispref/async.texi`

- [ ] **Step 1: Append File/Shell I/O, HTTP, and Error Symbols subsections**

Append to `man/lispref/async.texi`:

```texinfo

@node Async File and Shell IO, Async HTTP, Actor Model, Async API Reference
@subsection Async File and Shell I/O

@defun async-read-file path &rest keys
Read the file at @var{path} asynchronously.  Pass the return value to
@code{await} to obtain the file contents as a string.

@table @code
@item :coding @var{coding-system}
Decode the file using @var{coding-system}.  Defaults to @code{utf-8}.
@end table

@lisp
(let ((text (await (async-read-file "/etc/passwd" :coding 'utf-8))))
  (message "First line: %s" (car (split-string text "\n"))))
@end lisp
@end defun

@defun async-write-file path content &rest keys
Write the string @var{content} to @var{path} asynchronously.  Pass the
return value to @code{await}; the result is @code{t} on success.

@table @code
@item :coding @var{coding-system}
Encode @var{content} using @var{coding-system} before writing.
Defaults to @code{utf-8}.
@end table

@lisp
(await (async-write-file "/tmp/result.txt" "hello world"))
@end lisp
@end defun

@defun async-shell-command cmd &rest keys
Run the shell command @var{cmd} asynchronously using
@code{shell-file-name}.  Pass the return value to @code{await} to obtain
the command's standard output as a string.

@table @code
@item :timeout @var{ms}
Signal @code{async-timeout} if the command does not complete within
@var{ms} milliseconds.
@end table

@lisp
(let ((out (await (async-shell-command "uname -a"))))
  (message "System: %s" out))
@end lisp
@end defun

@node Async HTTP, Async Error Symbols, Async File and Shell IO, Async API Reference
@subsection Async HTTP
@cindex HTTP, async
@cindex SSE, server-sent events

  Async HTTP requires XEmacs to have been built with libcurl support
(@code{HAVE_LIBCURL}).  If libcurl is not available, calling these
functions signals @code{async-error}.

@defun http-get url &rest keys
Perform an asynchronous HTTP GET request to @var{url}.  Pass the return
value to @code{await} to obtain the result as a list
@code{(@var{status} @var{headers} @var{body})}, where @var{status} is the
HTTP response code (integer), @var{headers} is an alist of response
headers, and @var{body} is the response body string.

@table @code
@item :headers @var{alist}
Additional request headers as an alist of @code{("Name" . "Value")} pairs.
@item :timeout @var{ms}
Signal @code{async-timeout} after @var{ms} milliseconds.
@end table

@lisp
(let* ((resp   (await (http-get "https://example.com/"
                                :timeout 10000)))
       (status (nth 0 resp))
       (body   (nth 2 resp)))
  (message "HTTP %d, %d bytes" status (length body)))
@end lisp
@end defun

@defun http-post url &rest keys
Perform an asynchronous HTTP POST request to @var{url}.  Pass the return
value to @code{await} to obtain @code{(@var{status} @var{headers}
@var{body})}.

@table @code
@item :headers @var{alist}
Additional request headers.
@item :json @var{object}
Encode @var{object} as JSON and send it as the request body.  Sets
@code{Content-Type: application/json} automatically.
@item :body @var{string}
Send @var{string} as the raw request body (use instead of @code{:json}).
@item :on-chunk @var{function}
If provided, @var{function} is called with each response chunk string
as it arrives.  The coroutine does not suspend while chunks are arriving;
use this for streaming responses.
@item :timeout @var{ms}
Signal @code{async-timeout} after @var{ms} milliseconds.
@end table

@lisp
(let ((resp (await (http-post "https://api.example.com/submit"
                              :json '((key . "value"))
                              :timeout 5000))))
  (message "Status: %d" (nth 0 resp)))
@end lisp
@end defun

@defun http-stream url &rest keys
Perform an asynchronous streaming HTTP POST to @var{url}.  This is an
alias for @code{http-post} intended for use with @code{:on-chunk}.
The @code{:on-chunk} keyword is required.

@lisp
(await (http-stream "https://api.example.com/stream"
          :json '((model . "gpt-4"))
          :on-chunk (lambda (chunk) (princ chunk))))
@end lisp
@end defun

@defun sse-parse-data line
Parse a single Server-Sent Events line @var{line}.  If @var{line} begins
with @samp{data: }, returns the payload string (everything after
@samp{data: }).  Otherwise returns @code{nil}.

@lisp
(sse-parse-data "data: {\"choices\": []}")
  @result{} "{\"choices\": []}"
(sse-parse-data "event: ping")
  @result{} nil
@end lisp
@end defun

@defun sse-extract-text chunk
Parse the multi-line SSE @var{chunk}, extract and concatenate all
@code{content} delta strings from OpenAI- and Anthropic-compatible
streaming JSON payloads.  Returns the concatenated text string, or
@code{nil} if no content deltas were found.

@lisp
(sse-extract-text "data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n")
  @result{} "Hello"
@end lisp
@end defun

@node Async Error Symbols, Async Cookbook, Async HTTP, Async API Reference
@subsection Async Error Symbols

@defvar async-timeout
Signaled by @code{actor-receive} and @code{async-shell-command} when a
@code{:timeout} interval expires before the operation completes.
@end defvar

@defvar async-http-error
Signaled by @code{http-get}, @code{http-post}, and @code{http-stream}
when a transport-level failure occurs (a libcurl error).  This is
distinct from an HTTP error status code (4xx, 5xx), which is returned
normally in the @var{status} field of the result list.
@end defvar

@defvar async-error
General scheduler-level error.  Signaled when async primitives are
called from outside a coroutine context, or when the scheduler
encounters an internal inconsistency.
@end defvar
```

- [ ] **Step 2: Commit**

```bash
git add man/lispref/async.texi
git commit -m "docs(lispref): async.texi — File/Shell IO, HTTP, and Error Symbols API reference"
```

---

## Task 4: Add Async Cookbook

**Files:**
- Modify: `man/lispref/async.texi`

- [ ] **Step 1: Append the Cookbook section with all five examples**

Append to `man/lispref/async.texi`:

```texinfo

@node Async Cookbook,  , Async API Reference, Async Programming
@section Async Cookbook
@cindex async, examples

  This section shows complete, runnable examples illustrating common
async patterns.  Each example assumes the relevant libraries have been
loaded with @code{require}.

@subsection Fetch Two URLs in Parallel

  Use @code{async-let} when you need several independent HTTP responses
before proceeding.  Both requests are initiated concurrently; the body
runs only after both complete.

@lisp
(require 'async-core)
(require 'async-http)

(actor-spawn nil
  (lambda (_)
    (async-let ((r1 (http-get "https://example.com/api/a"))
                (r2 (http-get "https://example.com/api/b")))
      (let ((body1 (nth 2 r1))
            (body2 (nth 2 r2)))
        (with-current-buffer (get-buffer-create "*results*")
          (erase-buffer)
          (insert body1 "\n---\n" body2)
          (display-buffer (current-buffer))))))
  nil)
@end lisp

  @code{async-let} spawns both @code{http-get} calls as independent
coroutines.  Each suspends on its own curl operation.  The scheduler
resumes each as its response arrives.  @code{body1} and @code{body2} are
both bound before @code{with-current-buffer} executes.

@subsection Read a File, Transform It, Write It Back

  Use sequential @code{await} calls for operations that depend on each
other's results.

@lisp
(require 'async-core)
(require 'async-file)

(actor-spawn nil
  (lambda (_)
    (let* ((text    (await (async-read-file "/tmp/input.txt")))
           (upcased (upcase text)))
      (await (async-write-file "/tmp/output.txt" upcased))
      (message "Done.")))
  nil)
@end lisp

  The second @code{await} does not start until the first @code{await}
returns.  Error handling works naturally: wrap in @code{condition-case}
around either @code{await} call to catch @code{file-error}.

@subsection Named Worker Actor with Job Queue

  Use a named actor with a @code{while}/@code{actor-receive} loop to
build a persistent worker that processes jobs sent from elsewhere in the
program.

@lisp
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

;; Send jobs from anywhere, any time
(when-let ((w (actor-find 'my-worker)))
  (actor-send w '(:job "task-1"))
  (actor-send w '(:job "task-2"))
  (actor-send w '(:stop)))
@end lisp

  @code{actor-receive} suspends the worker until a message arrives.
@code{actor-find} retrieves the handle by name from the global registry.
Sending @code{(:stop)} causes the loop to exit and the coroutine to
finish normally.

@subsection Streaming HTTP with Server-Sent Events

  Use @code{http-stream} with @code{:on-chunk} to process a streaming
response incrementally as chunks arrive, rather than waiting for the full
response body.

@lisp
(require 'async-http)

(actor-spawn nil
  (lambda (_)
    (await
      (http-stream "https://api.anthropic.com/v1/messages"
        :headers '(("x-api-key"          . "YOUR-KEY")
                   ("anthropic-version"  . "2023-06-01")
                   ("content-type"       . "application/json"))
        :json `((model      . "claude-opus-4-7")
                (max_tokens . 256)
                (stream     . t)
                (messages   . [((role    . "user")
                                (content . "Say hello."))]))
        :on-chunk (lambda (chunk)
                    (let ((text (sse-extract-text chunk)))
                      (when text
                        (with-current-buffer (get-buffer-create "*stream*")
                          (goto-char (point-max))
                          (insert text))))))))
  nil)
@end lisp

  The @code{:on-chunk} callback is invoked synchronously in the scheduler
tick each time libcurl delivers a chunk.  @code{sse-extract-text} handles
the @samp{data: @{...@}} framing and JSON parsing, returning the
concatenated @code{content} delta text.  The coroutine does not suspend
between chunks.

@subsection Supervised Actor with Monitor

  Use @code{actor-monitor} to build a supervisor that automatically
restarts a worker actor if it crashes.

@lisp
(require 'actor)

(defun my-start-worker ()
  "Spawn a fragile worker actor and return its handle."
  (actor-spawn 'fragile-worker
    (lambda (_)
      (while t
        (let ((job (actor-receive)))
          ;; Work that might fail
          (do-risky-work job))))
    nil))

(actor-spawn 'supervisor
  (lambda (_)
    (let ((worker (my-start-worker)))
      (actor-monitor worker)
      (while t
        (let ((msg (actor-receive)))
          (pcase msg
            (`(:exit ,_reason)
             ;; Worker died -- restart and re-monitor
             (message "Worker exited, restarting...")
             (setq worker (my-start-worker))
             (actor-monitor worker))
            (job
             ;; Forward all other messages to the worker
             (actor-send worker job)))))))
  nil)
@end lisp

  @code{actor-monitor} arranges for @code{(:exit @var{reason})} to be
delivered to the supervisor's mailbox when the worker finishes, whether
normally or due to an unhandled error.  The supervisor restarts the
worker and calls @code{actor-monitor} again on the new handle.
```

- [ ] **Step 2: Commit**

```bash
git add man/lispref/async.texi
git commit -m "docs(lispref): async.texi — Async Cookbook (5 worked examples)"
```

---

## Task 5: Wire `async.texi` into `lispref.texi` and fix node chain

**Files:**
- Modify: `man/lispref/lispref.texi:1259`
- Modify: `man/lispref/processes.texi:5`
- Modify: `man/lispref/os.texi:5`

- [ ] **Step 1: Add `@include async.texi` in `lispref.texi` after `processes.texi`**

In `man/lispref/lispref.texi`, line 1259 currently reads:

```texinfo
@include processes.texi
@include os.texi
```

Change it to:

```texinfo
@include processes.texi
@include async.texi
@include os.texi
```

- [ ] **Step 2: Update the `@node` forward pointer in `processes.texi`**

In `man/lispref/processes.texi`, line 5 currently reads:

```texinfo
@node Processes, System Interface, Databases, Top
```

Change it to:

```texinfo
@node Processes, Async Programming, Databases, Top
```

- [ ] **Step 3: Update the `@node` back pointer in `os.texi`**

In `man/lispref/os.texi`, line 5 currently reads:

```texinfo
@node System Interface, X-Windows, Processes, Top
```

Change it to:

```texinfo
@node System Interface, X-Windows, Async Programming, Top
```

- [ ] **Step 4: Commit**

```bash
git add man/lispref/lispref.texi man/lispref/processes.texi man/lispref/os.texi
git commit -m "docs(lispref): wire async.texi into lispref node chain"
```

---

## Task 6: Create `man/internals/async.texi` — Architecture, Struct, Lifecycle

**Files:**
- Create: `man/internals/async.texi`

- [ ] **Step 1: Create the file with the first four sections**

Create `man/internals/async.texi` with this exact content:

```texinfo
@c -*-texinfo-*-
@c This is part of the XEmacs Internals Manual.
@c Copyright (C) 2026 the XEmacs Project.
@node Async Scheduler Internals, Index, Old Future Work, Top
@chapter Async Scheduler Internals
@cindex async scheduler
@cindex coroutine, C implementation
@cindex cooperative scheduling, internals

  This chapter documents the C implementation of the XEmacs coroutine
scheduler.  For the Lisp-facing API — @code{await}, @code{actor-spawn},
@code{http-get}, and related forms — see
@ref{Async Programming,,Async Programming,lispref,XEmacs Lisp Reference Manual}.

  The source files are @file{src/async-scheduler.c},
@file{src/async-scheduler.h}, @file{src/async-http.c}, and
@file{src/async-http.h}.

@menu
* Async Architecture Overview::    How the scheduler fits into the event loop.
* The xemacs_coro Structure::      Every field of the coroutine struct.
* Coroutine Lifecycle::            Spawn, run, suspend, resume, die.
* Context Switching::              The coro_swap assembly primitive.
* Per-Coroutine specpdl::          Why each coroutine has its own binding stack.
* Async GC Integration::           Keeping Lisp objects alive on C stacks.
* Async HTTP Integration::         libcurl multi-handle and the event loop.
* Adding New Async Primitives::    How to write a new C-level async operation.
@end menu

@node Async Architecture Overview, The xemacs_coro Structure, Async Scheduler Internals, Async Scheduler Internals
@section Architecture Overview

  The XEmacs async scheduler is a @dfn{cooperative, run-to-yield}
coroutine system.  There is no preemption: a coroutine holds the CPU
until it explicitly suspends itself.

  The scheduler is driven by the XEmacs event loop.  In
@file{src/event-unixoid.c}, after each @code{select()} call returns,
@code{async_scheduler_tick()} is called.  The tick performs three
actions:

@enumerate
@item
It calls @code{async_http_tick()} to drive the libcurl multi-handle and
complete any pending HTTP transfers (@pxref{Async HTTP Integration}).

@item
It iterates the doubly-linked list of all live coroutines
(@code{all_coros}) and resumes any that are now runnable — that is, whose
wait condition has been satisfied.

@item
It returns to the event loop.  The main Lisp thread then continues as
normal.
@end enumerate

  The execution path through a single resume looks like this:

@example
main thread
  --> async_scheduler_tick()
    --> coro_resume(c, result)
      --> coro_swap(&scheduler_ctx, &c->ctx)
        [coroutine runs]
        --> coro_yield(reason)
          --> coro_swap(&c->ctx, &c->caller_ctx)
        [back in coro_resume, returns to tick]
  --> async_scheduler_tick() returns
main thread continues
@end example

  The main Lisp thread is never a coroutine.  It is the @dfn{scheduler
context} — the fixed point that all coroutines return to when they yield.

@node The xemacs_coro Structure, Coroutine Lifecycle, Async Architecture Overview, Async Scheduler Internals
@section The @code{xemacs_coro} Structure
@cindex xemacs_coro

  Every live coroutine is represented by a @code{struct xemacs_coro}
allocated on the C heap.  The full definition is in
@file{src/async-scheduler.h}.  The fields are:

@table @code
@item ctx
@code{coro_context_t} — the saved stack pointer for this coroutine.
Restored by @code{coro_swap} when resuming this coroutine.

@item caller_ctx
@code{coro_context_t} — the scheduler's stack pointer at the time this
coroutine was last resumed.  @code{coro_yield} swaps back here to return
to the scheduler.

@item stack
@code{unsigned char *} — the @code{malloc}'d private C stack for this
coroutine.  Default size is 256@tie{}KB; configurable via the Lisp
variable @code{async-coroutine-stack-size}.

@item stack_size
@code{Bytecount} — the allocated size of @code{stack} in bytes.

@item state
@code{coro_state_t} — one of @code{CORO_RUNNABLE}, @code{CORO_RUNNING},
@code{CORO_SUSPENDED}, or @code{CORO_DEAD}.  @xref{Coroutine Lifecycle}.

@item wait_reason
@code{wait_reason_t} — the reason this coroutine is suspended.  One of:
@code{WAIT_NONE} (runnable), @code{WAIT_FD} (waiting for a file
descriptor), @code{WAIT_TIMER} (waiting for a timeout),
@code{WAIT_MAILBOX} (waiting for an actor message), or @code{WAIT_JOIN}
(waiting for another coroutine to finish).

@item result
@code{Lisp_Object} — dual purpose: holds the initial argument on first
entry; holds the return value after the coroutine finishes.  Also used
temporarily to pass the packed @code{(fn . arg)} cons to the trampoline.

@item error
@code{Lisp_Object} — if non-@code{nil}, contains error data
@code{(ERROR-SYMBOL . DATA)} that will be re-signaled at the next
@code{await} call.

@item lisp_fn
@code{Lisp_Object} — the Lisp lambda to call on first entry.  Kept
non-@code{nil} until after the first @code{coro_swap} so the GC does not
collect the closure.

@item gcpro_chain
@code{struct gcpro *} — the head of the @code{gcpro} chain for live
@code{Lisp_Object} values on this coroutine's C stack.  Saved at yield
and restored at resume.  @xref{Async GC Integration}.

@item saved_specpdl_ptr
@code{struct specbinding *} — the global @code{specpdl_ptr} value saved
when this coroutine yielded.

@item saved_specpdl_depth
@code{int} — the global @code{specpdl_depth_counter} saved at yield.

@item saved_backtrace_list
@code{struct backtrace *} — the @code{backtrace_list} chain saved at
yield.

@item saved_catchlist
@code{struct catchtag *} — the @code{catchlist} chain saved at yield.
The @code{catchlist} lives on the C stack and must be explicitly
saved/restored, unlike @code{Vcondition_handlers} which is a
GC-managed Lisp object.

@item coro_specpdl
@code{struct specbinding *} — the private specpdl array for this
coroutine.  Allocated at spawn with @code{CORO_SPECPDL_INITIAL_SIZE}
(200) entries.  @xref{Per-Coroutine specpdl}.

@item coro_specpdl_ptr
@code{struct specbinding *} — the current top of @code{coro_specpdl}.

@item coro_specpdl_size
@code{int} — number of allocated entries in @code{coro_specpdl}.

@item coro_inherit_depth
@code{int} — number of entries copied from the main thread's specpdl
at spawn time.  New bindings are appended above this point in the
private array.

@item wait_fd
@code{int} — the file descriptor this coroutine is waiting for
(@code{WAIT_FD}).

@item wait_deadline
@code{EMACS_TIME} — absolute deadline for timeout waits
(@code{WAIT_TIMER}).

@item mailbox
@code{Lisp_Object} — list of pending messages for this actor
(@code{WAIT_MAILBOX}).

@item actor_name
@code{Lisp_Object} — the name symbol if this is a named actor;
@code{Qnil} otherwise.

@item is_actor
@code{int} — non-zero if this coroutine has a mailbox and was spawned
via @code{actor-spawn}.

@item join_waiter
@code{struct xemacs_coro *} — pointer to the coroutine waiting for this
one to finish (@code{WAIT_JOIN}), or @code{NULL}.

@item next, prev
@code{struct xemacs_coro *} — intrusive doubly-linked list links
connecting all live coroutines via @code{all_coros}.
@end table

@node Coroutine Lifecycle, Context Switching, The xemacs_coro Structure, Async Scheduler Internals
@section Coroutine Lifecycle
@cindex coroutine lifecycle

  A coroutine passes through four states:

@example
CORO_RUNNABLE --> CORO_RUNNING --> CORO_SUSPENDED --> CORO_RUNNABLE
                                                           |
                                                      (repeats)
                                                           |
                                                      CORO_DEAD
@end example

@table @strong
@item Spawn (@code{coro_spawn})
@code{coro_spawn(fn, arg)} allocates a new @code{xemacs_coro} on the
heap, @code{malloc}s its stack, copies the inherited specpdl entries from
the main thread, writes @code{coro_trampoline}'s address at the top of
the stack so the first @code{coro_swap} lands there, and sets state to
@code{CORO_RUNNABLE}.  The coroutine is prepended to the @code{all_coros}
list.

@item First Run
The scheduler calls @code{coro_resume(c, initial_arg)}.  This saves the
scheduler's interpreter state, sets state to @code{CORO_RUNNING}, and
calls @code{coro_swap(&scheduler_ctx, &c->ctx)}, which jumps into
@code{coro_trampoline}.  The trampoline calls @code{condition_case_1}
wrapping the Lisp function, so that any unhandled errors are caught and
stored in @code{c->error} rather than longjmping out of the coroutine
context.

@item Suspend (@code{coro_yield})
When a coroutine calls @code{coro_yield(reason)}, the current
interpreter state (specpdl, backtrace, catchlist, gcpro chain) is saved
into the coroutine struct, @code{wait_reason} is set, state becomes
@code{CORO_SUSPENDED}, and @code{coro_swap(&c->ctx, &c->caller_ctx)}
returns control to the point in @code{coro_resume} just after the
outgoing @code{coro_swap} — which then returns to the scheduler tick.

@item Resume (@code{coro_resume})
@code{coro_resume(c, result)} delivers @var{result} into @code{c->result},
saves the scheduler's current interpreter state, restores
@code{c}'s saved state into the globals, sets state to
@code{CORO_RUNNING}, and calls @code{coro_swap(&scheduler_ctx, &c->ctx)}.
Execution continues inside @code{coro_yield} immediately after the swap,
which returns to wherever the coroutine was when it yielded.

@item Death
When the coroutine function returns (or @code{condition_case_1} catches
an unhandled error), @code{coro_trampoline} sets @code{c->result} to the
return value (or @code{c->error} to the error data), sets state to
@code{CORO_DEAD}, removes the coroutine from @code{all_coros}, and moves
it to @code{dead_coros}.  If @code{c->join_waiter} is non-@code{NULL},
that coroutine is immediately resumed with the result.
@end table
```

- [ ] **Step 2: Commit**

```bash
git add man/internals/async.texi
git commit -m "docs(internals): async.texi — Architecture, xemacs_coro struct, Lifecycle"
```

---

## Task 7: Add internals sections — coro_swap, specpdl, GC, HTTP, Contributor Guide

**Files:**
- Modify: `man/internals/async.texi`

- [ ] **Step 1: Append the remaining five sections**

Append to `man/internals/async.texi`:

```texinfo

@node Context Switching, Per-Coroutine specpdl, Coroutine Lifecycle, Async Scheduler Internals
@section Context Switching — @code{coro_swap}
@cindex coro_swap
@cindex context switch, coroutine
@cindex stack pointer, coroutine

  @code{coro_swap(from, to)} is an x86-64 assembly function defined in
@file{src/async-scheduler.c}.  It performs a minimal cooperative context
switch by swapping the C stack pointer:

@example
coro_swap:
    mov  %rsp, (%rdi)   /* save current SP into from->sp */
    mov  (%rsi), %rsp   /* load SP from to->sp           */
    ret                 /* "return" into the other stack  */
@end example

  The @code{ret} instruction pops the return address from whatever stack
@code{%rsp} now points to:

@itemize @bullet
@item
On the @emph{first} entry to a new coroutine, @code{to->sp} was set up
by @code{coro_spawn} to point just below @code{coro_trampoline}'s address
on the coroutine's private stack.  The @code{ret} therefore jumps into
the trampoline.

@item
On subsequent @emph{resumes}, @code{to->sp} points to the saved frame
inside @code{coro_yield} on the coroutine's stack, so execution resumes
exactly where it suspended.
@end itemize

  @strong{Why only the stack pointer?}  All other registers that C code
depends on across a call boundary are @dfn{caller-saved} by the System V
AMD64 ABI: the caller (the function that executed @code{coro_swap}) has
already saved them to its own stack frame.  Each coroutine's private
stack holds all its live C register values; swapping @code{%rsp}
effectively swaps the entire register state.

  @strong{Stack alignment.}  The System V AMD64 ABI requires @code{%rsp}
to be 16-byte aligned immediately before a @code{call} instruction (and
therefore 8-byte aligned at function entry due to the pushed return
address).  @code{coro_spawn} pads the initial stack pointer accordingly.

@node Per-Coroutine specpdl, Async GC Integration, Context Switching, Async Scheduler Internals
@section Per-Coroutine @code{specpdl}
@cindex specpdl, per-coroutine
@cindex let binding, coroutine isolation

  XEmacs uses a single global @code{specpdl} array to record all
dynamic @code{let} bindings and @code{unwind-protect} handlers.  If
coroutines shared this array, a binding made in one coroutine would be
visible in all others — and unbinding on coroutine exit would corrupt
bindings belonging to other coroutines.

  The solution is a @dfn{per-coroutine specpdl}.  Each @code{xemacs_coro}
has its own @code{coro_specpdl} array (initial size: 200 entries,
defined by @code{CORO_SPECPDL_INITIAL_SIZE}).

@subheading Inheritance at spawn time

  When a coroutine is spawned, the entries from the main thread's specpdl
up to the current depth are @emph{copied} into @code{coro_specpdl}.  This
count is stored in @code{coro_inherit_depth}.  These inherited entries
represent bindings that were in effect when the coroutine was created
(for example, @code{default-directory} or the current buffer), and the
coroutine should observe them.  New bindings made inside the coroutine
are appended above the inherited entries in the private array and do not
touch the main thread's specpdl.

@subheading Swapping on yield and resume

  When a coroutine yields, @code{coro_yield} saves the current global
@code{specpdl_ptr} and depth into the coroutine struct
(@code{saved_specpdl_ptr}, @code{saved_specpdl_depth}), then restores
the scheduler's saved values into the globals.  On resume, the reverse
happens: the scheduler's state is saved and the coroutine's state is
restored.

@subheading Symbol value swapping

  Simply saving and restoring @code{specpdl_ptr} is not enough: the
@emph{actual symbol values} must also be swapped, because XEmacs symbol
values live directly in the symbol objects (not on the specpdl).

  @code{specpdl_swap_one(p)} exchanges @code{symbol->value} with
@code{p->old_value} for one specpdl entry.  Walking the range
@emph{forward} (low index to high) re-applies a sequence of bindings;
walking it @emph{backward} undoes them.  Entries with a non-@code{NULL}
@code{func} pointer are @code{unwind-protect} handlers, not symbol
bindings; they are skipped during swapping.

  For symbols with magic values (buffer-local variables, variable
aliases), the swap falls back to @code{Fsymbol_value} and @code{Fset} to
keep all magic behaviour consistent.

@node Async GC Integration, Async HTTP Integration, Per-Coroutine specpdl, Async Scheduler Internals
@section GC Integration
@cindex GC, coroutine
@cindex garbage collection, async
@cindex gcpro, coroutine

  The XEmacs garbage collector traces @code{Lisp_Object} references
starting from known roots: the global @code{specpdl}, the @code{gcpro}
chain, @code{Vobarray}, etc.  Coroutine stacks are ordinary C stacks
allocated with @code{malloc}; the GC has no mechanism to scan them.

  Without special handling, a @code{Lisp_Object} stored in a C local
variable on a suspended coroutine's stack would be invisible to the GC
and could be freed while the coroutine is suspended.

@subheading @code{async_scheduler_mark_gcpros}

  @code{async_scheduler_mark_gcpros()} is registered with the GC and
called during the mark phase.  It iterates @code{all_coros} and marks
the following fields of each coroutine:

@itemize @bullet
@item @code{c->lisp_fn} — the lambda, kept alive until first run.
@item @code{c->result} — current result or packed @code{(fn . arg)} cons.
@item @code{c->error} — stored error data, if any.
@item @code{c->mailbox} — the list of pending messages.
@item @code{c->actor_name} — the name symbol.
@item Every @code{Lisp_Object} in the chain @code{c->gcpro_chain} — objects
explicitly protected on the coroutine's C stack via @code{GCPRO}/@code{UNGCPRO}.
@end itemize

@subheading Rule for C code that suspends a coroutine

  Any @code{Lisp_Object} local variable in a C function that must remain
live across a @code{coro_yield()} call @emph{must} be protected with
@code{GCPRO} before the yield and @code{UNGCPRO}d after the resume.  The
@code{gcpro_chain} field on the coroutine struct is set to the active
@code{gcpro} chain at yield time, so @code{async_scheduler_mark_gcpros}
can trace it.

  Objects stored directly in the coroutine struct fields listed above
(e.g.@: @code{c->result}, @code{c->mailbox}) are marked automatically
and do not need additional @code{GCPRO} protection.

@node Async HTTP Integration, Adding New Async Primitives, Async GC Integration, Async Scheduler Internals
@section HTTP Integration
@cindex libcurl, async
@cindex HTTP, C implementation
@cindex CURLM, multi-handle

  Async HTTP support is in @file{src/async-http.c} and requires libcurl
(@code{HAVE_LIBCURL}).  It wraps libcurl's multi-handle interface so that
HTTP I/O participates in XEmacs's existing @code{select()}-based event
loop without requiring a separate thread.

@table @strong
@item async_http_start
@code{async_http_start(url, method, headers, body, on_chunk, timeout_ms)}
allocates an @code{async_http_req} struct, creates a @code{CURL} easy
handle, configures it (URL, method, headers, write callback), and adds it
to the global @code{CURLM} multi-handle.  It then suspends the calling
coroutine via @code{coro_yield(WAIT_FD)} and returns the
@code{async_http_req} pointer wrapped in a @code{Lisp_Object} opaque.

@item Write callback
The libcurl write callback (@code{http_write_cb}) is called each time
curl delivers response data.  If @code{on_chunk} is non-@code{nil}, the
callback invokes it directly via @code{call1}, passing the chunk as a
Lisp string.  The coroutine is @emph{not} suspended during streaming;
@code{on_chunk} runs on the scheduler's own stack context, invoked from
@code{async_http_tick}.

@item async_http_tick
Called by @code{async_scheduler_tick} on every event-loop iteration.
Calls @code{curl_multi_perform()} to advance all in-progress transfers,
then calls @code{curl_multi_info_read()} in a loop.  For each completed
transfer, it locates the @code{async_http_req}, extracts the HTTP status
code and response headers, and calls @code{coro_resume} (or
@code{coro_resume_with_error} on curl failure) to wake the waiting
coroutine.

@item fd registration
@code{async_http_socket_ready(fd, data)} is registered with XEmacs's
fd-watching infrastructure via @code{register_event_cb} so that curl
sockets are included in the top-level @code{select()} call.  When
@code{select()} reports a curl socket ready, @code{async_http_tick} is
called to process it.  No background thread is needed.
@end table

@node Adding New Async Primitives,  , Async HTTP Integration, Async Scheduler Internals
@section Adding New Async Primitives
@cindex async primitive, how to write

  This section is a step-by-step guide for contributors who want to add a
new C-level async operation (for example, an async DNS resolver or an
async database query).

@enumerate
@item
@strong{Define a request struct.}
Create a struct to hold the operation's state.  It must contain a pointer
to the waiting coroutine:

@example
typedef struct my_async_req @{
  xemacs_coro *waiting_coro;
  /* ... operation-specific state ... */
  int          result_fd;
  Lisp_Object  result_value;
@} my_async_req;
@end example

@item
@strong{Implement the start function.}
The start function runs in the calling coroutine's context.  It
initialises the request, registers the operation with the relevant event
source, and suspends the coroutine:

@example
static Lisp_Object
my_async_start (Lisp_Object arg)
@{
  my_async_req *req = xnew (my_async_req);
  req->waiting_coro = async_scheduler_current_coro ();
  /* ... set up the operation ... */
  register_event_cb (req->result_fd, my_async_ready_cb, req);
  coro_yield (WAIT_FD);
  /* Execution resumes here after coro_resume() is called */
  return req->result_value;
@}
@end example

@item
@strong{Implement the event callback.}
The callback is invoked from the event loop when the operation completes.
It must call @code{coro_resume} or @code{coro_resume_with_error}:

@example
static void
my_async_ready_cb (int fd, void *data)
@{
  my_async_req *req = (my_async_req *) data;
  /* ... read results into req->result_value ... */
  unregister_event_cb (fd);
  coro_resume (req->waiting_coro, req->result_value);
  xfree (req);
@}
@end example

@item
@strong{Expose to Lisp via @code{DEFSUBR}.}
Write a @code{DEFSUBR} that calls the start function and returns its
result.  The Lisp wrapper passes the return value to @code{await}:

@example
DEFUN ("my-async-op", Fmy_async_op, 1, 1, 0, /*
Perform MY-ASYNC-OP on ARG asynchronously.
Pass to `await' for the result.
*/
       (arg))
@{
  return my_async_start (arg);
@}
@end example

@item
@strong{Register in @code{syms_of_async_scheduler}.}
Add @code{defsubr (&Smy_async_op);} to @code{syms_of_async_scheduler()}
in @file{src/async-scheduler.c}.

@item
@strong{Handle GC.}
If @code{req->result_value} or any other @code{Lisp_Object} in the
request struct must survive a GC cycle while the coroutine is suspended,
either store them in a field that @code{async_scheduler_mark_gcpros}
already marks (such as @code{waiting_coro->result}), or add an explicit
marking call for your struct's fields in
@code{async_scheduler_mark_gcpros}.  @xref{Async GC Integration}.
@end enumerate
```

- [ ] **Step 2: Commit**

```bash
git add man/internals/async.texi
git commit -m "docs(internals): async.texi — coro_swap, specpdl, GC, HTTP, contributor guide"
```

---

## Task 8: Wire `async.texi` into `internals.texi` and fix node chain

**Files:**
- Modify: `man/internals/internals.texi:33289` (the `@node Index` line)

The current last chapter node is:

```
@node Old Future Work -- Keyword Parameters,  , Old Future Work -- Display Tables, Old Future Work
```
(line 33102)

And the `@node Index` line at 33289 reads:

```
@node Index,  , Old Future Work, Top
```

- [ ] **Step 1: Update `@node Index` back-pointer in `internals.texi`**

In `man/internals/internals.texi`, find the line (around 33289):

```texinfo
@node Index,  , Old Future Work, Top
```

Change it to:

```texinfo
@node Index,  , Async Scheduler Internals, Top
```

- [ ] **Step 2: Add `@include async.texi` immediately before `@node Index`**

In `man/internals/internals.texi`, insert a blank line and then the include immediately before the `@node Index` line:

```texinfo
@include async.texi

@node Index,  , Async Scheduler Internals, Top
```

- [ ] **Step 3: Commit**

```bash
git add man/internals/internals.texi
git commit -m "docs(internals): wire async.texi into internals node chain"
```

---

## Self-Review

**Spec coverage check:**

| Spec requirement | Task |
|-----------------|------|
| lispref chapter header, @menu, @node | Task 1 |
| Section 1: Coroutine Concepts (6 topics) | Task 1 |
| Section 2.1: Core Primitives (`await`, `async-let`, `async-yield`) | Task 2 |
| Section 2.2: Actor Model (7 functions) | Task 2 |
| Section 2.3: File/Shell I/O (3 functions) | Task 3 |
| Section 2.4: HTTP (5 functions) | Task 3 |
| Section 2.5: Error Symbols (3 symbols) | Task 3 |
| Section 3: Cookbook — Example 1 (parallel fetch) | Task 4 |
| Section 3: Cookbook — Example 2 (read/transform/write) | Task 4 |
| Section 3: Cookbook — Example 3 (named worker actor) | Task 4 |
| Section 3: Cookbook — Example 4 (SSE streaming) | Task 4 |
| Section 3: Cookbook — Example 5 (supervised actor) | Task 4 |
| Wire lispref: `@include`, node chain fixes | Task 5 |
| internals chapter header, @menu, Sections 1–4 | Task 6 |
| internals Sections 5–9 (coro_swap, specpdl, GC, HTTP, guide) | Task 7 |
| Wire internals: `@include`, node chain fix | Task 8 |

All requirements covered. No gaps.

**Placeholder scan:** No TBDs, TODOs, or vague instructions found. Every step contains exact file content or exact edit descriptions.

**Type/name consistency:** `actor-spawn` signature (`name function &rest args`) is consistent across Task 2 `@defun` and all cookbook examples. `http-get` return value `(STATUS HEADERS BODY)` described in Task 3 matches cookbook usage (`nth 0 resp`, `nth 2 resp`) in Task 4. `sse-extract-text` named consistently throughout.
