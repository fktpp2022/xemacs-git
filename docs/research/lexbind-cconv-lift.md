# Distance between GNU's closure-conversion path and XEmacs's byte compiler

Research findings for wayfinder ticket [#131](https://github.com/fktpp2022/xemacs-git/issues/131), under
map [#128](https://github.com/fktpp2022/xemacs-git/issues/128) "give XEmacs byte-compiled lexical binding".
This is a **distance measurement**, not a plan: it sizes an implementation that ports GNU's *architecture*
(a separate closure-conversion pass, frame-relative stack-slot access, environment carried in the compiled
function) rather than lifting GNU's code. It ends with a cost shape.

Re-landed onto the mainline under wayfinder ticket [#141](https://github.com/fktpp2022/xemacs-git/issues/141); it previously lived only on the throwaway branch `research/lexbind-cconv-lift` (commit `2069ffebc`).

Correction notice: prototype ticket [#136](https://github.com/fktpp2022/xemacs-git/issues/136) confirmed this doc's cost shapes end to end, but measured that **half-activation fails** — interpreted defun-closures without env-aware macroexpansion corrupt dynamic code — so the route ships as one atomic changeset (closures + `macroexpand (&optional ENV)` + closure-tag consumers, ticket [#144](https://github.com/fktpp2022/xemacs-git/issues/144)), and the interpreted path follows GNU's `internal-interpreter-environment` shape rather than the spike's wrapper cells.

Scope note: this report deliberately does **not** re-derive the general feasibility picture. That lives in
the prior report
[`docs/research/lexical-binding.md`](lexical-binding.md)
(ticket #15, re-landed on `xemacs-cmake` by [#129](https://github.com/fktpp2022/xemacs-git/issues/129)), which establishes that XEmacs has no lexical binding at
any layer and that the byte compiler — not the interpreter — is the hard part. Here we assume that result
and go one level down, into the closure-conversion machinery specifically.

## Bottom line

- **XEmacs has no closure-conversion pass and no lexical variable model in the compiler.** What
  `bytecomp.el` calls "the current lexical environment" (`byte-compile-bound-variables`,
  `lisp/bytecomp.el:417-420`) is a flat, warning-only alist; every variable reference still compiles to a
  dynamic symbol-cell access. There is nothing to "extend" toward lexical access — the data it would need
  (a nested, frame-indexed environment) is exactly what is absent.
- **The VM cannot express frame-relative access at all.** XEmacs's value stack
  (`src/bytecode.c:767-838`) is a per-invocation `alloca` scratch array with no frame base and no way to
  address a slot by relative depth; bindings live on a separate global `specpdl`. GNU's `stack-ref` /
  `stack-set` are the opposite: the stack *is* the frame. This is the single largest architectural gap and
  it is structural, not a missing opcode.
- **`cconv.el` cannot be lifted as code.** It is written against GNU's `bytecomp` internals — the
  `internal-` byte-code representation, `byte-compile-*` globals, GNU's opcode set, and its own varinfo
  structure. It is ~705 lines of analysis tightly coupled to a compiler whose shape XEmacs's does not share.
  The *algorithm* transfers; the *code* does not. Realistically it is a rewrite of the analysis against
  XEmacs's compiler, reusing GNU only as a reference for what the passes must compute.
- **Carrying the environment is cheap at the C level and expensive at the format level.** Adding a slot to
  `Lisp_Compiled_Function` (`src/bytecode.h:51-80`) is mechanical because XEmacs's
  `DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT` auto-marks and auto-dumps `Lisp_Object` fields
  (`src/bytecode.c:3026-3034`). The expensive, release-scale parts are the `.elc` version bump
  (`;ELC` byte, `src/lread.c:700-707`; `byte-compile-version` `lisp/bytecomp.el:13`) against a fork that
  byte-compiles everything at build (`lisp/update-elc-2.el`) and at ELPA install (`lisp/elpa.el:1007-1022`),
  and rewriting arg pre-binding from `specbind` to stack slots (`src/eval.c:3658-3691`).
- `lexical-let` (`lisp/cl-macs.el`) is a **dead end** for this architecture: it gensym-renames one binding
  at a time at macro-expansion and never produces an environment, so it neither shares code with nor feeds a
  real closure converter. Its only reusable asset is the `lexical-let` *reader macro surface*, not machinery.

## 1. XEmacs's compiler state today

**`byte-compile-bound-variables` is a warning alist, not an environment.**
`lisp/bytecomp.el:417-420` documents it as "the current lexical environment … This list lives partly on the
specbind stack. The cdr of each cell is an integer bitmask." The bitmask has four flags, all about
*diagnostics*:

```
lisp/bytecomp.el:432  (defconst byte-compile-referenced-bit 1)
lisp/bytecomp.el:433  (defconst byte-compile-assigned-bit   2)
lisp/bytecomp.el:434  (defconst byte-compile-arglist-bit    4)
lisp/bytecomp.el:435  (defconst byte-compile-global-bit     8)
```

It is a **flat** list delimited by a `'new-scope` sentinel pushed per form (`:4509`, `:4535`, `:2949`,
`:4400`, `:4414`), scanned with `assq` for free-variable warnings (`:3385`) and unused-variable warnings
(`byte-compile-warn-about-unused-variables`, `:1718-1740`), and consulted to decide whether a free reference
is legal (`byte-compile-variable-ok-p` path, `:1700-1714`). Nothing here records *where* a variable lives,
*whether it is captured*, or *which enclosing frame* it belongs to. Because the sentinel is a symbol pushed
onto the same list, nested-scope identity is by list position, not by a first-class environment object. A
closure converter needs the latter; XEmacs has neither the representation nor the pass that would build it.

**Codegen is unconditionally dynamic.** `byte-compile-variable-ref` (`lisp/bytecomp.el:3347-3409`) is the
single funnel for reference, assignment and let-binding: after its warning logic it ends with

```
lisp/bytecomp.el:3405-3409
  (let ((tmp (assq var byte-compile-variables)))
    (or tmp (setq tmp (list var)
          byte-compile-variables (cons tmp byte-compile-variables)))
    (byte-compile-out base-op tmp))
```

`base-op` is one of `byte-varref` / `byte-varset` / `byte-varbind` (`:958`, `:959`, `:960`) and the operand
`tmp` is a **cell naming the symbol** in the constants vector. Dispatch sites: `byte-varref` for a value
context at `:3215`, `byte-varset` in `byte-compile-setq` at `:4122`, `byte-varbind` in `let` at `:4522` and
`let*` at `:4543`, each followed by `byte-unbind` (`:4532`, `:4549`). The comment at `:3344` ("kludge added
to XEmacs to work around the bogosities of a nonlexical lisp", defining `globally-boundp`) is the compiler
admitting it treats every non-constant symbol as a global cell. There is no lexical/special split: the same
`byte-varref` is emitted whether the variable is a `defvar` special or a `let` binding, which is precisely
the property GNU's `cconv` exists to remove.

**The lambda compiler does not touch the environment.** `byte-compile-lambda`
(`lisp/bytecomp.el:2938-3045`) rebinds `byte-compile-bound-variables` only to attach
`byte-compile-arglist-bit` cells for unused-argument warnings (`:2943-2949`), compiles the body with
`byte-compile-top-level` (`:3021`), then emits

```
lisp/bytecomp.el:3026-3035
  (apply 'make-byte-code
         (append (list arglist) (cdr compiled) ... doc ... interactive))
```

The arglist is handed verbatim to `make-byte-code` and the VM specbinds it; there is no analysis of which
arguments become stack slots, no captured-variable list, no environment argument. `byte-compile-byte-code-maker`
(`:2904-2933`) shows the same five-part shape when it re-serializes a lambda into a source `make-byte-code`
call: `(arglist instructions constants stack-depth [doc] [interactive])`.

**The constant pool has nowhere to hang an environment.** `byte-compile-constants-vector`
(`lisp/bytecomp.el:3046-3074`) interleaves *variables and constants in one vector*, ordered to keep the
access opcodes short: first 5 slots get the one-byte `byte-varref` codes, up to `byte-constant-limit` slots
get one-byte `byte-constant` codes, then two-byte and three-byte ranges (`:3050-3061`). `byte-constant-limit`
is `64` (`lisp/bytecomp.el:1149-1150`) and `byte-constant` reserves opcodes 192-255
(`lisp/bytecomp.el:1147-1148`). This layout is an optimization for symbol-indexed dynamic access. GNU's
approach is categorically different: it *prepends* the captured environment vector into the constant pool so
a closure can find its free variables through it (see the prior report's §Byte compiler, referencing GNU
`bytecomp.el:3352-3367`). XEmacs's frequency-sorted mixed vector is the wrong shape to graft an environment
into; adopting that convention means choosing a fixed environment position, which perturbs every one-byte
optimization above.

**The compiler's own TODOs concede the gap.** `lisp/bytecomp.el:209-210` ("Should implement declarations
and proclamations, notably special, unspecial, and ignore") is the missing prerequisite for a converter:
without special-variable proclamations, a pass cannot decide which names must stay dynamic. The
`free-vars` / `unused-vars` warning categories (`:339`, `:348`) are the only consumers of the
"environment" today.

## 2. GNU's `cconv.el`: what lifts and what does not

Reference: GNU Emacs 24.1 `lisp/emacs-lisp/cconv.el`, 705 lines
([mirror](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/cconv.el)); the
prior report already recorded its shape (analysis at `27-64`, `135-150`, `189-237`). I could not re-fetch it
here (no outbound network from this build host), so the characterisation below is from that pinned 24.1
mirror plus the prior report; treat the GNU line numbers as 24.1-tagged, not locally re-verified.

**What it computes** (the algorithm — this is what transfers):

- A **free-variable analysis** over the byte-code op list: which symbols referenced by `byte-varref`-style
  ops are bound lexically vs special vs global, and whether they are *assigned* (so a mutable, heap-allocated
  box is needed instead of a plain stack slot).
- **Lambda lifting / closure conversion**: for each `lambda`, the set of captured variables, and the rewrite
  of references to captured variables into accesses through the environment.
- **Mutable-capture handling via cons cells**: variables captured *and* `setq`-ed are boxed so several
  closures share one storage cell — the classic "dotbox" trick.
- The **inputs/outputs**: `cconv-closure-convert` takes the intermediate op list bytecomp has already
  produced (GNU's `internal-` byte-code form) and returns a rewritten op list, with closures emitted as
  `make-byte-code` objects whose constant vector carries the captured environment.

**How tightly it is bound to GNU's bytecomp** (the reason the code does not lift):

- It parses and emits GNU's **opcodes and operand encodings** (`byte-stack-ref`, `byte-discard1`,
  `byte-varbind`, `byte-call`, the `byte-opcodes`/`byte-opcodes-alist` tables), which differ from XEmacs's
  `byte-defop` table (`lisp/bytecomp.el:958-1148`). XEmacs's `Bvarref/Bvarset/Bvarbind` operand encoding is
  the `+5/+6/+7` extended-operand scheme (`src/bytecode.c:943-992`); GNU's is not.
- It walks GNU's **`internal-` byte-code representation** and GNU's `byte-compile-*` globals; XEmacs has no
  `internal-` function form (grep for `cconv`/`internal-`/`stack-ref` in `lisp/bytecomp.el`,
  `lisp/byte-optimize.el`, `src/bytecode.c` returns nothing).
- It depends on GNU's **varinfo/environment object model** (`cconv-env`, `cconv-var`, the lambda-env alist
  threaded through `cconv-convert`/`cconv-analyse-form`). XEmacs's analogue, `byte-compile-bound-variables`,
  is the flat warning alist from §1 — a different data type, so every traversal would need reworking.
- It assumes GNU's **`make-byte-code`/closure convention** (environment in the constant vector) and GNU's
  `special-variable-p` / `declared_special` bit, which XEmacs lacks (prior report §Symbol, §Evaluator).

Verdict: **rewrite the analysis against XEmacs's compiler; reuse GNU as the specification of what the pass
must compute.** Porting `cconv.el` verbatim would mean porting GNU's byte-code IR, opcode table and symbol
specialness machinery first — a larger job than writing an equivalent pass on XEmacs's own structures.

## 3. The VM: value stack vs frame-relative access

**XEmacs's value stack is scratch, not a frame.** In `execute_optimized_program`
(`src/bytecode.c:848-853`) the stack is a `alloca` array sized per call:

```
src/bytecode.c:861-862
  Lisp_Object *stack_beg = alloca_array (Lisp_Object, stack_depth + 1);
  REGISTER Lisp_Object *stack_ptr = stack_beg;
```

`PUSH`/`POP`/`TOP`/`DISCARD` (`:791-821`) all move `stack_ptr` within one invocation only; the comment at
`:769-784` is explicit that it "doesn't work quite like you'd expect" and that `stack_ptr` points at the top
value. There is **no frame base pointer**, no saved `stack_beg` reachable from a nested call, and no
addressing mode "slot N below the current top". `DISCARD(n)` (`:804-821`) is the one GNU-`discardN` analog
present, and `TOP_LVALUE` (`:833`) is writable, but only for the top slot — that is a `stack-set` of depth 0,
not arbitrary depth.

**Variable access goes through symbol cells, not the stack.** `do_varref` (`src/bytecode.c:951-958`) reads
`constants_data[n]` as a *symbol* and returns `XSYMBOL(symbol)->value`; `do_varset` (`:969-981`) writes the
symbol cell; `do_varbind` (`:992-1016`) saves the old cell value onto the **global `specpdl`**
(`specpdl_ptr->symbol/old_value`, `specpdl_depth_counter++`) — the dynamic-binding stack, unrelated to the
value stack. So `n` indexes a symbol name, not a storage location; the notion "the value of the variable in
the enclosing frame two levels up" is unrepresentable.

**Mapping to GNU's opcodes.** GNU's lexical access (`stack-ref N`, `stack-set N`, `stack-set2`, `discardN`)
presupposes that the *current function's frame is a fixed contiguous region of the value stack*, args and
lexical `let` variables pushed into known slots at entry, with `N` a depth-relative index. For XEmacs to run
that model it must:

1. introduce a frame-base concept reachable across nested calls (so a closure's captured slots survive and
   are addressable after the defining frame would have unwound — this is the hard part; it forces either
   heap-allocated frames or the cons-box captures GNU uses for escaping variables),
2. change arg/`let` binding from `specbind` to **pushing onto the value-stack frame** (see the `eval.c`
   pre-binding below), and
3. add depth-relative `stack-ref`/`stack-set` opcodes with their operand encoding.

The existing `varbind`/`unbind` design can be **co-existed-with** (specials keep using `specpdl`) but not
**extended** into lexical access: `varbind` fundamentally means "save the symbol cell on the specbinding
stack", which is the opposite of "reserve a stack slot". A lexical binding needs a new codegen path and new
VM ops; the symbol-cell path stays for specials. `funcall_compiled_function` (`src/eval.c:3641-3691`) shows
the pre-binding inversion directly — args are `SPECBIND_FAST_UNSAFE`'d onto `specpdl` after
`SPECPDL_RESERVE (f->specpdl_depth)`; making those args stack-frame slots is a core-change to call setup and
to `unwind_to`/non-local-exit bookkeeping (`src/bytecode.c:1050`, `:1139`).

**Opcode space.** `byte-constant` reserves opcodes 192-255 (`lisp/bytecomp.el:1147-1148`), leaving no
contiguous free single-byte block, so the new op family must either fit the existing extended-operand scheme
(`src/bytecode.c:943-992`) or restructure operand encoding — a bounded but nontrivial format change.

## 4. Carrying the environment in the compiled function

`Lisp_Compiled_Function` (`src/bytecode.h:51-80`) has a **fixed shape with no environment slot**:

```
src/bytecode.h:63-65
  Lisp_Object instructions;
  Lisp_Object constants;
  Lisp_Object arglist;
```
plus `Lisp_Object *args` (the unrolled arg array, `:68`), `min_args/max_args/args_in_array` (`:76-77`),
`doc_and_interactive` (`:79`), and the backward-compat slot enum `COMPILED_ARGLIST … COMPILED_DOMAIN`
(`:35-41`).

**What adding an environment slot costs — mostly mechanical, because of the declarative object system:**

- One `Lisp_Object environment;` field in the struct (`src/bytecode.h:51-80`) and a slot enum entry.
- Allocation/initialisation in `make_compiled_function` (`src/alloc.c:2120-2138`) and the
  `make-byte-code` DEFUN (`src/alloc.c:2141-2200`), whose signature is fixed at
  `(ARGLIST INSTRUCTIONS CONSTANTS STACK-DEPTH &optional DOC-STRING INTERACTIVE)`. GNU also carries the
  environment *through the constants vector* rather than as a separate `make-byte-code` argument, so the
  fork must decide: add a 7th arg (touching every caller and the doc-comment's five/six-arg compatibility
  warning at `src/alloc.c:2144-2148`), or follow GNU's constant-vector convention (§1) and only teach
  `make-byte-code` to interpret it.
- **GC marking and dumping are the easy part**: the record is declared with
  `DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT ("compiled-function", compiled_function, print_compiled_function,
  …)` (`src/bytecode.c:3026-3034`) with `OBJECT_HAS_METHOD … print_preprocess` /
  `nsubst_structures_descend`, so new `Lisp_Object` fields are marked and traversed automatically — no
  hand-written marker.
- **Printing** (`print_compiled_function`, `src/bytecode.c:2369+`) emits slot-by-slot (arglist, instructions,
  constants, …); a new environment slot needs a display branch so `prind1`/`#[…]` read-back stays sane.
- **Load/unserialize** (`src/lread.c:3257` rebuilds via `Fmake_byte_code`; `src/lread.c:452-468` fixes up an
  old-style compiled function) must agree with whatever representation is chosen.

**`make-byte-code` on each side.** XEmacs: 4-6 args, no environment (`src/alloc.c:2141-2170`), produced by
the compiler at `lisp/bytecomp.el:3026` and `:2916`. GNU 24.1: same `make-byte-code` name, but closures carry
the captured environment vector (prior report §Byte compiler, GNU `bytecomp.el:3352-3367`). The structural
addition (a place to put the environment) is small; the *semantic* work — making the VM set the environment
at call time and free it correctly on unwind — is where the cost sits.

## 5. The `.elc` format and the version bump

- **Cookie.** `.elc` files open with `;ELC` plus a **single version byte**: the compiler writes
  `(insert ";ELC" 20 "\000\000\000\n")` (`lisp/bytecomp.el:2248-2251`), and the loader reads 8 bytes, checks
  `strncmp(elc_header, ";ELC", 4)`, and takes the version from `elc_header[4]`, defaulting to 100 for
  source-like or pre-cookie files and erroring if `< 20` (`src/lread.c:698-717`). So the format already has a
  one-byte version channel that a change of closure/environment representation must move.
- **Compiler version.** `byte-compile-version` is pinned to `"2.28 XEmacs; 2009-08-09."`
  (`lisp/bytecomp.el:13`) and echoed into the file preface (`:2290`); the header also embeds a
  `byte-compile-checks-on-load` guard form (`:2252-2265`). Any lexical-binding output that changes the
  compiled-function shape or the meaning of `byte-varref` has to be gated so old `.elc` and new `.elc` are
  distinguishable at load, or all stale `.elc` must be discarded.
- **Why a bump is expensive here specifically.** This fork byte-compiles **everything at build**:
  `update-elc-2.el` calls `byte-compile-file` across the lisp tree (the compile loop around
  `lisp/update-elc-2.el:96`), and byte-compiles **on ELPA install**:
  `elpa--byte-compile-content` compiles every `.el` in a package, including generated autoloads, and *errors
  out* on failure (`lisp/elpa.el:1007-1022`). Consequences of a format/version change:
  1. every shipped and dumped `.elc` must be regenerated coherently with the new VM, and the static dump
     (`src/dumper.c`) that already bakes compiled functions has to match;
  2. third-party packages installed from ELPA get compiled by whatever `bytecomp` is loaded, so a compiler
     that emits new-style closures must be paired with a VM that can run them at the *same* install —
     you cannot ship the compiler ahead of the runtime;
  3. old `.elc` on disk must fail closed (`src/lread.c:712`) or fall back to source cleanly; the `20`/`< 20`
     channel is coarse (a single byte) and may need real sub-versioning.
  The bump itself is a one-line edit; the *coordination across build, dump, install, and stale files* is the
  release-scale cost.

## 6. The dead `(lexical-scope)` proposal in `byte-optimize.el`

`lisp/byte-optimize.el:131-151` preserves an early in-tree design note, still relevant as a warning list
rather than as reusable code:

- It proposes a **file-local `(lexical-scope)` proclamation**: "`the form (lexical-scope) in a file means
  that the file may be compiled lexically. This proclamation is file-local. Then, within that file, `let`
  would establish lexical bindings, and `let-dynamic` would do things the old way`" (`:136-139`). This is
  essentially GNU's eventual `-*- lexical-binding: t -*-` per-file flag — the note independently arrived at
  the two-dialects-per-file design.
- It flags the exact prerequisites GNU's `cconv` also had to solve: **notice `defvar`/`defconst` and keep
  them dynamic** (`:140-142`), and **know about variables not defvarred in the file being compiled**
  ("doing a boundp check isn't good enough", `:143-144`), which is why GNU added the
  `declared_special` bit / `special-variable-p` rather than a runtime `boundp`. This maps directly onto the
  missing declarations/proclamations TODO (`lisp/bytecomp.el:209-210`).
- It states the **fatal flaw of a compiler-only scheme**: "the interpreter and compiler would have different
  semantics for files compiled with (dynamic-scope) … there would be no way to modify the interpreter to
  obey this (unless the loader was hacked in some grody way)" (`:147-151`). GNU resolved this by making the
  *loader* set `lexical-binding` and threading it into the interpreter too (prior report §Loader); XEmacs
  inherits the same constraint — you cannot do this in the compiler alone.
- The `:156-158` paragraph ("Really the Right Thing is to make lexical scope the default … and FIX all the
  code") is the option GNU deliberately *rejected* in favour of per-file opt-in, and is retained here with
  the note that "RMS removed [it] from his version" (`:153-154`).

Reusable? **As a decision record, yes; as code, no** — it is a comment block, not an implementation, and it
describes a compiler-local hack that the note itself rejects. Its value is that it enumerates the same three
hard requirements (special-variable proclamations, non-defvarred-variable knowledge, interpreter/loader
agreement) that any port must satisfy.

## `lexical-let`: stepping stone or dead end

`lexical-let`/`lexical-let*` (`lisp/cl-macs.el:1707-1753`) and the underlying gensym machinery do
**per-binding, macro-level closure conversion**: each `lexical-let` variable is renamed to a fresh gensym and
closures capture it by expanding a `let` into explicit functional environments at *macro-expansion* time.
Verdict: **dead end for this architecture**, for three reasons:

1. **Wrong granularity and wrong mechanism.** It converts one binding at a time inside the macroexpander,
   producing source-level `((lambda …) …)` nests; it never builds a first-class environment object, never
   analyses free vs special vs global across a whole function body, and never touches the byte compiler. A
   `cconv` pass operates on the compiler's IR after optimization and must handle *all* `let`/`lambda` in a
   lexical-binding file, not just `lexical-let`. There is no shared code path.
2. **No interaction with the runtime model.** GNU's design makes the *VM* responsible for frames and the
   *compiled function* carry the environment; `lexical-let` bypasses both and cannot, because it is pure
   source macroexpansion. Its closures "will always run as interpreted code" (`lisp/files.el:919`,
   `find-file-create-switch-thunk` docstring, "because we don't have built-in lexical scope") — the opposite
   of the goal here.
3. **Different semantics.** It relies on gensym capture discipline that predates and differs from
   `lexical-binding` semantics; it cannot express mutual capture across `labels`, `setq` on captured vars,
   or the escaping-variable boxing `cconv` performs.

The only asset worth keeping is the existing `lexical-let` *user-facing macro surface* (for backward
compatibility once real lexical binding exists), not any of its internals.

## Cost shape (mechanical vs hard), not a plan

Grouped by how much judgement they require, with rough size:

| Piece | Nature | Rough size |
|---|---|---|
| Add `environment` slot to `Lisp_Compiled_Function` + slot enum | **Mechanical** (auto mark/dump via `DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT`) | small: struct + alloc + print branch (`src/bytecode.h`, `src/alloc.c:2120+`, `src/bytecode.c:2369+`) |
| Teach `make-byte-code` / lread to carry the environment (7th arg or constant-vector convention) | **Mechanical-to-semi-hard** — choice has format fallout | small edit, but ripples through `lisp/bytecomp.el:2916/3026`, `src/lread.c:3257` |
| Recognize `lexical-binding` per file; thread it from **loader into interpreter** (special bit, `let`, `setq`, `function`, `funcall_lambda`) | **Hard, prerequisite** (not this ticket's compiler focus, but blocks it) | medium-large — prior report §"What XEmacs would need" |
| **Special-variable proclamations** (`special-variable-p`, `declared_special`, `defvar`/`defconst` marking) — the TODO at `bytecomp.el:209-210` and the `byte-optimize.el:143-144` warning | **Hard** — needed *before* any converter can classify names | medium |
| **Closure-conversion pass** (free/mutated/escaping analysis, mutable-cons boxing) rewritten against XEmacs's compiler IR | **Hardest single deliverable**; GNU `cconv` is reference, not lift | large: GNU's is ~705 lines *plus* the IR/opcode coupling; expect comparable or more on XEmacs structures |
| **VM frame model**: replace arg/`let` `specbind` pre-binding with stack-frame slots; introduce frame-base reachable across nested calls; unwind/non-local-exit bookkeeping (`src/eval.c:3658-3691`, `src/bytecode.c:1050/1139`) | **Hard and structural** — the deepest gap; touches every call and exit path | large |
| New `stack-ref`/`stack-set`/`stack-set2` ops with operand encoding; `discardN` analog already exists as `DISCARD` (`src/bytecode.c:804`) | **Medium** — opcode space is tight (`byte-constant` owns 192-255) | small-medium, gated by format change |
| Constant-pool rework to give the environment a fixed position (`lisp/bytecomp.el:3046-3074`) | **Semi-hard** — interacts with the 5-var / 64-const one-byte optimizations | medium |
| `.elc` **version bump** + coordination across build (`update-elc-2.el`), ELPA install (`elpa.el:1007-1022`), static dump, stale files | **Mechanically trivial, operationally release-scale** | small edit; large rollout |

Summary of the distance: **the cheap parts are placing and plumbing an environment object; the expensive
parts are everything that assumes the compiler and VM can talk about *where* a variable lives.** XEmacs
today can only talk about *which symbol* a variable names (`byte-varref` → `constants_data[n]` → symbol cell).
GNU's closure conversion exists precisely to replace "which symbol" with "which frame slot / which captured
box", and that replacement is owned by two components XEmacs does not have: a first-class compiler
environment (vs the warning alist) and a stack-as-frame VM (vs `specpdl` + scratch stack). Porting the
*architecture* therefore means (a) building that environment in the compiler, (b) restructuring the VM's
binding model — both hard — while (c) the surrounding plumbing (struct slot, make-byte-code, printer, cookie
byte) is comparatively mechanical. The `.elc` rollout and interpreter/loader agreement are what make it
release-scale rather than a compiler patch.

## Evidence index

XEmacs (this fork, current tree, line numbers verified in worktree at HEAD `267afc46`):

- `lisp/bytecomp.el:13` `byte-compile-version`; `:209-210` declarations/proclamations TODO;
  `:417-420` `byte-compile-bound-variables`; `:432-435` bitmask bits; `:958-960,1147-1150` varref/varset/
  varbind + `byte-constant-limit`; `:1700-1740` warning consumers; `:2248-2265` `;ELC`+version-byte+load
  check; `:2904-2933` byte-code-maker; `:2938-3045` `byte-compile-lambda` + `make-byte-code`;
  `:3046-3074` `byte-compile-constants-vector`; `:3215` varref dispatch; `:3344` nonlexical kludge;
  `:3347-3409` `byte-compile-variable-ref`; `:4122` varset; `:4522/4543/4532/4549` let/let* varbind+unbind.
- `lisp/byte-optimize.el:131-158` `(lexical-scope)` proposal.
- `src/bytecode.c:767-838` value stack + PUSH/POP/TOP/DISCARD; `:848-863` `execute_optimized_program`;
  `:943-1016` `do_varref`/`do_varset`/`do_varbind`; `:2369+` `print_compiled_function`; `:3026-3034`
  `DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT`.
- `src/bytecode.h:35-41,51-80` compiled-function slots and struct.
- `src/alloc.c:2120-2138` `make_compiled_function`; `:2141-2200` `make-byte-code`/`Fmake_byte_code`.
- `src/eval.c:3641-3691` `funcall_compiled_function` specbind pre-binding.
- `src/lread.c:698-717` `;ELC` cookie + version byte; `:452-468`, `:3257` compiled-function load fixups.
- `lisp/update-elc-2.el:96` build-time compile loop; `lisp/elpa.el:1007-1022` install-time compile.
- `lisp/cl-macs.el:1707-1753` `lexical-let`/`lexical-let*` gensym closure conversion (macro-level);
  `lisp/files.el:919` "closures … always run as interpreted code".

GNU Emacs 24.1 (pinned mirror; not locally re-fetched — no network on build host):

- [`lisp/emacs-lisp/cconv.el`](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/cconv.el)
  (705 lines; analysis `27-64`, `135-150`, `189-237`);
  [`lisp/emacs-lisp/bytecomp.el`](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/bytecomp.el)
  (cconv call `2170-2180`, stack codegen `3055-3085`, closures/`make-byte-code` `3352-3367`,
  `special-variable-p` `3849-3854`);
  [`src/bytecode.c`](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/bytecode.c)
  (`stack-ref`/`stack-set`/`stack-set2`/`discardN` at `86-90`, `1761-1805`).

Prior work cited, not re-derived: `docs/research/lexical-binding.md` (ticket #15, now on the
mainline) — general feasibility, GNU 24.1 loader/evaluator/symbol detail, and the option
table this report narrows in on.
