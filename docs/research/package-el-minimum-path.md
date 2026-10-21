# GNU package.el's minimum install path

Research for wayfinder ticket [#14](https://github.com/fktpp2022/xemacs-git/issues/14), 2026-10-21. GNU Emacs references are to the `emacs-30.2` tag. Local XEmacs references are paths at commit `95dd9d7e` in this worktree.

## 1. The archive index (`archive-contents`)

GNU ELPA publishes one Lisp form, versioned by `package-archive-version` 1. It is an alist of package name to a vector `[VERSION REQS SUMMARY KIND EXTRAS]`, where `VERSION` is a version list, `REQS` is `((dep (n n ...)) ...)`, `KIND` is `tar` or `single`, and `EXTRAS` is an alist such as `:url`, `:keywords`, `:maintainer`, `:authors`, `:commit` ([package.el:1505-1557][P]; [archive-contents:1-10][AC]):

```elisp
(1
 (a68-mode
  . [(1 3) ((emacs (24 3))) "Major mode for editing Algol 68 code" tar
     ((:url . "https://git.sr.ht/~jemarch/a68-mode")
      (:keywords "languages")
      (:maintainer "Jose E. Marchesi" . "jemarch@gnu.org")
      (:commit . "a35b2fec07dcf9c3550cebc7f75e13f240088db2"))])
```

`elpa-admin.el` constructs exactly this vector when it publishes a package ([elpa-admin.el:1656-1667][EA]). `package-refresh-contents` fetches `<base>archive-contents` for each entry in `package-archives`, caches it under `package-user-dir/archives/<name>/archive-contents`, and later reads it with `read`; an archive version newer than 1 is an error ([package.el:211-235][P], [package.el:1606-1657][P], [package.el:1786-1864][P]). Archives may also be local absolute directories, not just HTTP(S).

XEmacs has the same concept but a different schema. `package-get-base` is an alist of package name to a list of plists with `version` (a string), `provides`/`requires` (symbols), `filename`, `size`, `md5sum`, `category`, and `type` (`regular`/`single-file`) ([lisp/package-get.el:118-162](lisp/package-get.el)). It is read from `package-index.LATEST.gpg` ([lisp/package-get.el:392](lisp/package-get.el)). Despite the suffix, that file is a PGP-clearsigned text block; the reader just scans for `(package-get-update-base-entry ...)` forms and never verifies the signature ([lisp/package-get.el:538](lisp/package-get.el), [lisp/package-get.el:562-583](lisp/package-get.el); [etc/package-index.LATEST.gpg:1-4](etc/package-index.LATEST.gpg)). A vestigial variable says "update the index even if it hasn't been signed," confirming verification was intended but not implemented ([lisp/package-get.el:405-406](lisp/package-get.el)).

## 2. Package metadata

For a single-file package, GNU metadata comes from library headers: `Version` (or `Package-Version`), `Package-Requires`, `Keywords`, `URL` ([package.texi:150-200][LT], [package.el:1165-1216][P]). For a multi-file package, the tarball must contain `<name>-pkg.el` with one `(define-package NAME VERSION DOCSTRING REQUIREMENTS ...)` form; `REQUIREMENTS` is a quoted list of `(dep "version")` pairs, and `emacs` is a pseudo-dependency ([package.texi:222-250][LT], [package.el:469-503][P], [package.el:756-769][P]). The client does not re-parse headers during an archive install: dependency resolution uses the already-parsed `archive-contents` data, and the tarball's `-pkg.el` is read to confirm name/version ([package.el:990-1039][P], [package.el:1234-1253][P]). GNU ELPA's `ace-window-0.10.0.tar` illustrates the layout: one top directory, `ace-window-pkg.el`, sources, no autoloads and no `.elc` ([ace-window-0.10.0.tar][AT]).

XEmacs's index has no equivalent of `Package-Requires`; dependencies are names of packages/provided symbols, with no minimum versions ([lisp/package-get.el:118-162](lisp/package-get.el), [lisp/package-get.el:816-856](lisp/package-get.el)). `package-get-package-provider` can match a `(symbol . version)` cons, but the dependency walker passes plain symbols and ignores versions ([lisp/package-get.el:1243-1276](lisp/package-get.el), [lisp/package-get.el:672-719](lisp/package-get.el)).

## 3. Versions and dependency resolution

GNU `version-to-list` parses dotted numeric versions plus `pre`, `beta`, `alpha`, and `snapshot` markers into integer lists with negative priorities (e.g. `"22.8beta3"` -> `(22 8 -2 3)`), and `version-list-<`, `version-list-<=`, `version-list-=` compare them ([subr.el:6895-6982][S], [subr.el:7028-7081][S]). `package-version-join` is the approximate inverse ([package.el:549-577][P]). `package-compute-transaction` recursively resolves `Package-Requires` against `package-archive-contents`, rejects candidates below the required minimum, checks what is already installed, special-cases `emacs` (fails with "requires Emacs version"), and returns dependencies before dependents ([package.el:1873-1965][P], [package.el:3259-3284][P]). It explicitly does not backtrack ([package.el:1888-1892][P]).

XEmacs does not have `version-to-list`; comparisons are numeric conversions of version strings (`string-to-number`) or exact string equality ([lisp/package-get.el:1013-1025](lisp/package-get.el), [lisp/package-get.el:1160-1169](lisp/package-get.el), [lisp/package-get.el:1230-1240](lisp/package-get.el)). Its recursive walker resolves required symbols through `provides`, tracks fetched symbols to break cycles, and installs providers, but has no version constraints ([lisp/package-get.el:672-719](lisp/package-get.el), [lisp/package-get.el:722-755](lisp/package-get.el)). The package UI only *offers* the computed dependencies for selection and then installs the selected list; it does not install the transaction automatically ([lisp/package-ui.el:411-463](lisp/package-ui.el), [lisp/package-ui.el:393-399](lisp/package-ui.el)).

## 4. Tarball download and extraction

GNU builds `<archive-base>/<name>-<version>.tar` for `kind` `tar`, or `.el` for `single`, and retrieves it with `url-retrieve`/`url-retrieve-synchronously` ([package.el:588-601][P], [package.el:2102-2146][P], [package.el:1382-1421][P]). `package-unpack` creates `package-user-dir/<name>-<version>/`, uses `tar-mode` to require that every member extracts under that directory, reads `<name>-pkg.el`, untars, generates metadata/autoloads, activates, and byte-compiles ([package.el:967-1039][P], [package.el:1234-1253][P]). Tarball naming and the single-top-directory rule are documented in [package.texi:222-250][LT].

XEmacs downloads `<pkg>-<version>-pkg.tar.gz` with EFS/ange-ftp path syntax (`/anonymous@host:dir/file`) and `copy-file`, verifies the index's MD5 with the core `md5` primitive, then runs `gunzip -c FILE | tar xvf -` into the package hierarchy ([lisp/package-get.el:1069-1078](lisp/package-get.el), [lisp/package-get.el:1094-1113](lisp/package-get.el), [lisp/package-get.el:1206-1228](lisp/package-get.el), [lisp/package-admin.el:123-135](lisp/package-admin.el)). There is no path-containment check; afterwards it relies on `MANIFEST.<pkg>` and a fixed tarball shape ([lisp/package-admin.el:266-403](lisp/package-admin.el)).

## 5. Byte-compilation and autoload generation

GNU generates `<name>-autoloads.el` by scanning every `.el` in the content directory (`loaddefs-generate`), embedding a form that adds the content directory to `load-path`, then byte-compiles the directory with `byte-recompile-directory` ([package.el:1090-1115][P], [package.el:1130-1137][P]). Activation loads `<name>-autoloads.el`; the generated file itself adds the package directory to `load-path` ([package.el:883-919][P], [package.el:1099-1112][P], [package.texi:98-112][LT]). The tarball must *not* ship an autoloads file; it is install-time output ([package.texi:273][LT]).

XEmacs installs *pre-bytecompiled* binary tarballs, not source ([lisp/package-admin.el:372-373](lisp/package-admin.el)). After extraction it adds `<hierarchy>/lisp/<pkg>/` to `load-path` and loads `auto-autoloads` (falling back to `_pkg`) ([lisp/package-get.el:766-783](lisp/package-get.el)). At startup, `packages-load-package-auto-autoloads` loads every `auto-autoloads.el` found on the package load path ([lisp/packages.el:482-485](lisp/packages.el), [lisp/startup.el:1696-1703](lisp/startup.el)). The primitives exist (`byte-recompile-directory`, `generate-file-autoloads`, `update-file-autoloads`; [lisp/bytecomp.el:1862](lisp/bytecomp.el), [lisp/autoload.el:363](lisp/autoload.el), [lisp/autoload.el:854](lisp/autoload.el)), but no installer wires them together for a downloaded source tarball.

## 6. Signature verification: optional by default

`package-check-signature` defaults to `allow-unsigned`: signatures are checked when a usable GnuPG configuration exists, unsigned packages are accepted, but a *bad* signature is always fatal. `t`/`all` require signatures; `nil` disables checking; `package-unsigned-archives` opts archives out ([package.el:354-389][P], [package.el:1425-1495][P], [package.el:391-397][P]). Both `archive-contents` and each tarball are checked before use, and good signatures are recorded in `.signed` files ([package.el:1806-1827][P], [package.el:2110-2146][P]). GNU ELPA serves `archive-contents.sig` and `<pkg>-<version>.tar.sig`; Emacs ships `etc/package-keyring.gpg` and imports it on refresh ([package-user.texi:443-481][UT], [package.el:1857-1863][P]; live `https://elpa.gnu.org/packages/archive-contents.sig` and `.../ace-window-0.10.0.tar.sig` both return 200).

XEmacs has no OpenPGP verification at all. The index is clearsigned but parsed as plain text, and tarballs are trusted via the MD5 value carried in that unverified index ([lisp/package-get.el:405-406](lisp/package-get.el), [lisp/package-get.el:562-583](lisp/package-get.el), [lisp/package-get.el:1094-1106](lisp/package-get.el)). Mandatory verification is therefore *not* required to match `package.el` defaults, but a faithful path must fail on a bad signature when one is present and allow opting into `t`/`all`.

## 7. Install layout, load-path, quickstart

GNU installs side-by-side content directories under `package-user-dir` (`~/.emacs.d/elpa/<name>-<version>/`), with additional system directories in `package-directory-list` ([package.el:303-332][P], [package.texi:89-112][LT]). Activation adds the content directory to `load-path` and loads `<name>-autoloads.el`. Quickstart is an optional optimization: `package-quickstart` concatenates the autoloads into `package-quickstart.el`, byte-compiles it, and `package-activate-all` loads that one file at startup instead of activating each package ([package.el:1704-1732][P], [package.el:4566-4655][P], [package-user.texi:520-540][UT]).

XEmacs installs into hierarchy directories (`xemacs-packages/`, `mule-packages/`, `site-packages/`) as `<hierarchy>/lisp/<pkg>/`, one version per package, and startup derives `load-path` by scanning one level below each `lisp/` directory ([lisp/package-admin.el:144-235](lisp/package-admin.el), [lisp/package-admin.el:432-438](lisp/package-admin.el), [lisp/packages.el:440-446](lisp/packages.el)). There is no versioned content directory and no quickstart precomputation. This is the cheapest integration seam: an ELPA content directory named `<name>-<version>` placed under a hierarchy's `lisp/` is already scanned, and an `auto-autoloads.el` there is loaded automatically; the GNU `<name>-autoloads.el` name is not.

## 8. Minimum faithful subset

1. **Index transport**: HTTPS (or local directory) fetch of `archive-contents`, cached per archive, with the version-1 gate. The async branch's HTTP client is the intended transport; XEmacs core has no `url-retrieve`.
2. **Index parser**: archive version 1, name -> `[version reqs summary kind extras]`, both `tar` and `single`.
3. **Version model**: port `version-to-list`, `version-list-<`/`<=`/`=`, and `package-version-join`, including prerelease markers.
4. **Resolver**: recursive minimum-version resolution over `archive-contents` and installed packages, cycle guard, dependencies installed first. Decide the `emacs` pseudo-dependency policy explicitly.
5. **Acquisition**: download `<name>-<version>.tar` (and `.el` for `single`), require one top-level `<name>-<version>/`, reject path escapes, validate `<name>-pkg.el` against the requested name/version.
6. **Install**: create `<name>-<version>/` content directory, generate autoloads from source, byte-compile the whole directory with dependencies on `load-path`.
7. **Activation**: add the content directory to `load-path`, load its autoloads at startup, and expose an explicit activate-all entry point. Quickstart can wait; it is a startup-time optimization, not semantics.
8. **Signatures**: optional, default allow-unsigned; bad signatures must fail. `gpg --verify` via `call-process` is the smallest viable implementation; XEmacs core has no EasyPG.

Defer `package-menu`/`tabulated-list`, upgrade/delete/selected-packages, `package-vc`, async refresh, and quickstart. None are on the install-and-run path.

## 9. Where the work lives

Do not grow `package-get`: its model differs at every layer (symbol provides instead of versioned package requirements, numeric string versions, binary precompiled tarballs, MD5-over-HTTP-less-FTP, index parsing without verification). A separate `package.el`-compatible client in core Lisp is the faithful shape; it can reuse `autoload.el` and `bytecomp.el`, and can initially use the existing hierarchy scan or its own `package-user-dir`. Tar extraction can stay external at first, but needs the path-containment check GNU gets from `tar-mode`. Signature checking can shell out to `gpg`.

Two gating facts for the map:

- **`emacs` requirements are the main compatibility wall.** GNU ELPA entries routinely require Emacs 24.3 or newer (e.g. `a68-mode` at [archive-contents:2-3][AC], `spinner` at [archive-contents:3083-3086][AC]). Faithful resolution rejects those on XEmacs 21.5. Either the first target set is chosen for absent/low `emacs` requirements, or the resolver adopts an explicit compatibility mapping. This is a decision, not an implementation detail.
- **`lexical-binding` is a run-time blocker independent of installation.** Pure-Elisp packages such as `ace-window` declare `lexical-binding: t` in the source ([ace-window.el:1][AT]); installing them does not make them run correctly on a dynamically bound XEmacs. The target set must be dynamically-binding-safe, or lexical binding must land first.

[P]: https://git.savannah.gnu.org/cgit/emacs.git/tree/lisp/emacs-lisp/package.el?h=emacs-30.2
[S]: https://git.savannah.gnu.org/cgit/emacs.git/tree/lisp/subr.el?h=emacs-30.2
[LT]: https://git.savannah.gnu.org/cgit/emacs.git/tree/doc/lispref/package.texi?h=emacs-30.2
[UT]: https://git.savannah.gnu.org/cgit/emacs.git/tree/doc/emacs/package.texi?h=emacs-30.2
[AC]: https://elpa.gnu.org/packages/archive-contents
[AT]: https://elpa.gnu.org/packages/ace-window-0.10.0.tar
[EA]: https://raw.githubusercontent.com/emacsmirror/elpa-admin/master/elpa-admin.el "mirror of elpa.git"
