# MELPA milestone 2: compatibility precedents and GNU package requirements

Research ticket: [#47](https://github.com/fktpp2022/xemacs-git/issues/47) ("Map the compatibility precedents and GNU package requirements"), part of map #45.
Branch: `research/melpa-m2-compat`. Date: 2026-10-24.
Target: XEmacs `21.5 (beta36) "leeks" XEmacs Lucid`, Mule, libcurl, built at
`/home/fankai/projects/xemacs/xemacs-git/build/bin/xemacs` (`emacs-major-version` = 21).

This document records evidence and design axes only. **It does not choose the
compatibility layer's architecture**; that decision belongs to ticket #49.

## Bottom line

- The in-tree `lisp/compat.el` is inert: its whole implementation sits inside a
  quoted list marked `;; #### not yet working`. It defines nothing, is never
  required, and was disabled deliberately in 2002 with the message "Not yet
  done. Currently doesn't work."
- `fsf-compat` is neither in-tree nor installed in this build. The overlay API
  is unavailable (`locate-library "overlay"` => nil), as is `advice`.
- GNU `cl-lib` 0.7.1 is the only one of the investigated packages that is
  substantially usable: it loads after a no-op `defadvice` stub and passes 21
  of 23 per-definition probes. Its `cl-flet` and `cl-position` do not work.
- `seq` 2.24, `let-alist` 1.0.6 and GNU `compat` 31.1.0.0 cannot be loaded on
  this build. `seq` and `let-alist` fail in the **reader**, so no shim can fix
  them. `compat` fails in its macro layer even with every identified helper
  stubbed.
- `nadvice` depends on the absent `advice`; `parse-time` depends on the absent
  `iso8601` (plus `cl-lib`/`subr-x`).
- XEmacs 21.5 also lacks `version<`/`version<=`, which several GNU packages use
  for dispatch. That is a prerequisite independent of any particular package.

## 1. Precedents

### 1.1 Dormant `lisp/compat.el` (Ben Wing, 2000)

What it was meant to provide (all of it inside the quoted block):

- A per-package, namespace-clean compatibility mechanism: copy `compat.el` to
  `<package>-compat.el`, set `compat-current-package`, wrap definitions in
  `compat-define-group`, and wrap callers in a generated `<package>-compat`
  macro. The macro uses `macrolet` to lexically rename generic calls to
  prefixed definitions, so only `<package>-compat-*` symbols escape.
- Definition helpers `compat-define-compat-functions`, `compat-define-group`,
  `compat-define-function`, `compat-defun`, `compat-clear-functions`, and
  runtime helpers `compat-fboundp`, `compat-wrap`, `compat-wrap-runtime`.
- A concrete `fsf-compat` group: GNU overlay API implemented over XEmacs
  extents (`make-overlay`, `move-overlay`, `delete-overlay`, `overlay-start`,
  `overlay-end`, `overlay-buffer`, `overlay-properties`, `overlays-at`,
  `overlays-in`, `next-overlay-change`, `previous-overlay-change`,
  `overlay-lists`, `overlay-recenter`, `overlay-get`, `overlay-put`), plus
  `extent-property`, `extent-at`, `map-extents`.

Why it was never adopted:

- The file is 729 lines, but the top-level quote starts at line 145 (after
  `;; #### not yet working`). Everything after it, including all 19
  `defun-compat` calls and the final `(fmakunbound 'compat-define-compat-functions)`,
  is list data. There is no `(provide 'compat)` and no `(require 'compat)`
  anywhere under `lisp/`. Even if the quote were removed, `defun-compat` is
  used but never defined.
- Git history: commit `95795ff31` (2002-05-05, Ben Wing) rewrote the file
  (+569/-32) and added the quote and marker. Its commit message says:
  "Some attempts to redo this to make it truly useful and fix the 'multiple
  versions interacting with each other' problem. Not yet done. Currently
  doesn't work." Later commits (`f71830929` GPLv3, `6a173f750` ends-here)
  are mechanical. It has been dead since 2002, not merely unmaintained.
- Its design required copying the file into every consumer and generating
  macrolet wrappers; the later Architecting-XEmacs proposal (below) was an
  attempt to replace exactly that workflow. Neither was completed.

Useful residue: the commented overlay-on-extents group is a useful behavioural
specification for milestone 2, but the code is unfinished and should not be
resurrected as-is.

### 1.2 `fsf-compat`

- Not in-tree: there is no `lisp/overlay.el` or `lisp/fsf-compat*`; the only
  occurrences of "fsf-compat" in `lisp/` are comments in `compat.el`
  (lines 95, 97, 108, 120, 137).
- Not installed: `locate-library "overlay"` => nil; there is no
  `~/.xemacs/xemacs-packages` directory in this environment.
- The overlay/nadvice probe therefore stops at `(require 'overlay)`:
  `overlay FAILED: (file-error "Cannot open load file" "overlay")`.
  `nadvice FAILED: (file-error "Cannot open load file" "advice")`.

Consequence: the overlay API must be supplied by whatever milestone 2 ships,
or by a separately installed package; the current build has no fallback.

### 1.3 Architecting-XEmacs "Elisp Compatibility Package" proposal

Source: `/tmp/m2compat/compat-package.html` (xemacs.org page, last modified
2026-09-25; author Ben Wing). It proposes replacing the earlier Sysdep package
with a package called Compat. Owner, effort and dependencies are all listed as
`???`, i.e. it was never staffed or implemented.

Design points worth carrying forward:

- A **compile-time generator** scans the consuming package's Lisp source
  (using `read` and a recursive list walk) and determines which compatibility
  calls are actually used, so only those definitions are emitted. This avoids
  Sysdep's problem of defining everything and causing cross-version
  interference.
- Generated definitions are prefixed with both package name and `compat`
  (`<package>-compat-*`), and are activated only inside a lexical
  `compat-execute` block (implemented with `macrolet`), so callers outside the
  block see native definitions.
- `compat-if-fboundp` / `compat-funcall` were proposed to avoid byte-compiler
  warnings when supporting multiple APIs. The page itself notes the warning
  suppression is "rather tricky" and needs more thought.
- Where an API already exists, wrappers call the native function (one extra
  call); the proposal explicitly rejects copying native function definitions
  because of interactions with advice.

Consequence: the namespace-clean, per-package, compile-time-generated approach
was the preferred direction, but its complexity (a build-time generator plus
macrolet code generation) is plausibly why it died. Any milestone-2 design that
revives it must budget for that machinery.

### 1.4 GNU `compat` 31.1.0.0

- Audience: package maintainers, not end users; no commands or user options.
- Layered: `compat.el` dispatches on `emacs-major-version` and pulls in
  `compat-31` down through `compat-26` for older hosts. On XEmacs
  (`emacs-major-version` 21) it tries the whole chain.
- Policy: never override an existing definition (`(unless (fboundp 'x) ...)`);
  signature extensions get separate `compat--foo` functions reachable through
  `compat-call` / `compat-function`.
- Requirements observed in source: `Package-Requires: ((emacs "25.1"))`;
  runtime `subr-x`; a `compat-25` feature; `version<`; `cl-lib`; GNU
  `macroexp-progn`; `with-eval-after-load`.
- Name collision: this repo already has `lisp/compat.el`, compiled to
  `build/lisp/compat.elc`; `locate-library "compat"` resolves to that file.
  An installed GNU package also named `compat.el` would collide with the core
  file depending on `load-path` order, and the core file provides nothing.
  Any decision to consume GNU `compat` by name must first remove or rename the
  dormant file.

## 2. GNU package findings (concrete probes)

All probes ran in batch against the build above with the extracted sources in
`/tmp/m2compat` (`src-*`) and probe scripts `t-cl-lib.el`, `t-compat-raw.el`,
`t-seq.el`, `t-overlay-nadvice.el`. Licences: all these GNU ELPA sources carry
GPLv3-or-later notices, compatible with this repo's GPLv3+.

| Package (version) | Host requirements | Probe result on 21.5.36 | Minimal shim set / blocker |
|---|---|---|---|
| `cl-lib` 0.7.1 | XEmacs `cl` (present); `defadvice` for `cl-position`/`cl-delete-duplicates` | Loads with a no-op `defadvice`; 21/23 probes pass. Pass: `cl-incf` 2, `cl-decf` 2, `cl-pushnew` (1), `cl-letf` 9, `cl-labels` 120 (warns "requires lexical-binding"), `cl-defstruct` 3, `cl-defun` 3, `cl-loop` (1 2 3), `cl-dolist` (2 1), `cl-remove-if` (1 3), `cl-remove-if-not` (2 4), `cl-find` 2, `cl-sort` (1 2 3), `cl-subseq` (2 3), `cl-mapcar` (2 3), `cl-every` t, `cl-some` t, `cl-coerce` [1 2], `cl-values` 1, `cl-multiple-value-bind` 3, `cl-gensym` t. Fail: `cl-flet` => `void-function cl-flet`; `cl-position` => autoload failed (its definition is inside the disabled advice). | No-op `defadvice`. Without it `require 'cl-lib` fails `void-function defadvice` and `cl-position` autoload is never installed. No `version<` or `subr-x` needed. |
| `seq` 2.24 | `cl-lib`, `version<` | Without `version<`: `require seq` fails `(void-function version<)`. With a comparator shim: fails `(invalid-read-syntax "Weird old-backquote syntax")` while loading `seq-24.el`; `seq-25.el` fails the same way. `seq.el` itself reads to EOF successfully; the dispatch selects `seq-24` on this host. Reading `seq-24.el` form-by-form also fails (offset 10706), i.e. it is a reader-level failure, not a runtime one. | None possible at Lisp level: source cannot be read. |
| `let-alist` 1.0.6 | `emacs` 24.1 per metadata | Source is unreadable: XEmacs reader signals `Invalid read syntax: . in wrong context` on `.field` symbols. | None possible at Lisp level; decisive, not debugged further. |
| `compat` 31.1.0.0 | `emacs` 25.1; `subr-x`, `compat-25`, `version<`, `cl-lib`, `macroexp-progn`, `with-eval-after-load` | No tested combination gets `require 'compat` to succeed. Plain load dies immediately on missing `subr-x`; with GNU helper stubs it still dies on `subr-x`; with `subr-x` + `version<` + helper stubs + `defadvice` stub it gets into `compat-26` and dies with `Malformed property list: ((with-current-buffer (or buffer-or-name ...) (sha1 ...))), ((...))` while expanding the `buffer-hash` `compat-defun`. No `compat loaded` line is ever printed. | Highest combination reached: `defadvice` stub + `GNU_STUB` (`macroexp-progn`, `with-eval-after-load`, `provide 'compat-25`) + `SUBRX_STUB` + `VERSION_SHIM`. Still fails in the `compat-macs--guard`/`compat-defun` macro path. |
| `nadvice` 0.4 | `advice` | `require nadvice` fails at `require advice` (`Cannot open load file: "advice"`). With the no-op `defadvice` stub, `advice-add` would still need real advice internals; not pursued. | `advice.el` is absent from the repo and the build. |
| `parse-time` (GNU source) | `cl-lib`, `iso8601`, `subr-x` (compile) | Fails immediately: `Cannot open load file: "iso8601"`. | `iso8601` absent; `subr-x` absent. |
| `time-date` (exploratory, not in ticket list) | `cl-lib`, `subr-x` (runtime) | Fails: `Cannot open load file: "subr-x"`. | A runtime `subr-x` shim might suffice; not pursued (out of scope). |
| Overlay API | `overlay` (fsf-compat) | `locate-library "overlay"` => nil; probe fails at `require overlay`. | `fsf-compat` not in tree, not installed. |

Host capability probes relevant to all of the above:

```
emacs-version        => "21.5  (beta36) \"leeks\" XEmacs Lucid"
emacs-major-version  => 21
version< / version<= => nil / nil
subr-x               => nil
pcase                => t
cl-defgeneric        => nil
gv-letplace          => nil
gv-define-setter     => t
```

### Exact probe commands

```sh
B=/home/fankai/projects/xemacs/xemacs-git/build/bin/xemacs
L=/tmp/m2compat
STUB='(defmacro defadvice (&rest a) nil)'

# cl-lib
DEFADVICE_STUB=1 CL_LIB_DIR=$L/src-cl-lib-0.7.1/cl-lib-0.7.1 \
  $B -batch -l $L/t-cl-lib.el

# compat, lowest and highest shim combinations
COMPAT_DIR=$L/src-compat-31.1.0.0/compat-31.1.0.0 \
  CL_LIB_DIR=$L/src-cl-lib-0.7.1/cl-lib-0.7.1 \
  $B -batch --eval "$STUB" -l $L/t-compat-raw.el

COMPAT_DIR=$L/src-compat-31.1.0.0/compat-31.1.0.0 \
  CL_LIB_DIR=$L/src-cl-lib-0.7.1/cl-lib-0.7.1 \
  GNU_STUB=1 SUBRX_STUB=1 VERSION_SHIM=1 \
  $B -batch --eval "$STUB" -l $L/t-compat-raw.el

# seq
SEQ_DIR=$L/src-seq-2.24/seq-2.24 \
  CL_LIB_DIR=$L/src-cl-lib-0.7.1/cl-lib-0.7.1 VERSION_SHIM=1 \
  $B -batch --eval "$STUB" -l $L/t-seq.el

# overlay + nadvice
ADVICE_DIR=$L/src-nadvice-0.4/nadvice-0.4 \
  CL_LIB_DIR=$L/src-cl-lib-0.7.1/cl-lib-0.7.1 \
  $B -batch --eval "$STUB" -l $L/t-overlay-nadvice.el
```

## 3. Design axes for ticket #49

These are the axes the architecture decision must cover, with the evidence
that constrains each. No choice is made here.

### 3.1 Core `lisp/` versus installable package

Evidence:

- This build has no XEmacs package tree at all (`~/.xemacs/xemacs-packages`
  does not exist), so an installable layer has no working delivery channel in
  the current environment; milestone 1's ELPA client is only now being built.
- A package named `compat.el` collides with the dormant core file, which is
  already on `load-path` and `locate-library`-visible.
- Several requirements are needed at dump/startup time and by core code
  (`version<`, `subr-x`, overlay), not only by installed packages.
- An installable package gets independent versioning and can be updated
  without rebuilding XEmacs; core code gets guaranteed availability and no
  package-system dependency.

Questions #49 must answer: which symbols must exist before any package loads;
whether the layer is dumped into the core or autoloaded; how a core layer is
versioned relative to XEmacs releases.

### 3.2 Feature provisioning

Evidence:

- `provide 'cl-lib` is all-or-nothing for consumers: a package that requires
  `cl-lib` will assume the full API. A partial layer that still provides
  `cl-lib` turns a load-time error into a runtime `void-function`. The cl-lib
  probe shows exactly this split (21 pass, `cl-flet`/`cl-position` fail).
- GNU packages also test features directly: `compat` needs a `compat-25`
  feature, `seq` dispatches on `version<` and loads `seq-24`, and `parse-time`
  needs `iso8601`.
- Overlay provisioning is a semantic mapping onto extents, not a mechanical
  alias; `provide 'overlay` asserts a contract (see 3.4).
- `subr-x` is a common transitive requirement (`compat`, `parse-time`,
  `time-date`) and is small enough to be a high-leverage provision.

Options to decide: provide only complete features; provide documented
sub-features in addition; or provide the feature plus a capability variable
that consumers may test. Also: should `provide` happen unconditionally at
load, or only after self-tests?

### 3.3 Namespacing and collision policy

Evidence:

- The dormant `compat.el` chose strict per-package prefixes plus lexical
  macrolet, at the cost of a generator and copy-per-package workflow.
- GNU `compat` keeps internals under `compat--` but defines public generic
  names only when absent; it never overrides natives.
- The overlay API must expose generic names (`overlay-put`, `make-overlay`)
  because third-party packages call them directly; prefixed wrappers alone
  would not satisfy MELPA packages.
- `lisp/compat.el` and a GNU `compat` package cannot both own the name.

Decision inputs: global generic names for APIs that packages call directly
(overlay, cl-lib, subr-x); prefixed internals for implementation helpers;
whether to retain any per-package wrapping at all; what to do with the dormant
`compat.el` (delete, rename, or leave as documentation).

### 3.4 Per-definition tests

Evidence:

- `require` success is a weak signal. `cl-lib` loads but two of its advertised
  functions fail; `seq` reads partly but cannot load; `compat` fails midway
  through its layer chain.
- The probe style used here (one process per package, a list of
  `(name . form)` cases, print value or `ERROR`) distinguished all of these.
- Overlay semantics need their own matrix: zero-length overlays, `evaporate`,
  front/rear advance, `overlay-buffer` under narrowing, `overlays-at` vs
  `overlays-in`, modification hooks, and `copy-overlay`. Extents are not
  overlays; each difference must be tested or documented as unsupported.

Decision inputs: test granularity (per symbol, per behaviour), where tests
live (core test suite vs package), and whether the layer refuses to `provide`
a feature when its self-tests fail.

### 3.5 Compiled-out-when-native

Evidence:

- GNU `compat`'s model is runtime `unless (fboundp ...)` plus separate
  `compat--foo` definitions for signature extensions; nothing is defined on a
  host that already has the API, so there is no performance cost.
- XEmacs core has some modern names already (`pcase`, `gv-define-setter`) and
  lacks others (`version<`, `subr-x`, `cl-defgeneric`, `gv-letplace`); a layer
  must not clobber the former.
- The byte compiler runs on the host, so "compiled out" means the defining
  forms are conditional at load/compile time, not removed from the source.
- The dormant `compat.el`'s `compat-fboundp`/`compat-wrap-runtime` idea and the
  proposal's `compat-if-fboundp` both target the same problem: suppressing
  compiler warnings without hiding genuine ones.

Decision inputs: definition guard style (`unless fboundp` vs feature tests),
whether to alias native functions or wrap them, and how to test both the
native-present and native-absent paths on one host.

### 3.6 Versioning

Evidence:

- GNU `compat` versions by target Emacs (31.1.0.0) and dispatches on
  `emacs-major-version`; `seq` dispatches on `version<`.
- XEmacs has no `version<`/`version<=` at all, so any GNU-derived dispatch
  breaks before it starts.
- XEmacs's own version string is `21.5 (beta36)`, with a separate beta
  component; a comparator must handle it.
- The layer itself needs a version independent of the XEmacs release if it is
  installable, or a documented tie to the release if it is core.

Decision inputs: implement `version<`/`version<=` in core first; define the
layer's own version scheme; decide whether consumers may request a minimum
layer version; record which upstream package versions the layer targets.

## 4. Recommendations

These are inputs to #49, not the decision.

1. **Do not vendor GNU `compat`.** It does not load even with every identified
   helper stubbed, and its macro layer trips the 21.5 macroexpander. A port of
   selected definitions is the realistic path.
2. **Treat `version<`/`version<=` and a minimal `subr-x` as prerequisites.**
   They are small, absent, and block multiple packages (`seq` dispatch,
   `compat`, `parse-time`, `time-date`). Implementing them first unblocks
   further measurements.
3. **Do not ship upstream `cl-lib` verbatim.** It depends on a no-op
   `defadvice` and leaves `cl-flet`/`cl-position` broken. Implement the
   missing pieces directly (no advice) or fix those two definitions; keep the
   21 passing behaviours as the regression baseline.
4. **Implement the overlay API over extents from scratch, using the commented
   `fsf-compat` group in `lisp/compat.el` as a behavioural checklist.** Do not
   unquote the dormant file; it is incomplete (`defun-compat` undefined) and
   predates the proposal it was superseded by.
5. **Defer `seq`, `let-alist`, and `parse-time`.** Their failures are
   reader-level (`.field`, old-backquote) or dependency-level (`iso8601`), not
   shimmable. Fixing them would require core reader work or bundling further
   libraries, both outside a compatibility layer.
6. **Resolve the `compat.el` name collision before any package named
   `compat` is considered.** Deleting or renaming the dormant core file is the
   cheapest option; leaving both is a latent load-order bug.
7. **Adopt per-definition tests as the acceptance criterion**, not
   `require` success, and include read/load smoke tests for whole packages in
   isolated batch processes.
8. **Keep the architecture choice in #49.** The evidence favours a core layer
   for availability and collision reasons, but an installable package has real
   versioning benefits; that trade-off is #49's to make.

### ADR candidates

- **Compatibility layer location and delivery**: core `lisp/` vs installable
  package; includes the `compat.el` collision resolution.
- **Feature provisioning policy**: when it is legitimate to `provide` a
  feature; all-or-nothing vs documented subsets; self-test gating.
- **No-override rule**: never redefine an existing native function; use
  separate compat-named definitions for signature extensions.
- **Overlay-on-extents semantic contract**: which overlay behaviours are
  supported, which differ, and how they are tested.
- **Version dispatch**: implementing `version<`/`version<=` and the layer's
  own versioning scheme.

## 5. Open questions and blockers

- **Reader limitations are decisive for `seq` and `let-alist`.** `seq-24.el`
  and `seq-25.el` fail with `Weird old-backquote syntax`; `let-alist` fails on
  `.field`. Are these fixable in the C reader, and is that in scope for any
  milestone? Until answered, those packages are out.
- **Root cause of GNU `compat`'s `Malformed property list`** in `compat-26`
  is unidentified. It happens while expanding a `compat-defun` whose body is
  duplicated in the error payload; this may be a 21.5 macroexpander
  limitation, but it was not chased (out of budget/scope).
- **`advice.el` is absent**, so `nadvice` cannot be evaluated at all. Is
  `advice.el` available in any XEmacs package release for this build? Not
  determinable here.
- **`iso8601` is absent**, so `parse-time` cannot be evaluated even with
  `cl-lib` and `subr-x`.
- **Demand is unmeasured**: which MELPA packages targeted by milestone 2
  actually require `cl-lib`, `seq`, `let-alist`, `compat`, overlay, or
  `parse-time`, and in which combinations. This should come from the package
  survey on #45.
- **Overlay/extent semantic gaps** (zero-length overlays, evaporate,
  front/rear advance, narrowing, hooks) need a test matrix before any
  `provide 'overlay` claim.
- **Licensing** is compatible (all investigated sources GPLv3+; repo GPLv3+),
  but copied definitions must retain FSF copyright headers and notices.

## Appendix: artifacts

- Worktree: `/home/fankai/projects/xemacs/.qoder/worktrees/research-melpa-m2-compat`
- Extracted sources and probes: `/tmp/m2compat/` (not committed; throwaway)
  - `src-cl-lib-0.7.1/`, `src-compat-31.1.0.0/`, `src-let-alist-1.0.6/`,
    `src-seq-2.24/`, `src-nadvice-0.4/`
  - `t-cl-lib.el`, `t-compat-raw.el`, `t-seq.el`, `t-overlay-nadvice.el`
  - `compat-package.html` (Architecting-XEmacs proposal)
  - `parse-time-gnu.el`, `time-date-gnu.el`
