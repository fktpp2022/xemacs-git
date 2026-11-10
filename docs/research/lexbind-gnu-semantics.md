# What GNU's `lexical-binding` actually guarantees

Research findings for wayfinder ticket [#130](https://github.com/fktpp2022/xemacs-git/issues/130), map [Give XEmacs byte-compiled lexical binding](https://github.com/fktpp2022/xemacs-git/issues/128).

This is a **semantics inventory**: it states what `-*- lexical-binding: t -*-` buys you in GNU Emacs, with citations, and ranks each guarantee by **who can observe it at runtime** — an ordinary vendored package versus only a macro-authoring or byte-compiling one. That ranking is the payload the subset decision in the map needs. It deliberately does *not* re-argue feasibility, cost, or strategy; for those see the prior report [`docs/research/lexical-binding.md`](lexical-binding.md), which this note complements rather than repeats.

Re-landed onto the mainline under wayfinder ticket [#141](https://github.com/fktpp2022/xemacs-git/issues/141); it previously lived only on the throwaway branch `research/lexbind-gnu-semantics` (commit `d58c77cc5`). Read its Correction notice for claims later corrected.

**Method.** Reference implementation is GNU Emacs 24.1, tag `emacs-24.1` in [`emacs-mirror/emacs`](https://github.com/emacs-mirror/emacs) — the release that introduced the feature. Every claim is cited to the Elisp manual (the *guarantee*), to the C/Lisp source (the *mechanism*), or both. Deltas to current `master` (Emacs 31 development) are noted where they move a guarantee. Local XEmacs facts are cited from this tree only where the map needs the contrast; they are not re-derived.

XEmacs baseline, for orientation only (do not re-verify): no lexical binding at any layer — `internal-interpreter-environment`, `declared_special`, and `special-variable-p` appear nowhere under `src/`; `Feval` is `DEFUN ("eval", Feval, 1, 1, …)` with no `lexical` argument ([`src/eval.c:3729`](src/eval.c)); `Lisp_Symbol` has `name`/`value`/`function`/`plist` and header bitfields but no special bit ([`src/lisp.h:2847-2872`](src/lisp.h)); `lexical-let` is a gensym macro workaround ([`lisp/cl-macs.el:1705-1745`](lisp/cl-macs.el)); and `lisp/cl-lib.el:418` carries a live `(and (boundp 'lexical-binding) lexical-binding)` guard — see §7.

### How to read the citations

Every citation's link text is the load-bearing part: `path:start-end` (or a manual `@node` + line range) points at an exact region. Resolve a short path against these base URLs:

- GNU 24.1: `https://github.com/emacs-mirror/emacs/blob/emacs-24.1/<path>` — append `#L<start>-L<end>`.
- GNU master: `https://github.com/emacs-mirror/emacs/blob/master/<path>` (used only where this note says `master`).
- Local XEmacs: `https://github.com/fktpp2022/xemacs-git/blob/xemacs-cmake/<path>` (paths under `src/`, `lisp/`, `doc/`, `etc/`).

Links written ``[`src/eval.c:…`](src/eval.c)`` carry the 24.1 tag by default; ones marked `master` carry master.

---

## Bottom line — the runtime-observability ranking

The guarantees split into three tiers by who can see them. This is the ordering the subset decision should read first.

**Tier R — observable at *runtime* to any ordinary vendored lexical package.** These must hold for interpreted code too; a package that relies on them misbehaves (silently or with a `void-variable` error) if the host does not provide them:

1. **Special-variable exemption.** `defvar`/`defconst`/`defcustom` variables stay dynamically bound inside lexical code. Every lexical file that binds a hook or customization variable (`case-fold-search`, `my-mode-hook`, …) depends on this. (§1)
2. **Lexical variable bindings.** `let`/`let*` and lambda arguments create environment-alist bindings shadowing globals; a free, undeclared symbol in a `let` is lexical, not dynamic. (§2)
3. **Closures with indefinite extent.** A `lambda`/`function` form captured in a lexical environment outlives the binding construct; the counter (`my-ticker`) pattern works. (§2, §4)
4. **Mutable capture.** A closure that `setq`s an outer binding shares the *cell*, so the mutation is visible to every holder of the closure and to the enclosing scope. (§3)
5. **`(eval FORM t)`.** `eval`'s optional `lexical` argument — used directly by testing harnesses and any code that evaluates captured environments. (§2)
6. **`special-variable-p`.** Read by some portable macro bodies and back-ports at expand/run boundaries. (§1)

**Tier C — observable only at *byte-compile time*; NOT required to run the same package interpreted.** These are how the guarantee is *realised in compiled code*; an interpreter-only subset can omit all of them and still be semantically correct for source-loaded packages:

7. `cconv-closure-convert` (closure conversion, λ-lifting, by-reference boxing). (§3, §6)
8. The VM's `stack-ref` / `stack-set` / `stack-set2` / `discardN` opcodes (compiled lexical access). (§6)
9. `make-byte-code`'s captured-environment slot and `.elc` version handling. (§6)
10. Compiler diagnostics — "reference/assignment to free variable", "unused lexical variable" — and the compiler-internal proclamation set `byte-compile-bound-variables`. (§1, §6)

**Tier M — observable only to a *macro-authoring* package, and only during expansion.** Not a runtime facility at all:

11. The macro-expansion environment: `macroexpand`'s `ENVIRONMENT` argument, `macroexpand-all-environment`, the `&environment` lambda-list keyword, and the `macroexp-*` helpers. A package *calling* `cl-labels`/`cl-flet`/`cl-symbol-macrolet`/`push`/`pop` needs XEmacs's *copy of that macro* to expand correctly; it does not need an environment object to exist at run time, because the expansion is consumed by load/compile and leaves nothing behind that the runtime must reproduce beyond Tier R. A package that *defines* an `&environment`-aware macro and relies on lexical-variable info in the env is the only real consumer. (§5)

**The one structural consequence.** Local *function* bindings (item in §4) are **not** a separate runtime guarantee in GNU. `cl-flet`/`cl-labels` compile down to Tier R: a lexical variable holding the function value plus a closure. There is no distinct "function binding" primitive in `internal-interpreter-environment` (it stores *variable* bindings and dynamic proclamations only, [`src/eval.c:3748-3755`](src/eval.c)). So a lexical-binding implementation can sensibly **omit a dedicated function-binding mechanism** — *provided* lexical variables, closures, and mutable capture (needed for `cl-labels` recursion) are present. If any Tier R piece is missing, function bindings inherit the gap (e.g. XEmacs's `lexical-let`-based `labels` cannot back a real `cl-labels` closure — confirmed by Emacs itself in [etc/NEWS (24.4)](etc/NEWS)).

---

## 1. How a symbol becomes *special*

### The `declared_special` bit

In 24.1, `struct Lisp_Symbol` gained a single bit:

```
/* Non-zero means that this variable has been explicitly declared
   special (with `defvar' etc), and shouldn't be lexically bound.  */
unsigned declared_special : 1;
```

([`src/lisp.h:1113-1135`](src/lisp.h), the bit at line 1135). XEmacs's `Lisp_Symbol` has no such bit ([`src/lisp.h:2847-2872`](src/lisp.h)).

`special-variable-p` is exactly the read of that bit — nothing more:

```c
return XSYMBOL (symbol)->declared_special ? Qt : Qnil;
```

([`src/eval.c:3477-3485`](src/eval.c); doc at [variables.texi *Lexical Binding*](doc/lispref/variables.texi) 1059-1064: "returns non-`nil` if `symbol` is a special variable (i.e. it has a `defvar`, `defcustom`, or `defconst` variable definition)").

### What sets it

- **`defvar` *with* a value form** sets the global bit before evaluating the init, so self-referential inits see the var as special: `XSYMBOL (sym)->declared_special = 1;` at [`src/eval.c:827`](src/eval.c), inside the `if (!NILP (tail))` (value present) branch. The manual states the rule plainly: "Every variable that has been defined with `defvar`, `defcustom` or `defconst` is a special variable … All other variables are subject to lexical binding" ([variables.texi:1051-1057](doc/lispref/variables.texi)).
- **`defconst`** sets it unconditionally, at [`src/eval.c:913`](src/eval.c) — its docstring adds that the symbol "is marked as a special variable so that it is never lexically bound" ([`src/eval.c:885-889`](src/eval.c)).
- **`defcustom`/`defgroup`** set it indirectly by calling `defvar` as a subroutine ([etc/NEWS 24.1:1052-1053](etc/NEWS)).

### Proclamations — `(defvar foo)` with no value

A valueless `(defvar foo)` is a *proclamation*, and its effect is deliberately **not** the global bit. In the interpreter it only conses the **bare symbol** onto the current environment, and only when lexical mode is already active:

```c
else if (!NILP (Vinternal_interpreter_environment) && !XSYMBOL (sym)->declared_special)
  /* A simple (defvar foo) with lexical scoping does "nothing" except
     declare that var to be dynamically scoped *locally* (i.e. within
     the current file or let-block).  */
  Vinternal_interpreter_environment = Fcons (sym, Vinternal_interpreter_environment);
```

([`src/eval.c:868-874`](src/eval.c)). The comment at [`src/eval.c:877-880`](src/eval.c) is explicit that a valueless `defvar` "should not count as a definition at all" — it does **not** set `declared_special`, does not define a global value, and does not interfere with unloading.

The consequence is an asymmetry a subset implementer must not flatten:

- A `(defvar foo)` proclamation makes `foo` dynamic **only within the current lexical scope** (the file or `let` block whose env is being mutated), by inserting a bare-symbol entry the `let`/`setq` code recognises (§2).
- `special-variable-p` still returns `nil` for a purely-proclaimed `foo` (it reads only the global bit) — the proclamation lives in the transient env alist, not the symbol. (Same in `master`: proclamation path [`src/eval.c:994-999`](src/eval.c).)
- In compiled code the proclamation is likewise *compile-time only*: `byte-compile-file-form-defvar` pushes the name onto `byte-compile-bound-variables` and, for the valueless `defvar` case, emits no code at all ([`lisp/emacs-lisp/bytecomp.el:2246-2248`](lisp/emacs-lisp/bytecomp.el); the push at line 2243). The compiler consults that list by `memq`, alongside `special-variable-p`, in `byte-compile-not-lexical-var-p` ([`bytecomp.el:3849-3854`](lisp/emacs-lisp/bytecomp.el)).

### `declare-function`

`declare-function` is a **function** declaration, not a variable proclamation, and it is purely a byte-compiler hint: it suppresses "function is not known to be defined" warnings and optionally checks the arglist ([functions.texi *Declaring Functions*:1253-1302](doc/lispref/functions.texi)). It never touches `declared_special` (the *variable* bit). In `bytecomp.el` it is handled by macro-expanding and recording the symbol ([`bytecomp.el:424`](lisp/emacs-lisp/bytecomp.el), `byte-compile-macroexpand-declare-function` at [`bytecomp.el:2846-2856`](lisp/emacs-lisp/bytecomp.el)). It appears in this ticket's question alongside proclamations because both are "tells the compiler things not textually visible" idioms; but only `defvar`-family proclamations bear on variable specialness. (The related, genuine *variable* idiom for silencing free-variable warnings is `special-variable-p`-invisible `(defvar foo)` above.)

### What happens to an *undeclared* variable that dynamic code expects

This is the central hazard of the feature, and it is a **runtime** hazard (Tier R):

- The value of a symbol is looked up **lexical-first, then dynamic**: `eval_sub` does `Fassq` against the env alist and only falls back to `Fsymbol_value` if absent ([`src/eval.c:2220-2232`](src/eval.c)). A binding made by `let` for an undeclared symbol is therefore lexical, so a function `getx` defined *outside* that `let` cannot see it — the manual's worked example is `(let ((x 1)) (getx))` → `Symbol's value as variable is void: x` ([variables.texi:940-961](doc/lispref/variables.texi)). Symmetrically, a `let`-bound undeclared symbol that some other dynamically-scoped code intended to rebind will not be seen.
- The escape hatch is to *declare* the variable special (`defvar`, or a `(defvar foo)` proclamation) so `let` binds it dynamically (§2). The manual's recommended workflow is to byte-compile and fix every "reference or assignment to a free variable" warning, which is precisely where an undeclared symbol is used outside a binding form ([variables.texi:1078-1084](doc/lispref/variables.texi)); the corresponding compiler warnings fire at [`bytecomp.el:3067`](lisp/emacs-lisp/bytecomp.el) (ref) and [`bytecomp.el:3083`](lisp/emacs-lisp/bytecomp.el) (set).
- Two more runtime-visible sharp edges: `symbol-value`, `boundp`, and `set` see **only the dynamic value cell**, never the lexical env ([variables.texi:1015-1017](doc/lispref/variables.texi); source in §2); and the body of `defun`/`defmacro` "cannot refer to surrounding lexical variables" ([variables.texi:1018-1019](doc/lispref/variables.texi)). Using a special variable as a formal argument gives "unspecified behavior" under lexical binding ([variables.texi:1066-1069](doc/lispref/variables.texi)).

---

## 2. `internal-interpreter-environment` and how the evaluator consults it

### The representation

The interpreter keeps one hidden dynamic variable that *is* the lexical scope:

```
/* When lexical binding is being used, internal_interpreter_environment is
   non-nil, and contains an alist of lexically-bound variables, or (t),
   indicating an empty environment. … Every element of this list can be
   either a cons (VAR . VAL) specifying a lexical binding, or a single
   symbol VAR indicating that this variable should use dynamic scoping.  */
```

([`src/eval.c:3748-3762`](src/eval.c)). Three shapes: `nil` (dynamic mode), `(t)` (empty lexical env), or a mixed alist of `(VAR . VAL)` cells (lexical bindings) and bare `VAR` symbols (dynamic proclamations from a valueless `defvar`, §1). It is **deliberately not reachable from Elisp** — `Funintern` is called on its name so no one can even make it buffer-local ([`src/eval.c:3756-3766`](src/eval.c)). This is why the env itself is *not* runtime-observable to a package (Tier R is about its *effects*, not the object).

### `let` / `let*`

`let*` (`FletX`) is where the decision is made per binding ([`src/eval.c:1028-1045`](src/eval.c)); `let` (`Flet`) uses the same test at [`src/eval.c:1107`](src/eval.c):

```c
if (!NILP (lexenv) && SYMBOLP (var)
    && !XSYMBOL (var)->declared_special
    && NILP (Fmemq (var, Vinternal_interpreter_environment)))
  {  /* Lexically bind VAR by adding it to the interpreter's binding alist. */
     Lisp_Object newenv = Fcons (Fcons (var, val), Vinternal_interpreter_environment);
     if (EQ (Vinternal_interpreter_environment, lexenv))
       specbind (Qinternal_interpreter_environment, newenv);   /* save old env once */
     else
       Vinternal_interpreter_environment = newenv;
  }
else
  specbind (var, val);   /* special, or proclaimed-dynamic, or dynamic mode */
```

Two facts to carry into any subset design:

- The lexical-bind branch is guarded by **both** `!declared_special` (global bit) **and** `Fmemq(var, env)` (the bare-symbol proclamation entries from §1). A variable is dynamic inside a lexical file if *either* it was `defvar`'d with a value *or* it was `(defvar var)`-proclaimed in this scope. Only truly undeclared symbols become lexical.
- Lexical bindings are `specbind`-ed **as the whole env alist** (only for the first binding — see the comment at [`src/eval.c:1037-1039`](src/eval.c)); the individual `(var . val)` cells are ordinary conses on the heap, which is exactly what makes them shareable/mutable (§3).

### `setq`

`setq` mutates the existing lexical binding if the symbol has one, else falls through to `Fset` ([`src/eval.c:464-472`](src/eval.c)):

```c
if (!NILP (Vinternal_interpreter_environment) && SYMBOLP (sym)
    && !NILP (lex_binding = Fassq (sym, Vinternal_interpreter_environment)))
  XSETCDR (lex_binding, val);   /* SYM is lexically bound — mutate the cell */
else
  Fset (sym, val);              /* dynamically bound */
```

Note `XSETCDR` mutates the shared cons in place. Same logic in `master` ([`src/eval.c:492`](src/eval.c)).

### `function` / `lambda` and `funcall_lambda`

`Ffunction` turns a `(lambda …)` into an interpreted closure when a lexical env is live ([`src/eval.c:512-518`](src/eval.c)):

```c
if (!NILP (Vinternal_interpreter_environment) && CONSP (quoted)
    && EQ (XCAR (quoted), Qlambda))
  return Fcons (Qclosure, Fcons (Vinternal_interpreter_environment, XCDR (quoted)));
else
  return quoted;   /* just quote */
```

So an interpreted closure is `(closure ENV ARGS . BODY)` (also [etc/NEWS 24.1:1047-1048](etc/NEWS); manual [functions.texi Closures:1129-1138](doc/lispref/functions.texi) shows `(closure (t) (x) (* x x))`). The bare `lambda` form (as a macro) and `#'` are both specified to route through `function`, which is what makes them closures/compiled; a *quoted* `'(lambda …)` stays a plain list (see §4).

`funcall_lambda` recognises the `closure` head, splits off the captured `lexenv`, and **binds the arguments into that env** by consing `(arg-name . value)` cells ([`src/eval.c:3134-3149`](src/eval.c) and [`src/eval.c:3206-3223`](src/eval.c)):

```c
if (EQ (XCAR (fun), Qclosure)) { fun = XCDR (fun); lexenv = XCAR (fun); … }
...
if (!NILP (lexenv) && SYMBOLP (next))
  lexenv = Fcons (Fcons (next, arg), lexenv);   /* lexically bind the arg */
else
  specbind (next, arg);                          /* dynamically bind it */
...
if (!EQ (lexenv, Vinternal_interpreter_environment))
  specbind (Qinternal_interpreter_environment, lexenv);   /* activate the env */
```

Crucially, the closure's *captured* `lexenv` becomes the interpreter's current env for the body — that is indefinite extent. Byte-code objects are dispatched differently: a compiled function with a numeric "push args" slot goes straight to `exec_byte_code` (the stack path, §6); compiled functions with nil/absent slot, and *all interpreted functions, lexically bound or not*, use the env-list code above ([`src/eval.c:3150-3173`](src/eval.c)).

### `symbol-value` / `boundp` / `makunbound` — these do **not** consult the env

The manual guarantees the primitives touch only the dynamic value cell ([variables.texi:1015-1017](doc/lispref/variables.texi)). Source confirms it, in `data.c` (the value-cell primitives live there, not in `eval.c`): `Fsymbol_value` → `find_symbol_value` + void error ([`src/data.c:1049-1060`](src/data.c)); `Fset` → `set_internal` ([`src/data.c:1062-1068`](src/data.c)); `Fboundp` switches only on the symbol's `redirect`/value cell, never the env ([`src/data.c:520-556`](src/data.c)); `Fmakunbound` = `(set sym 'unbound)` ([`src/data.c:566-576`](src/data.c)); `Ffmakunbound` clears the function cell ([`src/data.c:578-588`](src/data.c)). None reads `Vinternal_interpreter_environment`. A lexical binding on an otherwise-unbound symbol is therefore *invisible* to `boundp`/`symbol-value`, and `makunbound` cannot unbind it.

### `eval`'s `lexical` argument

`Feval` grew an optional second argument; when non-nil it seeds an empty lexical env (`(t)`) before evaluating ([`src/eval.c:2196-2204`](src/eval.c)):

```c
specbind (Qinternal_interpreter_environment, NILP (lexical) ? Qnil : Fcons (Qt, Qnil));
```

[etc/NEWS 24.1:1044-1045](etc/NEWS); [variables.texi:1047-1049](doc/lispref/variables.texi). This is a genuine Tier R API: code can call `(eval FORM t)`. XEmacs's `Feval` has no such argument ([`src/eval.c:3729`](src/eval.c)).

---

## 3. Mutable capture — the cons-cell box

The interpreter path already showed the mechanism: a lexical binding is a heap cons `(var . val)`, captured *by reference* because the closure stores the env list itself and `setq` does `XSETCDR` on that cons ([`src/eval.c:470`](src/eval.c), [`src/eval.c:1035`](src/eval.c)). Multiple closures made in one `let` share the same cons, and `my-ticker` increments survive because each call mutates that shared cell — the manual's canonical example ([variables.texi:982-1013](doc/lispref/variables.texi)).

The **byte-compiler** has no mutable env alist, so `cconv.el` reproduces the sharing by **boxing**:

```
;; If a variable is mutated (updated by setq), and it is used in a closure
;; we wrap its definition with list: (list val) and we also replace
;; var => (car var) wherever this variable is used, and also
;; (setq var value) => (setcar var value) where it is updated.
```

([`lisp/emacs-lisp/cconv.el:56-64`](lisp/emacs-lisp/cconv.el)). A `defun` argument that is closure-mutated is `let`-bound and boxed the same way ([`cconv.el:61-64`](lisp/emacs-lisp/cconv.el)).

`cconv-convert`'s env entries name the three capture shapes explicitly ([`cconv.el:243-250`](lisp/emacs-lisp/cconv.el)):

```
 (VAR . (car EXP)): VAR has been moved into the car of a cons-cell, and EXP
    is an expression that evaluates to this cons-cell.            <-- MUTABLE
 (VAR . (internal-get-closed-var N)): VAR has been copied into the closure
    environment's Nth slot.                                       <-- IMMUTABLE
 (VAR . (apply-partially F ARG1 ARG2 ..)): VAR has been λ-lifted.  <-- functions
```

So the distinction is:

- **Immutable capture** — the value is *copied* into the closure's constant vector; reads become `internal-get-closed-var N`, which `bytecomp` lowers to `byte-constant N` ([`bytecomp.el:3369-3372`](lisp/emacs-lisp/bytecomp.el)). No box; the closed-over value cannot be assigned through.
- **Mutable capture** — the variable is wrapped in a one-element cons `(list val)`; the *cell* (not the value) is copied into the closure, reads compile to `(car cell)` and writes to `(setcar cell val)` ([`cconv.el:455`](lisp/emacs-lisp/cconv.el), [`cconv.el:313-325`](lisp/emacs-lisp/cconv.el)). Because the cons is built once by the enclosing `let` and shared by every closure that captures it, `setcar` mutation is visible everywhere — identical observable behaviour to the interpreter's `XSETCDR`.

`byte-compile-make-closure` materialises this: a closure becomes a `make-byte-code` object whose constants vector is `(vconcat (vector . ENV) CONSTS)` — the captured environment (boxes and copied values alike) prepended ahead of the body's own constants ([`bytecomp.el:3355-3367`](lisp/emacs-lisp/bytecomp.el)).

`master` keeps the same model (still cons-cell boxing; the analysis/rewrite is larger at ~977 lines vs 705, and 28.1 added a `make-closure` bytecode function to build closures more efficiently than the raw `make-byte-code` trick — [etc/NEWS (28.1):3593-3596](etc/NEWS)).

---

## 4. Local function bindings

### `function`/`#'` vs `'(lambda …)`

The manual is explicit ([functions.texi *Anonymous Functions*:913-995](doc/lispref/functions.texi)):

- The `lambda` *macro* makes `(lambda …)` self-quoting **and** routes it through `function`, telling the evaluator/compiler "this is a function" ([functions.texi:913-927](doc/lispref/functions.texi)).
- `function` (and its `#'` read syntax) is the note that (a) byte-compiles the object and (b) **converts it into a closure under lexical binding** ([functions.texi:929-957](doc/lispref/functions.texi)).
- A **quoted** `'(lambda …)` (or any list built at run time) is *kept as a lambda list in the compiled code* and is **not** compiled and **not** closed over, "since it does not know that [the callee] intends to use it as a function" ([functions.texi:984-995](doc/lispref/functions.texi)). This asymmetry is the reason `#'f`-vs-`'(…)` matters and why a subset that silently closes over every list or none of them diverges from GNU.

At 24.1 `((lambda …) …)` direct-call in operator position was already deprecated ([etc/NEWS 24.1:1058](etc/NEWS)).

### `cl-flet` / `cl-labels`

These did **not exist in 24.1** — only the old CL `flet` (dynamic) and `labels` (built on `lexical-let`; [`lisp/emacs-lisp/cl-macs.el:1408-1430`](lisp/emacs-lisp/cl-macs.el)). `cl-flet`/`cl-labels` arrived in 24.4, and Emacs recorded the semantic dependency directly in the ticket's language:

```
*** 'cl-labels' is slightly different from 'labels'.
The difference is that it relies on the 'lexical-binding' machinery
(as opposed to the 'lexical-let' machinery used previously) to capture
definitions in closures, so such closures will only work if 'lexical-binding'
is in use.
```

([etc/NEWS.24 (24.4):1654-1658](etc/NEWS)). Read in `master` (`cl-macs.el`):

- **`cl-flet`** ([`lisp/emacs-lisp/cl-macs.el:2074-2134`](lisp/emacs-lisp/cl-macs.el)): each binding is turned into a **lexical variable** holding the function value, the body is wrapped with `macroexp-let*`, and calls are rewritten to `(funcall ,var …)` (line 2122-2129). It is explicitly **non-recursive** ("you can't write recursive function definitions. Use `cl-labels' for that", line 2082-2083). To rewrite `(function F)` references it installs a `function` expander into the macro-expansion environment (`macroexpand-all-environment`, `cl--labels-convert`, lines 2043-2071, 2133-2134).
- **`cl-labels`** ([`cl-macs.el:2281`](lisp/emacs-lisp/cl-macs.el)): same shape, but the function is captured as a **closure**, and recursion works only because §3's mutable capture lets the closure refer to the still-being-built binding. That is `lexical-binding` machinery proper, not `lexical-let`.

### Can a lexical-binding implementation omit function bindings?

Yes — **as a distinct runtime primitive**, because GNU has none. `internal-interpreter-environment` stores only *variable* bindings and dynamic proclamations ([`src/eval.c:3748-3755`](src/eval.c)); there is no function-binding slot. `cl-flet`/`cl-labels` reduce to Tier R: a lexical variable holding a function value, plus a closure, plus (for `cl-labels`) mutable capture. So a subset that implements §1–§3 faithfully gets local function bindings *for free*, provided XEmacs ships `cl-flet`/`cl-labels` macro definitions that expand to those primitives. What the subset **cannot** omit is the Tier R substrate they sit on: without real closures + mutable capture, a `cl-labels`-style recursion silently breaks — the exact failure Emacs itself flagged when moving off `lexical-let`, and the exact situation XEmacs is in, where `cl-lib.el`'s `cl-labels` falls back to dynamic `labels` and warns ([`lisp/cl-lib.el:416-425`](lisp/cl-lib.el)).

---

## 5. The macro-expansion environment

### `macroexpand`'s `ENVIRONMENT`

Both 24.1 and `master` describe the C primitive's `ENVIRONMENT` as a list of **macro definitions**, not lexical variables:

```
The second optional arg ENVIRONMENT specifies an environment of macro
definitions to shadow the loaded ones for use in file byte-compilation.
```

([`src/eval.c:1149-1156`](src/eval.c) at 24.1; unchanged in [`master:1283-1290`](src/eval.c)). The implementation confirms it: `Fmacroexpand` uses the env only via `Fassq (sym, environment)` to shadow a symbol's macro expander ([`src/eval.c:1177`](src/eval.c)); it never consults `Vinternal_interpreter_environment`. The manual adds "Byte compilation uses this feature" ([macros.texi:93-136](doc/lispref/macros.texi)), and `macroexpand-all` takes the same optional env ([macros.texi:139-154](doc/lispref/macros.texi)).

So at the C-primitive level, the expansion environment is about *which macros are visible during expansion* — a compile/expand-time concern, consumed entirely before the runtime sees anything.

### `&environment` and the `macroexp-*` helpers

`&environment` is a CL keyword in a macro's lambda list ([`cl-macs.el:229`](lisp/emacs-lisp/cl-macs.el), processed at [`cl-macs.el:288`](lisp/emacs-lisp/cl-macs.el), error-checked at [`cl-macs.el:331`](lisp/emacs-lisp/cl-macs.el)); XEmacs's own `lexical-let` already uses it ([`lisp/cl-macs.el:1707`](lisp/cl-macs.el)). It hands the macro the current expansion environment so the macro can thread it into recursive expansion. The supporting machinery is `macroexpand-all-environment` ([`master: lisp/emacs-lisp/macroexp.el:48`](lisp/emacs-lisp/macroexp.el)) and the `macroexp-*` helpers (`macroexp-let*`, `macroexp-parse-body`, `macroexp-copyable-p`) that `cl-flet`/`cl-labels` call (§4). Over time this env was *enriched* to also carry lexical-variable/symbol-macro information, which is what `cl-symbol-macrolet`, the `gv`/place macros (`push`, `pop`, `cl-incf`), `pcase-let`, and `cl-function` read during expansion. That enrichment is a `master`-era reality; at 24.1 the env passed around was essentially the macro-definition shadowing alist.

### Which packages need it

Only **macro-authoring** packages, and only while XEmacs expands their (or CL's) macros:

- Packages that merely *call* `cl-incf`/`cl-labels`/`push`/`pcase` need XEmacs's copy of those macros to expand correctly — and, once expanded, the result exercises only Tier R (§1–§4). They do **not** need any env object to survive into runtime.
- Packages that *define* their own `&environment`-aware or lexical-scope-introspecting macros (advanced macro libraries — the `use-package` family, `dash`-style combinators that generate closures, `pcase` extensibility) are the real consumers, because XEmacs must run their expansion with an env that reflects the surrounding lexical bindings.

This is why the macro-expansion environment lands in **Tier M** (weakest requirement) in the ranking: it is entirely an expand-time concern, and the runtime never observes it. A subset that ships correct, non-env-sensitive macro definitions (hand-written to avoid `&environment`) can load ordinary packages without providing an expansion env at all.

---

## 6. Division of labour: loader, interpreter, compiler, VM — and what is observable where

### Loader (`lread.c`) — decides the *mode*, per file/buffer

- `lisp_file_lexically_bound_p` hand-scans the **first line** only, looking for `lexical-binding:` inside `-*- … -*-` markers; any value except the literal string `nil` enables it (`rv = (strcmp (val, "nil") != 0)`, [`src/lread.c:872-876`](src/lread.c); whole function [`src/lread.c:782-884`](src/lread.c)). It is a first-line-only, comment-only scanner ([variables.texi:1042-1043](doc/lispref/variables.texi) — "this one must be set in the first line of a file"). `master` keeps the same first-line rule ([`master: src/lread.c:754,870`](src/lread.c)).
- `Fload` binds `lexical-binding` to `nil` for the entire load ([`src/lread.c:1172`](src/lread.c)), then, on the source path, sets it to `t` if the header said so ([`src/lread.c:1311-1312`](src/lread.c)).
- `readevalloop` is the pivot: it reads the current *dynamic* value `find_symbol_value (Qlexical_binding)` and binds `internal-interpreter-environment` to `nil` (dynamic) or `(t)` (empty lexical env) accordingly ([`src/lread.c:1731-1734`](src/lread.c)). `eval-buffer` first specbinds `lexical-binding` from the buffer's header line, then calls `readevalloop` ([`src/lread.c:1897`](src/lread.c)) — so the header is honoured for `M-x eval-buffer`/`load`, not just for `byte-compile-file`.
- `lexical-binding` is declared a normal buffer-local variable, default `nil` ([`src/lread.c:4590-4599`](src/lread.c)).
- **Deltas.** 27.1: interactive `M-:`/`--eval`/`*scratch*`/`ielm` now use lexical binding by default ([etc/NEWS.27 (27.1):365-373](etc/NEWS)). 28.1: a file's `lexical-binding` cookie is honoured even when `enable-local-variables` is nil (new `permanently-enabled-local-variables` to opt out), [`etc/NEWS.28 (28.1):2902-2905`](etc/NEWS); and `eval-buffer`'s path switched to `local_variable-p`/`get_lexical_binding` ([`master: src/lread.c:2410-2413`](src/lread.c)). None of these change Tier R semantics; they widen *where* the loader turns the mode on.

### Interpreter (`eval.c`) — the runtime guarantee (Tier R)

Covered in §2: `internal-interpreter-environment` (nil / `(t)` / mixed alist), `eval_sub` lexical-first lookup, `let`/`let*` binding with the special/proclaimed escape, `setq` `XSETCDR`, `function`→`(closure ENV …)`, `funcall_lambda` binding args into the captured env, `eval`'s `lexical` arg, and the `data.c` primitives that ignore the env. This layer is **fully runtime-observable** — it is what an interpreted `lexical-binding: t` package runs against.

### Byte compiler (`bytecomp.el` + `cconv.el`) — the compile-time realisation (Tier C)

`bytecomp.el` is itself a lexical file ([`bytecomp.el:1`](lisp/emacs-lisp/bytecomp.el)); it reads `lexical-binding` and runs `cconv-closure-convert` before code generation. The compiler-specific parts: `byte-compile--lexical-environment` maps a var to a **stack slot**; refs/sets emit `byte-stack-ref`/`byte-stack-set` ([`bytecomp.el:3055-3085`](lisp/emacs-lisp/bytecomp.el)); `byte-compile-not-lexical-var-p` decides lexical vs dynamic using `special-variable-p` *and* `byte-compile-bound-variables` *and* `nil`/`t`/keywords ([`bytecomp.el:3849-3854`](lisp/emacs-lisp/bytecomp.el)); `byte-compile-bind`/`byte-compile-unbind` juggle the value stack and emit `byte-unbind`/`discardN` ([`bytecomp.el:3856-3907`](lisp/emacs-lisp/bytecomp.el)); closures become `make-byte-code` with the env prepended to constants ([`bytecomp.el:3355-3367`](lisp/emacs-lisp/bytecomp.el)); free-variable/unused-lexical warnings and the proclamation set live here too ([`bytecomp.el:3067,3083`](lisp/emacs-lisp/bytecomp.el), [`bytecomp.el:2243-2248`](lisp/emacs-lisp/bytecomp.el)). All of this is **only** observable to someone who byte-compiles; run the same `.el` interpreted and none of it is exercised.

### VM (`bytecode.c`) — compiled access only (Tier C)

Four opcodes added for lexical binding: `Bstack_ref` (code 0), `Bstack_set` (0262), `Bstack_set2` (0263), `BdiscardN` (0266) ([`src/bytecode.c:86,233-235`](src/bytecode.c); compiler-side `byte-defop`s at [`bytecomp.el:515,672-684`](lisp/emacs-lisp/bytecomp.el)). Their handlers ([`src/bytecode.c:1762-1805`](src/bytecode.c)):

```c
case Bstack_ref+1 .. +5: { Lisp_Object *ptr = top - (op - Bstack_ref); PUSH (*ptr); }
case Bstack_set / Bstack_set2: { Lisp_Object *ptr = top - (FETCH/FETCH2); *ptr = POP; }
case BdiscardN: op = FETCH; if (op & 0x80) { op &= 0x7F; top[-op] = TOP; } DISCARD (op);
```

`stack-ref` reads a lexical var from the value stack, `stack-set`/`stack-set2` writes TOS into a stack slot (assignment), `discardN` pops N slots with a "preserve-tos" high-bit variant used by `let` unbinding. They are distinct from the *dynamic* `Bvarref`/`Bvarset`, which go through the symbol value cell ([`src/bytecode.c:570-585,681-694`](src/bytecode.c)). This whole layer is observable only when running a `.elc` produced by the Tier C compiler.

### The observable-to-whom table

| Guarantee | Layer | Observable to an ordinary *runtime* package? |
|---|---|---|
| defvar/defconst special exemption | eval.c (`let`/`setq`/`function`) | **Yes** (Tier R) |
| Lexical var bindings, `let`/args | eval.c | **Yes** (Tier R) |
| Closures, indefinite extent | eval.c (`function`/`funcall_lambda`) | **Yes** (Tier R) |
| Mutable capture (`setq` visible through closure) | eval.c `XSETCDR` / cconv box | **Yes** (Tier R) |
| `(eval FORM t)` | eval.c `Feval` | **Yes** (Tier R) |
| `special-variable-p` | eval.c | **Yes** (Tier R) |
| Local function bindings (`cl-flet`/`cl-labels`) | macro → eval.c env + cconv | **Yes**, but *derived* from Tier R; needs no new runtime primitive (§4) |
| Header scan, per-file mode switch | lread.c | Loader-internal; package sees only its *effect* (the mode is on) |
| `stack-ref/set/set2/discardN`, `cconv`, closure-in-`.elc`, free-var warnings | bytecomp.c/bytecode.c | **No** — only if you byte-compile (Tier C) |
| `macroexpand` ENV / `&environment` / `macroexp-*` | macroexp.el/cl-macs | **No** — expand-time only, and only for env-sensitive *macro authoring* (Tier M) |

The payload for the subset decision: an interpreter-only subset that nails Tier R (the first six rows plus the proclamation asymmetry in §1) is *semantically* correct for source-loaded lexical packages, and only loses the Tier C optimizations/diagnostics and the Tier M expansion-env generality. Conversely, Tier R is not omittable: get special-variable exemption or mutable capture wrong and *ordinary* packages silently misbehave.

---

## 7. What surprises a dynamically-scoped host

### The guard `(and (boundp 'lexical-binding) lexical-binding)`

XEmacs carries this guard at [`lisp/cl-lib.el:418`](lisp/cl-lib.el) (inside its `cl-labels` back-fill, lines 416-425): when it is false, `cl-labels` warns "This `cl-labels' requires `lexical-binding' to be non-nil" (line 424) and expands to plain dynamic `labels`. In GNU the same expression is the standard "am I being lexically bound right now?" idiom, and it is reliable because of **two loader properties**, not because the variable merely exists:

1. **`lexical-binding` is always bound and buffer-local**, default `nil` ([`src/lread.c:4590-4599`](src/lread.c)), so `(boundp 'lexical-binding)` is `t` and the value is meaningful.
2. **The loader *dynamically rebinds* it to the file's setting around evaluation** — `Fload` specbinds it to `nil`, then to `t` for a header-declaring file ([`src/lread.c:1172,1311-1312`](src/lread.c)); `readevalloop` reads it via `find_symbol_value` (the *dynamic* value cell) to pick the interpreter env ([`src/lread.c:1731`](src/lread.c)); `eval-buffer` specbinds it from the buffer's header ([`src/lread.c:1897`](src/lread.c)). Macroexpansion of a `cl-labels` call happens *inside* that dynamic binding, so the guard's value reflects the enclosing file's mode at the moment of expansion.

Note the self-referential subtlety that makes this work: **the flag that controls the binding mode is itself exempt from that mode** — it is read through the dynamic value cell (`find_symbol_value`, `specbind`), never captured lexically. Same design in `master` ([`src/lread.c:1323,1521,2216-2217,2410-2413`](src/lread.c)).

Consequences for XEmacs:

- **Merely `(defvar lexical-binding nil)` to satisfy `boundp` is safe but permanently false.** `boundp`→`t`, value→`nil`, so the guard is `nil` and `cl-labels` always takes the warning branch. Benign for this one guard — the package still runs as dynamic code — but it means the guard can *never* correctly report lexical mode, so every such test silently picks the dynamic path even for genuinely-lexical source. The shim makes the *message* fire, not a crash.
- **Defining `lexical-binding` to `t` globally as a "we support it" shim is actively wrong.** The guard then reads `t`, `cl-labels` omits its warning and emits real lexical closures (per §4 and [etc/NEWS 24.4](etc/NEWS)), which a dynamically-scoped host cannot back — a silent semantic break, worse than the message. GNU relies on the value being the *loader's per-file dynamic binding*, not a fixed global; a constant global `t` is a category error.
- The guard's premise ("the *existence* of the variable signals support") is itself unreliable even in GNU — the `cl-lib` comment XEmacs inherited already hedges that `lexical-binding`'s value at macro-expansion "does not necessarily reflect faithfully whether the output of this macro will be interpreted as lexically bound code or not" ([`lisp/cl-lib.el:419-423`](lisp/cl-lib.el)). The correct host-side guarantee is the loader's dynamic per-file binding, which a bare `defvar` cannot imitate.

### Other GNU behaviours a dynamic host would not expect

- **`defvar` sets a process-wide bit.** Because `declared_special` persists on the symbol ([`src/eval.c:827,913`](src/eval.c)), loading *any* lexical file that `defvar`s a name changes how *every later file* binds that name — the special marking is global and sticky, unlike XEmacs's file-local `lexical-let` gensyms ([`lisp/cl-macs.el:1705-1745`](lisp/cl-macs.el)). A subset needs a dump-aware symbol bit (per the prior feasibility report).
- **Valueless `(defvar foo)` is scope-local, not global.** Its proclamation mutates the transient env alist ([`src/eval.c:868-874`](src/eval.c)), so it is visible to later `let`/`setq` in the *same file/block* but leaves `special-variable-p` returning `nil` (§1). A host that treats all `defvar` forms alike gets this backwards in both directions.
- **`let`-binding an undeclared symbol is lexical, and invisible to `boundp`/`symbol-value`.** A dynamically-scoped host has no notion of a binding that `boundp` cannot see ([`src/data.c:1049-1060`](src/data.c), [variables.texi:1015-1017](doc/lispref/variables.texi)) — the classic "free variable" breakage of §1.
- **`#'(lambda …)` vs `'(lambda …)` changes compilation/closure.** A dynamic host treats the two identically; GNU does not ([functions.texi:984-995](doc/lispref/functions.texi)), so any code relying on that distinction behaves differently once the mode is on.
- **Interactive/`eval` defaults moved.** `M-:`, `--eval`, `*scratch*`, and `ielm` evaluate lexically by default since 27.1 ([etc/NEWS.27 (27.1):365-373](etc/NEWS)), which surprises dynamic hosts that treat the `*scratch*`/eval path as a no-mode zone.
- **`.elc` portability.** Compiled lexical code embeds the captured env in the constants vector and uses the new VM opcodes (§3, §6), so byte-compiled lexical `.elc` files are not loadable by a dynamic host at all — the compatibility constraint the prior feasibility report tabulated under ".elc compatibility".

---

## Sources

GNU Emacs 24.1 (tag `emacs-24.1`), unless noted `master`:

- Manual: [`doc/lispref/variables.texi`](doc/lispref/variables.texi) (Variable Scoping 761-806; Lexical Binding 924-1026; special-variable-p 1059-1064; Using Lexical Binding 1028-1090), [`doc/lispref/functions.texi`](doc/lispref/functions.texi) (Anonymous Functions 894-996; Closures 1109-1144; Declaring Functions 1253-1333), [`doc/lispref/macros.texi`](doc/lispref/macros.texi) (Expansion 58-155; Surprising Local Vars 430-497).
- Interpreter/VM/loader/symbols C: [`src/eval.c`](src/eval.c), [`src/data.c`](src/data.c), [`src/lread.c`](src/lread.c), [`src/bytecode.c`](src/bytecode.c), [`src/lisp.h`](src/lisp.h).
- Compiler: [`lisp/emacs-lisp/cconv.el`](lisp/emacs-lisp/cconv.el), [`lisp/emacs-lisp/bytecomp.el`](lisp/emacs-lisp/bytecomp.el), [`lisp/emacs-lisp/macroexp.el`](lisp/emacs-lisp/macroexp.el) (master), [`lisp/emacs-lisp/cl-macs.el`](lisp/emacs-lisp/cl-macs.el) (both refs).
- Release notes: [`etc/NEWS`](etc/NEWS) (24.1) 1038-1058; [`etc/NEWS.24`](etc/NEWS) 1654-1660 (24.4 `cl-labels`); [`etc/NEWS.27`](etc/NEWS) 365-375 (27.1 interactive); [`etc/NEWS.28`](etc/NEWS) 2902-2907 & 3593-3599 (28.1).

Local XEmacs tree: [`lisp/cl-lib.el:416-425`](lisp/cl-lib.el), [`lisp/cl-macs.el:1705-1745`](lisp/cl-macs.el), [`src/eval.c:3729`](src/eval.c) (`Feval` arity), [`src/lisp.h:2847-2872`](src/lisp.h) (`Lisp_Symbol`, no special bit).

Secondary, cited not re-derived: prior feasibility report `docs/research/lexical-binding.md`. Monnier & Sperber, "Evolution of Emacs Lisp," *PACMPL* 4 (HOPL) Art. 74 (2020), DOI [10.1145/3386324](https://doi.org/10.1145/3386324) — for the lexbind/cconv history referenced in §6.
