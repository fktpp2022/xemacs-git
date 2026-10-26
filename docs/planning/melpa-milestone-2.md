# MELPA Milestone 2: compatibility layer and cross-archive dependencies

Status: accepted through wayfinder tickets
[#46](https://github.com/fktpp2022/xemacs-git/issues/46),
[#47](https://github.com/fktpp2022/xemacs-git/issues/47),
[#48](https://github.com/fktpp2022/xemacs-git/issues/48),
[#49](https://github.com/fktpp2022/xemacs-git/issues/49),
[#50](https://github.com/fktpp2022/xemacs-git/issues/50),
[#51](https://github.com/fktpp2022/xemacs-git/issues/51), and
[#52](https://github.com/fktpp2022/xemacs-git/issues/52), 2026-10-26. This is the second
MELPA milestone for the map
[MELPA support for XEmacs](https://github.com/fktpp2022/xemacs-git/issues/30); it builds on
[MELPA Milestone 1](melpa-milestone-1.md), whose implementation is
`integration/melpa-milestone-1` (78389c1dc), itself built on ELPA Milestone 1
(`integration/elpa-milestone-1`, 12a9e735c).

Inputs:

- [#46](https://github.com/fktpp2022/xemacs-git/issues/46): behavioral audit; four of sixteen
  audited packages run, the rest expose dependency, reader, core-library, or lexical walls.
- [#47](https://github.com/fktpp2022/xemacs-git/issues/47): compatibility precedents; GNU `cl-lib`
  0.7.1 is loadable with a no-op `defadvice`, `seq`/`let-alist`/`compat` are not.
- [#48](https://github.com/fktpp2022/xemacs-git/issues/48): cross-archive map; the single-archive
  assumption lives at `elpa-install` and `elpa--install-dependency`.
- [#49](https://github.com/fktpp2022/xemacs-git/issues/49): compatibility-layer architecture,
  amended 2026-10-26 to the exercised surface.
- [#50](https://github.com/fktpp2022/xemacs-git/issues/50): cross-archive dependency semantics,
  amended 2026-10-26.
- [#51](https://github.com/fktpp2022/xemacs-git/issues/51): lexical-binding boundary; unchanged.
- [#52](https://github.com/fktpp2022/xemacs-git/issues/52): target set and success test.

Research evidence lives on throwaway branches: `docs/research/melpa-m2-audit.md`
(`research/melpa-m2-audit`, 2344b2c7e), `docs/research/melpa-m2-compat.md`
(`research/melpa-m2-compat`, 4cb711e34), and `docs/research/melpa-m2-cross-archive.md`
(`research/melpa-m2-cross-archive`, 532e95fd4).

## 1. Goal

XEmacs installs and runs four deliberately chosen, non-trivial MELPA packages through the
existing ELPA client, and gains the first compatibility layer for GNU-only package APIs plus
cross-archive dependency resolution. The milestone keeps milestone 1's shape and counts as done
when:

1. the deterministic offline fixture installs the four targets, the synthetic cross-archive pair
   (a MELPA-style dependent on GNU ELPA `queue`), and the carried-over cases, then exercises each
   in a fresh child XEmacs process; and
2. a live HTTPS smoke test refreshes the MELPA and GNU ELPA indexes in memory, installs `checkbox`
   from MELPA and `queue` from GNU ELPA without pinning versions, and exercises each.

No audited real package has a cross-archive dependency: all four targets declare only `cl-lib`,
which the layer provides. The resolver is therefore deliberate milestone-2 work, witnessed by a
synthetic dependent over a real vendored GNU artifact. The spec says so rather than dressing the
fixture as a package outcome.

## 2. User stories

1. As an XEmacs user, I want to install MELPA packages that require `cl-lib`, so that a large part
   of MELPA is no longer blocked by one missing library.
2. As an XEmacs user, I want a dependency that lives in GNU ELPA to install automatically from that
   archive, so that I do not need a second package system.
3. As an XEmacs user, I want a dependency missing from every archive, below its floor, or not on
   the allowlist to fail with one clear message, so that I know what to fix.
4. As an XEmacs user, I want a failed dependency install to leave neither a half-installed
   dependent nor staging residue, so that my package directory stays consistent.
5. As an XEmacs user, I want the same package name in two archives to resolve deterministically by
   configured order, so that installs are reproducible.
6. As an XEmacs user, I want a rolling target upgrade that still resolves its GNU ELPA dependency,
   so that upgrades do not break because a dependency moved archives.
7. As an XEmacs user, I want `when-let`, `if-let`, `defvar-local`, and `with-eval-after-load` to
   work without requiring anything first, so that unmodified upstream code runs.
8. As a package author, I want my unmodified tarball to install and run against the documented
   support surface, so that consuming MELPA does not require repackaging.
9. As a security-conscious user, I want every dependency name explicitly allowlisted, so that a
   dependency edge never grants trust implicitly.
10. As a security-conscious user, I want the trust model to remain allowlist-only, so that the
    cross-archive change does not quietly weaken ADR 0001.
11. As a maintainer, I want the offline gate to be network-free and pinned, so that CI is
    deterministic.
12. As a maintainer, I want the fixture to vendor only five real artifacts and build synthetics
    from checked-in sources, so that the repository stays reviewable and small.
13. As a maintainer, I want the live gate to prove both archives and the rolling path at real
    scale, so that the offline fixture cannot mask a network or index regression.
14. As a maintainer, I want the compatibility surface expressed as a tested contract, so that
    "supported" means "exercised", not "present in a vendored file".
15. As a test author, I want verification to use raw-byte `string=` rather than `md5` on binary
    content, so that the gate cannot crash the build.

## 3. Target set

Four packages, all installed from unmodified upstream tarballs at pinned MELPA date versions:

| Package | Pinned version | Manifest closure | Exercise form | Expected |
|---|---|---|---|---|
| `dmacro` | 20241027.830 | `emacs` (ignored), `cl-lib` 0.6 → layer | `(dmacro-array-search "ab" "xab" 0)` | `1` |
| `checkbox` | 20141117.58 | `emacs` (ignored), `cl-lib` 0.5 → layer | insert `"- Buy gin "` (trailing space, no newline), `(checkbox-toggle)`, read `buffer-string` | `"- [ ] Buy gin "` (trailing space) |
| `es-lib` | 20141111.1830 | `cl-lib` 0.3 → layer; bundled `es-lib-*`; core `apropos`, `loadhist` | `(es-string-remove-properties (propertize "abc" 'face 'bold))` | `"abc"` |
| `buffer-sets` | 20260817.2118 | `cl-lib` 0.5 → layer; optional `ibuf-macs` guarded by `with-eval-after-load` | create buffer `m2-buffer-sets-test`, associate with set `work`, then query membership | `t "work"` |

`queue` 0.2 (GNU ELPA) is a fixture and live artifact, not a promised target. `annalist`
(20260531.1558) and `general` (20250612.2309) remain hard controls, not targets: `annalist`'s one
`ob-core` compile error is fatal under `elpa--byte-compile-content`, and `general`'s macro layer
does not expand. Their audit evidence stays the boundary record.

## 4. Compatibility layer

### 4.1 Vendored `cl-lib`

GNU ELPA `cl-lib` 0.7.1 is vendored into the core tree, XEmacs-adapted, and delivered from
`lisp/`. The adaptation includes the no-op `defadvice` stub the source needs to load — it must not
claim to implement advice. The only behavioural patch is `cl-position`, which `checkbox` exercises;
`cl-flet`/`cl-flet*`/`cl-delete-duplicates` are **not** promised, and `cl-flet` remains a
documented gap until a target needs it.

Rules:

- all-or-nothing provisioning: `provide` only after every definition is installed;
- never override a native definition;
- no independent package version — the layer is core;
- per-definition tests in `tests/automated/`, including the `cl-position` patch.

### 4.2 Dumped `subr.el` additions

`when-let`, `if-let`, `defvar-local`, and `with-eval-after-load` are added to `lisp/subr.el` and
available at compile and load time. The targets call `when-let` (five sites) and `if-let` (three)
with no `require` at all, so a lazily loaded `subr-x` file could never satisfy them. `defvar-local`
and `with-eval-after-load` are `subr.el` upstream; dumping the two `*-let` macros is a deliberate
divergence from their upstream `subr-x` home and is documented as such.

There is no `subr-x` feature, no `version<`, and no `overlay` in this milestone. No target uses
them; `es-lib`'s overlay calls sit outside its exercised path. The dormant `lisp/compat.el` is
deleted after the orphan audit; there is no `compat-internal.el`.

### 4.3 Support contract

The contract is the set of symbols the four exercises actually touch, proven by compile + load +
exercise. The vendored `cl-lib` supplies its whole 0.7.1 surface, but only this subset is
guaranteed:

- `cl-lib`: `cl-defun`, `cl-defmacro`, `cl-destructuring-bind`, `cl-loop` (expanding to
  `cl-block`/`cl-return-from`), `cl-letf`, `cl-case`, `cl-incf`, `cl-dotimes`, `cl-macrolet`,
  `cl-assert`, `cl-position` (patched), `cl-rest`, `cl-member`, `cl-find-if`;
- dumped `subr.el`: `when-let`, `if-let`, `defvar-local`, `with-eval-after-load`;
- dumped `files.el` / `easy-mmode.el`: `locate-user-emacs-file` (buffer-sets evaluates it at
  load time) and `define-globalized-minor-mode` (dmacro calls it at load time; aliased to
  `easy-mmode-define-global-mode`);
- existing core: `derived`, `font-lock`, `apropos`, `loadhist`, `cl` (for `queue`'s compile-time
  require).

Anything outside this set is unsupported even if it happens to work. The manual states the
contract; the tests enforce it.

## 5. Cross-archive dependency resolution

### 5.1 Search order

A dependency requirement searches **all** configured archives in `elpa-archives` order and takes
the first entry satisfying the version floor. Top-level installs and explicitly versioned requests
keep the milestone-1 rule: the first configured archive. The dependent's own archive no longer
constrains the search — that is the change at `elpa--install-dependency`.

### 5.2 Allowlist and trust

Every dependency name, at every depth, must appear in `elpa-installable-packages`. There is no
automatic closure and no implicit trust edge. A name absent from the allowlist fails before any
download. ADR 0001 is unchanged: HTTPS plus the flat allowlist are the only controls.

### 5.3 Refresh

Refresh is lazy and per consulted archive: the first time resolution consults an archive in an
install operation, refresh it; no TTL and no background refresh. A top-level install keeps
milestone 1's refresh-before-install.

### 5.4 Identity and provenance

Package identity is the name plus version; archive identity is provenance, not identity. Artifacts
already carry `:archive`, which flows into the install record.

### 5.5 Atomicity

The whole required closure is preflighted before anything installs: every name allowlisted, present
in some archive, and satisfiable at its floor. After a green preflight, dependencies install in
order; on any install failure the dependent aborts and staging is cleaned. Completed dependencies
may remain (residue is limited to completed dependencies); there is no best-effort retry and no
second failure mode.

### 5.6 Core-provided requirements

`elpa-core-provided-packages` is a private `defconst` — not a Customize knob — defaulting to
`(cl cl-extra cl-lib)`. A requirement whose name appears here is satisfied without a download.
`emacs` is handled separately: ignored with one summary warning.

### 5.7 Cycles and errors

Dependency cycles are floor-checked and warned; they do not silently recurse. Missing packages,
unsatisfiable floors, and disallowed names all raise one unified dependency error naming the
offending package and the requirement that introduced it.

## 6. Client changes

- `elpa-install` keeps first-archive selection for the top-level package.
- `elpa--install-dependency` searches every configured archive in order rather than the
  dependent's archive.
- The core-provided list becomes a `defconst`.
- No other install, validation, compilation, activation, or replacement behaviour changes.
  Compilation errors remain fatal; warnings remain tolerated.
- `elpa-installable-packages` gains the four target names and `queue`. Synthetic fixture names are
  never defaults.

## 7. Security and trust

The milestone adds cross-archive resolution and a dependency allowlist, not a new trust mechanism.
The residual-risk statement from milestone 1 stands verbatim; the only extension is that a
dependency edge cannot introduce a name the user has not allowlisted. Synthetic packages exist
only in the test fixture and are never installed from a live archive.

## 8. Lexical binding

Unchanged from [#51](https://github.com/fktpp2022/xemacs-git/issues/51): interpreter-only lexical
binding stays out of milestone 2. The target rule is behavioural — `lexical-binding: t` is
non-disqualifying, the whole required closure is judged by compile/load/exercise, and a package is
excluded only when a closure member fails on closure capture. `ctable` and `fix-word` are named
exclusions, never patched.

## 9. Success test

### 9.1 Deterministic offline fixture

`fixtures/melpa-m2/` sits beside the ELPA-milestone-1 GNU fixture; m1's gates are untouched.
Layout:

- `melpa/archive-contents` — hand-written v1 index: the four targets and the synthetics;
- `melpa/<pkg>-<version>.tar` — the four real target tarballs, unmodified;
- `gnu/archive-contents` — hand-written v1 index: `queue`;
- `gnu/queue-0.2.tar` — the real GNU artifact, unmodified;
- `synthetic/<name>/{<name>.el,<name>-pkg.el}` — checked-in sources;
- `PROVENANCE` — for each real artifact: source URL, version, byte size, license.

The gate tars the synthetic sources into the fake archives at setup; no synthetic binary is
committed. Real artifact bytes: `dmacro` 10,240 + `checkbox` 20,480 + `es-lib` 71,680 +
`buffer-sets` 30,720 + `queue` 10,240 = **143,360 bytes**. Size cap: **192 KiB** for vendored
artifacts.

For each exercised package the gate installs from the two archives into a temporary package
directory and evaluates the form in a fresh child XEmacs, comparing the final non-empty output
line by raw-byte `string=`.

| Package | Exercise form | Expected |
|---|---|---|
| `dmacro` | `(dmacro-array-search "ab" "xab" 0)` | `1` |
| `checkbox` | toggle `"- Buy gin "` (trailing space, no newline) in a buffer, read `buffer-string` | `"- [ ] Buy gin "` |
| `es-lib` | `(es-string-remove-properties (propertize "abc" 'face 'bold))` | `"abc"` |
| `buffer-sets` | create `m2-buffer-sets-test`, associate `work`, query membership | `t "work"` |
| `m2-xdep` (v1) | `(let ((q (make-queue))) (queue-enqueue q 7) (m2-xdep-peek q))` | `7` |

`m2-xdep` v1 is a synthetic MELPA-style package (date version) whose `Package-Requires` names
`queue`; installing it must pull `queue` 0.2 from the fake GNU archive and record that provenance.

### 9.2 Negative cases

Carried over from milestone 1 unchanged: allowlist rejection before resolution; explicit version
no longer available; path-traversal and symlink tar members; missing artifact with no staging
residue; failed replacement leaves the old build intact; date-versioned upgrade replacement;
newest-keyed install queries.

New, all on the synthetic tree:

| Case | Fixture | Assertion |
|---|---|---|
| dependency not allowlisted | `m2-xdep-notallowed` requires `m2-untrusted`, which is in a fixture index but absent from the allowlist | one error naming `m2-untrusted`; neither package installed; no residue |
| dependency missing everywhere | `m2-xdep-missing` requires `m2-nowhere` | unified not-found error; no residue |
| version-floor conflict | `m2-xdep-floor` requires `queue` `9.9`; only 0.2 exists | floor error naming `queue`; no residue |
| failed dependency install | `m2-xdep-broken` requires `m2-corrupt`, whose artifact is truncated | dependent not installed; staging cleaned; completed dependencies may remain |
| duplicate across archives | `m2-dup` at `20261026.1200` in MELPA and `20261026.1100` in GNU, MELPA first | top-level install and dependency resolution both take the MELPA build; exactly one directory |
| rolling upgrade with a GNU dependency | `m2-xdep` v1 `20261026.1200` → v2 `20261026.1300`, still requiring `queue` | exactly one content directory at v2; `queue` 0.2 intact |

### 9.3 Live HTTPS smoke test

Gated on `ELPA_LIVE`, same shape as milestone 1:

- refresh both archives' full indexes in memory (MELPA's ~2.6 MB is the #37 full-scale check);
- install `checkbox` from MELPA and `queue` from GNU ELPA, both unpinned;
- assert each installed directory name matches the version the refreshed index advertised;
- exercise each in a fresh child XEmacs;
- keep the 404 index and 404 artifact negatives.

The live gate exercises top-level archive selection, not a transitive dependency path: no real
`MELPA → GNU ELPA` pair survives this build, so the transitive engine is proven offline only.
Excluding test files from byte-compilation to unlock `edn` → `peg` is rejected as speculative
compiler behaviour for a package the audit never ran.

### 9.4 Verification primitive

Raw-byte `string=` comparisons and behavioural assertions only. `md5` on binary string or buffer
content is banned in the success test; transport byte-exactness remains covered by the async-http
test from #37. No new checksum primitive is built.

## 10. Documentation

- `elpa.el` header: state that cross-archive dependency resolution exists and that every
  dependency name is allowlisted; keep the milestone-1 trust statement verbatim.
- `man/elpa.texi`: add a compatibility-layer chapter (the support contract, the dumped macros, the
  excluded APIs), a cross-archive chapter (search order, allowlist, preflight/atomicity,
  core-provided list, cycles), the target set, and the fixture/testing updates.
- `GLOSSARY.md`: add the terms this milestone introduces — **Compatibility layer**, **Support
  contract**, **Cross-archive dependency resolution**, **Core-provided package** — keeping the
  existing allowlist vocabulary aligned.

## 11. Out of scope

- Building packages from MELPA recipes; consuming prebuilt tarballs only.
- Publishing; signature or checksum verification (ADR 0001 unchanged).
- Lexical-binding support of any kind, including interpreter-only.
- `version<`, a `subr-x` feature, `overlay`, and any API surface outside the support contract.
- GNU core-library ports (`xml`, `color`, `help-mode`, `smie`, Org/BibTeX); the audit's hard
  controls.
- Excluding test files from byte-compilation.
- A real live cross-archive dependency pair; a package menu, deletion, rollback, or upgrade UX
  beyond replace-on-install.
- Windows and non-GNU `tar` variants.
- Milestone 1 implementation itself.

## 12. Further notes

- `dmacro` exercises `cl-loop`, whose macroexpansion produces `cl-block`/`cl-return-from`; the
  grep-visible source need not mention them.
- `checkbox` is the `cl-position` witness; without the patch its exercise fails.
- `queue` compiles with `(eval-when-compile (require 'cl))`; core `cl` satisfies that.
- `annalist` runs except for one fatal `ob-core` compile error; it is the boundary marker for
  Org-adjacent packages, not a target.
- MELPA index entries carry `:commit`/`:revdesc` extras; the client ignores extras.
- All vendored tarballs use the single-top-level-directory layout and a generated `-pkg.el`, so
  the existing tar and `-pkg.el` validation is exercised unchanged.
- The fixture's hand-written indexes use version 1 and minimal entries; they exist to drive
  resolution, not to mirror a live archive.
