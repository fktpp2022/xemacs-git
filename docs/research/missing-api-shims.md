# Missing Emacs Lisp APIs: inventory and cheapest sources

Research findings for wayfinder ticket [#16](https://github.com/fktpp2022/xemacs-git/issues/16). This report inventories the compatibility gaps and candidate sources. It does not choose where the work lives or which strategy to take.

## Bottom line

- The gaps split into three groups:
  1. **Official GNU ELPA backports aimed at older Emacs**: `cl-lib` 0.7.1 (which also supplies `cl-defstruct`), `seq` 2.24 (with `seq-24.el`), and `nadvice` 0.4 (which supplies `advice-add`). These are the cheapest starting sources. Each declares `lexical-binding: t` in its header, but each targets an Emacs generation that predates or straddles lexical binding; the declaration is not proof of reliance and each needs a closure audit before it is trusted under dynamic scope.
  2. **No GNU ELPA package; port from Emacs core**: `version-to-list` (small, critical), a `subr-x` subset (small), `tabulated-list` (UI only), and `json` (not on the ELPA install path).
  3. **Already in the XEmacs add-on tree, not core**: old `advice`, `tar-mode`, `url`/`url-http`, and `efs`. Using them from the installer creates a bootstrap dependency on the add-on tree.
- The minimum package.el path critically needs `version-to-list` and `cl-lib`/`cl-defstruct`. `seq`/`subr-x` are version-dependent. `tabulated-list`, `json`, and `nadvice` are deferrable for a batch install-and-run milestone.
- Vendoring an older package.el (Emacs 24.3-era) minimizes the API surface: it avoids `seq`, `subr-x`, and the whole `tabulated-list` UI, leaving `cl-lib` + `version-to-list` + transport/tar. A newer package.el pulls in more.
- The GNU ELPA `compat` package (31.1.0.0) requires Emacs 25.1 and is not a viable shortcut for XEmacs.

## Inventory

| API | XEmacs today | Cheapest faithful source | Lexical risk | Consumed by |
|---|---|---|---|---|
| `cl-lib` | Absent; `lisp/cl.el:623` provides the old `cl`, not `cl-lib` | GNU ELPA `cl-lib` 0.7.1, "Forward cl-lib compatibility library for Emacs<24.3" | Header declares `lexical-binding: t`; target predates lexical binding, so it should not rely on capture; audit | `package.el` requires it; `cl-loop`, `cl-labels`, `cl-incf` |
| `cl-defstruct` | Absent; old `defstruct` in `lisp/cl-macs.el:2654` | Included in `cl-lib` 0.7.1 | Same as `cl-lib` | `package-desc` and other structs in `package.el` |
| `seq` | Absent | GNU ELPA `seq` 2.24; tarball includes `seq-24.el` ("seq.el implementation for Emacs 24.x") and `seq.el` | Header declares `lexical-binding: t`; targets Emacs 24.x; audit closures; may be avoidable with an older `package.el` | Newer `package.el`; installed packages |
| `subr-x` | Absent | Port the needed subset from Emacs core `lisp/emacs-lisp/subr-x.el` (24.4-era); no GNU ELPA package | Upstream declares `lexical-binding:t`; mostly functions/macros; audit macros that expand to closures | Newer `package.el` (`when-let`, `string-trim`, `string-join`) |
| `json` | Absent | Port from Emacs core `lisp/json.el` (24.x); no GNU ELPA package | Sampled Emacs 24.x header has no lexical-binding declaration; still audit closure use | Async HTTP/JSON APIs and `package-vc`, not GNU ELPA's Lisp `archive-contents` path |
| `advice-add` / `nadvice` | Absent in core; old `advice` in the `xemacs-base` add-on (`etc/package-index.LATEST.gpg:440`) | GNU ELPA `nadvice` 0.4, "Forward compatibility for Emacs-24.4's nadvice" | Header declares `lexical-binding: t`; forward-compat target predates 24.4; audit | Installed packages using new-style advice; not the `package.el` install path |
| `tabulated-list` | Absent | Port from Emacs core (24.x); no GNU ELPA package | Sampled header declares `lexical-binding: t`; UI-only | `package-menu` UI; defer for a batch milestone |
| `version-to-list` | Absent; `lisp/version.el` is synched to FSF 19.34 (`lisp/version.el:23`) and has no version parser | Port from Emacs core `lisp/version.el`; the sampled copy is small and has no lexical header | Low | `package.el` version comparison and dependency resolution |

## Details

**`cl-lib` and `cl-defstruct`.** XEmacs's `lisp/cl.el` is the old `cl` package (`lisp/cl.el:623`), not the Emacs 24.3+ `cl-lib` namespace. The GNU ELPA `cl-lib` 0.7.1 package is explicitly a forward-compatibility library for Emacs versions before 24.3, so it is written for a world where lexical binding was not the default. It supplies `cl-defstruct`, which `package.el` uses for `package-desc`. This is the strongest candidate for a drop-in source, subject to a closure audit.

**`seq`.** GNU ELPA `seq` 2.24 ships `seq.el` plus `seq-24.el`, selected for Emacs 24.x. The sampled `seq-24.el` and `seq.el` both declare `lexical-binding: t`. The package has no `Package-Requires`, and the Emacs 24.x branch predates the broad lexical conversion, but the 24.x range includes versions that support lexical binding, so this needs an audit rather than an assumption. A vendored older `package.el` may avoid `seq` entirely.

**`subr-x`.** There is no GNU ELPA backport. The upstream `lisp/emacs-lisp/subr-x.el` declares `lexical-binding:t` (verified for Emacs 24.4, 25.1, and 26.1), but the functions `package.el` uses from it are a small, mostly dynamic-safe subset; porting them is cheaper than porting the whole library. Check any macro that expands into a closure.

**`json`.** GNU ELPA's `archive-contents` is Lisp, so the ELPA install path does not need JSON. `async-http.el` lists `json` as optional, and `package-vc` uses it. The sampled Emacs 24.x `json.el` header does not declare lexical binding. Defer unless the chosen transport or a target package needs it.

**`advice-add` / `nadvice`.** XEmacs core has no `nadvice`; the add-on tree has the old `advice` library in `xemacs-base` (`etc/package-index.LATEST.gpg:440`), which is a different API. GNU ELPA `nadvice` 0.4 provides the forward-compatible API for older Emacs. `package.el`'s install path does not need it; installed packages may. Defer for the first milestone unless a target package requires new-style advice.

**`tabulated-list`.** It is the base for `package-menu`. The package.el research explicitly defers the UI, so this is only needed if the milestone includes an interactive package list. There is no GNU ELPA package; port from Emacs core if needed.

**`version-to-list`.** XEmacs's `lisp/version.el` is FSF 19.34 vintage (`lisp/version.el:23`) and only has `emacs-version>=`-style helpers. `package.el` uses `version-to-list` and the `version-list-<` family for minimum-version requirements and the `emacs` pseudo-dependency. The sampled Emacs `version.el` has no lexical-binding header; a port is small and self-contained.

## Adjacent add-on sources

These are not missing APIs in the same sense, but the home/bootstrap decision must account for them:

- **`tar-mode`** is in `os-utils` (`etc/package-index.LATEST.gpg:1299`). GNU `package.el` uses it to unpack tarballs. XEmacs's existing `package-admin` instead shells out to `gunzip | tar` (`lisp/package-admin.el:123-135`), which is a viable first-milestone alternative.
- **`url`/`url-http`** are in `w3` (`etc/package-index.LATEST.gpg:2267`). GNU `package.el` uses `url-retrieve`; XEmacs core has no `url-retrieve`. The map has chosen the async/libcurl branch as the HTTP foundation, so `url` is a fallback path, not the first choice.
- **`efs`** is in the add-on tree (`etc/package-index.LATEST.gpg:2487`). It is the existing `package-get` transport, FTP/local only, not HTTPS.
- **Old `advice`** is in `xemacs-base` (`etc/package-index.LATEST.gpg:440`); it does not provide `advice-add`.

## What is actually on the minimum path

The package.el research ([ticket #14](https://github.com/fktpp2022/xemacs-git/issues/14), `docs/research/package-el-minimum-path.md` on `research/package-el-minimum-path`) identifies these steps: fetch and parse `archive-contents`; compute a transaction with minimum versions and the `emacs` pseudo-dependency; download and extract the tarball; byte-compile and generate autoloads; optionally verify signatures; install under a versioned directory.

Of those steps, the API-critical gaps are `version-to-list` and `cl-lib`/`cl-defstruct`. `seq` and `subr-x` depend on which `package.el` version is vendored. `tabulated-list`, `json`, and `nadvice` are not required for a batch install-and-run test.

## Audit items

- Do `cl-lib` 0.7.1, `seq-24.el`, and `nadvice` 0.4 actually avoid lexical capture? Search for closures that capture `let` bindings and run their tests under XEmacs with `lexical-binding` ignored.
- Which `package.el` version will be vendored? That determines whether `seq` and `subr-x` are on the path.
- Does the chosen transport need `json`? GNU ELPA's `archive-contents` is Lisp.
- Is the interactive package menu in the milestone? If not, `tabulated-list` is deferrable.

## Evidence

- Local paths: `lisp/cl.el:623`; `lisp/cl-macs.el:2654`; `lisp/version.el:23`; `lisp/package-admin.el:123-135`; `etc/package-index.LATEST.gpg:440,1299,2267,2487`.
- GNU ELPA archive: <https://elpa.gnu.org/packages/archive-contents> — `cl-lib` 0.7.1 ("Forward cl-lib compatibility library for Emacs<24.3"), `nadvice` 0.4 ("Forward compatibility for Emacs-24.4's nadvice"), `seq` 2.24, `compat` 31.1.0.0 (requires Emacs 25.1).
- Tarball headers inspected: `cl-lib-0.7.1/cl-lib.el:1` (`-*- lexical-binding: t; -*-`); `seq-2.24/seq-24.el:1` (`-*- lexical-binding: t -*-`); `seq-2.24/seq.el:1` (`-*- lexical-binding: t -*-`); `nadvice-0.4/nadvice.el:1` (`-*- lexical-binding: t; -*-`).
- Emacs core candidates: `lisp/emacs-lisp/subr-x.el` (24.4-era, `-*- lexical-binding:t -*-`), `lisp/json.el` (24.x, no lexical-binding declaration in the sampled header), `lisp/tabulated-list.el` (24.x, declares `lexical-binding: t`), `lisp/emacs-lisp/nadvice.el` (declares `lexical-binding: t`), and `lisp/version.el` (no header).
