# What can the manuals truthfully promise about the vendored use-package?

Research ticket [#121](https://github.com/fktpp2022/xemacs-git/issues/121). 2026-11-04,
branch `research/use-package-doc-surface` (from `origin/xemacs-cmake` at `b20d9fe4d`).
Every fact below is cited to in-tree primary sources; runtime probes were run with the
built binary `/home/fankai/projects/xemacs/build-install/bin/xemacs` (21.5-b36 "leeks").
Paths are relative to the repo root; line numbers are as of this branch.

## 0. What is vendored, and how faithfully

- Version string in the code: `use-package-version` = **"2.4.4"** (`lisp/use-package-core.el:68`).
  The milestone doc instead records upstream `2f09ce19` = ELPA 2.4.6
  (`docs/planning/use-package-milestone.md:113-124`, `:350`) and issue #72's title says 2.4.4 —
  the manuals must not promise a specific upstream version until this drift is reconciled.
- Vendored files: `lisp/use-package.el` (entry; requires core, `use-package-bind-key`,
  `use-package-diminish`, `use-package-delight`, `use-package-ensure` — `lisp/use-package.el:41-46`),
  `lisp/use-package-core.el`, `lisp/use-package-ensure.el`, `lisp/use-package-bind-key.el`,
  `lisp/use-package-diminish.el`, `lisp/use-package-delight.el`, `lisp/bind-key.el`, plus the
  fork-owned glue `lisp/use-package-xemacs.el` (`lisp/use-package-xemacs.el:8-13`).
- Fidelity: the original vendoring carried one guard patch (soft `require 'tabulated-list`);
  it was **removed** by `470985f4c` ("feat(lisp): render use-package-report on XEmacs") after a
  real minimal `lisp/tabulated-list.el` was ported, and `lisp/use-package-core.el:34` is now the
  plain upstream `(require 'tabulated-list)`. The vendored files are byte-faithful to upstream;
  all XEmacs adaptation is either core provision (`macroexp.el`, `subr-x.el`, `cl-lib.el`,
  `custom.el`, `keymap.el`, `files.el`, `simple.el`, `subr.el`) or the glue file
  (`lisp/use-package-xemacs.el:9-13`, `docs/planning/use-package-milestone.md:82-124`).

## 1. Keyword surface

`use-package-keywords` is the upstream default order
(`lisp/use-package-core.el:71-103`); loading `use-package` adds `:ensure` and `:pin` at the
head (`lisp/use-package-ensure.el:207-208`) and `:diminish`/`:delight` at the tail
(`lisp/use-package-diminish.el:73`, `lisp/use-package-delight.el:84`). Full set observed at
runtime in the built binary:
`(:pin :ensure :disabled :load-path :requires :defines :functions :preface :if :when :unless
:no-require :catch :after :custom :custom-face :bind :bind* :bind-keymap :bind-keymap*
:interpreter :mode :magic :magic-fallback :hook :commands :autoload :init :defer :demand :load
:config :diminish :delight)`. Unknown keywords error unless `use-package-ignore-unknown-keywords`
is set (`lisp/use-package-core.el:132-137`).

The truthful per-keyword story (asked set first):

| Keyword | Status | Evidence |
|---|---|---|
| `:ensure` | **honored, rewired backend** — routes to the `elpa.el` client, not GNU `package.el`. See §2. | `lisp/use-package-xemacs.el:52-90`; `lisp/use-package-ensure.el:184-196` |
| `:bind` | **honored** — expands to `bind-keys`, which binds through `bind-key` into `global-map` by default; commands autoloaded. Proven by test: key lands in `global-map` and the command stays an autoload. | `lisp/use-package-bind-key.el:129,143-152`; `lisp/bind-key.el:161-195` (default keymap `global-map` at :193-195); `tests/automated/use-package-tests.el:137-150` |
| `:bind*` | **honored but degraded to `:bind`** — the glue aliases `bind-key*`→`bind-key` and `bind-keys*`→`bind-keys` after `bind-key` loads, so the star handler (`lisp/use-package-bind-key.el:153-154`, emits `bind-keys*`) installs into the normal global map. XEmacs has no `emulation-mode-map-alists` lookup path (`lisp/keymap.el:828-834` — the variable exists only so `bind-key` loads, `lisp/bind-key.el:152-153` register into it; never consulted), so override priority is impossible and the degradation exists precisely to avoid a silently dead binding (`lisp/use-package-xemacs.el:37-45`; probe: `symbol-function 'bind-key*` is the alias of `bind-key`). Manual must document "documented synonym for `:bind`, no override priority", never GNU's promise. | |
| `:diminish` | **accepted but inert** — keyword registered and its handler emits `(if (fboundp 'diminish) …)`, so nothing happens unless the third-party `diminish` library is somehow loaded; out of scope for the fork. | `lisp/use-package-diminish.el:62-73`; `lisp/use-package.el:44`; `docs/planning/use-package-milestone.md:287-288` |
| `:preface` | **honored unchanged** — forms emitted before the `require`/autoload block. | `lisp/use-package-core.el:1195-1206` |
| `:config` | **honored unchanged** — runs eagerly without deferral, on load with it. | `lisp/use-package-core.el:1569-1590`; `tests/automated/use-package-tests.el:226-247` |
| `:custom` | **honored** — default theme path needs the GNU theme API, provided by XEmacs core: `enable-theme`/`custom-enabled-themes` compat in `lisp/custom.el:45-63` (marks the synthetic theme `theme-immediate` so values apply at once and a later `defcustom` cannot clobber them); `use-package-use-theme nil` takes the plain `customize-set-variable` path. | `lisp/use-package-core.el:139-144,1461-1505`; `lisp/custom.el:55-63` |
| `:hook` | **honored unchanged** — `add-hook` with autoloaded hook functions. | `lisp/use-package-core.el:1291-1330`; `docs/planning/use-package-milestone.md:183-207` |
| `:after` | **honored** — works only because `eval-after-load` was extended to accept a feature symbol (`lisp/subr.el:2112-2124`, normalizes symbol→name); without it `:after` would silently never fire. | `lisp/use-package-core.el:1398-1456` |
| `:requires` | **honored unchanged** — runtime `featurep` gate wrapping the body. | `lisp/use-package-core.el:1147-1156` |
| `:if` (`:when`/`:unless`) | **honored unchanged** — genuine conditional expansion; `:when/:unless` are normalized to `:if` (`:569`), repeated `:if` merge with `and` (`:221-224`). | `lisp/use-package-core.el:1123-1137,569,221-241` |
| `:pin` | **registered but rejected/unwired.** The keyword exists (`lisp/use-package-ensure.el:208`) and normalizes (`:77-85`), but its machinery drives GNU `package.el` state (`package-pinned-packages`, `package-archives`, `package-initialize` — `:104-115`) which XEmacs has none of. Probe: `(use-package dmacro :pin gnu :ensure t)` signals `void-variable: package-archives`. And a pinned `:ensure` spec `(foo :pin "x")` is *refused by design* with an error naming ADR 0001 (`lisp/use-package-xemacs.el:62-65`; normalized shape per `lisp/use-package-ensure.el:142-147`; `docs/planning/use-package-milestone.md:176-179`). Manual must say: **no pinning, at all.** |
| `:init`, `:defer`, `:demand`, `:commands`, `:mode`, `:load-path`, `:disabled`, `:defines`, `:functions`, `:catch`, `:no-require`, `:custom-face`, `:autoload`, `:load`, `:bind-keymap` | upstream handlers all present and unmodified (`:init` `lisp/use-package-core.el:1534`; `:defer` `:1384`; `:commands` `:1340`; `:bind-keymap` `lisp/use-package-bind-key.el:164-178`); `:load-path` expands relative paths against `user-emacs-directory`, provided as `~/.xemacs/` compat (`lisp/use-package-core.el:853`; `lisp/files.el:573-581`). Behavior-verified subset: `:init`/`:defer`/`:bind`/`:commands`/`:config`/`:ensure` (`tests/automated/use-package-tests.el:100-260`; `docs/planning/use-package-milestone.md:183-203`). `:bind-keymap*` degrades like `:bind*` (its `override` argument reaches `bind-key*` → alias → `bind-key`, `lisp/use-package-bind-key.el:170,180-181`). |
| `:interpreter`, `:magic`, `:magic-fallback` | handlers exist (`lisp/use-package-core.el:1257-1290`) but reference `magic-mode-alist`, which XEmacs does not implement anywhere in `lisp/` — out of scope; manuals must not promise them. | `docs/planning/use-package-milestone.md:285` |
| `:delight` | inert, same story as `:diminish`. | `lisp/use-package-delight.el:77-84`; `docs/planning/use-package-milestone.md:287-288` |

Not probed and therefore not promiseable even among honored keywords: `:defer N` idle timers,
`:after` `:or`/`:and` conjunctions, `:bind` `:map` subtleties
(`docs/planning/use-package-milestone.md:203`).

What `:bind`-family expansion needs under XEmacs keymaps: `read-kbd-macro` for `(kbd …)`-style
strings (ported in MELPA milestone 2, issue #60 body; used at `lisp/bind-key.el:190-192`),
`use-package-core`'s `macroexp-progn` (`lisp/macroexp.el`, dumped `lisp/dumped-lisp.el:309`),
and `cl-flet` (`lisp/cl-lib.el`; only `lisp/bind-key.el:366`). The glue also no-ops
`declare-function` and provides `condition-case-unless-debug`
(`lisp/use-package-xemacs.el:23-35`).

## 2. The `:ensure` backend contract as shipped

- **Routing without patching upstream**: `use-package-handler/:ensure` always calls
  `use-package-ensure-function` (`lisp/use-package-ensure.el:184-196`; when byte-compiling it
  runs immediately, otherwise the call is pushed to run at load/init time). The glue
  `setq-default`s that variable before the defcustom loads so the defcustom keeps it
  (`lisp/use-package-xemacs.el:87`; defcustom at `lisp/use-package-ensure.el:57-73` honors an
  already-bound default), then `eval-after-load` **repoints the symbol**
  `use-package-ensure-elpa` to `use-package-ensure-elpa-xemacs`, shadowing the upstream
  GNU function (`lisp/use-package-xemacs.el:89-90`). Probe: after `(require 'use-package)` the
  function value is real and named `use-package-ensure-elpa`.
- **Normalized input**: `:ensure` bare → `(t)`; a symbol; or `(foo :pin "archive")` →
  `(foo . "archive")` (`lisp/use-package-ensure.el:134-151`). Any other shape is a macro error.
- **Backend behavior** (`lisp/use-package-xemacs.el:52-79`), in order:
  1. `(require 'elpa)` (`:57`).
  2. A `(pkg . "archive")` spec (i.e. pinned) **hard-errors** naming ADR 0001 (`:62-65`).
  3. Names not in `elpa-installable-packages` **hard-error** *outside* the `condition-case`
     — a trust refusal is never downgraded to a warning (`:69-71`; `docs/planning/use-package-milestone.md:163-168`). Test: `tests/automated/use-package-tests.el:162-169`.
  4. Already installed → return, zero network (`:72`; `elpa-installed-p` `lisp/elpa.el:454`;
     re-install short-circuit asserted `tests/automated/use-package-tests.el:251-260`).
  5. Otherwise `(elpa-install pkg)` — refresh-on-miss lives inside `elpa-install`
     (`lisp/elpa.el:478`; `docs/planning/use-package-milestone.md:169-172`) — followed by
     `(elpa-activate-all)` so the fresh package's autoloads are usable in the *same* session
     (`lisp/use-package-xemacs.el:74`; `lisp/elpa.el:1042`; issue #72 body).
  6. Install failure → `display-warning` `:warning` and **continue**; init is not aborted
     (`lisp/use-package-xemacs.el:75-79`; `:warning` levels accepted via the GNU-keyword-level
     compat of `display-warning`, `lisp/simple.el:4747,4858`; warn-and-continue asserted
     `tests/automated/use-package-tests.el:192-204`).
- **The `elpa.el` side of the contract**: `elpa-install` re-checks the allowlist before
  touching any index (`lisp/elpa.el:402-404,483`), enforces it for **every transitive
  dependency** (`lisp/elpa.el:600-602`; comment `:44-49`), treats `cl`/`cl-extra`/`cl-lib` as
  core-provided (never downloaded, `lisp/elpa.el:96-99`), and ignores the `emacs`
  pseudo-dependency (`lisp/elpa.el:117-118`). No `package.el`, no `package-get`, no
  Tools→Packages coupling (`docs/planning/use-package-milestone.md:181-182,290`).
- **Divergences from GNU** to state plainly in any manual:
  - GNU's default backend calls `package-install`/`package-refresh-contents` and honors
    `package-pinned-packages` (`lisp/use-package-ensure.el:153-181` — still in-tree but
    shadowed by the alias); this fork installs via the first archive that has the package in
    `elpa-archives` order, no pinning.
  - The defcustom docstring/`:type` still advertise "package.el"
    (`lisp/use-package-ensure.el:57-73`) — stale GNU text on this fork.
  - `use-package-always-pin` (`lisp/use-package-ensure.el:51-55,204-205`) is a no-op here;
    `use-package-always-ensure` works but every defaulted install then hits the allowlist
    check.
  - Return value is ignored; side effects matter (`docs/planning/use-package-milestone.md:157-161`).

## 3. Startup ordering that works from init today

New `normal-top-level` order, as shipped:

```
startup-setup-paths → startup-load-autoloads (lisp/startup.el:565) → frames/faces/menubar
  → startup-activate-elpa (lisp/startup.el:782) → load-init-file (lisp/startup.el:785)
```

- ELPA activation runs **before** init files so `:ensure` during init sees an activated
  `load-path`/autoload state (comment `lisp/startup.el:776-781`; design A-c
  `docs/planning/use-package-milestone.md:245-257`). `startup-activate-elpa`
  (`lisp/startup.el:1719-1744`) lazily `(require 'elpa)` and calls `elpa-activate-all` when
  `elpa-user-dir` exists, guarded by `inhibit-all-packages` (`-no-packages`) and
  `elpa-activate-at-startup` (`:1731-1735`; `lisp/elpa.el:102-109`); activation errors warn,
  never abort.
- `use-package` itself needs **no bootstrap**: `(use-package …)` in an init file resolves
  through the autoload generated from `lisp/use-package-core.el:1614`
  (`;;;###autoload` + `defmacro use-package :1615`) and collected into the build's
  `auto-autoloads.elc`, loaded at `lisp/startup.el:565`. Probe on the built binary with an
  empty HOME: `fboundp 'use-package` is `t` before init, and its function cell is the
  autoload into `use-package-core`. The glue itself (`use-package-xemacs`), plus `macroexp`
  and `tabulated-list`, is **dumped before loaddefs** so the aliases are in place first
  (`lisp/dumped-lisp.el:309-312`). (The checked-in `lisp/loaddefs.el` carries no use-package
  entry; the autoloads come from the generated `auto-autoloads` file, not `loaddefs.el`.)
- Consequence to document: because activation precedes init, `elpa-activate-at-startup`
  **cannot be set to nil from an init file** — set it pre-startup, e.g. via `XEMACSDEBUG`
  (`lisp/startup.el:543-544` evaluated at top of `normal-top-level`; comment `:776-781`;
  "recorded consequence" `docs/planning/use-package-milestone.md:269-273`).
- The ordering is test-guarded (C-a): a fresh child XEmacs with a real init file runs
  `(use-package queue :ensure t :defer t :commands make-queue :config …)` against the offline
  fixture, asserting autoload-live / body-deferred / first-use-loads / no-reinstall
  (`tests/automated/use-package-tests.el:206-260`).

## 4. A minimal working install-and-configure stanza

The real stanza from this development machine (`/home/fankai/.xemacs/init.el:10-26`, yadm'd)
qualifies — it is exactly the shape GNU users would paste, adapted only in archive choice and
timeout:

```elisp
(require 'elpa)
(setq elpa--index-timeout 300000)            ; MELPA's index is ~2.6 MB; default is 60 s
(setq elpa-archives
      '(("melpa" . "https://melpa.org/packages/")
        ("gnu" . "https://elpa.gnu.org/packages/")))
(require 'cnfonts-xemacs)                     ; XEmacs backend before cnfonts loads
(use-package cnfonts
  :ensure t
  :config
  (cnfonts-enable)
  (cnfonts-set-font))                         ; XEmacs ran window-setup-hook before init
```

Notes the manual must carry with it: `cnfonts` must be in `elpa-installable-packages` (it is,
`lisp/elpa.el:89`); `elpa-archives` order decides which archive wins, so MELPA is first
because `cnfonts` exists only there (`/home/fankai/.xemacs/init.el:4-5`); `elpa--index-timeout`
is an internal `defconst` (`lisp/elpa.el:196`), so the raise is a stopgap worth documenting as
such. The canonical *portable* example (no network, allowlisted `queue`) is the test init
(`tests/automated/use-package-tests.el:220-230`):

```elisp
(use-package queue
  :ensure t
  :defer t
  :commands make-queue
  :config (message "queue configured"))
```

## 5. Supported archive set and allowlist policy, user-visible

- **Archives**: `elpa-archives` is an alist of name → location, where a location is either a
  local absolute directory or an HTTP(S) URL going through the async transport
  (`lisp/elpa.el:69-76`); default is GNU ELPA only (`:70`). Both GNU ELPA and MELPA are
  supported; MELPA is treated as a rolling archive — the client refreshes its index before
  resolving and installs the currently-advertised build, possibly replacing an older one
  (`lisp/elpa.el:22-33`). Index fetch timeout 60 s, artifact timeout 300 s
  (`lisp/elpa.el:54-56,196-199`). Installs land in `elpa-user-dir`, default
  `~/.emacs.d/elpa/` (`lisp/elpa.el:78-82` — note: *not* `~/.xemacs/`).
- **Allowlist policy (ADR 0001)**: the client performs **no signature verification**; HTTPS
  transport plus the per-package allowlist `elpa-installable-packages` are the only trust
  controls (`docs/adr/0001-allowlist-only-trust-for-unsigned-package-archives.md:3`;
  `lisp/elpa.el:35-42`). The shipped default allowlist is 16 names: `queue loc-changes dmacro
  checkbox es-lib buffer-sets loop levenshtein csv vector-utils number rings
  shell-split-string kv unidecode cnfonts` (`lisp/elpa.el:84-94`). Trust is explicit per
  package — a dependency edge never grants it (`lisp/elpa.el:44-49`; same rule in the
  milestone-2 semantics, issue #60 body). The user-facing way to widen it is
  `(add-to-list 'elpa-installable-packages 'foo)` in init (`docs/planning/use-package-milestone.md:166-168`;
  the error text itself points at the list, `lisp/use-package-xemacs.el:70`). Residual risk is
  documented and accepted: a compromised archive or tampered connection means any allowlisted
  package can run arbitrary code; the allowlist "is not a cast-iron guarantee"
  (`docs/adr/...0001...:15`, `lisp/elpa.el:38-42`). **Pinning is not a security feature here
  and is rejected outright** (`docs/adr/...0001...:9`; `lisp/use-package-xemacs.el:62-65`) —
  byte-level pinning is impossible against MELPA's pruned artifacts (`docs/adr/...0001...:11`).

## 6. Honest-manual checklist (what may be promised vs what may not)

May promise (code + tests agree): the keyword subset of §1's table (untested-but-handled
keywords should be listed as "upstream behavior, unverified here"), the `:ensure` contract of
§2 including warn-and-continue and allowlist hard-fail, bootstrap-free `(use-package …)` in
init, install-and-use in the same session, GNU-ELPA+MELPA archives under the allowlist-only
trust model.

May **not** promise: `:pin` in any form; `:bind*` override priority; `:diminish`/`:delight`
effect; `:interpreter`/`:magic`/`:magic-fallback`; `package.el` compatibility (`List Packages`,
`package-install`, `package-refresh-contents` do not exist); uninstall/upgrade UI/rollback
(`lisp/elpa.el:26-28` — install-only client); a specific vendored upstream version number
until the 2.4.4/2.4.6 drift between `lisp/use-package-core.el:68` and
`docs/planning/use-package-milestone.md:113-124` is settled.

Known doc-surface defects found during this inventory:
1. No user manual exists yet: nothing in `man/*.texi` mentions `use-package` (grep over
   `man/`), although the milestone planned an `elpa.texi` chapter
   (`docs/planning/use-package-milestone.md:274-280`) and `GLOSSARY.md` has no use-package
   entries either.
2. `lisp/use-package.el:48-49` declares an autoload for `use-package-jump-to-package-form`
   from `use-package-jump`, but `lisp/use-package-jump.el` is **not shipped** — invoking it
   errors; it must not appear in manuals.
3. `use-package-ensure-function`'s own Customize text still says "package.el"
   (`lisp/use-package-ensure.el:57-73`) and `use-package-ensure-elpa`'s GNU body still
   `(require 'package)` (`:159`), both shadowed but visible.
4. The `use-package-info` machinery points at GNU's Info manual
   (`lisp/use-package-core.el:65`), which does not describe this fork's divergences.
