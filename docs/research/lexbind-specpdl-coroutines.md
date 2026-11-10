# Lexical binding vs. specpdl-based tooling: coroutines, debugger, `internal-<>`

Measurement for wayfinder [#143](https://github.com/fktpp2022/xemacs-git/issues/143)
(part of map #128), against branch `xemacs-cmake` plus the spike
(`prototype/lexbind-capture-spike`, #136). Input to the milestone spec (#139).

## Bottom line

1. **Coroutines: the spike's env-as-specbind design survives swapping for free.**
   A lexical `let` compiles to a plain `specbind (Qinternal_interpreter_environment, …)`
   (spike `src/eval.c:1113-1123, 1192-1212`) — an ordinary `func == NULL` specpdl entry —
   and every stage of the coroutine swap walks exactly those entries:
   `specpdl_unwind_range` on yield (`src/async-scheduler.c:515-516`),
   `specpdl_rebind_range` on resume (`:696-697`), both via `specpdl_swap_one` (`:147-162`).
   **Zero coroutine code needs to change.** No `Vcondition_handlers`-style special case is
   needed (`coro_detach_condition_handlers`, `:196+` — the env is a symbol binding, not a
   cons chain threaded through a global). GC is already covered: suspended coroutines mark
   every `p->symbol` / `p->old_value` (`:929-940`), which roots the env list and its heap
   value cells. A lexical `let` spanning a yield therefore unwinds/rebinds like any
   dynamic binding — measured live (Evidence section).

2. **Two semantic edges, neither a code change, both worth a spec sentence:**
   - *Inherited-range env bindings are never re-applied.* `specpdl_rebind_range` starts at
     `coro_inherit_depth` (`:696`), so entries memcpy'd at spawn (`:395-412`) rely on the
     symbol value simply still holding them. That invariant holds because unwind/rebind of
     own-range entries is balanced, but it means env correctness depends on every coroutine
     spawned from the same context sharing that context's env — true for the loader
     cookie's file-extent specbind (`spike src/lread.c:695-732`), and a new coro spawned
     from *inside* a running coroutine deliberately inherits the **scheduler's** specpdl
     (`:402-406`), so the spawner's lexical `let` env correctly does *not* leak into the
     child — the desired semantics, arrived at by accident, not design.
   - *Closure-created-in-A-invoked-in-B* carries the env in the closure **object**
     (interpreted: `(closure ENV …)` list, spike `src/eval.c:1384-1388`; compiled:
     `Lisp_Compiled_Function.environment`, spike `src/bytecode.h:82-84`), and
     `funcall_lambda` re-specbinds it for the duration of the call
     (spike `src/eval.c:4631`) — so the mechanism is coroutine-safe. Measured caveat:
     closures created via bare `lambda` forms *inside* a coroutine are malformed by the
     macroexpansion bug (Evidence section); `#'lambda`/compiled closures are unaffected.

3. **Alternative design (env as explicit `Feval`/`funcall` parameter, heap-only):**
   coroutine exposure drops to **zero** — the env never touches specpdl, so
   `specpdl_swap_one`/unwind/rebind, the GC mark loop, and `coro_inherit_depth` all stay
   untouched. Cost moves to the ~8 `Feval` function-pointer call sites
   (`src/lread.c:719,1469,1526`, `src/cmdloop.c:246`, per map #128 notes) — the same
   `Feval`-env-parameter item #132 already priced as "costliest single item". The spike's
   specbind shape is the cheaper one and, per (1), costs coroutines nothing; keep it.

4. **edebug: absent, and the debugger surface cannot see lexical vars today.**
   - No `edebug.el` in `lisp/` (verified: no file matches; only a dangling menu item at
     `lisp/lisp-mode.el:115` referencing `edebug-defun`, and edebug spec props in
     `lisp/subr.el:1111`, `lisp/lisp-mode.el:706`).
   - `struct backtrace` (`src/backtrace.h:52-66`) records `function`, `args`, `pdlcount`,
     `debug_on_exit` — no environment, no variable info.
   - `do_debug_on_call` (`src/eval.c:607`) / `do_debug_on_exit` (`:585`) pass only
     `Qexit`/value or a mode symbol to `call_debugger`; `Fbacktrace_frame` (`:7262`)
     returns `(t FUNCTION ARGS…)` — function and args only.
   - `Fbacktrace` DETAILED (`:7129`) prints specpdl bindings as symbol *names* only,
     via `backtrace_specials` (`:7096-7120`: `Fprin1 (specpdl[…].symbol)` — never
     `old_value`). Under the spike a lexical `let` shows as
     `# bind (internal-interpreter-environment)` — it reveals that *something* lexical is
     bound, but no variables or values.
   - **Minimal change for a debugger:** zero C change is strictly required — a Lisp
     debugger can read the env alist from the symbol value at the right pdl depth (env
     entries are ordinary specbinds). Practical minimum: make `backtrace_specials` print
     `old_value` too (one line, `:7108`), and/or have `Fbacktrace_frame` append the
     frame's env. edebug itself would additionally need `funcall_lambda`'s env alist
     (closure arglist + captured cells) surfaced per frame — a Lisp-side walk once the
     env is reachable.

5. **`internal-<>` / `cl-iterate`: absent.** No `DEFUN ("internal-…` anywhere in `src/`
   (verified by grep); no `cl-iterate*` files in `lisp/`; no specpdl entry-kind enum in
   this fork's `struct specbinding` (one `symbol`/`old_value`/`func` shape, so GNU's
   per-kind `SPECPDL_*` machinery — which is what `internal-<>` leaned on — has no
   counterpart to break). Nothing to measure; nothing to protect. If such tooling is ever
   ported it needs the same one thing edebug needs: a way to enumerate an env alist.

## Evidence: live experiments on the spike image

Spike image rebuilt from scratch in a throwaway worktree and driven through
`temacs -batch -l loadup` with a whitelisted test file (loadup's
`load-lexical-files` whitelist; run recipe per #136). All results below are
reproducible from the same inputs.

### What works

- **Lexical `let` spanning a yield.** A coroutine `(let ((n 40)) …)` that yields mid-body
  and resumes: the code after the yield still sees `n`, and a closure created before the
  yield still prints its captured env `((n 40 . t) t)` after the resume — i.e. the env
  specbind was correctly swapped out by `specpdl_unwind_range` at yield and re-applied by
  `specpdl_rebind_range` at resume (`src/async-scheduler.c:515-516, 696-697`).
- **Closure creation inside a coroutine.** `#'lambda` inside a coroutine `let` produces a
  correct `(closure ((n 40 . t) t) (y) (+ y n))` and calls to 42.
- **Dynamic, `&rest`, and mixed plain+`&rest` arg binding inside coroutines**, with and
  without an active env: all correct (`collect-rest` → `(1 2 3)`, `(lambda (a &rest bs) …)`
  → `(t ((41)))`).
- **Macros inside coroutines without a lexical env** (non-whitelisted file): correct
  (`(when t 99)` → 99).
- **Macroexpansion at top level under an active env**: correct —
  `(macroexpand '(when t (+ n 1)))` → `(if t (progn (+ n 1)))`.

### What breaks (one finding, precisely located)

**Macroexpansion whose expansion happens *inside a running coroutine* while the file's
lexical env is active.** Reproduced three ways:

- `(macroexpand '(when t (+ n 1)))` inside a coroutine → `(if nil (progn))` — the
  expander's `cond` and `body` both read nil. Top level, same env → correct expansion.
- Direct `funcall` of `when`'s expander object inside a coroutine → nil (so it is not
  Feval's macro-dispatch path; the expander body itself misbehaves).
- A bare `(lambda (y) (+ y n))` inside a coroutine → `Ffunction` receives
  `args = ((lambda))` — the `lambda` macro (subr.el:45, `` `(function (lambda ,@cdr)) ``)
  expanded with `cdr` nil, producing a malformed `(closure ENV)` with no arglist/body,
  which faults at `funcall_lambda`'s `arglist = XCAR (XCDR (tail))` assert
  (spike `src/eval.c:4601`, `RECORD_TYPEP (obj, lrecord_type_cons)`). Confirmed under
  gdb: the closure object at the crash is literally `(closure ((n 40 . t) t))`.

The sharp-quote path (`#'lambda`, i.e. `Ffunction` reached directly, without the `lambda`
macro) is unaffected — same coroutine, same env, correct closure and correct call. So the
failure is in **macro-expander execution inside coroutine context under an active env**,
not in the specpdl swap, not in closure application, and not in binding (plain/`&rest`
binds in coroutines are correct). It is the coroutine-context face of #136's headline
finding — *half-activation without env-aware macroexpansion corrupts code* — and it does
not require a swap to trigger. Root cause beyond that (why an expander's backquote-built
body misbinds only under `current_coro != NULL`) is left open below.

### Secondary gap re-confirmed

Plain `defun` arguments still bind **dynamically** in the spike (spike
`src/eval.c` `funcall_lambda` normal path: `specbind (symbol, args[i++])` for every arg),
so `(defun make-adder (n) (lambda (x) (+ x n)))` called at top level captures an env
without `n` and the body errors `void-variable n`. Coroutine-independent; cconv will
erase it.

## Shape estimate

- Required edits for coroutines: **none** (0 lines). The swap machinery, GC marking, and
  `coro_inherit_depth` already handle env entries generically.
- Debugger minimum: ~2-5 lines in `src/eval.c` (`backtrace_specials` value printing,
  optional `Fbacktrace_frame` env return) + whatever edebug-the-package costs when ported.
- `internal-<>`: 0 lines; re-measure only if ported.

## Open unknowns

- Why macro-expander bodies misbind only when `current_coro != NULL` and the env is
  active (expander args read nil) — reproducible, gdb-confirmed at the `Ffunction`
  boundary, but the exact mechanism inside the backquote/expander execution was not
  traced to an instruction. Needs the env-aware macroexpansion work (#137) anyway; if
  that work makes expanders env-correct, re-test this repro before shipping Phase 2.
- Coroutine death does not obviously unwind own-range specbinds before `xfree`
  (`src/async-scheduler.c:1154`); whether a dying coroutine's bindings leak into the
  scheduler was not measured (the closing of #142-class concerns may already cover it).
