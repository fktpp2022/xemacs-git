# Cross-archive dependency resolution in the ELPA client

Research for wayfinder ticket [#48](https://github.com/fktpp2022/xemacs-git/issues/48)
(map [#45](https://github.com/fktpp2022/xemacs-git/issues/45)), 2026-10-24.
Decision-support map only: no semantics are chosen here; the open points are
listed for the grilling ticket [#50](https://github.com/fktpp2022/xemacs-git/issues/50).

Sources are primary: `lisp/elpa.el`, `tests/automated/elpa-tests.el`,
`tests/automated/elpa-live-tests.el`, the fixtures, `man/elpa.texi`, and ADR
0001, all at commit `78389c1dc` on `integration/melpa-milestone-1` (the
commit this worktree was cut from; `git diff integration/melpa-milestone-1`
on the cited files is empty). Prior evidence: "MELPA's archive mechanics"
([ticket #31](https://github.com/fktpp2022/xemacs-git/issues/31), doc on
`research/melpa-archive-mechanics`), especially §5 (the dependency wall) and
§6 items 1, 7, 8. External precedent: GNU Emacs `package.el`
(emacs-mirror master, fetched 2026-10-24) for priority, pinning, transaction,
and cycle semantics.

## Bottom line

The client has exactly one archive-selection point per install: `elpa-install`
picks `(caar elpa-archives)` (`lisp/elpa.el:458`), refreshes only that archive
(`lisp/elpa.el:462`), and threads that one archive down through dependency
resolution (`lisp/elpa.el:500`, `lisp/elpa.el:518`). Every dependency is
looked up in the *dependent's* archive (`lisp/elpa.el:430`). A dependency that
lives only in another configured archive is never seen, and the failure is a
plain "not available in archive X" error.

The artifact path is already multi-archive-safe: a resolved info plist carries
`:archive` (`lisp/elpa.el:284`), and the download base is derived from that,
not from the caller (`lisp/elpa.el:570`). What is missing is a deterministic
search that produces such an info from another archive, a refresh policy that
keeps every consulted archive fresh, a trust decision for transitive names,
and a defined failure/atomicity story for multi-package installs.

## 1. Code map: every place an archive is chosen or threaded

| Site | What it decides | Current behavior |
|---|---|---|
| `elpa-archives` defcustom (`elpa.el:62-69`) | Configured archive list | Alist name -> location; default is GNU ELPA only |
| `elpa--archive-index-alist` (`elpa.el:96-97`) | Per-process parsed indexes | Alist keyed by archive name string |
| `elpa--archive-location` (`elpa.el:116-126`) | Resolve archive name to base | Accepts symbol/string; first matching `elpa-archives` cell |
| `elpa--index-cache-file` (`elpa.el:132-136`) | On-disk index location | `archives/<archive>/archive-contents` under `elpa-user-dir` |
| `elpa--index` (`elpa.el:290-303`) | Choose cached vs fetched index | Memory, else disk, else lazy refresh of *that one* archive (`:302`); never revalidates age |
| `elpa-package-info` (`elpa.el:305-313`) | Lookup in one archive | `archive` defaults to first configured (`:310`); `assoc` in that index (`:311`) |
| `elpa--entry-info` (`elpa.el:273-288`) | Provenance stamp | Adds `:archive` from the index the entry came from (`:284`) |
| `elpa-refresh-contents` (`elpa.el:332-359`) | Which index is fetched | One archive, defaults to first (`:340`); HTTP or local path |
| `elpa--cache-index` (`elpa.el:315-330`) | Cache write | Writes `archives/<name>/...` after the version-1 gate |
| `elpa-install` (`elpa.el:446-464`) | Top-level archive choice | `(caar elpa-archives)` (`:458`), allowlist (`:461`), refresh that archive (`:462`) |
| `elpa--resolve` (`elpa.el:425-444`) | Version check within one archive | Calls `elpa-package-info name archive` (`:430`); floor/exact check (`:436-438`) |
| `elpa--install-1` (`elpa.el:475-502`) | Recursion driver | Resolves in `archive` (`:489`); walks reqs (`:499-500`) |
| `elpa--install-dependency` (`elpa.el:504-518`) | Dependency edge | Special-cases `emacs` (`:511-512`); allowlist (`:515-517`); recurses with the *dependent's* archive (`:518`) |
| `elpa--install-artifact` (`elpa.el:563-603`) | Artifact source | Base from `info :archive` (`:570`); `archive` argument (`:563`) is unused |
| `elpa--acquire-artifact` (`elpa.el:536-561`) | URL/file name | `<base>/<name>-<version>.tar` or `.el` |
| `elpa--installed-at-least` (`elpa.el:466-473`) | Installed satisfaction | Newest local `<name>-<version>` dir; archive-agnostic |
| `elpa--delete-old-builds` (`elpa.el:407-414`) | Replacement pruning | Deletes builds strictly older than the new version, after publish |
| Allowlist | Trust boundary | Defcustom (`elpa.el:77-85`); checks at `:461`, `:482`, `:515-517` |
| Staging / publish (`elpa.el:572-603`) | Atomicity unit | `.staging/<name>-<version>/`, rename on success, cleanup in `unwind-protect` |
| Cycle guard (`elpa.el:99-100`, `:487`, `:497-498`) | Recursion stop | Name-only in-progress list, dynamic binding |
| Autoloads (`elpa.el:823-848`) | Published path | Generated file records the final `<name>-<version>` dir (`:831`) |
| Activation (`elpa.el:885-908`) | Discovery | Scans `<name>-<version>` dirs directly under `elpa-user-dir`; no archive concept |

## 2. Where the single-archive assumption lives

The call chain is:

```
elpa-install
  archive = (caar elpa-archives)                      elpa.el:458
  elpa-refresh-contents archive                       elpa.el:462
  elpa--install-1 name version archive                elpa.el:463
    elpa--resolve -> elpa-package-info name archive   elpa.el:430
    for dep in reqs:
      elpa--install-dependency dep name archive       elpa.el:500  <-- only edge
        elpa--install-1 dep floor archive 'minimum    elpa.el:518
          elpa--resolve ... same archive              elpa.el:489
    elpa--install-artifact info archive               elpa.el:501
```

The assumption is the pair of lines `elpa.el:500` and `elpa.el:518`: the
argument named `archive` at the dependency call site is the **dependent's**
archive, and the dependency is resolved with it. Nothing iterates
`elpa-archives`; `elpa--resolve` has no search loop.

The top level is equally single-archive: `elpa-install` never searches for
`NAME` itself, so a target that lives in the second configured archive fails
the same way (`elpa.el:458-463`).

### What breaks, precisely

Let archive A be the first configured archive (the dependent's) and B another
configured archive:

| Situation | Failure path | User-visible result |
|---|---|---|
| Dependency only in B | `elpa--resolve` -> `elpa-package-info` nil -> error `elpa.el:432-433` | ``Package `queue' is not available in archive melpa`` |
| Dependency in A below the floor, satisfying version in B | floor check errors at `elpa.el:439-441` | `version X or newer is not available in archive A (found Y)` |
| Top-level target only in B | same as first row via `elpa.el:463` | "not available in archive A" |
| Dependency name not allowlisted | `elpa.el:515-517` (before lookup) | ``Required package `compat' for `cider' is not in elpa-installable-packages`` |
| Dependency in B satisfies, but B's cache is stale | would fail later at the artifact (`elpa.el:548-550`) | 404; today unreachable because B is never consulted |

This is exactly the wall measured in the prior evidence §5: `queue`, `seq`,
`compat`, `spinner`, and `let-alist` appear in MELPA `REQS` but exist only on
GNU ELPA (or in GNU Emacs core, which this client does not model).

### The good news: provenance already flows with the info

`elpa--entry-info` stamps `:archive` from the index that produced the entry
(`elpa.el:284`). `elpa--install-artifact` computes the base from
`(plist-get info :archive)` (`elpa.el:570`) and `elpa--acquire-artifact` joins
the artifact name onto it (`elpa.el:543-550`). The `archive` parameter of
`elpa--install-artifact` (`elpa.el:563`) is vestigial; `elpa--install-1` still
passes it (`elpa.el:501`) but it is unread. Consequently, a multi-archive
resolver only needs to return the info found in the winning archive; artifact
acquisition needs no change.

## 3. Deterministic multi-archive search

### 3.1 Where the search should live (interface options)

1. **Change `elpa-package-info`'s nil semantics** (`elpa.el:305-313`). Today
   nil means "first archive"; making nil mean "search all" changes a
   documented function (`man/elpa.texi:228-233`) and every internal caller.
2. **New internal resolver**, e.g. `elpa--find-package NAME [VERSION MINIMUM]`,
   returning an info or nil and leaving `elpa-package-info` archive-explicit.
   `elpa--resolve` (`elpa.el:425-444`) becomes the caller, and
   `elpa--install-1`/`elpa--install-dependency` stop taking `archive` except
   for error reporting.
3. **Search only inside `elpa--resolve`**, keeping the current signatures but
   ignoring the passed archive when the name is absent from it. Smallest
   diff; muddies the meaning of the `archive` argument and the error text.

All three are deterministic; the choice affects public API stability and the
error messages the tests pin (`elpa-tests.el:388`, `:670`, `:756`; code text
at `elpa.el:439-441`).

### 3.2 Ordering and precedence

The only ordering available today is the order of `elpa-archives`
(`elpa.el:62-69`), which the manual already treats as meaningful: "list the
archive you install from first" (`man/elpa.texi:165-171`). First-satisfying-
in-order needs no new knob.

The GNU precedent is different: `package.el` keeps every descriptor for a
name and sorts each name's list by `(archive-priority . version)`
(`package-archive-priorities`, `package.el:237-255`;
`package--add-to-archive-contents`, `package.el:1483-1506`;
`package-desc-priority-version`, `package.el:3988-3994`). Archives not listed
get priority 0. It also offers `package-pinned-packages`
(`package.el:257-272`) to force one archive for one name. ADR 0001 already
rejects a *per-archive allowlist*, but it explicitly distinguishes trust from
source selection and leaves pinning as a separate concept
(`docs/adr/0001-allowlist-only-trust-for-unsigned-package-archives.md:9`).

Options for #50: config order; a priority map defaulting to config order;
a per-package pin list; or highest-version-wins. They differ under
duplicates; all are deterministic given configuration.

### 3.3 Duplicate names and version selection across archives

`assoc` (`elpa.el:311`) returns the first entry in one index; duplicate names
inside one index are not checked (the parser only checks entry shape,
`elpa.el:266-271`). Across archives duplicates are normal and need a rule.

The sharp edge is that version schemes are not comparable in meaning across
archives. MELPA versions are pure date lists `(YYYYMMDD HHMM)`; GNU ELPA uses
semantic versions such as `(31 0)` (prior evidence §2; `queue` is `(0 2)`).
`version-list-<` compares them mechanically, but "newest" across schemes is
meaningless: any MELPA date build outranks every GNU semantic version. A
highest-version-wins policy would therefore systematically prefer MELPA for a
name that exists on both, independent of which archive is the better source.
GNU avoids this by making archive priority dominate version
(`package.el:237-242`).

Two further details matter:

- Equality ignores trailing zeros (`elpa-tests.el:314-317`), so `(0 2)` and
  `(0 2 0)` are the same version.
- Installed directory and artifact names are rendered with
  `elpa-version-join` (`elpa.el:161-165`), so two archives advertising the
  same rendered version produce the same `<name>-<version>` path.

### 3.4 Dependency floors and exact requests

Requirements are minimums, not pins: the index floor is compared with
`version-list-<=` (`elpa.el:436-438`), and an installed build at or above the
floor satisfies it (`elpa.el:466-473`, `:484`, `:513`). The behavior is tested
in `elpa-tests.el:637-675` (a `(1 0)` requirement installs index `(1 1)` and
is not pinned to `(1 0)`).

A multi-archive search must therefore walk candidates until one satisfies the
floor, not stop at the first archive that has the name. GNU's loop is the
model: `package-compute-transaction` walks the per-name descriptor list and
errors only when no candidate meets the floor (`package.el:1744-1815`).

Exact top-level requests are a separate rule: with `MINIMUM` nil an explicit
version must equal the index version (`elpa.el:436-438`), and the error says
the version is "no longer available in archive X" (`elpa.el:439-441`; tests
at `elpa-tests.el:387-389`, `:755-757`). Cross-archive, #50 must decide
whether an explicit version searches all archives, the selected archive only,
or all but with precedence when two archives advertise it.

Installed satisfaction is archive-agnostic (`elpa--installed-at-least`,
`elpa.el:466-473`): an installed dependency from any source satisfies every
dependent, and a build newer than the index is never downgraded (`elpa.el:484`;
rollback test `elpa-tests.el:884-921`). That is convenient for cross-archive
resolution but means dependency provenance is not verified.

### 3.5 Installed identity and replacement

An installed build is identified only by `<name>-<version>` under
`elpa-user-dir` (`elpa.el:368-372`, `:374-391`); the archive that supplied it
is not recorded. Consequently:

- `elpa-installed-p name version` (`elpa.el:416-423`) cannot distinguish the
  same name+version from two archives.
- When the advertised build is already installed, `elpa--install-1` skips the
  download and prunes older builds (`elpa.el:490-496`). A cross-archive
  install resolving to an already-installed identical version is a no-op even
  if the bytes differ.
- Replacement deletes only strictly older builds (`elpa.el:407-414`), so a
  different-archive build of the same version is never replaced.

If same-name/same-version collisions across archives are considered
meaningful, this needs an identity decision (accept version as identity, or
record archive/checksum). Otherwise document that version is identity.

## 4. Refresh semantics

Current facts:

- `elpa-install` refreshes exactly one archive (`elpa.el:462`), then resolves
  entirely within it. `elpa-refresh-contents` also handles one archive,
  defaulting to the first (`elpa.el:340`); there is no "refresh all" entry
  point (`man/elpa.texi:181-190`).
- `elpa--index` prefers the in-memory alist, then a readable disk cache, and
  only fetches when neither exists (`elpa.el:290-303`). There is no age,
  ETag, or revalidation check. The cache is keyed by archive name only
  (`elpa.el:132-136`), so changing a location under the same name keeps the
  old bytes until an explicit refresh (test at `elpa-tests.el:600-610` relies
  on install's refresh).
- The manual promises that "a cached index is never trusted for a rolling
  archive" (`man/elpa.texi:203-205`, repeated `:458-460`), because MELPA prunes
  old artifacts (prior evidence §4, §6 item 8). That promise holds only for
  the one archive install refreshes.

Once dependencies can come from another archive, that archive's index would
be read from cache (or lazily fetched if absent) and never revalidated, so a
stale rolling index can name a pruned artifact. This is the direct
cross-archive contradiction with the documented guarantee.

Options for #50:

1. **Refresh every configured archive at the start of an install.** Simple,
   deterministic, restores the rolling guarantee for dependencies. GNU's
   documented behavior is "descriptions of all configured ELPA packages"
   (`package.el:1714-1727`), and `package-read-all-archive-contents`
   iterates `package-archives` (`package.el:1523-1560`). Cost: MELPA's
   2.6 MB index plus GNU's per
   install (prior evidence §1, §6 item 9), and a failure of an archive that
   is never consulted would fail the install unless that is deliberately
   tolerated.
2. **Refresh lazily**: the target's archive up front, plus any archive first
   consulted during resolution. Cheaper; needs a per-transaction "already
   refreshed" set, and the rule must say whether an existing memory/disk
   cache counts as consulted before refresh.
3. **Cache TTL**: refresh if the cached index is older than N. Avoids a fetch
   per install but reintroduces the stale-artifact window and adds
   time-dependent behavior (offline fixtures unaffected; live tests at the
   boundary).
4. **Refresh on miss only** (today's `elpa--index` behavior): insufficient,
   because a cached-but-stale index can satisfy resolution and then 404 at
   artifact time.

Also decide the no-argument API: keep `elpa-refresh-contents` meaning "first"
for compatibility and add a separate all-archives function, or change it to
mean "all" and update the manual and tests. Tests currently assert exact
single-archive counts (`elpa-tests.el:324`, `:354`, `:423`, `:1102`, `:1284`;
live `:157`, `:220`).

## 5. Allowlist semantics for transitive dependencies

Today every dependency name is checked explicitly:

- Top-level check before any network work: `elpa.el:461` (and again at
  `elpa.el:482`).
- Transitive check with a dependent-specific message: `elpa.el:515-517`.
- The `emacs` pseudo-dependency bypasses the check by returning first
  (`elpa.el:511-512`).
- An already-installed dependency is accepted before the allowlist check
  (`elpa.el:513` precedes `:515`), so removing a name from the allowlist does
  not distrust an installed build.
- The default allowlist (`elpa.el:77-85`) is the milestone-1 set; it does not
  include `seq`, `compat`, `spinner`, or `let-alist`, and includes `queue`
  only because it is a milestone-1 target.

ADR 0001 makes the allowlist the only trust control and states that it grants
trust by name across all configured archives, not per archive
(`docs/adr/0001-allowlist-only-trust-for-unsigned-package-archives.md:3`,
`:9`, `:15`). Per-archive allowlisting was explicitly rejected there.

Options for #50:

1. **Explicit dependency names** (status quo extended): add the GNU-only
   names, and their transitive closure, to `elpa-installable-packages`.
   Preserves trust-by-name and needs no ADR change. Side effect: those names
   also become top-level installable, because the list has no role marker.
2. **Two lists**: keep `elpa-installable-packages` for top-level targets and
   add a dependency-only list (or a trust marker) so GNU dependencies are
   trusted without becoming user-facing install targets. Same trust
   granularity; one new knob.
3. **Automatic closure**: check only the requested package; trust any name an
   allowlisted package's `REQS` mentions. This materially weakens ADR 0001: a
   compromised or malicious index entry for an allowlisted package could name
   an arbitrary package from any configured archive and get its code
   executed. Requires an ADR/spec change and new security wording.
4. **No network dependency install**: allowlist the target only and require
   its dependencies to be preinstalled or supplied by core/compat. Preserves
   the trust story but does not satisfy "install from scratch" in one step.
5. **Per-archive allowlist**: already rejected by ADR 0001; archive priority
   or pinning is the source-selection tool, not trust.

Whether the closure is checked eagerly (whole graph before downloads) or
discovered during recursion is a separate decision; today it is discovered,
so an unallowlisted transitive name can fail after earlier dependencies have
already been installed.

## 6. Failure and atomicity for multi-package installs

Per-package guarantees are strong and tested:

- One staging directory per artifact: `.staging/<name>-<version>/`
  (`elpa.el:572-573`); stale staging is deleted first (`elpa.el:580-582`);
  `unwind-protect` removes it on any failure and removes the staging root when
  empty (`elpa.el:583-603`).
- Publication is a rename (`elpa.el:598`) only after tar validation
  (`elpa.el:614`), `-pkg.el` validation (`elpa.el:591`), autoload generation
  (`elpa.el:593`), and byte-compilation (`elpa.el:594`); a duplicate check
  runs immediately before publish (`elpa.el:577-578`, `:595-597`).
- An old build is deleted only after the new one is published, and only if
  strictly older (`elpa.el:407-414`, `:599`). Failure tests:
  `elpa-tests.el:769-842`; rollback test `:884-921`.

Across packages there is no transaction. Dependencies install first, depth-
first and sequentially (`elpa.el:499-501`). Therefore:

- Failure at any node leaves every previously completed node published. A
  missing/denied/404/compile-failing later dependency leaves earlier
  dependencies installed; a target failure leaves all its dependencies
  installed. `.staging/` is clean, but package residue is real and valid.
- If a dependency was upgraded before a later failure, its old build was
  already deleted (`elpa.el:599`), so the machine is observably changed and
  the client cannot restore it (no deletion/rollback UX; map out of scope).
- Already-installed dependencies are skipped (`elpa.el:513`, `:484`); if the
  target itself is already installed at the advertised version,
  `elpa--install-1` returns at `elpa.el:490-496` without walking requirements,
  so missing dependencies are never repaired by a re-install.
- A floating target install never upgrades a dependency that already meets
  the floor, even if the dependency's archive advertises newer
  (`elpa.el:513`).

Options for #50:

1. **Best-effort (current)**: document that completed dependencies remain;
   reruns are idempotent; staging never leaks. Simplest; residue is defined
   as installed dependencies.
2. **Preflight the whole closure**: resolve names, floors, allowlist, and
   archive availability for the entire graph before the first download. GNU
   does this (`package-compute-transaction` before
   `package-download-transaction`, `package.el:1744-2062`). Leaves only
   artifact/compile failures as mid-way risks.
3. **Revert the transaction on failure**: record what this transaction
   published (and what it replaced) and undo it. GNU deletes the package
   descriptors it installed during the failed transaction
   (`package-download-transaction`, `package.el:2049-2062`) and leaves any
   pre-existing version directories in place; this client
   publishes-before-delete (`elpa.el:598-599`), so reverting a replacement
   would need to retain the old build until the whole transaction commits.
4. **Stage the whole closure, publish all at once**: strongest atomicity,
   largest change; the publish-before-delete replacement rule must extend to
   the transaction.

The decision also defines what "no residue" means for tests: no staging, no
partial package dirs, but possibly installed dependencies and upgraded builds.

## 7. Cycles and the `emacs` pseudo-dependency

- `elpa--install-in-progress` (`elpa.el:99-100`) is a name-only list, reset
  per top-level install (`elpa.el:460`), checked before resolution
  (`elpa.el:487`), and dynamically extended around the recursion
  (`elpa.el:497-498`). A cycle edge returns the name immediately; the outer
  frame is still installing it, so the graph completes.
- Consequences: the cycle edge skips version-floor validation and archive
  resolution, and same-name packages from different archives are one node.
  Cycles are silent; GNU messages "Dependency cycle going through ..."
  (`package.el:1764-1765`).
- `emacs` is special-cased in the dependency function: warn and ignore
  (`elpa.el:511-512`), before any search or allowlist check. It is not a
  compatibility test; a MELPA `(emacs (28 1))` is accepted on a much older
  XEmacs. It is also ubiquitous on MELPA (prior evidence §7), so every
  install that walks requirements warns at least once.
- There is no core/compat satisfaction model: `elpa--installed-at-least`
  (`elpa.el:466-473`) only looks at `<name>-<version>` directories under
  `elpa-user-dir`, so on XEmacs `seq` and `let-alist` are always "missing"
  and must be installed from GNU ELPA (or a future compat layer would have to
  be recognized explicitly).

Cross-archive resolution does not require changing the cycle mechanism, but
#50 should state whether a cycle edge must still satisfy its floor/allowlist,
and whether `emacs` warnings stay per-install or are summarized.

## 8. Fixtures and test harness for a two-archive offline gate

### Current harness

- `tests/automated/elpa-tests.el` is a flat batch of top-level `Assert`s run
  by CTest (`tests/CMakeLists.txt:8-27`); each case rebinds `elpa-user-dir`
  (temp) and `elpa-archives` (temp or vendored).
- GNU fixture `tests/elpa/fixture/` (`archive-contents`, `queue-0.2.tar`,
  `loc-changes-1.2.el`), path helper `elpa-tests.el:43-46`.
- MELPA fixture `tests/elpa/melpa-fixture/`: nine tarballs at pinned date
  versions, hand-written index whose entries all have `nil` requirements.
  Provenance is pinned by asserts: exactly 9 tarballs totalling 593,920 bytes
  (`elpa-tests.el:1197-1204`), and the README states the fixture carries only
  the nine targets.
- Runtime-built archives are already the pattern for dependency tests:
  `elpa-tests-make-archive` (`elpa-tests.el:67-73`) writes an index,
  `elpa-tests-build-tar-package` (`elpa-tests.el:75-96`) builds tarballs; the
  emacs/floor tests use them (`elpa-tests.el:612-675`).
- Single-archive assumptions in the suite: every `elpa-archives` binding is a
  one-element list; exact refresh counts (`elpa-tests.el:324`, `:354`,
  `:423`, `:1102`, `:1284`); HTTP tests start one server per root on fixed
  ports 18743/18744 (`elpa-tests.el:1033-1064`, `:1067-1152`); the memory
  index alist is cleared only in some tests (`elpa-tests.el:1274`, `:1315`;
  live `:217`, `:289`).

### Two-archive gate options

1. **Synthetic runtime fixture**: bind `elpa-archives` to two temp archives;
   put a dependent declaring `((queue (0 2)))` in one and `queue-0.2.tar` in
   the other; assert resolution order, that the dependency's artifact came
   from the second base, floors, allowlist, and residue. Cheapest; matches
   the existing dependency-test style; no new provenance story.
2. **Vendored cross fixture**: add a small real MELPA tarball whose `REQS`
   include a GNU-only package (prior evidence names `async-job-queue` ->
   `queue`; `cider`/`citeproc` are heavier closures), plus an index.
   Reusing `tests/elpa/fixture/` as the GNU side and a new
   `tests/elpa/cross-fixture/` (or extending `melpa-fixture/`) preserves the
   "unmodified upstream" property. Extending `melpa-fixture/` breaks its
   9-tarball/593,920-byte asserts and README, so a separate directory or
   updated totals is required.
3. **Loopback HTTP with two bases**: one Python server rooted at a parent
   containing `gnu/` and `melpa/` subdirectories works because
   `elpa--remote-url` only concatenates (`elpa.el:184-186`); or start two
   servers on two ports as the existing HTTP test does
   (`elpa-tests.el:1050-1064`). This is the way to exercise the archive
   selection over HTTP without external network.

Assertions the gate should carry, independent of the chosen policy:

- target resolved from archive 1 while a dependency is resolved from archive
  2, and the dependency artifact is fetched from archive 2 (`:archive`,
  `elpa.el:570`);
- a floor below archive 2's advertised version installs; a floor above every
  archive fails with the decided message;
- a duplicate name in both archives selects per the decided precedence;
- an explicit version request across archives;
- an unallowlisted transitive name fails at the decided point (before any
  download vs after earlier deps) with the decided residue;
- refresh failure of an archive that is never consulted (fail vs ignore, per
  the refresh decision);
- a cycle that crosses archives;
- an `emacs`-only dependency still installs with a warning and no lookup.

### Live smoke

The live suite currently isolates GNU and MELPA in separate let-blocks
(`elpa-live-tests.el:151-209` and `:211-267`) and pins nothing. A
cross-archive live test would bind both archives and install an unpinned
MELPA target whose closure includes a GNU-only dependency; the target choice
belongs to the target-set ticket, and the allowlist must include the closure.
The network timeout is already 300 s (`tests/CMakeLists.txt:35-38`), which
matters if refresh moves to all archives and MELPA's 2.6 MB index is fetched
more than once per run.

### Documentation that moves with the decision

`elpa-install`'s docstring says "from the first configured archive"
(`elpa.el:447-454`), and the manual repeats it (`man/elpa.texi:165-171`,
`:195-198`). Whichever semantics are chosen, those texts and the exact error
messages asserted in tests (`elpa-tests.el:388`, `:670`, `:756`) are
part of the change.

## 9. Decision points for #50

1. **Scope**: does multi-archive search apply only to dependencies, or also to
   the top-level target named in `elpa-install`?
2. **Precedence and duplicates**: configuration order, an explicit priority
   map, a per-package pin, or highest-version-wins; how a name present in two
   archives is chosen.
3. **Cross-scheme versions**: confirm that version lists from different
   archives are not comparable in meaning (date vs semantic), and whether the
   chosen rule depends on that.
4. **Exact-version requests**: search all archives, selected archive only, or
   all with precedence.
5. **Floors**: skip below-floor archives and continue, versus erroring on the
   first archive that has the name; required error wording.
6. **Installed identity**: version-only identity, or record the source
   archive; what happens when two archives advertise the same name+version;
   whether a re-install repairs missing dependencies of an already-installed
   target.
7. **Refresh**: all archives, lazy per consulted archive, or TTL; no-arg
   `elpa-refresh-contents` semantics; whether an unrelated archive's failure
   fails the install.
8. **Allowlist**: explicit dependency names, a separate dependency-only list,
   or automatic closure; whether an installed dependency bypasses the check;
   whether ADR 0001 needs amending and how the security wording changes.
9. **Closure checking time**: preflight the whole graph before downloads, or
   discover during recursion (current).
10. **Failure/atomicity**: acceptable residue (completed dependencies,
    upgraded dependencies), preflight-only, transaction revert, or
    stage/publish the whole closure.
11. **Cycles and `emacs`**: whether a cycle edge must still check floor and
    allowlist; whether to warn on cycles; `emacs` warning policy; whether
    core/compat-provided dependencies may satisfy a requirement.
12. **Gates**: synthetic vs vendored two-archive fixture; live target with a
    real GNU-only dependency; which exact messages/asserts change.

## Evidence index

- Client: `lisp/elpa.el` at `78389c1dc` (all line references above).
- Tests: `tests/automated/elpa-tests.el`, `tests/automated/elpa-live-tests.el`,
  `tests/CMakeLists.txt`; fixtures `tests/elpa/fixture/`,
  `tests/elpa/melpa-fixture/` (indexes and READMEs read directly).
- Manual: `man/elpa.texi` §Installing Packages, §Install Pipeline,
  §Testing.
- Trust: `docs/adr/0001-allowlist-only-trust-for-unsigned-package-archives.md`.
- Prior evidence: `docs/research/melpa-archive-mechanics.md` on
  `research/melpa-archive-mechanics` (ticket #31), §1 index scale, §2
  versions, §4 artifacts, §5 dependency wall, §6 items 1, 7, 8.
- External precedent: GNU Emacs `package.el`
  (`https://raw.githubusercontent.com/emacs-mirror/emacs/master/lisp/emacs-lisp/package.el`,
  fetched 2026-10-24): `package-archive-priorities` (`:237-255`),
  `package-pinned-packages` (`:257-272`), duplicate accumulation
  (`:1483-1506`), refresh/read-all (`:1523-1560`, `:1714-1727`), transaction computation
  and cycle message (`:1744-1815`), download transaction and revert
  (`:2049-2062`), priority-version comparison (`:3988-3994`),
  compatibility/`emacs` handling (`:3382-3410`).
