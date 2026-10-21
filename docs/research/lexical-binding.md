# Lexical binding in XEmacs: feasibility and cost

Research findings for wayfinder ticket [#15](https://github.com/fktpp2022/xemacs-git/issues/15). This report assesses feasibility and cost; it does not choose a strategy.

## Bottom line

- XEmacs has no lexical binding at any layer. `lexical-let` is a macro-level workaround, not a file mode.
- A complete implementation spans loader, evaluator, symbol representation, byte compiler with closure conversion, bytecode VM, and `.elc` compatibility. That is release-scale work, not first-milestone work.
- The smallest *real* subset is interpreter-only lexical binding, but it still needs most runtime pieces and forces affected packages to run as source.
- For the first ELPA milestone, the only genuinely small option is restricting the package set to code that does not depend on lexical capture, backed by an audit. That is a package-selection strategy, not lexical-binding support.
- A source translator or a no-op compatibility shim is not a cheap substitute.

## How GNU Emacs 24.1 implements it

GNU Emacs 24.1 (tag `emacs-24.1`) is the reference implementation. Lexical binding is opt-in per file via `-*- lexical-binding: t -*-`; dynamic binding remains the default ([etc/NEWS:1038-1056](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/etc/NEWS)).

**Loader and reader.** `lisp_file_lexically_bound_p` scans the first line's file-local variables ([src/lread.c:783-886](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/lread.c)). `load` binds `lexical-binding` to nil by default (`lread.c:1168-1172`) and sets it to t when the header requests it (`1310-1312`). `readevalloop` then binds `internal-interpreter-environment` to nil or to `(t)`, the empty lexical environment (`1728-1734`); `eval-buffer` does the same (`1897`). The variable is buffer-local (`4590-4599`).

**Evaluator and closures.** `internal-interpreter-environment` is either nil (dynamic mode), `(t)` (empty lexical environment), or an alist of active bindings ([src/eval.c:3748-3763](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/eval.c)). `eval_sub` looks a symbol up in that alist before calling `Fsymbol_value` (`eval.c:2220-2233`). `let` and `let*` extend the alist and `specbind` the environment variable (`1007-1045`, `1095-1118`); `setq` updates an existing lexical binding and otherwise falls back to `Fset` (`466-472`). `(function (lambda ...))` returns `(closure ENV ARGS . BODY)` in lexical mode (`512-518`), and `funcall_lambda` recognizes closures and binds arguments into the saved environment (`3134-3143`, `3207-3223`). `defvar`/`defconst` set a `declared_special` bit (`868-874`, `913`), exposed by the new `special-variable-p` (`3477-3485`); the bit lives in `struct Lisp_Symbol` ([src/lisp.h:1113-1135](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/lisp.h)).

**Byte compiler and VM.** The compiler file is itself lexical ([lisp/emacs-lisp/bytecomp.el:1](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/bytecomp.el)); it reads `lexical-binding` through `normal-mode` (`1740-1747`) and runs `cconv-closure-convert` before code generation (`2170-2180`). `cconv.el` is a 705-line closure-conversion pass that analyzes free and mutated variables, lambda-lifts, and uses cons cells for mutable captures (`27-64`, `135-150`, `189-237`). Lexical references and assignments compile to stack operations (`3055-3085`); `let` distinguishes lexical from dynamic bindings (`3909-3939`); `special-variable-p` excludes specials (`3849-3854`); closures become `make-byte-code` objects with the captured environment prepended to constants (`3352-3367`). The VM gained `stack-ref`, `stack-set`, `stack-set2`, and `discardN` opcodes ([src/bytecode.c:86-90, 1761-1805](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/bytecode.c)).

**History.** Miles Bader's 2001 `lexbind` branch chose the per-file two-dialect design; interpreter changes looked minor, but byte-compiler changes were "more problematic" and were never finished on that branch. Igor Kuzmin's 2010 closure-conversion pass (supervised by Stefan Monnier) separated conversion from code generation and made the feature tractable; Emacs 24.1 shipped two years later. As of Emacs 26 only about half of well-maintained third-party packages had converted ([HOPL paper, main.tex:2588-2670](https://raw.githubusercontent.com/include-yy/hopl-4-emacs-lisp/master/main.tex); DOI [10.1145/3386324](https://doi.org/10.1145/3386324)).

## What XEmacs would need

| Component | XEmacs today | Required for lexical binding |
|---|---|---|
| Reader | Already reads `#'` and `#:` (`src/lread.c:2548-2565`) | Nothing for syntax; only recognize the file variable |
| Loader | `Fload_internal` calls `readevalloop` with no header scan (`src/lread.c:719`, `1364-1419`; `eval-buffer`/`eval-region` at `1469`, `1526`) | Scan first line, bind `lexical-binding`, initialize the interpreter environment |
| `let`/`let*` | Every binding is `specbind` (`src/eval.c:1096-1165`, `1158`) | Lexical alist plus a special-variable escape |
| `setq` | Writes the symbol value cell | Update an existing lexical binding first |
| `function`/`lambda` | Returns the raw lambda (`src/eval.c:1298-1324`) | Closure object |
| Function calls | `funcall_lambda` specbinds every argument (`src/eval.c:4459-4509`) | Recognize closures; bind into their environment |
| `defvar`/`defconst` | Set value and doc only (`src/eval.c:1397-1452`, `1454-1496`); no `special-variable-p` | Mark variables special; expose the predicate |
| Symbol | `name`, `value`, `function`, `plist`, no special bit (`src/lisp.h:2847-2872`) | Add a dump-aware special flag |
| Byte compiler | No lexical environment; emits `byte-varref`/`byte-varset`/`byte-varbind` (`lisp/bytecomp.el:3347-3409`); `let`/`let*` emit `byte-varbind`/`byte-unbind` (`4501-4549`); lambda compiler only tracks warnings (`2938-3035`) | Lexical environment, closure conversion, stack code generation |
| VM and `.elc` | Symbol/specpdl-based ops (`src/bytecode.c:943-1053`); compiled functions have no environment slot (`src/bytecode.h:51-80`); args pre-bound (`src/eval.c:3633-3719`) | Stack opcodes, closure environment in compiled functions, `.elc` version handling (`src/bytecode.c:691-717`) |

The compiler already records the gap: "Should implement declarations and proclamations, notably special" (`lisp/bytecomp.el:209-210`) and a "kludge added to XEmacs to work around the bogosities of a nonlexical lisp" (`3344`). `lisp/byte-optimize.el:131-151` contains an older proposal for a file-local `(lexical-scope)` proclamation and notes the loader and `defvar`-tracking problems. `lexical-let` (`lisp/cl-macs.el:1707-1745`) performs local closure conversion with gensyms, but is per-binding and cannot change file semantics; `lisp/files.el:892-896` notes that its closures "will always run as interpreted code."

## Prior art

- GNU Emacs `lexbind`/`cconv` is the only complete implementation found.
- SXEmacs, the main XEmacs fork (`njsf/sxemacs`, master last pushed 2015-06-09), also lacks lexical binding: `let` still `specbind`s (`src/eval.c:815`, `871`), `funcall_lambda` is dynamic (`3010`), and only `lexical-let` exists (`lisp/cl-macs.el:1666`).
- Matthias Neubauer's 2001 analysis proposed mechanically converting dynamic-scope packages while preserving semantics, but the project was never completed or productized (HOPL `main.tex:2578-2586`).

## Options and cost

1. **Restrict the first package set to non-lexical code.** Cost: per-package audit and version pinning; no core changes. Risk: of five current GNU ELPA packages sampled, all declare `lexical-binding: t` on line 1 (`seq 2.24`, `let-alist 1.0.6`, `map 3.3.1`, `dash 2.20.0`, `compat 31.1.0.0`). A declaration is not proof of reliance, but the candidate pool becomes small and audit-heavy.
2. **Compatibility shim.** Defining `lexical-binding` and ignoring it helps only code that never captures a lexical variable; closures silently change semantics. Tiny cost, false compatibility, high risk.
3. **Source translator to `lexical-let`/explicit environments.** Requires closure conversion and special-variable analysis; no productized tool exists, and Neubauer's effort was never finished. Large cost, high risk.
4. **Interpreter-only lexical binding.** Needs the loader scan, symbol special bit, closure object, and changes to `let`, `setq`, `function`, `eval`, `funcall_lambda`, `defvar`/`defconst`. Medium-large. XEmacs bytecode cannot express lexical access, so affected packages must load as source, with startup and run-time cost. This is the smallest *real* implementation.
5. **Full support.** Adds a `cconv`-style pass (GNU's is 705 lines plus compiler integration), new VM opcodes, environments in compiled functions, and `.elc` versioning. Release-scale; the GNU history shows the compiler, not the interpreter, was the hard part.

## Feasibility

Technically feasible: the reader already has the syntax, and GNU's design is a proven reference. The cost is coordinated runtime, compiler, and bytecode work rather than one isolated patch; partial implementations risk fragmenting XEmacs Lisp into dialects. For the first ELPA milestone, both full and interpreter-only lexical binding are larger than the milestone itself. Restricting the package set is the only small option, at the price of a much smaller candidate pool. Choosing among these is a product decision for the map.

## Evidence

A local XEmacs 21.5 beta36 build ignored a `-*- lexical-binding: t; -*-` header: `lexical-binding` was unbound and a closure failed with `(void-variable n)`. The equivalent `lexical-let` closure returned 1, then 2, both interpreted and byte-compiled, confirming that only the macro-level workaround exists.

## Sources

- GNU Emacs 24.1: [lread.c](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/lread.c), [eval.c](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/eval.c), [lisp.h](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/lisp.h), [bytecode.c](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/src/bytecode.c), [bytecomp.el](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/bytecomp.el), [cconv.el](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/lisp/emacs-lisp/cconv.el), [NEWS](https://raw.githubusercontent.com/emacs-mirror/emacs/emacs-24.1/etc/NEWS).
- Monnier, S. and Sperber, M. "Evolution of Emacs Lisp." *Proc. ACM Program. Lang.* 4, HOPL, Article 74 (2020). DOI [10.1145/3386324](https://doi.org/10.1145/3386324); [author LaTeX source](https://raw.githubusercontent.com/include-yy/hopl-4-emacs-lisp/master/main.tex).
- SXEmacs mirror: [njsf/sxemacs](https://github.com/njsf/sxemacs).
- GNU ELPA: [seq](https://elpa.gnu.org/packages/seq.html), [let-alist](https://elpa.gnu.org/packages/let-alist.html), [map](https://elpa.gnu.org/packages/map.html), [dash](https://elpa.gnu.org/packages/dash.html), [compat](https://elpa.gnu.org/packages/compat.html).
- Local XEmacs tree: `src/lread.c`, `src/eval.c`, `src/lisp.h`, `src/bytecode.c`, `src/bytecode.h`, `lisp/bytecomp.el`, `lisp/cl-macs.el`, `lisp/files.el`, `lisp/byte-optimize.el` (line references above).
