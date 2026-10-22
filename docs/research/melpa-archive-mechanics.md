# MELPA's archive mechanics

Research for wayfinder ticket [#31](https://github.com/fktpp2022/xemacs-git/issues/31) (map [#30](https://github.com/fktpp2022/xemacs-git/issues/30)), 2026-10-23. All live evidence fetched from `https://melpa.org/packages/` on this date. Client references are to `lisp/elpa.el` on `integration/elpa-milestone-1`. Prior art: [package-el-minimum-path.md](package-el-minimum-path.md) (GNU side).

Method note: the index was fetched with `curl` and parsed with the milestone build of XEmacs (`build/bin/xemacs -batch`, loading `build/lisp/subr.elc`) used read-only; version experiments ran the same way against the `version-to-list` port.

## 1. The index

`https://melpa.org/packages/archive-contents` (2,639,462 bytes, `Content-Type: text/plain`):

```
(1
 (0blayout . [(20190703 527) nil "Layout grouping with ease" tar ((:url . "https://github.com/etu/0blayout") (:commit . "fd9a8f353...") (:revdesc . "fd9a8f353dbd") (:keywords "convenience" "window-management"))])
 (0x0 . [(20230823 2214) ((emacs (26 1))) "Upload sharin...
```

Facts, measured over all entries with the milestone build:

- **First element is `1`** — the same `package-archive-version` 1 gate `elpa--parse-index` enforces. No version negotiation needed.
- **6,333 entries**, one per package. Shape is exactly `[VERSION REQS SUMMARY KIND EXTRAS]`, name in the alist car, fields in a vector. GNU's 185 KB index has the identical shape (and GNU's `queue`: `[(0 2) nil "Queue data structure" tar ...]`).
- **All 6,333 entries are kind `tar`. `single` does not exist on MELPA** (verified by census: zero entries with kind `single`; `curl -I .../0blayout-20190703.527.el` → 404). GNU ELPA has both kinds.
- `REQS` uses the same `((name (n n ...)) ...)` shape; `nil` when there are none. `EXTRAS` is the GNU extras alist plus MELPA-specific keys `:commit`, `:revdesc` — ignorable by a client.
- **Freshness**: `Last-Modified: Thu, 22 Oct 2026 16:16:07 GMT` — the index is rebuilt continuously (the newest version list in it is dated 20261022, the day before this fetch). MELPA is a rolling archive: the index is a same-day snapshot, not a release artifact.
- **`http` redirects to `https`**: `http://melpa.org/packages/archive-contents` → `301 Moved Permanently`, `Location: https://...`. The async HTTP transport sets `CURLOPT_FOLLOWLOCATION` (max 10 redirects), so this is transparent to `elpa.el`.

## 2. Versions

**Every one of the 6,333 version lists is exactly two elements: `(YYYYMMDD HHMM)`** — a date and a time-of-day as integers. Distribution of list lengths over the whole index: `[(2, 6333)]`; first elements range 20090428–20261022. There are no `snapshot` symbols, no 3-element versions, no strings — versions arrive as integer lists and are never parsed from the index at all. Time-of-day leading zeros do not exist (integer 104, not "0104"): the index for `dash-at-point` reads `(20211023 104)` and the served artifact is literally named `dash-at-point-20211023.104.tar`.

Experiments against the repo's `version-to-list` port (milestone build, exact inputs/outputs):

```
version-to-list "20240331.1300" -> (20240331 1300) ; elpa-version-join -> "20240331.1300"
version-to-list "20191024.450"  -> (20191024 450)  ; elpa-version-join -> "20191024.450"
version-to-list "20211023.104"  -> (20211023 104)  ; elpa-version-join -> "20211023.104"
version-to-list "20090428.0"    -> (20090428 0)    ; elpa-version-join -> "20090428.0"
version-to-list "20240331"      -> (20240331)      ; elpa-version-join -> "20240331"
version-to-list "1.2.3"         -> (1 2 3)         ; elpa-version-join -> "1.2.3"
version-to-list "22.8beta3"     -> (22 8 -2 3)     ; elpa-version-join -> "22.8.-2.3"
version-to-list "0.9snapshot"   -> (0 9 -4)        ; elpa-version-join -> "0.9.-4"
version-to-list "1.0-git"       -> (1 0 -4)        ; elpa-version-join -> "1.0.-4"
(version-list-= '(20240331 1300) (version-to-list "20240331.1300")) -> t
(version-list-= '(20240331 1300) '(20240331 1300 0))                -> t   ; trailing zeros insignificant
(version-list-<= '(0 2) '(20240331 1300))                           -> t   ; dependency floors compare sanely
```

Comparison with GNU Emacs's documented `version-to-list` (subr.el): the port is a faithful copy for everything MELPA needs. For a dotted date string GNU produces the same `(20240331 1300)`; the regexp-alist defaults (`snapshot`/`cvs`/`git` → -4, `alpha` → -3, `beta` → -2, `pre`/`rc` → -1) and trailing-zero-insignificant equality are identical. Date versions carry no prerelease markers, so the port's "knows snapshot but not date-priority semantics" worry never fires on real MELPA data: plain integer lists compare correctly, and newer dates always sort higher.

The one latent semantic mismatch (GNU and port alike): a *snapshot-marked* version sorts below every release, so if MELPA ever emitted `snapshot` forms — it does not — rolling versions would order incorrectly. Dormant for MELPA; relevant only if the client later points at mixed archives.

## 3. Signatures

MELPA publishes **no signatures at all**:

- `https://melpa.org/packages/archive-contents.sig` → **404**
- `https://melpa.org/packages/0blayout-20190703.527.tar.sig` → **404**

GNU ELPA, by contrast, signs everything (re-verified live): `archive-contents.sig` → 200, `queue-0.2.tar.sig` → 200 (557 bytes).

Consequence for the client: `elpa-check-signature` defaulting to nil (allow-unsigned, the GNU `package-check-signature` default posture) is not merely tolerable for MELPA — it is the only possible posture, since no `.sig` exists to verify and `elpa.el` already ignores them.

## 4. Artifacts

- **URL shape**: `<base><name>-<version-join>.tar` for every package (there is no `.el` single-file artifact; see §1). Verified live against the exact rendering `elpa-version-join` produces from the index list:

| URL (rendered from index version) | Status | Size |
|---|---|---|
| `0blayout-20190703.527.tar` | 200 | 10,240 |
| `dash-alfred-20191024.450.tar` | 200 | 10,240 |
| `dash-at-point-20211023.104.tar` | 200 | 20,480 |
| `hydra-20250316.1254.tar` | 200 | 92,160 |
| `dash-20260221.1346.tar` | 200 | 358,400 |
| `magit-20261022.827.tar` | 200 | 2,027,520 |

Typical MELPA tarball is tens of KB; the tail reaches a few MB.

- **Rolling release, no history**: only the *current* build of each package is served. `dash-20240101.0.tar` → 404 while `dash-20260221.1346.tar` → 200. Version pinning/rollback against melpa.org is impossible; `stable.melpa.org` (same index format, 1.45 MB) serves upstream semantic versions like `(1 0 2)` if determinism is ever needed — the MELPA project itself does not recommend stable.
- **Tarball layout** (two samples unpacked; both satisfy `elpa--validate-tar`):
  - Single top-level directory `<name>-<version-join>/`.
  - `<name>-pkg.el` always present, generated by package-build. `0blayout`'s, in full:

    ```elisp
    ;; -*- no-byte-compile: t; lexical-binding: nil -*-
    (define-package "0blayout" "20190703.527"
      "Layout grouping with ease."
      ()
      :url "https://github.com/etu/0blayout"
      :commit "fd9a8f353dbd45b4628b5f84b8d8c2525ebf571d"
      :revdesc "fd9a8f353dbd"
      :keywords '("convenience" "window-management"))
    ```

    The version is a *string* (`"20190703.527"`); `elpa--validate-pkg-file` formats it back through `version-to-list` and compares equal to the index list. The trailing plist (`:url` `:commit` ...) is extra data GNU also allows; the validator ignores it. `dash-pkg.el` likewise declares `((emacs "24"))` matching the index's `((emacs (24)))`.
  - **No `<name>-autoloads.el` and no `.elc` files are shipped** in either sample (both 0blayout and dash carry sources + pkg.el only; dash additionally ships `dash.info` and a texinfo `dir` file, which land harmlessly in the content directory). The MELPA README states `-pkg.el` must not be checked into version control — it and the autoloads are build outputs; package-build ships pkg.el but leaves autoload generation to the installing client's layout. `elpa.el`'s defensive `elpa--remove-shipped-autoloads` remains correct, and its install-time autoload generation + byte-compilation path is exercised as designed.

## 5. The dependency wall: MELPA does not host everything its packages require

Package names **absent from MELPA's 6,333 entries** that nevertheless appear in other entries' `REQS` (each checked both by index lookup and by absence of a recipe file in `melpa/melpa` master, where all return 404):

| package | example MELPA dependent | hosted on |
|---|---|---|
| `queue` | cider, citeproc, async-job-queue (`(queue (0 2))`) | GNU ELPA only |
| `seq` | many (`(seq (2 22))` ...) | GNU ELPA + Emacs core |
| `compat` | cider, magit (`(compat (31 0))`), citeproc | GNU ELPA only |
| `spinner` | cider (`(spinner (1 7))`) | GNU ELPA only |
| `let-alist` | aqi, biblio-core, atomic-chrome | GNU ELPA + Emacs core |

All five verified live on `elpa.gnu.org/packages/<name>.html` → 200. So **MELPA's index alone cannot satisfy its own dependency graph**: a real-world MELPA setup always pairs melpa.org with GNU ELPA (and/or core). This is the single sharpest divergence from the GNU-only milestone, where one archive was self-sufficient.

## 6. Implications for elpa.el

Concrete gaps and confirmations, in rough priority order:

1. **Dependency resolution is single-archive.** `elpa--install-dependency` resolves every requirement *in the dependent's archive*, and `elpa-install`/`elpa-package-info` default to the first entry of `elpa-archives`. Installing any MELPA package whose deps include `queue`/`seq`/`compat`/`spinner`/`let-alist` fails with "Package `queue' is not available in archive melpa". MELPA support requires cross-archive resolution (search all `elpa-archives`) or a target set whose deps are all MELPA-hosted — the latter is hard to audit because dependency lists are not visible without the index of every archive.
2. **`version-to-list` accepts the MELPA date forms; nothing to change.** `"20240331.1300"` → `(20240331 1300)`, equal under `version-list-=` to the index list, and dependency floors like `(queue (0 2))` compare correctly against any list. GNU and the port agree on every real MELPA input.
3. **`elpa-version-join` renders `(20240331 1300)` as `20240331.1300`, which is exactly the live artifact name and a safe directory name.** Verified byte-for-byte against six live URLs spanning 2019–2026 builds (incl. the no-leading-zero case `...104`). Directory regexes (`elpa--find-installed-dir`, `elpa--autoloads-file`, `[0-9]+(\.[0-9]+)*`) match date-versioned names. The negative-number rendering hazard (`(0 9 -4)` → `"0.9.-4"`) is dormant for MELPA (no markers ever occur) but is a live hazard if GNU ELPA prereleases are ever joined.
4. **No `.sig` anywhere; allow-unsigned is mandatory, and unimplemented signature support costs nothing on MELPA.** If the spec ever wants tamper evidence on MELPA, it must come from transport (HTTPS) or index pinning, not signatures.
5. **`single` never occurs on MELPA** (0/6,333). The `.el` artifact path is GNU-only; it can stay as-is but gets no live coverage from any MELPA milestone test.
6. **Tarballs need no layout changes**: version-1 gate passes, single top dir, `-pkg.el` validates (string date version parses equal to the index list), no shipped autoloads/.elc to fight, extra texinfo files are inert. Existing validation (`elpa--validate-tar`, `-pkg.el` check, staged autoload generation, byte-compilation) is exercised unchanged.
7. **The `emacs` pseudo-dependency is ubiquitous on MELPA** (dash `(24)`, magit `(28 1)`, cider `(28)`, ...). `elpa.el` already warns-and-ignores it, consistent with the declared distrust posture — but note the map's open question: on MELPA this distrust hits nearly *every* package, so the target-set choice and the per-package compatibility decision (#30's "Not yet specified") get thin fast.
8. **Rolling release breaks cache-as-is installs.** `elpa--index` reuses a cached index forever, but melpa.org prunes old artifacts; a stale cache can point at a 404. A `elpa-refresh-contents` before install (or a cache age bound) is needed for any MELPA archive, and "install version X" is only meaningful as "install whatever is current when you look".
9. **Index scale**: 2.6 MB / 6,333 entries vs GNU's 185 KB. It arrives via one `http-get` whose body *does* round-trip through a Lisp string (artifacts don't; the index does). Fine in practice, worth a test with the real index in the fixture set.
10. **Transport**: `http://` locations 301 to `https://` and the async transport follows redirects (curl, max 10) — no client change needed, but the spec can simply require `https://` URLs for MELPA and sidestep the question.

## Evidence index

- Live index: `https://melpa.org/packages/archive-contents` (fetched 2026-10-23, 2,639,462 bytes, `Last-Modified: Thu, 22 Oct 2026 16:16:07 GMT`); stable variant `https://stable.melpa.org/packages/archive-contents` (1,453,810 bytes, version-1, semantic versions).
- Census and version experiments: milestone XEmacs build, `-batch -load build/lisp/subr.elc -eval ...` (transcripts inline above).
- Live artifact probes: HEAD/listing results inline above; tar listings of `0blayout-20190703.527.tar` and `dash-20260221.1346.tar` unpacked and quoted inline.
- Recipe-absence checks: `https://raw.githubusercontent.com/melpa/melpa/master/recipes/<name>` → 404 for `compat`, `spinner`, `queue`, `seq`, `let-alist`; 200 for `dash`.
- GNU contrast: `https://elpa.gnu.org/packages/archive-contents.sig` → 200; `queue-0.2.tar.sig` → 200; package pages for the five excluded names → 200.
- Client: `lisp/elpa.el` and `src/async-http.c` at `integration/elpa-milestone-1`.
