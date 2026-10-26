# MELPA Milestone 1: install and run nine MELPA packages

Status: accepted through wayfinder tickets [#33](https://github.com/fktpp2022/xemacs-git/issues/33),
[#34](https://github.com/fktpp2022/xemacs-git/issues/34), [#35](https://github.com/fktpp2022/xemacs-git/issues/35),
[#36](https://github.com/fktpp2022/xemacs-git/issues/36), and [#37](https://github.com/fktpp2022/xemacs-git/issues/37),
2026-10-23. This is the first MELPA milestone for the map
[MELPA support for XEmacs](https://github.com/fktpp2022/xemacs-git/issues/30); it builds on
[ELPA Milestone 1](elpa-milestone-1.md).

Inputs:

- [#31](https://github.com/fktpp2022/xemacs-git/issues/31): MELPA archive mechanics; date versions,
  tar-only artifacts, no signatures, rolling index, no write path through `elpa.el`.
- [#32](https://github.com/fktpp2022/xemacs-git/issues/32): pool survey; 16 of 27 dependency-free
  candidates run dynamically, 1 needs lexical binding, and the large in-memory body crash.
- [#33](https://github.com/fktpp2022/xemacs-git/issues/33): lexical binding stays deferred; the
  behavioral audit, not the header, decides.
- [#34](https://github.com/fktpp2022/xemacs-git/issues/34): rolling-release version and upgrade policy.
- [#35](https://github.com/fktpp2022/xemacs-git/issues/35): allowlist-only trust for unsigned archives.
- [#36](https://github.com/fktpp2022/xemacs-git/issues/36): target set and success test.
- [#37](https://github.com/fktpp2022/xemacs-git/issues/37): large-body GC fix, already landed on
  `integration/elpa-milestone-1` (12a9e735c) with a regression test.

Research evidence lives on throwaway branches: `docs/research/melpa-archive-mechanics.md`
(`research/melpa-archive-mechanics`) and `docs/research/melpa-pool-survey.md`
(`research/melpa-pool-survey`).

## 1. Goal

XEmacs installs and runs a small, deliberately chosen set of packages from MELPA through the
existing ELPA client (`lisp/elpa.el`), consuming **prebuilt tarballs only**, with **zero new
compatibility code**. The milestone counts as done when:

1. the deterministic offline fixture installs all nine target packages and exercises one
   representative function in a fresh child XEmacs process, and
2. a live HTTPS smoke test refreshes `melpa.org`'s full index in memory, installs `levenshtein`
   without pinning a version, and exercises it.

Rolling releases force three client behavior changes: refresh the index before install, treat
"install" as "install whatever is current", and replace an older build with a newer one while
keeping exactly one active build per package. Date versions require no parser or comparator
change; they are ordinary integer version lists.

## User stories

1. As an XEmacs user, I want to install packages from MELPA through the same client that installs GNU ELPA packages, so that I do not need a second package system.
2. As an XEmacs user, I want `elpa-install` to install whatever build MELPA currently advertises, so that I get a working artifact instead of a 404 from a stale cache.
3. As an XEmacs user, I want re-running install to replace an older build with the current one, so that upgrades work without manual deletion.
4. As an XEmacs user, I want a failed replacement to leave the old build intact, so that a broken download never leaves me without the package.
5. As an XEmacs user, I want exactly one active build per package, so that load-path order is deterministic.
6. As an XEmacs user, I want an explicit version request to fail clearly when MELPA has pruned it, so that I understand why a pinned install cannot work.
7. As an XEmacs user, I want `elpa-installed-p` and `elpa--find-installed-dir` to name the newest build, so that install decisions reflect what I actually run.
8. As a package author, I want my unmodified prebuilt tarball to install, so that consuming MELPA does not require repackaging.
9. As a package author, I want a `lexical-binding: t` header to be evaluated behaviorally rather than treated as disqualifying, so that packages that run dynamically are not excluded on appearance.
10. As a security-conscious user, I want the trust model stated plainly in the manual and the client header, so that I know the allowlist is the only boundary besides HTTPS.
11. As a security-conscious user, I want the inert `elpa-check-signature` knob removed rather than left as a false promise, so that Customize does not imply verification.
12. As a maintainer, I want date-based versions to flow through the existing version machinery unchanged, so that MELPA support does not fork the version model.
13. As a maintainer, I want the offline success gate to pin exact date versions with vendored artifacts, so that CI is deterministic and network-free.
14. As a maintainer, I want the live smoke to refresh MELPA's full index in memory and install unpinned, so that the rolling path and the large-body fix are proven at real scale.
15. As a maintainer, I want negative cases (allowlist, unavailable version, traversal, symlink, missing artifact) to fail atomically, so that a bad archive cannot leave residue.
16. As a maintainer, I want the zero-shim policy enforced by the target set, so that this milestone ships no compatibility code.
17. As a maintainer, I want the per-package GNU API gap recorded as data, so that the future compatibility effort inherits requirements instead of rediscovering them.
18. As a future compatibility-effort owner, I want `memoize` documented as the lexical-binding negative, so that interpreter-only lexical binding has a concrete motivating case.
19. As a GNU ELPA user, I want the existing GNU install and live smoke paths to keep working, so that adding MELPA does not regress milestone 1.
20. As a test author, I want verification to use raw-byte `string=` rather than `md5` on binary content, so that the gate cannot crash the build.

## 2. Target set

Nine packages, all installed from unmodified upstream tarballs, all dependency-free apart from
the ignored `emacs` pseudo-dependency:

| Package | Pinned version | Note |
|---|---|---|
| `loop` | 20160813.1407 | control flow |
| `levenshtein` | 20090830.1040 | unchanged upstream since 2009; live-smoke pick |
| `csv` | 20231216.1243 | declares `lexical-binding: nil` |
| `vector-utils` | 20140508.2041 | `(require 'cl)`; stock `cl` covers it |
| `number` | 20170901.1312 | |
| `rings` | 20160531.2027 | `(require 'cl)` |
| `shell-split-string` | 20151224.1008 | |
| `kv` | 20140108.1534 | |
| `unidecode` | 20201213.1449 | `lexical-binding: t` and runs stock; the witness that a header alone is never disqualifying; Latin-only exercise |

**Scope policy: zero-shim.** This milestone ships no new compatibility code. The following are
excluded by that rule, with evidence in [#36](https://github.com/fktpp2022/xemacs-git/issues/36):

- `anaphora`, `move-text`, `xmlgen`, `list-utils`, `toml`: fail a stock load
  (`Cannot open load file: cl-lib`; `toml` also needs `parse-time` and other GNU APIs).
- `jump-char`, `persistent-overlays`, `rainbow-delimiters`, `string-inflection`: need shims.
- `memoize`: needs lexical binding; documented negative below.
- `password-generator`: stock load crashes the Mule reader without a coding-cookie change; the
  client consumes published artifacts unmodified.
- `browse-kill-ring`, `nginx-mode`, `paredit`, `regex-dsl`, `diffview`, `highlight-symbol`:
  deep-shim or dead in the survey.

The per-package API gap table in #36 is the requirements document for the future compatibility
effort; this milestone adds no compat layer and does not reverse any prior decision.

## 3. Client behavior changes

### 3.1 Date versions pass through unchanged

`version-to-list "20240331.1300"` returns `(20240331 1300)` and `elpa-version-join` renders it as
`20240331.1300`, byte-identical to the live artifact name and safe as a directory name. No new
version type, comparator, or parsing rule. Trailing-zero equality and dependency floors already
behave correctly. MELPA never emits prerelease markers or `single` artifacts, so the dormant
negative-number rendering hazard is out of scope.

### 3.2 Floating install with refresh-before-install

`elpa-install` refreshes the selected archive's index before resolving. A cached index is never
trusted for a rolling archive: old artifacts 404, so a stale cache can point at a dead download.
The local-directory "refresh" remains a file read, not a network fetch.

An explicitly requested version is honored only if the current index still offers it. Otherwise
the error is explicit: version N is no longer available in archive A (found M).

### 3.3 Replace-in-place, install-new-then-delete-old

Installing a package whose newer build is advertised installs the new build under
`<name>-<version>/`, publishes it, and only then deletes the old build. A failed new install
leaves the old build intact. After a successful replacement exactly one content directory exists
for the package. The layout stays GNU-shaped; there is no rollback or retention policy.

### 3.4 Newest-keyed install queries

`elpa--find-installed-dir` returns the **newest** installed version, not the first directory
encountered. `elpa-installed-p` without a version and `elpa--installed-at-least` key off the
newest build. This fixes today's bug where any on-disk build shadows the archive, re-install
silently does nothing, and an explicit version installs alongside the old one.

### 3.5 Allowlist default

`elpa-installable-packages` gains the nine target names. This is the milestone's trust grant; it
is inert until a MELPA archive is configured (trust is not source selection). `elpa-archives`
keeps its GNU-only default.

### 3.6 Unchanged behavior

The `single` artifact kind stays supported (GNU ELPA only; MELPA has none). Tar validation,
`-pkg.el` validation, byte-compilation, atomic staging, and activation are unchanged. Autoload
generation changes in one respect: a docstring-less `;;;###autoload` definition must not emit its
body form as the autoload docstring (see section 6.1). The `emacs` pseudo-dependency remains
ignored with a warning, which is expected for nearly every MELPA package.

## 4. Security and trust

This milestone adds an **unsigned-by-design archive** beside a **verification-deferred archive**.
The only controls are HTTPS transport and the flat per-package allowlist. There is no signature
verification, checksum pinning, or TLS pinning. The residual-risk statement, which goes verbatim
into the `elpa.el` header and expands here, is:

> Trust model: this client performs no signature verification. MELPA publishes no signatures, by
> design; GNU ELPA signature checking is deferred to a future core effort. The only controls are
> HTTPS transport and the `elpa-installable-packages' allowlist: if an archive is compromised or
> the connection is tampered with, any allowlisted package can run arbitrary code. The allowlist
> is not a cast-iron guarantee — keep it short, and install only packages you would trust with
> your keystrokes.

The dead `elpa-check-signature` defcustom is removed, not reworded: a boolean no code reads is an
inert Customize control. When verification lands it lands GNU-shaped: a multi-valued check knob
(`allow-unsigned` default), bad signatures never suppressed, per-archive exemptions, and both
the index and artifacts verified. The reasoning is recorded in
`docs/adr/0001-allowlist-only-trust-for-unsigned-package-archives.md`.

## 5. Lexical binding

Lexical binding is not on this milestone's critical path. The audit-only strategy carries over
from ELPA Milestone 1, now measured: 16 of 17 viable dependency-free candidates run under dynamic
binding. A `lexical-binding: t` header is **not disqualifying**; the behavioral audit decides.
`unidecode` is the target-set witness (declares `lexical-binding: t`, runs stock). `memoize` is
the documented negative: it closes over a `let`-bound hash table, and interpreter-only lexical
binding is deferred as its own future core effort. This milestone ships no interpreter or
compiler work.

## 6. Success test

### 6.1 Deterministic offline fixture

Vendor the nine real upstream tarballs at the pinned versions (593,920 bytes, about 580 KiB,
total) plus a hand-written version-1 `archive-contents` carrying those exact date versions. The
fixture lives beside the existing GNU fixture and documents provenance (source URL, version,
size, license); the artifacts remain unmodified. Synthetic hand-built packages cover the negative
cases.

For each target, the offline gate installs it from the fixture and exercises one representative
function in a fresh child XEmacs process. The no-kit audit fixed these forms and outputs; all
nine run with no shims:

| Package | Exercise form | Expected |
|---|---|---|
| `loop` | `(let ((i 0) (acc nil)) (loop-while t (setq i (1+ i)) (if (= i 3) (loop-continue)) (if (> i 5) (loop-break)) (push i acc)) (nreverse acc))` | `(1 2 4 5)` |
| `levenshtein` | `(levenshtein-distance "kitten" "sitting")` | `3` |
| `csv` | parse `"a,b,c\n1,2,3\n"` with `csv-parse-buffer` | `(("a" "b" "c") ("1" "2" "3"))` |
| `vector-utils` | `(vector-utils-flatten '[1 2 [3 4 [5 6 7]]])` | `[1 2 3 4 5 6 7]` |
| `number` | `(number-format (number-modify :number (lambda (x) (+ x 1)) (number-read "6.30")))` | `"7.30"` |
| `rings` | in a temp buffer, `(rings-add-buffer "demo")`, record `(length (rings-buffers "demo"))`, `(rings-remove-buffer "demo")`, then the second length | `(1 0)` |
| `shell-split-string` | `(shell-split-string "abc \"def ghi\"")` | `("abc" "def ghi")` |
| `kv` | `(list (kva 'b '((a . 1) (b . 2))) (kvalist->keys '((a . 1) (b . 2))))` | `(2 (a b))` |
| `unidecode` | `(unidecode "Crème Brûlée")` | `"Creme Brulee"` |

Each exercise's child prints the form's value; the gate compares the final non-empty line of
output (`rings-add-buffer` emits a message line before the value, so that child's output has a
preceding line).

The audit also found a client bug that the nine-target gate exposes: the autoload generator used
`(nth 3 form)` as a docstring unconditionally, so a docstring-less cookie'd macro (as in `rings`)
embedded its body in the autoloads file and aborted `elpa-activate-all` with `void-variable key`.
The generator now emits a docstring only when it is a string. This is client correctness, not a
compatibility shim; the MELPA tarball stays unmodified. `rings` activation is the regression
witness. The target exercises deliberately avoid APIs stock XEmacs lacks (`number`'s point
commands need `deactivate-mark`; `loop`'s line iterator needs `rx`/`0+`); they are not part of the
guarantee.

**Verification primitive:** raw-byte `string=` comparisons and behavioral assertions only.
`md5` on binary string or buffer content is banned in the success test (it aborts on some byte
patterns regardless of transport). Transport byte-exactness remains covered by the async-http
test from #37. No new checksum primitive is built.

### 6.2 Negative cases

Carried over from ELPA Milestone 1 and updated for date versions:

- allowlist rejection before any resolution;
- explicit version unavailable in the current index ("no longer available in archive");
- path-traversal and symlink tar members rejected before extraction;
- missing artifact errors cleanly with no staging residue.

New, red until the rolling-release client changes land:

- date-versioned upgrade: install v1 from an index, bump the index to v2, re-install, and assert
  install-new-then-delete-old with exactly one directory remaining;
- newest-keyed `elpa-installed-p` and `elpa--find-installed-dir`;
- failed replacement leaves the old build intact.

### 6.3 Live HTTPS smoke test

Gated on `ELPA_LIVE`, same shape as ELPA Milestone 1:

- refresh `https://melpa.org/packages/`'s full index **in memory** (about 2.6 MB; the #37 live
  gate at full scale);
- install `levenshtein` with no version pinned;
- assert the installed directory name matches the date version the index advertised;
- exercise it in a fresh child XEmacs;
- keep the 404 index and 404 artifact negatives.

The existing GNU ELPA live smoke stays in place. `stable.melpa.org` is excluded: its semantic
versions do not exercise the date-version machinery.

## 7. Documentation

- `elpa.el` header: replace the deferred-features line with the trust-model statement and record
  that MELPA and rolling releases are supported.
- `man/elpa.texi`: add MELPA to the introduction and installation guide; rewrite the upgrade
  paragraph for replace-in-place; document floating installs and refresh-before-install; remove
  the `elpa-check-signature` entry; expand Security for unsigned-by-design archives; record the
  nine-package target set, the zero-shim policy, and `memoize` as a documented negative; update
  the Testing and Limitations chapters.
- Keep the allowlist and trust vocabulary aligned with `GLOSSARY.md`.

## 8. Out of scope

- Building packages from MELPA recipes; consuming prebuilt tarballs only.
- Full lexical-binding compiler or VM support; interpreter-only lexical binding is a named future
  core effort.
- A compatibility layer for GNU-only package APIs (`cl-lib`, `parse-time`, overlays). The #36 gap
  table is its requirements document.
- Cross-archive dependency resolution: the target set is dependency-free; MELPA's dependency wall
  is deferred to the future compat effort.
- Signature or checksum verification; publishing packages; a package menu, deletion, rollback, or
  user-facing upgrade command beyond replace-on-install.
- Windows and non-GNU `tar` variants.

## 9. Further notes

- Ticket [#43](https://github.com/fktpp2022/xemacs-git/issues/43) fixes the autoload docstring bug
  the no-kit audit found; it is a prerequisite for the `rings` witness and carries its own
  regression test.

- The large in-memory body crash is fixed in 12a9e735c on `integration/elpa-milestone-1`; the live
  smoke's 2.6 MB refresh is its full-scale acceptance check.
- MELPA index entries include `:commit` and `:revdesc` extras; the client ignores extras.
- All target tarballs have the single-top-level-directory layout and a generated `-pkg.el`, so
  the existing tar validation is exercised unchanged.
- `single` kind gets no live coverage from MELPA; GNU ELPA's `loc-changes` remains its test.
- `md5` on binary content is a known crash; use byte-length and `string=` comparisons.
