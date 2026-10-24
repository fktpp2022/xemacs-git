# MELPA milestone 2: behavioral audit of dependency-bearing packages

Ticket: #46. Branch: `research/melpa-m2-audit`. Date: 2026-10-24.

This is a decision-support audit, not a shipping proposal. All behavioral
evidence was produced in a child XEmacs 21.5 build, using the draft
milestone-1 compatibility kit. No header-only classification is recorded.

## Method

1. **Pool analysis.** The full MELPA index (`melpa-index.txt`) and the GNU ELPA
   index (`gnu-index.txt`) were parsed to count entries, dependency-bearing
   entries, and closures resolvable entirely through MELPA + GNU ELPA. Results
   are in `/tmp/m2audit/analysis.txt`; its trailing parser error occurs after
   all headline numbers were printed and was not debugged.
2. **Candidate slice.** Sixteen low-dependency, behaviorally diverse packages
   were selected (`/tmp/m2audit/more.txt`): string/XML, table rendering, key
   binding, note taking, buffer management, modes, and CLI-style helpers.
3. **Kit.** `/tmp/m2audit/kit/load-kit.el` loads the reconstructed #32 shims
   (`overlay`, `timer`, `kbd`, `ns-map`, `parse-time`, `thingatpt`, `prog-mode`,
   `magic-fallback`, `subr-x`, `misc`) plus real GNU ELPA `cl-lib` 0.7.1,
   `seq` 2.24 (`pcase` forms rewritten for the XEmacs reader), `let-alist`,
   `queue`, and `spinner`.
4. **Exercise.** Each package was byte-compiled with
   `/tmp/m2audit/runone.sh <pkg> <dir>`, then loaded in a fresh batch XEmacs
   and a representative public-API form was evaluated. Compile errors and
   runtime results are appended to `/tmp/m2audit/results.txt`.
5. **Classification.**
   - `RUNS` — exercise produced a value with zero compile errors.
   - `RUNS-WITH-KIT` — loads and a representative function works, though the
     build is partial (e.g. one optional integration fails to compile).
   - `SHIM/API-GAP` — loads but needs more than the kit (lexical closures,
     character semantics, parser behavior, macro expansion).
   - `BLOCKED` — dependency, reader, or missing core library prevents the
     exercise from running.
   One fix attempt per failing exercise was allowed; failures were then
   recorded rather than debugged.

## Dependency-wall statistics

From `/tmp/m2audit/analysis.txt`:

| Metric | Count |
|---|---:|
| MELPA entries | 6,333 |
| Dependency-bearing entries | 3,237 |
| Resolvable through MELPA + GNU ELPA | 3,186 |
| Missing (unresolvable closure) | 51 |
| Candidate pool after filtering | 3,173 |
| GNU ELPA entries indexed | 509 |

Top declared dependencies (MELPA / GNU availability):

| Dependency | Dependents | MELPA | GNU |
|---|---:|---|---|
| `dash` | 544 | yes | yes |
| `cl-lib` | 544 | no | yes |
| `s` | 417 | yes | no |
| `org` | 220 | no | yes |
| `f` | 175 | yes | no |
| `compat` | 168 | no | yes |
| `helm` | 168 | yes | no |
| `flycheck` | 140 | yes | no |
| `request` | 140 | yes | no |
| `transient` | 125 | yes | yes |
| `evil` | 91 | yes | no |
| `seq` | 88 | no | yes |

Two walls dominate. First, the declared-closure wall: nearly half of MELPA has
at least one dependency, and most closures route through GNU ELPA packages
(`cl-lib`, `seq`, `compat`, `let-alist`) rather than MELPA-only libraries.
Second, the **undeclared runtime wall**: every package audited below declares
only `emacs` and `cl-lib`, yet several `require` GNU core libraries that the
XEmacs tree lacks (`xml`, `color`, `help-mode`, `smie`, `bibtex`, `org`).
Package headers therefore understate milestone-2 work.

## Candidate results

`compile` is the count of `Error occurred processing` lines from
`batch-byte-compile`. `cl-lib` versions below are the declared minimums; the
kit supplies GNU ELPA `cl-lib` 0.7.1 in every case. `emacs` pseudo-deps are
ignored.

| Package | Version | Declared deps | Undeclared runtime needs | Compile | Exercise result | Class |
|---|---|---|---:|---|---|---|
| `dmacro` | 20241027.830 | emacs 24.1, cl-lib 0.6 | — | 0 | `RESULT 1` | RUNS |
| `checkbox` | 20141117.58 | emacs 24, cl-lib 0.5 | — | 0 | `RESULT "- [ ] Buy gin "` | RUNS |
| `es-lib` | 20141111.1830 | cl-lib 0.3 | `apropos`, `loadhist`, bundled `es-lib-*` files | 0 | `RESULT "abc"` | RUNS |
| `buffer-sets` | 20260817.2118 | cl-lib 0.5 | optional `ibuf-macs` | 0 | `RESULT t "work"` | RUNS |
| `annalist` | 20260531.1558 | emacs 24.4, cl-lib 0.5 | `ob-core` (Org Babel) | 1 | `RESULT ((:b 2 :a 1) (:a (:c 2 :b 1)))` | RUNS-WITH-KIT |
| `ctable` | 20210128.629 | emacs 24.3, cl-lib 0.5 | — | 0 | `EX-ERR (void-variable buffer)` | SHIM/API-GAP |
| `fix-word` | 20260802.938 | emacs 24.1, cl-lib 0.5 | — | 0 | `EX-ERR (void-variable fnc)` | SHIM/API-GAP |
| `drawille` | 20160418.1838 | cl-lib 0.5 | `lisp-float-type` | 0 | `EX-ERR (wrong-type-argument characterp 10240)` | SHIM/API-GAP |
| `html-to-markdown` | 20151105.840 | cl-lib 0.5 | `thingatpt` (kit) | 0 | `EX-ERR (error "File ended before closing </h1>.")` | SHIM/API-GAP |
| `general` | 20250612.2309 | emacs 24.4, cl-lib 0.5 | optional `evil`, `use-package` integration | 0 | `EX-ERR (invalid-function (run-edef-functions ...))` | SHIM/API-GAP |
| `esxml` | 20260329.1617 | emacs 24.1, cl-lib 0.5 | `xml`, `pcase` | 2 | `EX-ERR (file-error "Cannot open load file" "xml")` | BLOCKED |
| `contrast-color` | 20160903.1807 | emacs 24.3, cl-lib 0.5 | `color` | 1 | `EX-ERR (file-error "Cannot open load file" "color")` | BLOCKED |
| `elisp-slime-nav` | 20210510.528 | emacs 24.1, cl-lib 0.2 | `help-mode`, `etags` | 1 | `EX-ERR (file-error "Cannot open load file" "help-mode")` | BLOCKED |
| `forth-mode` | 20260315.803 | cl-lib 0.2 | `smie`, `comint`, optional `speedbar` | 6 | `EX-ERR (file-error "Cannot open load file" "smie")` | BLOCKED |
| `bog` | 20240215.27 | cl-lib 0.5 | `bibtex`, `org`, `org-agenda`, `org-compat` | 1 | `EX-ERR (file-error "Cannot open load file" "bibtex")` | BLOCKED |
| `easy-kill` | 20260121.752 | emacs 25, cl-lib 0.5 | `thingatpt` (kit) | 1 | `EX-ERR (invalid-read-syntax "Weird old-backquote syntax")` | BLOCKED |

Summary: 4 RUNS, 1 RUNS-WITH-KIT, 5 SHIM/API-GAP, 6 BLOCKED.

## Gap table

| Gap | Packages | Concrete symptom | What would close it |
|---|---|---|---|
| GNU core library absent: `xml` | `esxml` | `Cannot open load file "xml"`; 2 compile errors | Ship GNU `xml.el` (or a compatible reader) plus `pcase` handling |
| GNU core library absent: `color` | `contrast-color` | `Cannot open load file "color"`; 1 compile error | Ship GNU `color.el` (`color-name-to-rgb`, LAB conversion) |
| GNU core library absent: `help-mode` | `elisp-slime-nav` | `Cannot open load file "help-mode"` | Shim `help-mode`/`help-xref` or port GNU help.el |
| GNU core library absent: `smie` | `forth-mode` | `Cannot open load file "smie"`; 6 compile errors | Port SMIE (large; milestone-3 material) |
| Org/BibTeX stack | `bog`, `annalist` | `bibtex` / `ob-core` load failures | Org + BibTeX port; not kit-sized |
| GNU lexical closures | `ctable`, `fix-word` | `void-variable buffer` / `void-variable fnc` | Emulate lexical binding for returned lambdas, or patch packages |
| Reader: old-backquote syntax | `easy-kill` | `Invalid read syntax: "Weird old-backquote syntax"` | Reader support for GNU backquote shorthand; pre-processing not viable at scale |
| Character semantics | `drawille` | `(char-to-string 10240)` → `wrong-type-argument characterp` | GNU-compatible `char-to-string`/`characterp` for Unicode code points |
| HTML parser edge | `html-to-markdown` | `File ended before closing </h1>.` on any input | Diagnose regexp/`thingatpt` interaction; may be buffer narrowing |
| Macro expansion edge | `general` | `invalid-function (run-edef-functions ...)` from `general-define-key` | Investigate `cl-lib`/`defmacro` expansion on XEmacs |

Key observation: `ctable` and `fix-word` both fail on **lexical closure
capture**, not on missing functions. `ctable`'s inline destination stores
`(lambda () (with-current-buffer buffer ...))`; `fix-word` returns a command
closure over its `fnc` argument. Under dynamic binding the captured variable is
gone by call time. This is a semantic gap that no amount of GNU ELPA packages
closes; it needs either a lexical-binding emulation strategy or per-package
patches.

## Milestone-2 shortlist

Four low-risk targets plus two hard controls. Each is pinned to the exact MELPA
date version audited. The kit API surface listed is what the package exercises
beyond stock XEmacs.

### 1. `dmacro` 20241027.830

- Closure: `emacs` 24.1 (ignored), `cl-lib` 0.6 → GNU ELPA `cl-lib` 0.7.1.
- Exercise: `(dmacro-array-search "ab" "xab" 0)`.
- Expected: `1`.
- Kit surface: `cl-lib` (`cl-loop`, `cl-block`), byte-compile only.

### 2. `checkbox` 20141117.58

- Closure: `emacs` 24 (ignored), `cl-lib` 0.5 → kit `cl-lib`.
- Exercise: insert `"- Buy gin\n"`, `(checkbox-toggle)`, read `buffer-string`.
- Expected: `"- [ ] Buy gin "` (the trailing space is significant).
- Kit surface: `cl-lib`, `derived`/`font-lock` (already core).

### 3. `es-lib` 20141111.1830

- Closure: `cl-lib` 0.3 → kit `cl-lib`; bundled `es-lib-core-*`,
  `es-lib-text-navigate`, `es-lib-total-line`; core `apropos`, `loadhist`.
- Exercise: `(es-string-remove-properties (propertize "abc" 'face 'bold))`.
- Expected: `"abc"`.
- Kit surface: `cl-lib`, `defvar-local`/`with-eval-after-load` equivalents from
  `misc-shim`.

### 4. `buffer-sets` 20260817.2118

- Closure: `cl-lib` 0.5 → kit `cl-lib`; optional `ibuf-macs` guarded by
  `with-eval-after-load`; core `ibuffer` not required for the exercise.
- Exercise: create buffer `"m2-buffer-sets-test"`, associate with set
  `"work"`, then check `buffer-sets-buffer-in-set-p` and
  `buffer-sets-buffer-has-set-p`.
- Expected: `t "work"`.
- Kit surface: `cl-lib`, `defvar-local`, `with-eval-after-load`.

### Hard control A: `annalist` 20260531.1558

- Declared closure: `emacs` 24.4, `cl-lib` 0.5. Runtime `require`s `ob-core`
  (Org Babel), which the kit cannot supply, producing one compile error.
- Exercise: merge plists `'(:a 1)` and `'(:b 2)`, and nested plists
  `'(:a (:b 1))` with `'(:a (:c 2))`.
- Expected: `((:b 2 :a 1) (:a (:c 2 :b 1)))` (the core does load and run).
- Why hard: Org is a 220-dependent hub. Any shortlist that admits Org-adjacent
  packages inherits the whole Org port. Use this to mark the boundary:
  `annalist` itself runs, its Org integration does not.

### Hard control B: `general` 20250612.2309

- Declared closure: `emacs` 24.4, `cl-lib` 0.5. Optional integrations pull
  `evil` (91 dependents) and `use-package`; neither is in the kit.
- Exercise: `(general-define-key "C-c z" 'm2-test-command)` then
  `(lookup-key global-map (kbd "C-c z"))`.
- Observed: `invalid-function (run-edef-functions ...)` during macroexpansion
  of the extended definition, so the binding never happens.
- Why hard: `general` is the lightest visible member of the key-binding /
  `evil` / `use-package` cluster. If its macro layer cannot be made to expand,
  that whole cluster is out of scope for milestone 2.

## Evidence paths

- Index analysis: `/tmp/m2audit/analysis.txt` (stats), `/tmp/m2audit/melpa-index.txt`,
  `/tmp/m2audit/gnu-index.txt`.
- Candidate list: `/tmp/m2audit/more.txt`.
- Per-package results: `/tmp/m2audit/results.txt`.
- Compile logs: `/tmp/m2audit/logs/<pkg>-compile.log`.
- Kit: `/tmp/m2audit/kit/load-kit.el` and sibling shims.
- Exercise forms: `/tmp/m2audit/exercises/<pkg>.el`.
- Extracted sources and GNU dependencies: `/tmp/m2audit/work/<pkg>-<version>/`.
- Runner: `/tmp/m2audit/runone.sh`.

The `/tmp` artifacts are ephemeral. The exercise forms and classification table
above are the durable record; regenerate the binaries if a rerun is needed.

## Open questions

1. Should milestone 2 ship GNU core files (`xml.el`, `color.el`, `help-mode.el`)
   directly, or gate those packages out? `esxml` and `contrast-color` are
   otherwise clean and small; `help-mode` has a wider surface (`help-xref`,
   `help-fns`).
2. Can lexical closures be handled generically? `ctable` and `fix-word` fail
   only because a lambda captures a local. Options: a `lexical-let`-style
   rewrite layer, targeted patches, or declaring such packages unsupported.
3. Why does `html-to-markdown` reject even `<h1>Title</h1>`? The parser error is
   downstream of load, so it may be a `thingatpt` shim interaction or a regexp
   difference; one focused debugging session would settle whether it belongs in
   the shortlist.
4. Is `char-to-string` on XEmacs intentionally limited to Latin-1? `drawille`
   needs code points above 255; if the core cannot represent them in strings,
   that blocks all Unicode-art packages regardless of shims.
5. How much of `general`'s failure is `cl-lib` macroexpansion versus XEmacs's
   `defmacro`/`run-hooks` behavior? Resolving it would unlock a large cluster
   but may require a core change rather than a kit addition.
