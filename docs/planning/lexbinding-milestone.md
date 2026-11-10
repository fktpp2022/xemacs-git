# Lexical binding milestone: phase-by-phase implementation spec

Status: accepted in wayfinder [#139](https://github.com/fktpp2022/xemacs-git/issues/139),
strategy recorded in [ADR-0005](../adr/0005-lexical-binding-staged-commitment.md). Feeds
map [#128](https://github.com/fktpp2022/xemacs-git/issues/128) "give XEmacs byte-compiled
lexical binding". Phasing and landing policy decided in
[#137](https://github.com/fktpp2022/xemacs-git/issues/137); the go/no-go rule and its risk
gates in [#138](https://github.com/fktpp2022/xemacs-git/issues/138); the interpreted-path
environment shape in [#144](https://github.com/fktpp2022/xemacs-git/issues/144).

## Success test

The **twelve** `lisp/` files that already declare the `-*- lexical-binding: t -*-` header —
`tabulated-list.el`, `macroexp.el`, `subr-x.el`, `cl-lib.el`, `bind-key.el`,
`use-package.el`, `use-package-core.el`, `use-package-bind-key.el`,
`use-package-delight.el`, `use-package-diminish.el`, `use-package-ensure.el`,
`use-package-xemacs.el` — plus the upstream packages **`ctable`**, **`fix-word`** and
**`memoize`**, run **stock and byte-compiled**. Nothing is left to decide before someone
writes C.

Reference implementation (cite, do not merge): branch
[`prototype/lexbind-capture-spike`](https://github.com/fktpp2022/xemacs-git/tree/prototype/lexbind-capture-spike)
([#136](https://github.com/fktpp2022/xemacs-git/issues/136), ~450 insertions across 12
files). Evidence base: `docs/research/lexbind-gnu-semantics.md`,
`docs/research/lexbind-cconv-lift.md`, `docs/research/lexbind-c-surface.md`,
`docs/research/lexbind-reliance-audit.md`, `docs/research/lexical-binding.md` (its
`src/`/`lisp/` line references still check out; read its Correction notice),
`docs/research/lexbind-specpdl-coroutines.md` (still on its throwaway branch; landing it
is a #141 follow-up).

## Subset (the floor, from [#133](https://github.com/fktpp2022/xemacs-git/issues/133))

**Runtime variable capture, including escaping closures, with mutable cells — and nothing
else.** Local function bindings are *not* needed (nothing in the acceptance set calls
`cl-labels`; `bind-key`'s lone `cl-flet` is served by the existing dynamic shim); the
expansion-time environment is empty across the whole set; six of the twelve headers
declare the feature and need none of it. GNU-complete means Tier R
(`docs/research/lexbind-gnu-semantics.md` Bottom line): special-variable exemption via
`defvar`/`defconst`, lexical `let`/arg bindings, escaping closures, mutable capture,
`(eval FORM t)`, `special-variable-p`. P1 ships exactly this, interpreted; P2 adds Tier C
(cconv + VM); Tier M (`&environment`) stays in the fog until a route needs it — the
acceptance set needs none of it.

## P1 — interpreted GNU-complete subset, opt-in per file

One **atomic changeset** ([#144](https://github.com/fktpp2022/xemacs-git/issues/144) Q3):
closures + `macroexpand (&optional ENV)` + `macro-function` env-awareness + ALL
closure-tag consumers, in one PR. Any split recreates the measured half-activation state
([#136](https://github.com/fktpp2022/xemacs-git/issues/136) finding 1: defun-closures
without env-aware macroexpansion corrupt dynamic code — subr-x's `dolist` misresolves,
update-elc bootstrap crashes).

Environment shape — GNU's, not the spike's
([#144](https://github.com/fktpp2022/xemacs-git/issues/144) Q1):
`Vinternal_interpreter_environment` = `(t)` plus `(sym . value)` binding conses; `setq`
mutates the cons cdr (`XSETCDR`); bare symbols are `(defvar foo)` proclamations. Replace
the spike's `(sym . (value . t))` wrapper cells. Closure docstrings: GNU style, inside the
body (Q4). The env is specbound for lexical extents, so coroutines need **zero changes**
([#143](https://github.com/fktpp2022/xemacs-git/issues/143): `specpdl_unwind_range` /
`specpdl_rebind_range` / `specpdl_swap_one` handle a lexical `let` spanning a yield; GC
already marks the entries).

Work items, with touch points (spike diff is the reference for each):

1. **Loader header recognition** — `src/lread.c`: byte-scan the first 512 bytes for
   `lexical-binding: t` (spike patch at `src/lread.c`, +55 lines); specbind
   `internal-interpreter-environment` to `(t)` for the file's extent, behind the
   `load-lexical-files` whitelist. The cookie reader sits near the `;ELC` handling
   (`src/lread.c:700-707`) and the `.elc:.el:` search order (`src/lread.c:566-569`).
2. **`load-lexical-files`** — new user option (default nil) whitelisting file base names;
   documented XEmacs-only deviation ([#144](https://github.com/fktpp2022/xemacs-git/issues/144)
   Q2). P1 whitelist: **all twelve** lexbound files. The three preloaded ones MUST be on
   it before bootstrap can pass (see Fog fact 1 below).
3. **Interpreted closures** — `src/eval.c`: `Ffunction` (`src/eval.c:1305`), `Fdefun`
   (`:1346`), `Fdefmacro` (`:1360`) wrap under an active env → `(closure ENV ARGLIST . BODY)`;
   `funcall_lambda` (`:4466-4509`) gains the Qclosure branch, binding args into fresh cells
   extending the captured env; `Feval` (`:3729`) and `apply`/`funcall` dispatch
   (`Qlambda || Qclosure`) learn the tag. GCPRO the `Flet` varlist once the branch conses
   — the spike's one real segfault, and the exemplar of the allowed bug class.
4. **Lexical `let`/`let*`** — `src/eval.c:1060` (`FletX`) / `:1103` (`Flet`): bind into
   env conses, specbind the env around the body. Special-variable exemption: raw `flags`
   word in `Lisp_Symbol` (was 4 bytes of alignment padding — `src/lisp.h:2846-2902`; zero
   bytes, no dump-format change), set by `Fdefvar`/`Fdefconst`/`defvar_magic`
   (`src/eval.c`, `src/symbols.c`); `special-variable-p` subr; a special `let` specbinds
   dynamically. Never ride the value-cell magic (`docs/research/lexbind-c-surface.md` §3).
5. **Env-aware macroexpansion** — `Fmacroexpand` + `macro-function` thread
   `Vinternal_interpreter_environment` (`src/eval.c`). This is the half the spike missed
   first: `Fmacroexpand` consulted only the shadow alist, so `(macro closure …)` expander
   lookup misresolved (the measured `when` → `(if nil (progn))` failure).
6. **Closure-tag consumers** (all of them, per #144):
   `make-docfile` (the doc snarfer's `not lambda or autoload` case is fatal at dump time —
   spike patched `lisp/make-docfile.el`); documentation/help/Verify-documentation;
   `functionp`/`function_argcount`; printer and `equal` come free with GNU's list shape.
7. **C-zero trap** — `Lisp_Type_Record == 0`, so `CONSP` on an uninitialized
   `DEFVAR_LISP` global faults; every pre-`syms_of_eval` reader needs an explicit zero
   test (spike macro `LEXBIND_ENV_ACTIVE_P`), and `DEFVAR_LISP` does not initialize —
   keep the explicit `= Qnil` convention.
8. **Compile refusal stays** — `lisp/bytecomp.el` errors on a `lexical-binding: t` file
   ("no closure conversion yet") instead of silently dynamic-binding it; unchanged until
   P2. `byte-compile-bound-variables` (`lisp/bytecomp.el:417-420`) stays warning-only.

**Known open root cause, carried not blocking**: macroexpansion *inside a running
coroutine* under an active env misbinds the expander
([#143](https://github.com/fktpp2022/xemacs-git/issues/143) finding 2) — the coroutine
face of half-activation; investigate at P1 acceptance.

### P1 gate checklist (fires [#138](https://github.com/fktpp2022/xemacs-git/issues/138))

- [ ] Full dump green (recipe in #136 resolution: `temacs` + `xemacs` build, remove
      `.dmp`, `loadup.el` dump, image reloads the dumped closures).
- [ ] update-elc bootstrap green (`lisp/update-elc-2.el:53-64`, `:96`).
- [ ] `tests/elpa` fixtures green.
- [ ] `ctest -L gui` probes green (GUI probes: `docs/agents/gui-xemacs-probes.md`).
- [ ] All twelve whitelisted files load and run interpreted from the dumped image,
      including the three dumped ones (macroexp, tabulated-list, use-package-xemacs).
- [ ] Every bug found was in the proven class (GC-protection). **If P1 bootstrap fails
      repeatedly on the same out-of-class bug → trip-wire (i)**: re-decide stay-deferred
      at the P2 gate only.

## P2 — cconv rewrite, compiler env slot, `;ELC` bump

**Decision point: commit to P2 only if P1's gates stayed green**
([#138](https://github.com/fktpp2022/xemacs-git/issues/138)).

Work items:

1. **cconv rewrite against XEmacs's compiler** — a rewrite, not a lift
   ([#131](https://github.com/fktpp2022/xemacs-git/issues/131)): GNU's algorithm, XEmacs's
   shapes. The funnel is `byte-compile-variable-ref` (`lisp/bytecomp.el:3347-3409`),
   emitting `byte-varref`/`byte-varset`/`byte-varbind` (`:958-960`) via dispatch at `:3215`,
   `:4122`, `:4522`, `:4543`; `byte-compile-lambda` (`:2938-3045`) currently hands the
   arglist verbatim to `make-byte-code` with no captured-var analysis; the constants vector
   (`:3046-3074`, `byte-constant-limit` 64 at `:1149-1150`) interleaves vars and constants.
   **Trip-wire (ii): if the rewrite floor exceeds 2× the interpreted floor (~350–450
   lines), re-decide stay-deferred — at the P2 gate only.**
2. **Frame cells, not GNU's stack-as-frame** — XEmacs's value stack is a per-call `alloca`
   with no frame base (`src/bytecode.c:767-838`); keep `specbind`-style binding and carry
   captured cells in the env instead. Arg pre-binding stays off `SPECBIND` only where a
   closure's args need cells (`src/eval.c:3658-3691` for the shape today).
3. **Compiler env slot** — `environment` slot on `Lisp_Compiled_Function`
   (`src/bytecode.h:51-78`, 72 bytes now); one `{ XD_LISP_OBJECT, offsetof … }` row in
   `compiled_function_description` (`src/bytecode.c:2537-2547`) auto-marks and auto-dumps;
   zero `src/dumper.c` edits (`docs/research/lexbind-c-surface.md` §1).
4. **Opcodes** — `Blexref`/`Blexset` in the free gap before `Bconstant`
   (spike: 0270/0271, `src/bytecode-ops.h`, `src/bytecode.c:943-1053` for the dispatch
   neighbourhood); each new opcode needs an operand-size case in `optimize_byte_code` or
   the optimizer mis-aligns the stream (#136 finding 4).
5. **`make-byte-code` env argument** — optional 7th arg; `Fmake_byte_code` arity bump and
   `.elc` reader bound (`src/lread.c` `read_compiled_function`, `src/lread.c:3193-3275`
   region) per the no-backcompat decision — old ≤6-element `#[…]` still load.
6. **`;ELC` bump + full rebuild** — bump `byte-compile-version` (`lisp/bytecomp.el:13`)
   and the cookie check (`src/lread.c:700-707`). Exactly here, the only layout-changing
   phase; the cookie mismatch is the stale-`.elc` guard; no `pdump_header` change
   ([#137](https://github.com/fktpp2022/xemacs-git/issues/137)). Landing checklist
   includes a from-clean-configure rebuild.
7. **Benchmark ticket inside P2, before the `;ELC` bump ships**
   ([#138](https://github.com/fktpp2022/xemacs-git/issues/138) performance clause);
   go-if-within-budget is a P2-gate check, not an upfront rule.
8. **Retire the whitelist** — `load-lexical-files` sunset fires here: once the real
   compiler converts lexbound files, the cookie alone activates (GNU behaviour). Keep the
   variable one release as a no-op alias for diagnostics, then remove.
   ([#144](https://github.com/fktpp2022/xemacs-git/issues/144) Q2 sunset trigger: **cconv
   landing**, not P3.)
9. **Consumer updates** — `compiled_function_equal` (closures over different envs: not
   equal), `print_compiled_function` (`#[…]` readably), `compiled-function-annotation`
   path in `define-function` (`src/symbols.c:342`) still works (env slot orthogonal).

### P2 gate checklist

- [ ] P1 gates all stayed green through P2 development (proven bug class only; **repeated
      out-of-class bootstrap failure → trip-wire (i)**).
- [ ] Clean-configure rebuild green (from `rm -rf build && cmake …`, not incremental).
- [ ] Full dump green on the bumped format; dumped compiled closures survive dump/reload.
- [ ] update-elc bootstrap green after the `;ELC` bump — every tree `.elc` rebuilt.
- [ ] `tests/elpa` fixtures green, byte-compiled path included.
- [ ] `ctest -L gui` probes green.
- [ ] Benchmark run and within budget (budget set by the benchmark ticket, against the
      stock dynamic build as baseline).
- [ ] cconv floor ≤ 2× interpreted floor. **Above it → trip-wire (ii)**.

## P3 — acceptance-set conversion + vendor enablement

**Decision point: commit to P3 only if P2's gates stayed green.**

Work items:

1. **MELPA fixtures** — `ctable`, `fix-word`, `memoize` run stock and byte-compiled
   (`tests/automated/elpa-tests.el`; upstream pins in
   `docs/research/lexbind-reliance-audit.md`: ctable @ `48b7374`, fix-word @ `e29889f`,
   memoize @ `51b0759`). `lisp/elpa.el:1007-1022` compiles on install — the compiled path
   is exercised by every install.
2. **`tabulated-list.el` sorter reverts to upstream** — restore the closure-based sorter,
   dropping the two specials `tabulated-list--sort-column-index` /
   `tabulated-list--sorter` (`lisp/tabulated-list.el:79-117`) installed by PR #74. Adds
   two escaping closures, no new category
   ([#133](https://github.com/fktpp2022/xemacs-git/issues/133)). Deferred to P3, not P2:
   only once compiled lexbound `.elc` works.
3. **The twelve vendored files verified** stock and byte-compiled (success test below).
4. Core XEmacs lisp is **not** converted (standing rule).

### P3 gate checklist

- [ ] All prior gates still green.
- [ ] Success test passes: twelve `lisp/` files + `ctable` + `fix-word` + `memoize`, stock
      **and** byte-compiled, fresh XEmacs process.
- [ ] `ctest -L gui` probes green; dump + bootstrap green (dumped lexbound files compiled).

## Deliberately unfrozen (fog, not decisions)

- What `lexical-let` (`lisp/cl-macs.el:1705-1745`) becomes once real binding exists —
  deprecated, kept, or rewritten. `lisp/subr.el`, `lisp/pcase.el` depend on it today.
- Tier M `&environment`, `macroexpand-all-environment`, `symbol-macrolet` — deferred to
  cconv-era work; the acceptance set needs none of it (#133).
- Debugger surface for lexical vars (minimum ~2-5 lines in `src/eval.c` + Lisp env walk;
  `backtrace_specials`, `src/eval.c:7096-7120`) — measured in #143, scheduled nowhere.
- Coroutine-context macroexpansion misbinding (#143 finding 2) — root cause open.
- Manual surface: `man/lispref/variables.texi:718`, `man/internals/internals.texi` still
  describe the gap.

## Fog facts resolved by this spec (from [#144](https://github.com/fktpp2022/xemacs-git/issues/144))

1. **Lexbind-marked preloaded files at bootstrap**: exactly three — `"macroexp"`,
   `"tabulated-list"`, `"use-package-xemacs"` (`lisp/dumped-lisp.el:309-311`), loaded by
   `loadup.el`'s `mapc #'load preloaded-file-list` (`lisp/loadup.el:113-119`) under
   `load-warn-when-source-only t` (`lisp/loadup.el:104-105`). They must be on the P1
   whitelist (or, post-sunset, carry only the cookie) **before** any bootstrap run, or the
   dump trips on half-activated source loads.
2. **defalias/Ffset routing**: XEmacs's `defalias` is an alias for `define-function`
   (`src/symbols.c:3539`), which calls `Ffset` directly (`src/symbols.c:342-360`) — it
   does **not** route through `Ffunction`. This already matches GNU's split: `fset` /
   `defalias` store raw and never wrap; only `Ffunction` / `Fdefun` / `Fdefmacro`
   (`src/eval.c:1305`, `:1346`, `:1360`) are closure producers. So no `Ffunction`-equivalent
   wrapping is needed in the fset path; the closure-tag consumers in P1 item 6 are the
   complete producer-side surface. `define-function`'s compiled-function annotation slot
   (`set_compiled_function_annotation`) is orthogonal to the env slot.
