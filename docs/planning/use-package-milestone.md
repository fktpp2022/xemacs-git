# use-package on XEmacs: declarative package management

Status: accepted through wayfinder tickets
[#64](https://github.com/fktpp2022/xemacs-git/issues/64),
[#65](https://github.com/fktpp2022/xemacs-git/issues/65),
[#66](https://github.com/fktpp2022/xemacs-git/issues/66),
[#67](https://github.com/fktpp2022/xemacs-git/issues/67),
[#68](https://github.com/fktpp2022/xemacs-git/issues/68),
[#69](https://github.com/fktpp2022/xemacs-git/issues/69), and
[#71](https://github.com/fktpp2022/xemacs-git/issues/71), 2026-10-27. This is the
`use-package` milestone for the map
[use-package on XEmacs](https://github.com/fktpp2022/xemacs-git/issues/63); it builds on
[MELPA Milestone 2](melpa-milestone-2.md) and its compatibility layer and extends that
layer's support contract.

Inputs:

- [#64](https://github.com/fktpp2022/xemacs-git/issues/64): feasibility report; upstream
  `use-package` + `bind-key` do not load as-is, but P3 (unmodified plus a small shim) is
  viable, and `package.el` is not load-coupled. Evidence on
  `research/use-package-feasibility` (`docs/planning/use-package-feasibility.md`).
- [#65](https://github.com/fktpp2022/xemacs-git/issues/65): port strategy; vendor into core,
  split the shim, guard `tabulated-list`, keep the vendored files byte-identical but for one
  patch.
- [#66](https://github.com/fktpp2022/xemacs-git/issues/66): the `:ensure` backend contract;
  refresh-on-miss, warn-and-continue, reject pinning.
- [#67](https://github.com/fktpp2022/xemacs-git/issues/67): keyword prototype; every in-scope
  keyword behaves except `:bind*`. Artifact on `prototype/use-package-keywords-67`
  (`prototype/`).
- [#68](https://github.com/fktpp2022/xemacs-git/issues/68): startup ordering; activate ELPA
  before init, enter `use-package` by autoload, guard the ordering with a test.
- [#69](https://github.com/fktpp2022/xemacs-git/issues/69): `subr-x`; ship a real minimal
  `lisp/subr-x.el`.
- [#71](https://github.com/fktpp2022/xemacs-git/issues/71): `:bind*`; degrade it to `:bind`
  in the local shim.

## 1. Goal

XEmacs gains a declarative `(use-package foo :ensure t :defer t ...)` that installs through the
existing `elpa.el` client, lazy-loads through the generated `<name>-autoloads.el`, and is
available before init files run — matching GNU Emacs user expectations. `use-package` and
`bind-key` are part of the core tree, so no install step precedes their use. The
`elpa-installable-packages` allowlist remains the sole trust boundary (ADR 0001).

The milestone counts as done when:

1. a deterministic offline fixture installs an allowlisted package from a `:ensure` clause in a
   real init file and, in a fresh XEmacs, the package's autoload is live before the first
   command while the package body stays deferred until first use; and
2. an optional live HTTPS smoke test installs an allowlisted real package through `:ensure`
   without pinning versions.

Nothing else is required. This milestone does not build an installer; it builds the ensure
backend and the startup readiness for it.

## 2. User stories

1. As an XEmacs user, I want `(use-package foo :ensure t)` in my init file to install `foo`
   from my configured archives, so that I declare packages instead of installing them by hand.
2. As an XEmacs user, I want `:defer`, `:commands`, `:mode`, `:hook`, and `:bind` to defer the
   package body until first use, so that startup stays fast.
3. As an XEmacs user, I want `(use-package foo :ensure t)` to work with `use-package` already
   available, so that I need no bootstrap before my first declaration.
4. As an XEmacs user, I want a failed install to warn and let startup continue, so that one
   unavailable package does not break the whole session.
5. As a security-conscious user, I want `:ensure` on a name I have not allowlisted to fail
   loudly, so that a declaration never grants install trust implicitly.
6. As a security-conscious user, I want `:ensure` not to touch the legacy `package-get` /
   Tools→Packages path, so that the milestone does not revive the dead EFS transport.
7. As a package author, I want the published keyword subset to behave as GNU documents it, so
   that snippets I copy from upstream work unchanged.
8. As a maintainer, I want the vendored upstream kept byte-identical but for one guard patch, so
   that the fork cost stays one reviewable diff.
9. As a maintainer, I want `:bind*` to work as a documented synonym for `:bind` rather than
   silently placing an unreachable binding, so that the limitation is explicit.
10. As a maintainer, I want the compatibility additions to be core provision, so that packages
    after this milestone inherit them.
11. As a test author, I want the startup ordering guarded by a batch test, so that a later
    change cannot silently move ELPA activation back after init.

## 3. Port strategy

**V1 — core-vendored.** `use-package` and `bind-key` are copied into `lisp/`, available at
startup with no install step. This is GNU 29+ parity (built-in there, so `(use-package foo
:ensure t)` works in init) and removes the bootstrap chicken-and-egg a package-installed
`use-package` would create. Upstream is archived at 2.4.6, so the usual drift objection does not
apply; GPL-3+ matches XEmacs.

**S-a — split shim.** Generic additions go to the core tree as core provision so later packages
inherit them; pieces that exist only to make `use-package` itself load go in one
`lisp/use-package-xemacs.el` required before the port. Section 5 lists the split.

**T-a — guard `tabulated-list`, no lying stub.** `use-package-core.el` hard-fails at its L34
`(require 'tabulated-list)`, which XEmacs lacks. The require and the display-only
`use-package-statistics-mode` body (L1051–L1095) are wrapped in `(when (fboundp
'tabulated-list-mode) …)`. Statistics is a debug aid outside the keyword scope; a stub
`provide`ing `tabulated-list` was rejected because a feature that lies becomes silent breakage
for any other consumer.

**P-a — seven files byte-identical, one guard patch.** `use-package.el` (entry),
`use-package-core.el`, `use-package-ensure.el`, `use-package-bind-key.el`,
`use-package-diminish.el`, `use-package-delight.el`, and `bind-key.el` are vendored unchanged
except for the `tabulated-list` guard in `use-package-core.el`. `use-package.el` hard-requires
diminish and delight, so trimming the entry point would mean *replacing* it — a worse patch than
shipping two ~90-line inert files. `:diminish` / `:delight` remain out of scope.

## 4. Vendored files and the vendoring note

| File | Changes |
|---|---|
| `lisp/use-package.el` | none (entry point) |
| `lisp/use-package-core.el` | `tabulated-list` guard (L34, L1051–L1095) |
| `lisp/use-package-ensure.el` | none |
| `lisp/use-package-bind-key.el` | none |
| `lisp/use-package-diminish.el` | none |
| `lisp/use-package-delight.el` | none |
| `lisp/bind-key.el` | none |

A header block in `lisp/use-package.el`, mirroring the vendored `lisp/cl-lib.el`, records
upstream commit `2f09ce19` (= GNU ELPA `use-package` 2.4.6, archived) and the patch list. The
vendoring note claims **one guard patch**; every other compatibility need is core provision or
the local shim, never an edit to a vendored file.

## 5. Compatibility layer additions

The feasibility report examined exactly the symbols upstream references that XEmacs 21.5-b36
lacks. The prototype (#67) corrected four of them. The split below is the contract; anything
outside it is unsupported even if it happens to work.

### 5.1 Core provision (generic)

| Symbol | Home | Shape |
|---|---|---|
| `macroexp-progn` | new `lisp/macroexp.el` (provides `macroexp`) | **`defun`, not `defmacro`** — as a macro it returns the unevaluated call and expansion fails with `void-variable use-package-concat` |
| `macroexp-unquote` | new `lisp/macroexp.el` | identity |
| `hash-table-keys`, `string-suffix-p`, `string-trim`, `when-let*`, `if-let*` | new `lisp/subr-x.el` (provides `subr-x`) | section 8 |
| `cl-flet` | vendored `lisp/cl-lib.el` | `defmacro` approximating on top of dynamic `flet`; only `bind-key.el` L366 uses it, benignly |
| `eval-after-load` | core `lisp/subr.el` (L2112) | accept a **feature symbol** as well as a string; XEmacs matches the literal name passed to `load`, and `use-package` emits `'feature`, so without this `:defer`+`:config` and `:after` silently never fire |
| `display-warning` | core `lisp/simple.el` (L4766) | accept GNU keyword levels (`:error`/`:warning`/`:debug`); XEmacs rejects `:error`, so a failing `:config` would crash the warning handler instead of warning |

`font-lock-add-keywords` is **already present** (`lisp/font-lock.el` L912); no addition is
needed. The feasibility report's absence claim was against a stale binary.

### 5.2 Local shim (`lisp/use-package-xemacs.el`)

| Symbol | Shape |
|---|---|
| `enable-theme` | record the theme and mark it `theme-immediate` so `:custom` values apply at once; a plain no-op records but never applies, and a later `defcustom` overwrites them. The alternative without the shim is `(setq use-package-use-theme nil)`, which takes the plain `customize-set-variable` path. |
| `custom-enabled-themes` | `defvar` nil |
| `user-emacs-directory` | `defvar`, `"~/.xemacs/"` |
| `emulation-mode-map-alists` | nil `defvar` for load cleanliness only; never consulted (section 9) |
| `use-package-ensure-function` | `setq-default` to the `elpa` backend (section 6) |
| `bind-key*`, `bind-keys*` | redefined to `bind-key`, `bind-keys` (section 9) |

## 6. The `:ensure` backend contract

`:ensure` routes through upstream's `use-package-ensure-function` defcustom; the backend is
`use-package-ensure-elpa` in `lisp/use-package-xemacs.el`, wired by `setq-default`. No patch to
`use-package-ensure.el`. Upstream calls it as `(funcall fn name args state)`; the return value is
ignored, side effects matter. `args` arrives already normalized: `(t)`, `(foo)`, or
`((foo . "archive"))`.

Shape, in order:

1. **Allowlist check — hard error, before the `condition-case`.** If the resolved name is not in
   `elpa-installable-packages`, signal an `error` naming the name and pointing at the allowlist.
   A trust refusal is never downgraded to a warning. Users extend trust with
   `(add-to-list 'elpa-installable-packages 'foo)` in init — the documented pattern.
2. **Refresh-on-miss (R-a).** If `(elpa-installed-p name)` is true, return immediately — zero
   network. Otherwise call `(elpa-install name)`, which refreshes the index and installs
   internally (the milestone-1 refresh-before-install). No new API.
3. **Warn and continue (F-a).** Wrap the `elpa-install` call in a `condition-case`; on error call
   `(display-warning 'use-package …)` and proceed. Init is not aborted; the `:init` body still
   runs and `:config` fails later if the package never loads. This is upstream parity.

**Pinning is rejected (P-x-a).** A spec element shaped `(name . "archive")` signals an error
pointing at ADR 0001. Pinning is version disambiguation, not security (the ADR says so); and
byte-level pinning is impossible against MELPA's rolling archive, which 404s deleted artifacts.
`:pin` is out of scope as its own concept (section 12).

The backend never touches `package-get`, `package-ui`, or the Tools→Packages menu.

## 7. Keyword scope

Verified by the #67 prototype against upstream `2f09ce19` on XEmacs 21.5-b36 (32 checks, 0
failed, one GAP).

| Keyword | Behaviour |
|---|---|
| `:init` | runs at declaration; package not loaded |
| `:defer t` | holds `:config`; package not loaded |
| `:config` | runs eagerly without `:defer t`, on load with it |
| `:commands` | command autoloaded, package deferred |
| `:hook` | `add-hook` applied, hook function autoloaded, package deferred |
| `:mode` | `auto-mode-alist` entry added, mode autoloaded, package deferred |
| `:bind` | binds in `global-map`, command autoloaded, package deferred |
| `:bind*` | **degraded to `:bind`** (section 9) |
| `:after` | holds load and `:config` until the named feature loads |
| `:if` / `:when` / `:unless` | expansion is genuinely conditional; a false branch loads nothing |
| `:custom` | applies to already-bound and package-defined variables, via the theme path or with `use-package-use-theme nil` |
| `:ensure` | routes to the backend (section 6) |

Not probed, and not promised: `:defer N` idle timers, `:after :or`/`:and`, `:bind :map`.

## 8. `subr-x`

Upstream `use-package-core.el` L57 is `(require 'subr-x)` inside `eval-when-compile`, which fires
because XEmacs's major version is 21. Per P-a the line is not patched out; the feature is
satisfied.

Ship a real, minimal `lisp/subr-x.el` (provides `subr-x`) with the five documented upstream
symbols: `hash-table-keys`, `string-suffix-p`, `string-trim`, `when-let*`, `if-let*`. By
definition guard (`unless (fboundp …)`), as core provision. No patch to `use-package-core.el`.
Tests land in `tests/automated/subr-x-compat-tests.el`.

The five-symbol surface is the upstream contract, not an over-provision: it is what any package
declaring `subr-x` may expect. In practice only `hash-table-keys` is exercised by `use-package`
(`use-package-core.el` L1053); `when-let*`, `if-let*`, and `string-trim` are shipped for the
feature contract. The core `when-let` / `if-let` macros dumped in `lisp/subr.el` by milestone 2
are the non-star names and are unchanged.

Milestone 2 deliberately promised **no `subr-x` feature**; this milestone extends the contract.
The M2 boundary note (`docs/planning/melpa-milestone-2.md` §4.2, "There is no `subr-x`
feature …") is preserved as a historical fact about M2 and is not edited.

## 9. `:bind*`

XEmacs has no `emulation-mode-map-alists`, the lookup path GNU consults for `override-global-map`,
so upstream `bind-key*` / `bind-keys*` create a binding that is never reached. `lisp/use-package-xemacs.el`
redefines them after `(require 'bind-key)`:

- `bind-key*` → `bind-key`
- `bind-keys*` → `bind-keys`

Bindings install through the normal global keymap. The only loss is override priority, which was
never reachable on XEmacs anyway. The vendored `bind-key.el` stays byte-identical. The limitation
is documented (section 11) and `emulation-mode-map-alists` is never consulted.

## 10. Startup ordering

Today `elpa-activate-all` runs **after** init files (`lisp/startup.el` L784) while native package
autoloads load before init (L565). `(use-package foo :ensure t)` in an init file needs both the
`elpa.el` API and the installed autoloads ready first.

**A-c.** Move `startup-activate-elpa` to just before `load-init-file` in `normal-top-level` so
both `(require 'elpa)` and `elpa-activate-all` run before site-start and the user init. This is
GNU 27+ `package-initialize` parity. New ordering:

```
startup-setup-paths → startup-load-autoloads (L565) → …frame/faces/menubar…
  → startup-activate-elpa (moved from L784) → load-init-file (L777) → command-line-1
```

Both existing guards are preserved: `inhibit-all-packages` (the `-no-packages` flag) and
`elpa-activate-at-startup`. `elpa` stays non-preloaded; the `(require 'elpa)` inside
`startup-activate-elpa` remains lazy.

**B-a.** `lisp/use-package.el` enters via its existing `;;;###autoload` cookies wired into
`lisp/loaddefs.el` — the same mechanism every native `lisp/` package uses. `(use-package …)` in
init triggers an autoload-driven `require` that loads from `lisp/`. Zero setup, zero cost for
users who never call it.

**C-a.** A batch startup-ordering test starts XEmacs with an init containing `(use-package
queue :ensure t :defer t :commands make-queue)` and asserts the autoload is live after init and
before the first command. It is added under `tests/automated/` so the CTest glob discovers it
(the #68 decision named it `lisp/startup-tests.el`; relocated to match the automated-test
directory and the rest of the ELPA gate).

**Recorded consequence (accepted, not re-grilled).** Activation now runs before init, so
`elpa-activate-at-startup` can no longer be set to nil *from an init file*. The mitigation is the
pre-startup mechanism already documented in `startup-activate-elpa`: batch mode sets the variable
before startup, for example through `XEMACSDEBUG`. No new `early-init.el` knob is added.

## 11. Documentation

- `lisp/use-package.el` header: the vendoring note (section 4).
- `man/elpa.texi`: a `use-package` chapter covering the keyword scope table, the `:ensure`
  contract, the allowlist requirement, the startup ordering, the `subr-x` extension, and the
  `:bind*` degradation.
- `GLOSSARY.md`: add **ensure backend**, **use-package ensure function**, and **subr-x
  compatibility surface**; keep the existing allowlist vocabulary aligned.

## 12. Out of scope

- `:interpreter` / `:magic` — require `magic-mode-alist`, which XEmacs lacks.
- `:pin` — GNU archive pinning; ADR 0001 has no pinning concept.
- `:diminish` / `:delight` — third-party mode-line packages; vendored inert, keywords out of
  scope.
- Real `:bind*` override priority — needs an `emulation-mode-map-alists` equivalent.
- Legacy `package-get` / Tools→Packages coupling — `:ensure` routes only to `elpa.el`.
- Auto-install on bare `require` — not `use-package`'s job.
- `package-quickstart` and deferred-configuration bundling.
- `:defer N` idle timers, `:after :or`/`:and`, `:bind :map`, and any keyword outside the table in
  section 7.

## 13. Success test

### 13.1 Offline fixture (mandatory, no network)

Reuse the milestone-2 fixture `tests/elpa/melpa-m2-fixture/`, which already vendors real GNU
ELPA `queue` 0.2 (with its `make-queue` `;;;###autoload` cookie) and a version-1 index. Point
`elpa-archives` at its `gnu/` directory and a temporary `elpa-user-dir`. `queue` is already in
the default allowlist.

A driver runs a fresh XEmacs with an init file containing:

```elisp
(use-package queue
  :ensure t
  :defer t
  :commands make-queue
  :config (setq use-package-test-config-ran t))
```

Assertions:

1. `:ensure` installed `queue` into `elpa-user-dir` (`queue-0.2/` exists).
2. After startup, before any command: `(fboundp 'make-queue)` is `t` (autoload live) and
   `(featurep 'queue)` is `nil` (body deferred).
3. Calling `(make-queue)` loads `queue`; `(featurep 'queue)` becomes `t` and
   `use-package-test-config-ran` is `t`.
4. A second startup with the same `elpa-user-dir` does not re-install (`:ensure` short-circuits on
   `elpa-installed-p`), and the autoload is still live.

The startup-ordering guard (section 10, C-a) is the same shape, asserting only (2) after init.

### 13.2 Negative cases

| Case | Assertion |
|---|---|
| `(use-package not-allowed :ensure t)` | a hard `error` naming the allowlist; not caught by the warn+continue |
| allowlisted name absent from every archive | `display-warning`, startup continues, `:init` body ran |
| `(use-package foo :ensure (foo . "archive"))` | an `error` pointing at ADR 0001 |
| `subr-x` loaded with each of the five symbols absent beforehand | each becomes `fboundp`; `(featurep 'subr-x)` is `t` |

### 13.3 Live HTTPS smoke test (optional)

Gated on `ELPA_LIVE`, same shape as the milestone-2 live gate: refresh a real archive index,
install an allowlisted real package through `:ensure` unpinned (GNU ELPA `queue` is the
minimal witness), and exercise it in a fresh child XEmacs. No version is pinned.

### 13.4 Verification primitive

Behavioural assertions only. No new checksum primitive; raw-byte `string=` remains the comparison
primitive from milestone 2.

## 14. Further notes

- `bind-key` has no standalone repository; it ships inside `jwiegley/use-package`.
- Upstream is archived at 2.4.6 (`2f09ce19`), so P-a's byte-identical fork carries no drift cost.
- The feasibility report's absence claims were measured against the pre-built 21.5-b36 binary;
  `font-lock-add-keywords` is present in the current source and needs no addition.
- `hash-table-keys` is the only `subr-x` symbol `use-package` actually calls
  (`use-package-core.el` L1053); the other four ship for the feature contract.
- `use-package` references `macroexp-progn` at `use-package-core.el` L438, L1214, L1393, L1426,
  L1448, L1602, L1679 and `bind-key.el` L371, L374, L440, L449; the `defun` shape is load-bearing.
- The prototype's throwaway shim lived in `prototype/xemacs-shim.el`; all corrections from #67 are
  folded into sections 5.1–5.2.
- `es-lib`'s overlay calls, `define-globalized-minor-mode`, and the rest of the milestone-2 support
  contract are unchanged.
