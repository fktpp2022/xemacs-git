# ELPA Milestone 1: install and run `queue` and `loc-changes`

Status: accepted in wayfinder [#21](https://github.com/fktpp2022/xemacs-git/issues/21), 2026-10-22.

This is the scoped first milestone for the map [ELPA/MELPA compatibility for XEmacs](https://github.com/fktpp2022/xemacs-git/issues/12). It turns the layer, install-strategy, lexical-binding, and transport decisions into a concrete deliverable and an executable success test.

Inputs:

- [#14](https://github.com/fktpp2022/xemacs-git/issues/14): GNU `package.el` minimum install path.
- [#16](https://github.com/fktpp2022/xemacs-git/issues/16): missing compatibility APIs; `version-to-list` and `cl-lib` are the minimum shims.
- [#17](https://github.com/fktpp2022/xemacs-git/issues/17): `async-http`/libcurl is the transport; the body accumulator is not binary-safe and there is no file sink.
- [#18](https://github.com/fktpp2022/xemacs-git/issues/18): core Lisp owns the installer; core C owns transport; external `tar`; reuse `autoload.el` and `bytecomp.el`.
- [#19](https://github.com/fktpp2022/xemacs-git/issues/19): minimal purpose-built installer, GNU-style `<name>-<version>` directories, allow-unsigned, install only, byte-compile on install, no UI.
- [#20](https://github.com/fktpp2022/xemacs-git/issues/20): restrict the target set to packages audited to work under dynamic binding.
- [#21](https://github.com/fktpp2022/xemacs-git/issues/21): target set, `emacs` pseudo-dependency policy, signature scope, and test form decided below.

## 1. Goal

XEmacs can consume two real GNU ELPA packages through a new, small `package.el`-compatible client:

1. `queue` 0.2, a `tar` package with no dependencies, and
2. `loc-changes` 1.2, a `single`-file package with no dependencies,

and then load and run their core functionality in a fresh XEmacs process. Both archive kinds, byte-compilation, autoload generation, and explicit activation are exercised. Nothing else is required for the milestone to count as done.

## 2. Target packages

| Package | Version | Kind | Requirements | Artifact | Audit result |
|---|---|---|---|---|---|
| `queue` | 0.2 | `tar` | none | `queue-0.2.tar`, 10,240 bytes, md5 `c1f635568dce3ab01b5adad023833ff2` | Byte-compiles cleanly; loads; `queue-create`/`enqueue`/`dequeue`/`empty`/`length`/`first` pass. Header declares `lexical-binding: t`, but only `defstruct` accessors and non-capturing functions are used. The only `iter-defun` is guarded by `(require 'generator nil 'noerror)` at macro-expansion time, so it is omitted when `generator` is absent. |
| `loc-changes` | 1.2 | `single` | none | `loc-changes-1.2.el`, 9,667 bytes, md5 `67efcdb515c75aea196429bea1504516` | Byte-compiles with warnings for GNU-only functions; loads; marker tracking (`loc-changes-add-and-goto`, `loc-changes-goto`) passes after three one-line compatibility shims. No `lexical-binding` declaration. |

Both packages are dependencies-free in the GNU ELPA index:

```elisp
(queue . [(0 2) nil "Queue data structure" tar ...])
(loc-changes . [(1 2) nil "keep track of positions even after buffer changes" single ...])
```

### Why this pair

- It covers both archive kinds (`tar` and `single`) with runnable packages, not install-only fixtures.
- Both pass the dynamic-binding audit behaviorally: compiled, loaded, and exercised, not merely inspected for a header.
- Neither needs `cl-lib`, `seq`, GNU timers, `message`, or `thingatpt`. Those are the APIs that eliminated the other candidates.
- `queue` includes a `;;;###autoload` cookie for `queue-create`; `loc-changes` has four, so autoload generation is observable in both.

### Audit evidence

Run with the `xemacs-cmake` build at `8e6ec57c6` (XEmacs 21.5 beta36, Mule):

```
queue:        deq1=a deq2=b empty=t len=0 first=nil
loc-changes:  before=7 after=11 line=2 text="beta"
```

The `loc-changes` result requires these compatibility definitions, which are milestone deliverables:

```elisp
(defalias 'wholenump 'natnump)

(defun line-number-at-pos (&optional pos)
  (save-excursion
    (when pos (goto-char pos))
    (1+ (count-lines (point-min) (point)))))

;; Force a redisplay; batch mode treats it as a no-op.
(defun redisplay (&rest _)
  (sit-for 0))
```

The remaining GNU-only calls in `loc-changes` (`posn-point`, `event-end`, `insert-text-button`, `move-end-of-line`) are on interactive paths that milestone 1 does not exercise; they are deferred, not silently claimed to work.

### Rejected candidates

| Package | Kind | Blocker |
|---|---|---|
| `spinner` 1.7.4 | tar | Needs `cl-lib` and GNU timer API (`timer-create`, `timer-set-function`, `cancel-timer`, `timer-next-integral-multiple-of-time`); XEmacs has no GNU timer library. |
| `seq` 2.24 | tar | Loader needs `version<`; the Emacs 24 fallback fails to read under XEmacs ("Weird old-backquote syntax"). |
| `timerfunctions` 1.4.2 | single | Loads and `tf-time-difference` works, but the timer core needs `run-with-timer`/`run-with-idle-timer`/`cancel-timer`, none in XEmacs. |
| `diffview` 1.0 | single | Requires the add-on `message` library. |
| `f90-interface-browser`, `rcirc-menu` | single | Require `thingatpt`/`rcirc`, not in core. |
| `epoch-view` | single | Needs `seconds-to-time` and `font-lock-add-keywords`, absent in XEmacs. |
| `pspp-mode`, `windower`, `register-list` | single/tar | Load or byte-compile fails on XEmacs syntax/API differences. |

## 3. Scope

In scope:

1. A new core Lisp client, `lisp/elpa.el`, implementing archive refresh, index parsing, resolution, download, install, autoload generation, byte-compilation, and activation.
2. Binary-safe artifact download and a file sink in `src/async-http.c` (the #17 prerequisite). Package bytes must never be round-tripped through a Lisp string.
3. The `version-to-list`/`version-list-*` core port from #18, and the three target-driven compatibility definitions above.
4. Local-directory archive support for deterministic tests, alongside HTTPS.
5. An offline CTest suite using a vendored fixture archive, plus one live HTTPS smoke test.

Out of scope: everything in section 9.

The `cl-lib`/`cl-defstruct` vendor decided in #18 remains a core deliverable, but neither target package uses it, so it does not gate this milestone's acceptance test. (It also would not unlock `spinner` or `timerfunctions`, which fail on the GNU timer API.)

## 4. Layers and files

| Layer | File | Responsibility |
|---|---|---|
| Core C | `src/async-http.c` | Binary-safe response accumulator; file-destination write callback; redirect cap; connect timeout. |
| Core Lisp | `lisp/elpa.el` | Index parsing, resolution, install pipeline, activation. |
| Core Lisp | `lisp/subr.el` | `version-to-list`, `version-list-<`, `version-list-<=`, `version-list-=`, `line-number-at-pos`, `wholenump`, `redisplay` fallback (only define when absent). |
| Core Lisp | `lisp/startup.el` or `lisp/loadup.el` | Call `elpa-activate-all` at startup when enabled. |
| External | `tar` | Extract validated tarballs only. |
| Core Lisp | `lisp/autoload.el`, `lisp/bytecomp.el` | Autoload extraction and byte-compilation. |
| Tests | `tests/elpa-tests.el`, `tests/elpa/` | Offline fixture, negative tests, fresh-process checks. |

## 5. API and data model

### Variables

```elisp
(defcustom elpa-archives
  '(("gnu" . "https://elpa.gnu.org/packages/"))
  ...)

(defcustom elpa-user-dir
  (expand-file-name "~/.emacs.d/elpa/")
  ...)

(defcustom elpa-installable-packages
  '(queue loc-changes)
  "Allowlist of packages milestone 1 may install.
The emacs pseudo-dependency is not trusted; this list is the safety boundary.")

(defcustom elpa-check-signature nil
  "Milestone 1 installs unsigned archives; .sig files are ignored.")

(defcustom elpa-activate-at-startup t
  ...)
```

`elpa-user-dir` is created on demand. The layout is GNU-style and unversioned outside the package directory:

```
~/.emacs.d/elpa/
  archives/gnu/archive-contents        ; cached index
  queue-0.2/queue.el queue-pkg.el queue.elc queue-autoloads.el queue-autoloads.elc
  loc-changes-1.2/loc-changes.el loc-changes.elc loc-changes-autoloads.el loc-changes-autoloads.elc
  .staging/                            ; removed after install
```

### Functions

```elisp
(elpa-refresh-contents &optional archive)   ; fetch/parse, return package count
(elpa-install name &optional version)       ; resolve, download, install, return name
(elpa-installed-p name &optional version)   ; content directory exists
(elpa-activate-all)                         ; add load-path, load autoloads, idempotent
```

`elpa-install` is a Lisp API, not a menu. An interactive wrapper is optional; no `package-menu` work is included.

### Archive index

`archive-contents` is read as one Lisp form. The first element must be the integer `1`; any other archive version is an error. Each entry is `(name . [VERSION REQS SUMMARY KIND EXTRAS])`, where `VERSION` is a version list, `REQS` is `((dep (n n ...)) ...)`, and `KIND` is `tar` or `single`. `EXTRAS` is ignored except for diagnostics.

Accept a local directory as an archive location (absolute path) for tests; HTTP(S) locations go through `async-http`. The cached copy lives at `elpa-user-dir/archives/<archive>/archive-contents`; local archives are read directly on refresh.

### Resolution

- The name must be in `elpa-installable-packages`; otherwise signal an error naming the allowlist.
- If a content directory already exists, return it without overwriting. There is no upgrade path.
- Ignore `emacs` requirements entirely, but record a warning: `"ignoring emacs requirement %s for %s"`. The allowlist audit, not the declared Emacs version, decides compatibility.
- Resolve every other requirement: it must already be installed or be in the allowlist and installable. Install dependencies before dependents. Use a seen-set to break cycles. There are no requirements for the milestone targets, so this path is exercised only by unit-level tests.
- If a version is requested, require an exact match with the index entry; otherwise use the index version.

### Versions

Port GNU's `version-to-list` including `pre`, `beta`, `alpha`, and `snapshot` markers, plus `version-list-<`, `version-list-<=`, and `version-list-=`. Add `elpa-version-join` for directory and artifact names (`(0 2)` -> `"0.2"`). The milestone targets use only plain numeric versions, but the resolver must compare lists rather than strings.

## 6. Install pipeline

1. **Refresh**: fetch `<base>/archive-contents` (HTTPS or local). HTTP must return 200; 404 is a clear "archive not found" error. Cache it and parse it with the version-1 gate.
2. **Resolve**: apply the allowlist and dependency rules above.
3. **Download**: build `<base>/<name>-<version>.tar` for `tar` or `.el` for `single`. Use `async-http` with a file destination so bytes are written directly by C, never accumulated in a Lisp string. Index timeout 60 s, artifact timeout 300 s. Set `CURLOPT_MAXREDIRS` to 10.
4. **Stage**: download into `elpa-user-dir/.staging/`. For `tar`, list members with `tar -tf` first. Every member must be under the single top-level `<name>-<version>/`; reject absolute names, any `..` component, and symlink/hardlink entries. Then extract with external `tar -xf` into staging. For `single`, create `<name>-<version>/` and write the file as `<name>.el`; verify it is non-empty and looks like the named library.
5. **Validate metadata**: for `tar`, require `<name>-pkg.el` and read its `(define-package NAME VERSION ...)` form; name and version must match the index. `queue-0.2/queue-pkg.el` satisfies this.
6. **Generate autoloads**: scan every `.el` in the content directory (excluding the generated file) for `;;;###autoload` cookies, using `autoload.el` where practical and a minimal cookie scanner as fallback. Write `<name>-autoloads.el` whose first form adds the content directory to `load-path`, followed by the autoload forms. Do not trust or retain any autoloads file shipped in the tarball.
7. **Byte-compile**: add the content directory to `load-path`, then byte-compile every `.el` in it, including the generated autoloads. Warnings are logged and tolerated; a compile error fails the install. `loc-changes`' unknown-function warnings are expected.
8. **Publish**: rename the staged `<name>-<version>/` into `elpa-user-dir/<name>-<version>/` only after all steps succeed. Remove staging on failure. Then remove the downloaded artifact.

Installation is all-or-nothing at the directory level; concurrent installs are not supported in milestone 1.

## 7. Activation

`elpa-activate-all` scans the immediate subdirectories of `elpa-user-dir` whose names look like `<name>-<version>` and which contain `<name>-autoloads.el`. For each, it adds the directory to `load-path` (once) and loads the autoloads file (once). It ignores `archives/`, `.staging/`, and unrelated entries. It is idempotent.

Startup calls `elpa-activate-all` when `elpa-activate-at-startup` is non-nil and `elpa-user-dir` exists. This is the explicit activation seam from #19; it does not depend on XEmacs's hierarchy scan or the `auto-autoloads.el` convention. Quickstart/precomputation is deferred.

Success criteria for activation are observable without `require`:

- after activation, `(fboundp 'queue-create)` is `t` (from the queue autoloads);
- after activation, `(fboundp 'loc-changes-goto)` is `t` (from the loc-changes autoloads).

## 8. Signatures

Milestone 1 defers signature verification. `elpa-check-signature` is `nil`, `.sig` files are ignored, and a bad signature is not detected. This is the accepted allow-unsigned scope from [#19](https://github.com/fktpp2022/xemacs-git/issues/19) and [#21](https://github.com/fktpp2022/xemacs-git/issues/21), not a claim of GNU-default parity: GNU's `allow-unsigned` still rejects bad signatures when a signature is present.

Before the client is used against archives whose integrity matters, the follow-up must implement `gpg --verify`, fail on bad signatures, and keep unsigned installs allowed. That work is deferred, not dropped.

## 9. Success test

### Offline fixture

Vendor the two real artifacts and a version-1 index under `tests/elpa/fixture/`:

```
tests/elpa/fixture/archive-contents
tests/elpa/fixture/queue-0.2.tar
tests/elpa/fixture/loc-changes-1.2.el
tests/elpa/fixture/README
```

The README records source URLs, versions, commit hashes, GPL notices, sizes, and the md5 values above. The fixture index contains exactly the `queue` and `loc-changes` entries in the shape GNU ELPA publishes.

### Offline test (mandatory, no network)

Run with the existing batch harness, e.g.:

```
ctest --test-dir build --output-on-failure -R elpa
```

The driver sets a temporary `elpa-user-dir` (or `HOME`), points `elpa-archives` at the fixture directory, and asserts:

1. `elpa-refresh-contents` returns 2 and both names are present with the expected version lists and kinds.
2. `elpa-install "queue"` creates `queue-0.2/queue.el`, `queue.elc`, and `queue-autoloads.el`; the downloaded artifact is gone; staging is empty.
3. A fresh XEmacs process with the same `elpa-user-dir` calls `elpa-activate-all`; `(fboundp 'queue-create)` is `t` before `require`; then `(require 'queue)` and the functional check prints:
   `deq1=a deq2=b empty=t len=0 first=nil`.
4. `elpa-install "loc-changes"` creates `loc-changes-1.2/loc-changes.el`, `loc-changes.elc`, and `loc-changes-autoloads.el`.
5. A fresh process after activation has `(fboundp 'loc-changes-goto)` `t`; the marker test prints:
   `before=7 after=11 line=2 text="beta"`.
6. Negative cases:
   - `elpa-install "spinner"` errors because it is not in `elpa-installable-packages`;
   - an index whose first element is `2` errors;
   - a tarball containing `../evil.el` is rejected before extraction and no file appears outside staging;
   - a missing artifact produces a clear error and leaves no package directory behind.

Expected terminal line: `elpa tests: passed`.

### Live smoke (optional, network)

A separate CTest test labeled `network`, skipped unless enabled:

```
ELPA_LIVE=1 ctest --test-dir build --output-on-failure -R elpa-live
```

It fetches the live `https://elpa.gnu.org/packages/archive-contents`, installs whichever version of `queue` the index currently advertises into a temporary directory, and repeats the queue functional check. It must not pin `0.2`; the offline fixture is the deterministic version gate. The live test also proves binary-safe HTTPS tarball download end to end (the #17 crash path).

## 10. Acceptance checklist

- [ ] `async-http` writes artifact bodies through a file sink; the accumulator uses `make_extstring` with `Qbinary`; a downloaded `queue-0.2.tar` is byte-identical to `curl` (md5 match).
- [ ] Redirects are capped at 10 and a connect timeout is set.
- [ ] `version-to-list` and the three version comparators exist and pass unit checks including a prerelease case.
- [ ] `wholenump`, `line-number-at-pos`, and `redisplay` are available in core.
- [ ] `elpa.el` implements refresh, v1 gate, allowlist, emacs-pseudo-dep warning, local and HTTPS transport, download, validation, autoloads, byte-compilation, atomic publish, and activation.
- [ ] `queue` 0.2 installs from the fixture and runs.
- [ ] `loc-changes` 1.2 installs from the fixture and runs.
- [ ] Autoloads make `queue-create` and `loc-changes-goto` callable before `require`.
- [ ] Negative tests pass.
- [ ] Offline CTest suite passes; live smoke passes when enabled.
- [ ] No `package-menu`, upgrade, delete, quickstart, or signature code is required for the milestone.

## 11. Deferred

- Signature verification, keyrings, `.sig` handling, and fatal bad signatures (section 8).
- `package-menu`/`tabulated-list`, interactive browsing, upgrade, delete, and selected-package flows.
- Installing packages outside `elpa-installable-packages`; the allowlist is the milestone boundary.
- Semantic `emacs` version compatibility; the pseudo-dependency is ignored by policy.
- Dependency version conflict resolution and backtracking beyond the simple recursive resolver.
- `package-quickstart` and any startup precomputation.
- Lexical binding support; the next core step is interpreter-only lexical binding per #20.
- Full `loc-changes` interactive behavior (`posn-point`, `event-end`, `insert-text-button`, `move-end-of-line`).
- `cl-lib`/`cl-defstruct` vendor from #18 as a gate for this milestone.
- Cancellation, structured transport errors, proxy/CA knobs, synchronous DNS, and batch-safe `:on-chunk`.
- Checksum verification; the v1 index carries no checksum.
- Windows support and non-GNU `tar` variants.
- MELPA (rolling releases, recipes, build expectations) and `package-vc`.

## 12. Risks

- **Autoload generation.** XEmacs `autoload.el` may not reproduce GNU's generated file exactly. A minimal cookie scanner is an acceptable fallback; the acceptance test only requires the two cookies to become callable and the load-path form to run.
- **`redisplay` shim.** Defining it globally changes interactive redraw calls from a native redisplay to `(sit-for 0)`. This is intentional for `loc-changes`; it should be revisited if a core redisplay primitive is added.
- **Partial `loc-changes` support.** Only marker tracking is in scope. Users invoking the button/event paths will still hit missing GNU APIs; the doc and test must not imply full compatibility.
- **Live archive drift.** GNU ELPA may update or remove `queue`/`loc-changes`. The offline fixture pins the milestone; the live test follows the current index and must not hardcode a version.
- **Transport fixes are on the critical path.** Without the `Qbinary` fix and file sink, the HTTPS tarball install crashes or corrupts bytes. The milestone is not done while those remain open.
