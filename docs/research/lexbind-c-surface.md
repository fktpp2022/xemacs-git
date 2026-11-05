# The C-side runtime surface for closures and a special-variable bit

Research findings for wayfinder ticket [#132](https://github.com/fktpp2022/xemacs-git/issues/132),
part of the map
[#128](https://github.com/fktpp2022/xemacs-git/issues/128) "give XEmacs byte-compiled lexical binding".

Scope: the **runtime / interpreter** cost in `src/`. The byte-compiler and bytecode-VM half is
ticket [#131](https://github.com/fktpp2022/xemacs-git/issues/131); the subset decision is
[#134](https://github.com/fktpp2022/xemacs-git/issues/134). The touch points were first located by
the feasibility report on branch `origin/research/lexical-binding`
(`git show origin/research/lexical-binding:docs/research/lexical-binding.md`), which this document
prices and, where noted, corrects.

Line references are to this tree (`src/` unless stated). Numbers in "shape" lines are edits, not
hours. Unless qualified, every claim below was read out of the source, and the struct sizes / enum
values were measured by compiling a probe against `build/src/config.h` with the flags from
`build/compile_commands.json` (see "Measured facts").

## Bottom line

1. **You probably do not need a new object type at all.** The cheapest workable closure is a new
   `Lisp_Object environment` slot on `Lisp_Compiled_Function` (`src/bytecode.h:51-78`) plus
   list-shaped closures for interpreted code. That route touches **4 files, ~40-60 lines** and
   *zero* of the five pipelines the ticket lists. A genuinely distinct `Lisp_Closure` type costs
   **8 files, ~180-260 lines, ~40 audit sites** — and one of the believed five touch points
   (`pdump_object_table` row) turns out not to exist, while another (fixed-type allocator + sweeper)
   is optional.
2. **The special-variable bit is free.** `Lisp_Symbol` is 40 bytes with 4 bytes of padding between
   the 4-byte header word and `name`; an `unsigned int flags` word costs 0 bytes, does not change
   the dump layout size, and rides the portable dump image untouched (pdump writes
   `static_size` raw bytes and relocates only the *described* slots). Do **not** try to put the bit
   inside the header word: all 32 bits are spoken for, and `set_lheader_implementation`
   (`src/lrecord.h:204-211`) overwrites the aliased `free`/`uid` bits on every allocation.
3. **Specialness cannot be carried by the existing value-cell magic.** It would not distinguish a
   `defvar`'d global from an undeclared one, and it is destroyed by the three operations that
   replace the value cell (`make-variable-buffer-local`, `defvaralias`, `makunbound`/`set` to
   unbound). Buffer-locals are the reason to prefer the struct flag: the flag is orthogonal to the
   value cell, so `defvar`'d globals keep their buffer-local behaviour for free, and `Fdefvar`
   already writes only via `Fdefault_boundp`/`Fset_default` (`src/eval.c:1428-1438`).
4. **`Feval`'s missing environment parameter is the most expensive single item**, because `Feval`
   is not merely called 59 times — it is *passed as a function pointer* of shape
   `Lisp_Object (*)(Lisp_Object)` at 8 sites, including the loader's evaluator hook
   (`readevalloop (…, Feval, …)`, `src/lread.c:719,1469,1526`) and the top-level/command-loop error
   handlers (`src/cmdloop.c:246`). The cheap alternative — specbind a global
   `internal-interpreter-environment` — is ~30 lines and composes with throw/unbind/GC, but it
   cannot support escaping capture (specpdl entries are unwound at exit and the array is
   `realloc`'d), and it must be added to this fork's coroutine specpdl swapping.
5. **Dump is not the blocker.** Closures whose environment is a Lisp object (vector or cons) are
   dumpable by the existing generic machinery; nothing in `src/dumper.c` is type-specific. The three
   `lexical-binding: t` files on `preloaded-file-list` therefore need **not** come off the dump
   list, *provided* their closures are dumpable objects. Interpreter-only lexical binding without
   compiler/VM support instead forces those files to load as source at dump time — allowed, but it
   trips `load-warn-when-source-only`, which `lisp/loadup.el:104-105` turns on for exactly that
   window.

## Measured facts

Probe compiled with the real `xemacs-core` flags (`-DHAVE_CONFIG_H -Demacs -I src -I build/src`):

```
sizeof (Lisp_Symbol)            = 40   offsetof name=8 value=16 function=24 plist=32
sizeof (struct lrecord_header)  = 4
sizeof (Lisp_Compiled_Function) = 72
sizeof (struct specbinding)     = 24   sizeof (Lisp_Object) = 8, align 8
lrecord_type_compiled_function  = 20   lrecord_type_symbol = 62
lrecord_type_last_built_in_type = 75   MODULE_DEFINABLE_TYPE_COUNT = 32  (75+32 = 107)
```

Counts from `grep` over `src/*.c`: 652 `DEFVAR` sites, of which 300 `DEFVAR_LISP`, 79 `DEFVAR_BOOL`,
69 `DEFVAR_SPECIFIER`, 50 `DEFVAR_INT` (+ 18 `*_MAGIC`, − 32 `DEFVAR_CONST_*`); 25 `DEFUN` forms with
`max_args == UNEVALLED` (19 in `eval.c`, 3 in `editfns.c`, 1 each in `symbols.c`, `print.c`,
`callint.c`); 59 `Feval (` call sites (33 in `eval.c`); 25 `COMPILED_FUNCTIONP` sites; ~14 sites that
recognise a function as `(EQ (XCAR (fun), Qlambda))`; ~1083 `defvar` + ~320 `defconst` forms in `lisp/`.

---

## 1. A new closure object type

### 1.1 Verdict on the believed five

| Believed touch point | Verdict | Evidence |
|---|---|---|
| `lrecord_type` enum entry | **Real.** 1 line, must stay sorted | `src/lrecord.h:264` (`lrecord_type_compiled_function`); ordering rule at `src/lrecord.h:246-249` |
| `DECLARE_LISP_OBJECT` / struct decl | **Real.** ~20 lines | `src/bytecode.h:51-78` struct, `:106` `DECLARE_LISP_OBJECT`, `:113-118` the five `X…/wrap…/…P/CHECK/CONCHECK` macros |
| `DEFINE_DUMPABLE_FROB_BLOCK_LISP_OBJECT` | **Real, but the frob-block variant is a choice** | `src/bytecode.c:3026-3036`; the non-frob sibling is `DEFINE_DUMPABLE_LISP_OBJECT` (`src/lrecord.h:1281-1284`), used by e.g. `hash-table` (`src/elhash.c:3358`) |
| fixed-type allocator + sweeper | **Only if frob-block: 5 sites, ~25 lines. Otherwise 0 lines.** | `src/alloc.c:2116-2138` (`DECLARE_FIXED_TYPE_ALLOC`, `MINIMUM_ALLOWED_…`, `make_compiled_function`), `:4907-4914` (`sweep_compiled_functions`), `:5253` (call in the sweep sequence). Non-frob types go through `ALLOC_NORMAL_LISP_OBJECT` (`src/lrecord.h:141`) → `alloc_automanaged_lcrecord` and are swept via the shared `lcrecord_list` machinery set up in `define_lisp_object` (`src/alloc.c:4505-4517`) |
| `pdump_object_table` row | **Does not exist.** Zero `dumper.c` edits | `pdump_object_table` is `xnew_array (pdump_block_list, lrecord_type_count)` at dump time (`src/dumper.c:2076`), indexed by `objh->type` at registration (`src/dumper.c:984`), iterated `for (i = 0; i < lrecord_type_count; i++)` (`src/dumper.c:2125,1766,2222`). No type is named anywhere in `src/dumper.c` |

The authoritative checklist for a new type is the comment at `src/lrecord.h:1426-1462` ("Steps to
create a new object", with a worked `toolbar_button` example): struct + standard junk in a header,
header added to `src/inline.c`, methods, memory description, `DEFINE_*_LISP_OBJECT` in `syms_of_foo`,
enum entry, `.gdbinit.in.in`. Note it does **not** list `alloc.c` (the toolbar type uses the generic
allocator) and does **not** list the dumper.

### 1.2 What the survey missed

1. **Memory description array** — the single most load-bearing item, because it *is* the GC mark
   spec and the dump relocation spec: `src/bytecode.c:2537-2547`
   (`compiled_function_description`), consumed by `RECORD_DESCRIPTION` in the GC
   (`src/gc.c:779`) and by `pdump_register_object`/`pdump_register_sub` (`src/dumper.c:979-997`).
   One `{ XD_LISP_OBJECT, offsetof … }` row per Lisp slot; `XD_END` terminated.
2. **Printer + read syntax.** `print_compiled_function` (`src/bytecode.c:2368-2430`) emits `#[…]`
   when `print_readably`; the reader case is `src/lread.c:2565` (`case '['`) and
   `read_compiled_function` (`src/lread.c:3193-3275`), which bounds the element count with
   `COMPILED_STACK_DEPTH + 1 .. COMPILED_DOMAIN + 1` (`src/bytecode.h:33-40`). A new readable
   closure syntax needs a new dispatch character there. `#'` → `(function …)` already exists
   (`src/lread.c:2568`), so no reader work is needed for the *notation*.
3. **`equal` / `hash` methods** — `compiled_function_equal` (`src/bytecode.c:2433-2452`) and
   `compiled_function_hash` (`src/bytecode.c:2524-2535`), passed as arguments to the `DEFINE_…`
   macro. Required if closures may be hash-table keys or compared with `equal`.
4. **`print_preprocess` and `nsubst_structures_descend`** — registered at
   `src/bytecode.c:3033-3034`; implementations at `:2457-2478` (doc-reference numbering) and
   `:2480-2523` (load-time doc-string substitution, driven from `src/lread.c:445-475` via
   `Vload_force_doc_string_list`). A closure carrying a docstring needs both.
5. **The one type switch in the evaluator** — `src/eval.c:1268-1298`, `Fquote_maybe`, lists
   self-quoting record types including `case lrecord_type_compiled_function:`. This is the *only*
   `case lrecord_type_compiled_function` in the tree (verified by grep), and a closure missing from
   it becomes `(quote <closure>)` — silently data, not a function.
6. **Predicate symbol** — `DEFSYMBOL_MULTIWORD_PREDICATE (Qcompiled_functionp)`
   (`src/bytecode.c:3040`). `type-of` needs nothing: it returns
   `XRECORD_LHEADER_IMPLEMENTATION (object)->name` (`src/data.c:378-393`).
7. **Dump-time obarray finalization pass** — `finalize_functions_in_obarray`
   (`src/alloc.c:5364-5420`) dispatches on `SUBRP` / `COMPILED_FUNCTIONP` of each symbol's function
   slot to de-duplicate and `SET_C_READONLY` interactive prompts. Only relevant if a closure can be
   a symbol's *definition* (in the GNU model it cannot: `defun` yields a non-capturing function).
8. **Debug tables, mandated by comment** — `DON'T FORGET to update .gdbinit.in.in` at
   `src/lrecord.h:236`; entry is 4 lines per type (`src/.gdbinit.in.in:360-362`) plus 2 lines in
   `src/.dbxrc.in:282-284`.
9. **`src/inline.c`** — checklist step 3 (`src/lrecord.h:1436`); `bytecode.h` is already listed
   (`src/inline.c:48`), so a closure declared in `bytecode.h` costs 0 lines there.
10. **`.elc` arity plumbing** — `Fmake_byte_code` is `4, MANY` with an explicit
    `nargs < 4 || nargs > 6` check (`src/alloc.c:2140-2169`), and the reader bound is
    `COMPILED_DOMAIN + 1` (`src/lread.c:3207-3212`). Any added element must be accepted by both, and
    old-vs-new `.elc` mismatch fails as `continuable_read_syntax_error`.
11. **There is no `src/lrecord.c`.** Marshalling/registration lives in
    `define_lisp_object` (`src/alloc.c:4499-4520`) and the `lrecord_implementations_table`
    (`src/alloc.c:104`, `lrecord.h:512-515`); the Windows export file lists only the two tables
    (`src/xemacs.def.in.in:258-259`), not per-type rows. There is no `emulathons`, and the `mule-*`
    files contain no object-type table (`mule-ccl.c:1692` only tests `Qlambda` as a function
    marker).

### 1.3 Two designs, priced

**A. Slot on the existing compiled function (recommended shape).**
`Lisp_Object environment` in `Lisp_Compiled_Function` (`src/bytecode.h:51-78`) → 72 → 80 bytes
(~1765 compiled functions in a typical image per the accounting comment at `src/bytecode.h:122-143`,
so ~14 KB). Edits: `bytecode.h` +1 field; `bytecode.c` description +1 row, printer +~6, accessors
+~10; `alloc.c` `make_compiled_function` init +1 (`:2120-2138`), `Fmake_byte_code` arity +~8
(`:2140`); `lread.c` `read_compiled_function` bound +~8 (`:3193`); `eval.c` pass env into the VM in
`funcall_compiled_function` +~5 (`:3711-3726`). **4 files, ~40-60 lines, 0 new type pipelines.**
All 25 `COMPILED_FUNCTIONP` sites keep working unchanged (a closure *is* a compiled function); they
still need an audit for the semantics of `equal`/`hash` (`src/bytecode.c:2433,2524`) — should two
closures over different environments be `equal`?

**B. Distinct `Lisp_Closure` type.** Enum (1), header (20), description (6), printer (20),
`equal`/`hash` (15), `print_preprocess`/`nsubst` (2), `DEFINE_…` + predicate + accessors +
`make-closure` in `syms_of_bytecode` (~40), `make_*` allocator+sweeper if frob-block (~25, else 0),
reader case + `read_closure` (~25), `quote-maybe` (1), `funcall_closure` (~40), `functionp`/
`function_argcount`/dispatch (~5), `.gdbinit.in.in` (4), `.dbxrc.in` (2).
**8 files, ~180-260 lines**, plus ~40 audit sites (25 `COMPILED_FUNCTIONP` + 14 `(car fun) == lambda`)
and ~10 new function-shape sites.

**C. Interpreted closures as `(closure ENV ARGLIST . BODY)` lists** (GNU 24.1's shape, per the
feasibility report) — 0 new types, but every function-shape recognizer must learn the tag:
`src/eval.c:3300` (`Fcommandp`/interactive), `:3960` (`Feval`), `:4175` (`Ffuncall`), `:4226`
(`Ffunctionp`), `:4307` (`function_argcount`), `:4854`, `:5227`, `:5255`; `src/callint.c:380,425,585,969`;
`src/doc.c:500,635`; `src/keymap.c:4453`; `src/extents.c:4664`; `src/gui.c:90`; `src/glade.c:79`.
Note `Ffuncall`/`Feval` currently reject anything that is not `Qautoload`/`Qmacro`/`Qlambda`
(`src/eval.c:3993-4004`) → `invalid-function`, which at least fails loudly.
**~14 files-touching edits, ~120-180 lines** including the new `funcall_closure`.

## 2. The special-variable bit

### 2.1 Where it can live

`struct Lisp_Symbol` is `src/lisp.h:2847-2872`; the slot accessors are `:2888-2899`.

* **A new `unsigned int flags` word after the header union — free.** Measured `sizeof` 40 with
  `name` at offset 8, i.e. 4 bytes of alignment padding after the 4-byte header word. Adding the
  field keeps the size at 40, so `TYPE_ALLOC_SIZE` (`src/alloc.c:962-965`), the frob-block layout,
  `lrecord_implementations_table[tipo]->static_size` (`src/alloc.c:4508`) and every block stride are
  unchanged.
* **A bit inside the header word — possible but hostile.** The word is saturated:
  `type:8 mark:1 c_readonly:1 lisp_readonly:1` (`src/lrecord.h:157-181`) aliased in the symbol to
  `package_count:2` (over `free:1`) and `first_package_id:19` (over `uid:20`)
  (`src/lisp.h:2849-2866`, with the comment "Everything before package_count must agree exactly with
  struct lrecord_header"). A new bit must be stolen from `first_package_id` (19→18, harmless: 262143
  packages), and `set_lheader_implementation` (`src/lrecord.h:204-211`) writes `free = 0` and
  `uid = lrecord_uid_counter[type]++` on **every** allocation — which is precisely why
  `Fmake_symbol` re-zeroes `package_count`/`first_package_id` immediately afterwards
  (`src/alloc.c:2269-2273`). Any header-word bit needs a matching edit in that macro, or it is
  silently randomized at birth.
* **A plist property (`(put sym 'permanent-local …)`-style) or a global hash set of special
  symbols** — 0 struct change, dump-safe (plists and hash tables are ordinary described Lisp
  objects), ~15-25 lines; costs an O(plist) or hash lookup at every binding decision, and cannot be
  made atomic with `makunbound`/`defvaralias`.

### 2.2 What breaks

* **Dump format: nothing.** pdump serializes `static_size` bytes verbatim
  (`src/dumper.c:1273` `memcpy (pdump_buf, elt->obj, size*count)`, `:1284` `retry_fwrite`) and
  relocates only slots named in `symbol_description` (`src/symbols.c:92-98`, four
  `XD_LISP_OBJECT` rows). A plain integer flag therefore rides the image with no description entry
  and no GC consequence (`src/gc.c:779` walks the description only). `pdump_header`
  (`src/dumper.c:400-415`) carries only `"XEmacsDP"` + `id`; there is **no** layout version field,
  and `dump_id` is a `string(RANDOM)` baked at CMake *configure* time
  (`src/CMakeLists.txt:731-739`) and matched at load (`src/dumper.c:2359-2375`). Stale-dump
  detection therefore relies on the file name/id, not on layout.
* **Sizing/alignment:** unchanged (see above).
* **Initialization:** one site — `Fmake_symbol` (`src/alloc.c:2259-2279`) is the only symbol
  allocator in the tree; interning reaches it via `src/elhash.c:2237`. Set `flags = 0` there.

### 2.3 Sites that must set the bit

`Fdefvar` `src/eval.c:1404-1459`, `Fdefconst` `src/eval.c:1461-1509` (1 line each), plus the C
variables: every `DEFVAR_LISP/INT/BOOL/SPECIFIER/*_MAGIC` funnel through one function,
`defvar_magic` (`src/symbols.c:3465-3485`, macros at `src/symeval.h:509-548`) → **1 line covers all
~500 C-declared variables**. `defkeyword` (`src/symbols.c:3325`) and constants need no bit
(`reject_constant_symbols`, `src/symbols.c:205+`). A new `special-variable-p` subr is ~15 lines in
`symbols.c` + `DEFSUBR`.

### 2.4 Can the value-cell magic carry it instead? No.

The existing indirection is `SYMBOL_VALUE_MAGIC_P` — a single `<=` on the header type word
(`src/symeval.h:85-87`, enum of value-magic types at `:30-71`, six lrecord types at
`src/lrecord.h:241-249`), resolved by `find_symbol_value`/`find_symbol_value_1`/
`do_symval_forwarding` (`src/symbols.c:1392-1420,1281-1345`).

* It cannot mark the case that matters: a `defvar`'d global with a plain value is
  **indistinguishable** from a never-`defvar`d symbol. Only C variables and buffer-locals/aliases
  already have magics.
* Encoding specialness as a new wrapper magic means **one extra heap record per special symbol**
  (~1083 `defvar` + ~320 `defconst` in `lisp/`, plus ~500 C, plus every package variable) and moves
  *every* `symbol-value` from a direct slot read to `find_symbol_value_1` forwarding — a regression
  on the hottest operation in the interpreter.
* It is destroyed by value-cell replacement, all of which write `XSYMBOL (sym)->value`:
  `Fmake_variable_buffer_local` / `Fmake_local_variable` install a `symbol_value_buffer_local`
  magic (`src/symbols.c:1865,1966`, struct at `src/symeval.h:274-352`), `Fdefvaralias` installs a
  varalias magic (`src/symbols.c:2995`), `Fmakunbound`/`Fset (sym, Qunbound)` write `Qunbound`
  (`src/symbols.c:270,1471-1500`), and the `SPECBIND` fast paths poke the slot directly
  (`src/backtrace.h:229-261` `SB_sym->value = …`, `:270-293` `SFU_sym->value = …`). Each would have
  to re-wrap or copy the flag; the copy-on-unwrap logic is exactly the kind of thing that gets
  missed.

### 2.5 Buffer-locals (the requirement in the ticket)

With a **struct flag** the answer is "nothing to do": specialness and value-cell shape are
orthogonal, so a `defvar`'d global keeps its `symbol_value_buffer_local` magic and its per-buffer
values, and `Fdefvar` already only touches the default (`Fdefault_boundp` + `Fset_default`,
`src/eval.c:1428-1438`; `Fset_default` at `src/symbols.c:1767`). `specbind_magic`
(`src/eval.c:6769-6800`) continues to handle binding a buffer-local as a *function argument*.
With a **value-cell encoding**, `make-variable-buffer-local` on a special symbol replaces the
specialness marker with the buffer-local magic and the symbol silently reverts to "not special" —
the precise failure the ticket worries about. This is the strongest argument for the struct flag.

**Shape:** 1 field + ~6 macros/sets, 3 setter sites (`defvar_magic`, `Fdefvar`, `Fdefconst`),
1 init site, 1 new predicate. **~25-35 lines across `lisp.h`, `symbols.c`, `eval.c`; 0 bytes;
0 dump-format change.** Risk: low.

## 3. The evaluator

### 3.1 Inventory

* `Feval` `src/eval.c:3729` — `DEFUN ("eval", Feval, 1, 1, 0)`: **no environment parameter**. Its
  symbol case *is* the variable reference: `if (SYMBOLP (form)) return Fsymbol_value (form);`
  (`src/eval.c:3744-3747`). There is no separate `eval_sub`; `Feval` is its own `eval_sub`, and it
  contains a **second full call dispatch** (`src/eval.c:3830-4020`) duplicating `Ffuncall`'s
  (`src/eval.c:4031-4207`) — `SUBRP` at `:3830`, `COMPILED_FUNCTIONP` at `:3911`, `CONSP`
  (`Qautoload`/`Qmacro`/`Qlambda`) at `:3941-3990`, `invalid_function` at `:4002`. Any env threading
  is doubled by construction.
* Binding sites: `FletX` `src/eval.c:1060` (`specbind` at `:1096`), `Flet` `:1103` (`specbind` at
  `:1165`), `funcall_lambda` `:4466-4509` (args specbound at `:4494,4498,4502`),
  `funcall_compiled_function` `:3641` (the `SPECBIND_FAST_UNSAFE (f->args[i], args[i])` loop at
  `:3661`/`:3676-3679`, with `SPECPDL_RESERVE (f->specpdl_depth)` at `:3655`),
  `run_condition_case_handlers` `:2157-2183` (`specbind (var, …)` at `:2179`), `Fsetq` `:1195` →
  `Fset`, `Ffunction` `:1305-1324` (returns `XCAR (args)` unchanged — the closure-creation site),
  `Fdefun`/`Fdefmacro` `:1346,1360` (which build `(macro lambda …)` at `:1401`).
* Read/set paths: `Fsymbol_value` `src/symbols.c:1456`, `Fset` `:1471`, `Fboundp` `:140`,
  `Fglobally_boundp` `:149`, `Fset_default` `:1767`, `find_symbol_value` `:1392`.
* Special forms: there is **no special-operator table**. A special form is a subr with
  `max_args == UNEVALLED` — `Fspecial_operator_p` is literally
  `SUBRP (object) && XSUBR (object)->max_args == UNEVALLED` (`src/symbols.c:383-401`) — and such
  subrs are invoked with exactly one argument:
  `val = (((Lisp_Object (*) (Lisp_Object)) subr_function (subr)) (original_args));`
  (`src/eval.c:3844-3847`). 25 such forms exist (eval.c: `or and if cond progn prog1 prog2 let* let
  while setq quote function defun defmacro defvar defconst catch unwind-protect condition-case
  multiple-value-call multiple-value-prog1`; plus 3 in `editfns.c`, 1 each in `symbols.c`,
  `print.c`, `callint.c`).

### 3.2 The comment that already anticipated this

`src/eval.c:3589-3602`, verbatim:

```
/* NOTE: If you are hearing the endless complaint that function calls in
   elisp are extremely slow, it just isn't true any more!  The stuff below
   -- in particular, the calling of subrs and compiled functions, the most
   common cases -- has been highly optimized.  There isn't a whole lot left
   to do to squeeze more speed out except by switching to lexical
   variables, which would eliminate the specbind loop. (But the real gain
   from lexical variables would come from better optimization -- with
   dynamic binding, you have the constant problem that any function call
   that you haven't explicitly proven to be side-effect-free might
   potentially side effect your local variables, which makes optimization
   extremely difficult when there are function calls anywhere in a chunk of
   code to be optimized.  Even worse, you don't know that *your* local
   variables aren't side-effecting an outer function's local variables, so
   it's impossible to optimize away almost *any* variable assignment.) */
```

### 3.3 Where an environment has to be threaded (explicit-parameter route)

`Feval` → both dispatch ladders (`src/eval.c:3830-4020` and `4031-4207`) → `funcall_lambda`
(`:4466`) and `funcall_compiled_function` (`:3641`) → `execute_optimized_program` (`:3712`,
prototype `src/bytecode.h:96-101`, called with `XVECTOR_DATA (f->constants)`) → all 25 UNEVALLED
handlers → every `Feval` recursion inside them (33 in `eval.c` alone) → `Fapply` (`:4378`),
`apply1`/`call1..call3`, `Fmultiple_value_call` (`:4826`) → the two macro expanders
(`Fmacroexpand`'s `apply1 (expander, …)` at `:1613`) → `condition_case_1`/`internal_catch`, which
take the evaluator as a callback.

The structural blocker is that `Feval` is used as a value, not only as a call:
`src/bytecode.c:1605` (`record_unwind_protect (Feval, …)`), `:1621`
(`internal_catch (TOP, Feval, arg, …)`), `src/cmdloop.c:246` (`condition_case_1 (Qerror, Feval, …)`),
`src/eval.c:2219`, `src/event-stream.c:861`, and `src/lread.c:719,1469,1526`
(`readevalloop (stream, name, Feval, …)` — the loader's and `eval-buffer`/`eval-region`'s eval
hook). **8 callback sites** fix the shape `Lisp_Object (*)(Lisp_Object)`.

**Shape:** 1 signature + 8 wrappers + 25 UNEVALLED handlers + ~2 dispatch ladders + ~60 call sites
= **~150-250 lines across `eval.c`, `bytecode.c`, `lread.c`, `cmdloop.c`, `event-stream.c`**, i.e.
every file that evaluates.

### 3.4 The specbind-array alternative

`specpdl` is `struct specbinding *specpdl` / `*specpdl_ptr` (`src/eval.c:284-287`, declared
`src/backtrace.h:183-215`), entry `{ symbol, old_value, func }` (24 bytes, `src/backtrace.h:183-187`),
`specbind` at `src/eval.c:6764` with the inline `SPECBIND`/`SPECBIND_FAST_UNSAFE` at
`src/backtrace.h:229-293`, growth by `XREALLOC_ARRAY` at `src/eval.c:6671,6685`, GC marking at
`src/gc.c:1309-1316`, `SPECPDL_RESERVE` at `src/backtrace.h:295-300`. Precedent for non-symbol
bindings on the same stack: `internal_bind_int` (`src/eval.c:6897`) and
`condition_case_unwind`.

Binding a global `internal_interpreter_environment` (a new `staticpro`'d `Lisp_Object` holding
`nil` = dynamic mode, `(t)` = empty lexical env, or an alist/vector of active bindings — GNU 24.1's
shape per the feasibility report) is the cheap route: **~30 lines**, and it composes for free with
`throw`/`unbind_to`/`condition-case` unwinding and GC marking of the specpdl.

Three hard limits on that route, all structural:

1. **Escaping capture is impossible.** Entries are unwound at `unbind_to`, and the array is
   `realloc`'d (`src/eval.c:6671,6685`) so no address is stable; `src/dumper.c` has no knowledge of
   `specpdl` at all. A closure that outlives its dynamic extent therefore cannot point at a
   specpdl slot — captured-and-mutated variables need heap cells (GNU used cons "value cells").
2. **Lookup cost.** An alist env is O(depth) per reference from the interpreter; that is the price
   of not threading the environment explicitly.
3. **This fork's coroutines.** `src/async-scheduler.c` keeps a *per-coroutine* specpdl array
   (`coro_specpdl`, `:100-101` in the header; `xmalloc` at `:395-410`), swaps symbol values in and
   out across a yield (`specpdl_swap_one` `:147-162`, `specpdl_unwind_range` `:165-173`,
   `specpdl_rebind_range` `:175-183`), special-cases the global `Vcondition_handlers` chain
   (`coro_detach_condition_handlers` `:196+`), and marks `coro_specpdl` for GC (`:929-940`). Any
   global environment variable must be saved/restored like `Vcondition_handlers`, and any new
   specpdl entry kind must be recognized by `specpdl_swap_one`, which assumes
   `p->symbol` is a symbol (`Lisp_Symbol *sym = XSYMBOL (p->symbol);` `:149`). **+1 to +4 edits there
   per design** — the single largest thing the prior survey missed, because it is new code in this
   fork rather than upstream XEmacs.

**Shape:** ~30 lines for the binding machinery, + ~100-200 lines for the env lookup/extension in
`Flet`/`FletX`/`Fsetq`/`Ffunction`/`funcall_lambda`/`Feval`'s symbol case, + ~10-30 lines in
`async-scheduler.c`.

## 4. Dump stability

* The three `lexical-binding: t` files on the dump list are `macroexp`, `tabulated-list`,
  `use-package-xemacs` (`lisp/dumped-lisp.el:309-311`); `lisp/loadup.el:115` loads `dumped-lisp.el`
  and `:118` does `(mapc #'load preloaded-file-list)`. Of the 12 files in `lisp/` that declare
  `lexical-binding: t`, these 3 are dumped.
* They are **byte-compiled before the dump**: `src/CMakeLists.txt:912-922` runs
  `update-elc.el` to produce `NEEDTODUMP`, then `:926-933` runs `temacs -l loadup.el dump`. So the
  dumped forms are compiled functions, not source.
* **Compiled closures are dumpable** by the existing generic path: `pdump_register_object` dumps any
  object whose implementation is `dumpable` and has a description (`src/dumper.c:979-997`); the
  precedent that a *C-side* block is dumped too is `Lisp_Compiled_Function::args`, described as
  `XD_BLOCK_PTR` with its count taken from `args_in_array` (`src/bytecode.c:2537-2547`). Optimized
  instruction strings are `Lisp_Opaque`, which is `DEFINE_DUMPABLE_SIZABLE_LISP_OBJECT`
  (`src/opaque.c:164`). A `Lisp_Object environment` slot is one `XD_LISP_OBJECT` row → marked, dumped,
  relocated with no further work.
* **What is not dumpable:** an environment that reaches C memory not named by a description
  (specpdl slots, stack frames, `Lisp_Object *` not declared as a block). The dump does not
  silently corrupt in that case — it reports and refuses:
  `pdump_alert_undump_object[objh->type]++` plus `"Undumpable object type : %s"`
  (`src/dumper.c:988-995`), then the whole dump aborts with an "Undumpable types list" summary
  (`src/dumper.c:2222-2240`). At top level in a lexbind file the environment is empty (`(t)` in
  GNU's design), so a dumped closure normally captures nothing; the failure mode only bites if a
  dumped file's *top-level evaluation* creates a closure over a non-dumpable object (a frame,
  window, process, lstream — the `DEFINE_NODUMP_*` family, `src/lrecord.h:1300-1330`).
* **The special bit survives trivially** (raw struct word, §2.2), and C-declared variables get it at
  `syms_of_*` time, i.e. before `pdump`.
* **Consequence for the subset decision (#134).** Closures do **not** have to be kept off the dump
  list. Two coherent options: (a) with #131's compiler/VM work, the 3 files stay byte-compiled and
  dumped; (b) interpreter-only lexical binding means those 3 files must run as source at dump time
  (XEmacs bytecode cannot express lexical access), which `load` tolerates but warns about because
  `lisp/loadup.el:104-105` sets `load-warn-when-source-only t`, handled at
  `src/lread.c:592-614` (`load_warn_when_source_only`, `src/lread.c:90,3367`). Option (b) is a
  startup-cost decision, not a correctness constraint; alternatively the 3 entries can be dropped
  from `preloaded-file-list` and loaded at run time.
* **`lisp/dumped-lisp.el:26-31`** ("Invalid opcode" for extended characters in byte-code data) is
  about *non-ASCII in `.elc` instruction strings*, not about object types. It does not forbid dumped
  closures, but it is the standing evidence that the dump path bakes assumptions about `.elc`
  content; the analogous new hazard is the `#[…]` element-count contract (§1.2 items 2 and 10).

**Shape:** 0 lines of `dumper.c`; 1 description row per environment slot; ~8 lines if a run-time
fixup were ever needed (no hook exists today — `pdump` has no per-type post-undump callback, only
the generic relocation walk `src/dumper.c:1400-1520`).

## 5. Value-cell-keyed machinery that would be silently wrong

* **Direct value-slot reads/writes.** `symbol_value(s)` / `XSYMBOL_VALUE(s)`
  (`src/lisp.h:2890,2902`) appear ~180 times in `src/`. The hot ones that must become
  "lexical-aware" are `SPECBIND`/`SPECBIND_FAST_UNSAFE` (`src/backtrace.h:236-260,275-292`),
  `Fset`'s no-magic fast path (`src/symbols.c:1490-1495`), `find_symbol_value`
  (`src/symbols.c:1397-1400`), `Feval`'s symbol case (`src/eval.c:3746`), and the VM's
  `Bvarref`/`Bvarset`/`Bvarbind` block (`src/bytecode.c:943-1053`, #131's territory but they are
  value-cell paths).
* **`macroexpand`'s `byte-compile-macro-environment` specbind** (`src/eval.c:1530-1533`,
  `Vbyte_compile_macro_environment` declared at `src/eval.c:273`):
  ```
  if (!EQ (environment, Vbyte_compile_macro_environment))
    { specbind (Qbyte_compile_macro_environment, environment); }
  ```
  Three consequences: the shadowing is *dynamically* scoped, so `byte-compile-macro-environment`
  must stay special even inside a lexbind file (it is `defvar`'d in Lisp, so it needs the bit);
  the loop reads `def = XSYMBOL (sym)->function;` directly (`src/eval.c:1570`) bypassing
  `indirect_function`; and it calls the expander with `apply1 (expander, XCDR (form))`
  (`src/eval.c:1613`) where `expander` is the `(lambda …)` inside `(macro lambda …)` — for a macro
  defined in a lexbind file that object is a closure, so this is a closure-application site
  reachable from *dynamic* code. `Fdefmacro` builds that shape at `src/eval.c:1401`.
* **`Fboundp` and friends.** `Fboundp` (`src/symbols.c:140-147`), `Fglobally_boundp` (`:149-157`),
  `Fmakunbound` (`:270`), `Fvariable_binding_locus` (`:3110`), `Flocal_variable_p` (`:2433`),
  `Fkill_local_variable` (`:2117`), `Fmake_variable_buffer_local` (`:1865`) all answer about the
  value cell. For a lexical name they report "unbound / no locus" — matching GNU's `boundp`, but the
  failures are **silent**, not errors, and `variable-binding-locus` returning `nil` is how
  `setq-default`-style code quietly does nothing.
* **`quote-maybe`'s record-type switch** (`src/eval.c:1268-1298`) — see §1.2 item 5.
* **Special-form handling as such:** `Fspecial_operator_p` (`src/symbols.c:383-401`) and the
  `UNEVALLED` convention (`src/eval.c:3843-3848`). Under lexical binding every UNEVALLED handler
  needs the environment (§3.3), and a *new* lexical special form cannot be added as a subr without
  deciding how it receives the env — the `UNEVALLED` ABI is single-argument, so the specbind-global
  route (§3.4) is the only one that leaves the 25 handlers' signatures alone.
* **Instrumentation / debugging.** XEmacs has **no edebug** in this tree (`lisp/edebug.el` absent; no
  `Qedebug` in `src/`), so the edebug concern from the survey does not apply. The equivalent surface
  here is `do_debug_on_call (Qlambda)` (`src/eval.c:4089-4090`), `struct backtrace`
  (`src/backtrace.h`, fields `function`/`args`/`nargs`/`evalargs`/`debug_on_exit`, pushed at
  `src/eval.c:3809-3816` and `:4080-4087`), `Fbacktrace_frame` (`src/eval.c:7262-7297`), and the
  backtrace printer's `COMPILED_FUNCTIONP` case (`src/print.c:2970-2976`). None of them records a
  frame's environment, so a debugger cannot display lexical variables until `struct backtrace`
  grows a field (+1 struct, +~6 sites). `src/profile.c:131` also keys off compiled functions.
* **`internal-<>` machinery**: XEmacs has no `internal-<foo>` interpreter functions in C (verified:
  no `DEFUN ("internal-` in `src/`); what exists is the C-global `DEFVAR_LISP` forwarding family
  (`src/symeval.h:509-548` → `defvar_magic`, `src/symbols.c:3465`), which is special by construction
  and therefore safe (§2.3). `src/file-coding.c:1200` builds symbols named
  `internal-eol-copy-*` — unrelated.
* **Coroutines** — see §3.4 item 3; `src/async-scheduler.c:147,165,175,395-410,929-940` is the
  newest and least-covered consumer of the specbinding representation.

## Shape and order of magnitude

| Item | Files | Edits / lines | Risk |
|---|---|---|---|
| 1A closure as an `environment` slot on `Lisp_Compiled_Function` | 4 (`bytecode.h`, `bytecode.c`, `alloc.c`, `lread.c`) | ~40-60 | low; 25 `COMPILED_FUNCTIONP` sites keep working |
| 1B distinct `Lisp_Closure` type | 8 (`lrecord.h`, `bytecode.h`, `bytecode.c`, `lread.c`, `eval.c`, `alloc.c` (opt), `.gdbinit.in.in`, `.dbxrc.in`) | ~180-260 + ~40 audit sites | medium; 8-bit `type` field, no `.elc` change |
| 1C interpreted closures as `(closure ENV …)` lists | 10 (eval.c, callint.c, doc.c, keymap.c, extents.c, gui.c, glade.c…) | ~120-180 | medium; 14 recognizers, fails loudly as `invalid-function` |
| 2 special-variable bit (struct flag) | 3 (`lisp.h`, `symbols.c`, `eval.c`) | ~25-35, **0 bytes**, 0 dump change | low |
| 2′ specialness via value-cell magic | — | ~200+, one extra record per special symbol | **rejected**: cannot mark plain `defvar`s; destroyed by `make-variable-buffer-local`/`defvaralias`/`makunbound` |
| 3′ env as specbound global (`internal-interpreter-environment`) | 2 (`eval.c`, `async-scheduler.c`) | ~30 + ~100-200 use sites | medium; no escaping capture without heap cells; O(n) lookup |
| 3″ env threaded as an explicit parameter | 5 (`eval.c`, `bytecode.c`, `lread.c`, `cmdloop.c`, `event-stream.c`) | ~150-250; 8 callback sites + 25 UNEVALLED handlers | high |
| 4 dump | 0 (`dumper.c`) | 1 description row per slot | low; undumpable env aborts the dump visibly (`dumper.c:2222-2240`) |
| 5 debugger env visibility | 2 (`backtrace.h`, `eval.c`) | ~10 | low, deferrable |

Rough total for a runtime that supports both interpreted and byte-code closures, choosing 1A + 1C +
2 + 3′: **~350-550 lines of C across ~12 files**, no new object type, no dump-format change. The
dominant cost is not the object pipeline the ticket enumerated — it is the 25 UNEVALLED special
forms and the ~40 function-shape/value-cell sites.

## Three highest-risk unknowns

1. **`specpdl` cannot hold an escaping environment, and every cheap design wants it to.** The array
   is `realloc`'d (`src/eval.c:6671,6685`), unwound at `unbind_to`, and invisible to `pdump`
   (`src/dumper.c:979-997` walks only described objects). Mutable captured variables therefore need
   heap value cells (a cons or one-slot record per binding), which changes the cost model of
   interpreted `let` from "24 bytes on a stack array" to "≥ 2 conses per binding per call" — and this
   fork's coroutines (`src/async-scheduler.c:147-183,395-410,929-940`) then interact with a
   representation the async code was written against. Unmeasured: how much slower interpreted lexbind
   code gets, and whether `specpdl_swap_one`'s `XSYMBOL (p->symbol)` assumption (`:149`) breaks on a
   frame-style entry.
2. **Whether `Feval`'s single-argument ABI can be changed at all without a global.** It is a value in
   8 places (§3.3), including `readevalloop`'s evaluator hook (`src/lread.c:719,1469,1526`) and the
   top-level error handler (`src/cmdloop.c:246`). If a global is used, per-file lexical state must be
   specbound around each file's load — which means the loader (`src/lread.c`) and the evaluator share
   hidden state, and `condition-case`/`catch`/coroutine unwinding must restore it exactly. The
   failure mode if they do not is a *leaked environment* into unrelated evaluation, which is silent
   and non-reproducible. Unmeasured: whether `record_unwind_protect (Feval, …)`
   (`src/bytecode.c:1605`, `src/event-stream.c:861`) and `internal_catch (TOP, Feval, arg, …)`
   (`src/bytecode.c:1621`) can be re-pointed at a wrapper without changing behaviour.
3. **`.elc` and dump-image compatibility has no version negotiation.** `read_compiled_function`
   bounds the `#[…]` element count (`src/lread.c:3207-3212`) and `Fmake_byte_code` rejects
   `nargs > 6` (`src/alloc.c:2165-2168`), so an env element breaks old-XEmacs/new-`.elc` pairing in a
   way that reads as a syntax error; and `pdump_header` has no layout version
   (`src/dumper.c:400-415`) — the only guard is `dump_id`, a random value fixed at CMake *configure*
   time (`src/CMakeLists.txt:731-739`) and matched at load (`src/dumper.c:2359-2375`). Unmeasured:
   whether a structurally-changed image can ever be paired with a rebuilt binary under an unchanged
   configure cache, and whether the vendored `lisp/` `.elc` files in installed trees need a
   `byte-compile-version`-style stamp (XEmacs currently has none; cf. `src/bytecode.c:691-717` for
   the opcode-validation path #131 covers).

## Sources

Local tree (`src/` unless noted): `lrecord.h` (enum `:238-320`, header `:157-193`,
`set_lheader_implementation` `:204-211`, new-object checklist `:1426-1462`,
`DEFINE_*_LISP_OBJECT` `:1244-1330`, `MAKE_LISP_OBJECT` `:1343-1357`); `bytecode.h:33-40,51-78,106-118`;
`bytecode.c:2368-2478,2480-2547,2960-3050,943-1053`; `alloc.c:962-1040,1090-1186,2116-2238,2256-2279,
4499-4520,4907-4914,5235-5270,5364-5420,5629-5640,5680`; `lisp.h:2846-2902`; `symeval.h:30-87,135-352,509-548`;
`symbols.c:92-98,140-157,205-260,383-401,1392-1500,1767,1865,1966,2117,2433,2995,3110,3465-3485`;
`eval.c:60-240,273,284-287,1060-1170,1195-1255,1268-1324,1346-1401,1404-1509,1514-1650,2157-2183,2219,
3589-3602,3641-3726,3729-4020,4031-4207,4209-4347,4378,4466-4509,6671-6700,6764-6800,6897,7262-7297`;
`backtrace.h:170-300`; `gc.c:779,1309-1316`; `dumper.c:380-415,523,530-620,955-1000,1273-1284,2060-2140,
2200-2260,2359-2375`; `lread.c:90,445-475,592-614,719,1469,1526,2480-2575,3193-3275,3367`;
`opaque.c:164`; `elhash.c:2237-2240,2411-2418`; `data.c:378-393`; `async-scheduler.c:147-196,395-410,929-940`;
`cmdloop.c:246`; `event-stream.c:861`; `inline.c:48`; `.gdbinit.in.in:360`; `.dbxrc.in:282`;
`xemacs.def.in.in:258-259`; `src/CMakeLists.txt:725-745,895-945`; `lisp/loadup.el:100-119`;
`lisp/dumped-lisp.el:26-31,309-311`; `lisp/update-elc.el:31-45`; `lisp/{macroexp,tabulated-list,use-package-xemacs}.el`
headers. Probe toolchain: `build/compile_commands.json`.
Prior art: `git show origin/research/lexical-binding:docs/research/lexical-binding.md`
(GNU Emacs 24.1 design summary and the `internal-interpreter-environment` / `declared_special`
line references it cites).
