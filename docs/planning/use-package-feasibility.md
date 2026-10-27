# Research #64: Does upstream `use-package` + `bind-key` load on XEmacs?

**Branch:** `research/use-package-feasibility` (local, not pushed) · **Verdict:** Not as-is (hard failure). But **P3 (unmodified + shim) is viable**; `package.el` is not load-coupled. Determines ticket #65.

## Sources examined

- GitHub `jwiegley/use-package` HEAD `2f09ce19` (2025-08-23, "Note archiving of the repository") — `/tmp/use-package-research/use-package/`.
- GNU ELPA release **2.4.6** (dir dated 2024-08-21) — `https://elpa.gnu.org/packages/use-package.tar`.
- `bind-key` standalone repo is **gone** (`jwiegley/bind-key` → 404); `bind-key.el` (v2.4.1) ships in the `use-package` repo.
- Require structure byte-identical across both. **Citations are against GitHub HEAD.**
- Tested against the pre-built binary `/home/fankai/projects/xemacs/install/bin/xemacs` (21.5 beta36 "leeks") via `featurep`/`fboundp` probes and real `byte-compile-file`.

## `(require …)` inventory vs XEmacs 21.5-b36

**`use-package-core.el`**

| Line | Require | XEmacs | |
|---|---|---|---|
| L32 | `bytecomp` | ✅ | |
| L33 | `cl-lib` | ✅ (vendored 0.7.1) | |
| L34 | `tabulated-list` | ❌ | **HARD FAIL at compile/load** ("Cannot open load file: tabulated-list") |
| L57 | `subr-x` (in `eval-when-compile`, else-branch of emacs-24.3 check; fires since major=21) | ❌ | |
| L60 | `regexp-opt` (`eval-when-compile`) | ✅ | |

Non-require top-level hazards: L38 `(deftheme …)` ✅; **L42 `(enable-theme 'use-package)` ❌ unbound**; **L43 `(setq custom-enabled-themes …)` ❌ unbound var**. And **`macroexp-progn` used at L438, L1214, L1393, L1426, L1448, L1602, L1679 ❌** — XEmacs has **no `macroexp` feature** (`(require 'macroexp)` fails; `macroexp-progn` unbound). This **contradicts the milestone-2 compat inventory**, which listed `macroexp` as provided.

**`bind-key.el`**

| Line | Require | XEmacs | |
|---|---|---|---|
| L103 | `cl-lib` | ✅ | |
| L104 | `easy-mmode` | ✅ | |

But uses **`macroexp-progn`** (L371, L374, L440, L449) ❌ and **`cl-flet`** (L366) ❌ — `cl-flet` was added in cl-lib 1.0; 0.7.1 lacks it. As-is it **byte-compiles with warnings only** because both are macro-time references, not compile-time calls — but fails at macro expansion.

## Gap classification

| # | Gap | Class | Fix |
|---|---|---|---|
| 1 | `tabulated-list` (L34) | **(b) source patch** | Guard the require + `use-package-statistics-mode` body (L1051–L1095, display-only) with `(when (fboundp 'tabulated-list-mode) …)`. Or (a) a stub `tabulated-list.el`. |
| 2 | `subr-x` (L57) | **(a) extend compat** | One-liner: `(provide 'subr-x)` + `hash-table-keys` (maphash collect) + `string-suffix-p` (compare-strings). **Only `hash-table-keys` is actually called** (L1053); `when-let*`/`if-let*`/`string-trim` are **not used anywhere**. |
| 3 | `enable-theme`, `custom-enabled-themes` (L42–43) | **(a) extend compat** | `enable-theme` → no-op `(defun enable-theme (t) t)`; `custom-enabled-themes` → `(defvar … nil)`. |
| 4 | `macroexp-progn` (core ×7, bind-key ×4) | **(a) extend compat** | One-liner macro: `(defmacro macroexp-progn (exps) (cons 'progn exps))`. **New finding: `macroexp` is absent, not present.** |
| 5 | `cl-flet` (bind-key L366) | **(a)/(b)** | cl-lib 0.7.1 lacks it; lexically-scoped, hard on dynamic-binding XEmacs. Cheapest: stub expanding to dynamic `flet` (bind-key's single use is benign), or patch to `flet`/inline. |
| 6 | `font-lock-add-keywords` (core L336), `user-emacs-directory`, `emulation-mode-map-alists` | **(a) extend compat** | Cosmetic/var defaults; no-op stubs. |
| 7 | `package.el` | **(c) genuinely absent — but irrelevant** | see § below |

## The `:ensure` / `package.el` question (P3 viability hinge)

**`package.el` is not hard-required at load time.** Evidence:
- `use-package-ensure.el` L33 only `(require 'use-package-core)`; **no top-level `package` require**. L42 is a `declare-function` (compile-time decl only).
- `(require 'package)` lives **inside `use-package-ensure-elpa`** at **L159** — executed lazily, only when an `:ensure` clause actually fires.
- `:ensure` dispatches through the **defcustom `use-package-ensure-function`** (L57, default `'use-package-ensure-elpa`). Handler `use-package-handler/:ensure` (L187) `funcall`s it; setting the defcustom to a no-op / XEmacs `x-package` shim **removes `package.el` entirely**.
- `use-package-always-ensure` defaults to `nil` (L45) → with no `:ensure`, the elpa fn is never called.

**Verified:** after `require 'use-package`, `(featurep 'package)` = **nil** — package.el never loaded.

## P3 proof (concrete)

With a ~25-line shim providing `subr-x`/`macroexp-progn`/`enable-theme`/`custom-enabled-themes`/`tabulated-list`/`cl-flet`/`font-lock-add-keywords`/`user-emacs-directory`/`emulation-mode-map-alists` (`/tmp/use-package-research/xemacs-shim.el`):

```
bind-key.el       => Wrote bind-key.elc
use-package-core  => Wrote use-package-core.elc
LOAD use-package (full: core+bind-key+diminish+delight+ensure) => OK ; macro=t bind-key=t
package feature after load = nil   (package.el not pulled in)
use-package-ensure-function default = use-package-ensure-elpa (defcustom, patchable)
SHIM P3-LOAD TEST PASSED
```

Without the shim: `use-package-core.el` **hard-fails** at L34 (`tabulated-list`); `bind-key.el` compiles but cannot expand macros.

## Recommendation for #65

- **P3 (unmodified + shim) is viable.** The shim is small (8 symbols, mostly one-liners); `package.el` decoupling is real. Only snag: `tabulated-list` — strict P3 needs a stub `tabulated-list.el` providing the mode; statistics-mode is inert.
- **P2 (source patch) is cheaper** if edits are allowed: guard `tabulated-list` (L34 + L1051–95), flip L49's condition to `(unless (fboundp 'hash-table-keys) …)` (drops `subr-x`), replace L42–43 with `(ignore)`. Leaves `macroexp-progn`/`cl-flet` still needing compat — P2 shrinks the shim, doesn't remove it.
- **P1 (vendored fork) not warranted** — upstream is archived (frozen at 2.4.6); P3 already loads it unmodified.

**Suggested: P3 + a 1-line source guard for `tabulated-list`** (hybrid) → ~6-symbol shim, zero behavior change for the macro's core contract.

## Corrections to prior assumptions

- **`macroexp` is NOT provided by XEmacs 21.5-b36** — milestone-2 compat inventory is wrong; `macroexp-progn` unbound. Add to the compat layer.
- **`bind-key` has no standalone repo** — fetch from `jwiegley/use-package`.
- **`enable-theme`/`custom-enabled-themes`** are absent even though `deftheme`/`custom-theme-p` exist.
