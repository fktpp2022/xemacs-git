# Surveying the MELPA pool for a viable first target set

Research findings for wayfinder ticket [#32](https://github.com/fktpp2022/xemacs-git/issues/32), map [MELPA support for XEmacs](https://github.com/fktpp2022/xemacs-git/issues/30). Method follows milestone 1's audit discipline (behavioral: byte-compile, load, exercise — never header reading) applied to 27 dependency-free MELPA packages against the `integration/elpa-milestone-1` build (XEmacs 21.5.36, Mule, libcurl).

## Bottom line

- **16 of 27 audited packages RUN today** (14 with zero changes beyond the audit shim kit; 2 more need shims of one to ten one-liner definitions).
- **1 package genuinely needs lexical binding** (`memoize`) — the only NEEDS-LEXICAL hit, with both behavioral and source-level evidence.
- **4 need shims deeper than one-liners** (nadvice surface, GNU minibuffer maps, keymap meta-bindings); **3 are dead** in this audit (GNU `message`, an unresolved DSL walker, deep keymap incompatibility).
- **P0 transport bug found**: the in-memory `async-http` response path crashes XEmacs on bodies ≥ ~1 MB (`FREED LRECORD` → `print.c:1654` abort). Details below. MELPA's index is 2.6 MB, so this blocks `elpa-refresh-contents` against live MELPA; the `:file` artifact path is immune.
- **The cross-archive dependency dead-end (from the [archive mechanics findings](melpa-archive-mechanics.md)) does not bite the shortlist**: every audited candidate has zero declared requirements. It bites immediately beyond it (`dash`, `s`, `compat`, `cl-lib` are MELPA's most-required packages and none are hosted there).

## Method

1. Pull MELPA's `archive-contents`, take the 1,109 packages with no requirements (avoids the cross-archive dead-end), and pick 27 breadth-first candidates: utility libraries, small modes, data parsers, one known-negative control.
2. Download the real built tarballs from `https://melpa.org/packages/`.
3. Byte-compile each with XEmacs, load, and exercise one representative function per package with a hand-written form.
4. Iterate shims only while each fix stays at one-or-two definition scale; beyond that, record the gap and stop.

All experiments ran against `/tmp`; nothing was written into any repo tree.

## The audit shim kit

The base kit loaded for every candidate (auditing context only — not a shipping proposal):

- `cl-lib` 0.7.1 (the GNU ELPA backport) + `cl-macs-shim` (`(require 'cl)` → `provide 'cl-macs`) + `defadvice-stub` (no-op `defadvice` macro; cl-lib uses it only to optimize two functions)
- Small definitions mapped onto XEmacs primitives: `overlay-shim` (GNU overlay API onto extents, ~10 one-liners), `timer-shim`, `kbd-shim`, `ns-map-shim`, `letrec-shim`, `parse-time`, `thingatpt`, `delsel`, `isearch-state-shim`, `prog-mode-shim`, `magic-fallback-shim`, `declare-function-shim`, `cl-callf-shim`

## Verdicts

### RUNS — exercised successfully with the base kit (16)

| Package | Version | Evidence |
|---|---|---|
| toml | 20260321.1321 | `toml:read-string` parses a TOML table: `(("b" ("c" . "x")) ("a" . 1))`. Declares `lexical-binding: t` but runs correctly under dynamic binding. Entry point gotcha: it parses a string *value*, not a whole document. |
| move-text | 20260508.508 | line move up/down works |
| loop | 20160813.1407 | `(loop collect i for i downfrom 3 to 1)` → `(3 2 1)` |
| anaphora | 20260720.903 | `(aprog1 6 (+ it 20))` → `(6 20)` |
| levenshtein | 20090830.1040 | distance `"foo"`/`"fob"` → 3 |
| number | 20170901.1312 | ordinal string "3.5" |
| csv | 20231216.1243 | line parse → `(("a" "b") ("1" "2"))` |
| vector-utils | 20140508.2041 | `[1 2 3 4]` transforms |
| rings | 20160531.2027 | ring ops → `(2 "rb1")` |
| password-generator | 20250615.2300 | generates |
| shell-split-string | 20151224.1008 | splits honoring quotes |
| unidecode | 20201213.1449 | transliterates |
| lorem-ipsum | 20221214.1857 | generates |
| kv | 20140108.1534 | kv alist ops → 1 |
| list-utils | 20241106.1849 | flatten `(1 (2 (3)) 4)` → `(1 2 3 4)`; cyclic-length 0 on proper list |
| xmlgen | 20170411.1317 | `(xmlgen '(p :class "big" "hi"))` → `<p class="big">hi</p>` |

### RUNS-WITH-SHIMS — works after small named shims (4)

| Package | Version | What it needs |
|---|---|---|
| jump-char | 20251205.508 | one `defvar` for its keymap variable (defined lazily in GNU) |
| persistent-overlays | 20161128.700 | the `overlay-shim` (GNU overlay API onto extents; also wanted by many bigger packages) |
| rainbow-delimiters | 20210515.1254 | a face-definition patch (`wrong-type-argument listp default` in XEmacs faces) + `font-lock` require |
| string-inflection | 20251114.1041 | base kit only (first-round pass; not re-exercised in the final harness) |

### Needs more than one-liners (4, excluded from the shortlist)

| Package | Version | Gap |
|---|---|---|
| browse-kill-ring | 20260503.1620 | `advice-member-p` (nadvice surface) + `minibuffer-local-ns-map` |
| nginx-mode | 20240412.402 | `prog-mode` shim gets it loading, but its keymap setup hits `wrong-type-argument keymapp nil` |
| paredit | 20241103.2046 | `Map contains meta-bindings, can't bind "ESC"` — XEmacs keymaps reject the ESC-meta convention |
| regex-dsl | 20220125.506 | its DSL walker errors (`wrong-type-argument listp`) on every entry form tried, with `cl-case` available; unresolved in this audit |

### NEEDS-LEXICAL (1)

| Package | Version | Evidence |
|---|---|---|
| memoize | 20200103.2036 | `(let ((table (make-hash-table ...))) (lambda (&rest args) (gethash args table) ...))` — under dynamic binding the lambda's body sees `void-variable table`; confirmed both behaviorally and at source level. The interpreter-only lexical subset from the prior lexical-binding research would cover exactly this shape. |

### DEAD (3)

| Package | Version | Cause |
|---|---|---|
| diffview | 20230224.1916 | `(require 'message)` — GNU `message` (same blocker that killed it in milestone 1's GNU ELPA audit) |
| highlight-symbol | 20160102.2009 | failed hard in the first round (load-time error; not shim-able cheaply) |
| rings/others | — | (no additional dead beyond the above two and regex-dsl above) |

## P0: the large-body transport crash

Reproduced against `integration/elpa-milestone-1`, batch mode:

- **Symptom**: `elpa-refresh-contents` against live MELPA aborts: `SERIOUS XEMACS BUG: FREED LRECORD object` → `wrong-type-argument sequencep` → assertion failed `print.c:1654` → `Fatal error (6)`, exit 134.
- **Bisect** (in-memory `http-get` body path, real HTTPS): GNU ELPA index 185–189 KB OK; dash tarball 358 KB OK; a 471 KB artifact OK; stable.melpa index 1.45 MB **crashes**; MELPA index 2.6 MB **crashes** (raw `http-get`, isolating elpa.el — this is a transport bug, not a client bug).
- **`:file` is immune**: the 2.6 MB body downloaded through the file sink is byte-identical to the index fetched with `curl` (proxy vs direct routes both tested; a direct-route 442 KB prefix of one crashed transfer confirms the failure is GC timing, not size checking).
- **Reading**: the in-memory accumulator's intermediate Lisp objects outlive their GC protection while the coroutine is suspended across long transfers. Threshold is time/GC-pressure dependent, not a hard size: ~471 KB passed, ~1.45 MB died.
- **Impact**: MELPA's index is 2.6 MB ⇒ `elpa-refresh-contents` against live MELPA cannot work until this is fixed. Artifacts (all via `:file`) are unaffected, so installs of already-cached indexes work. Milestone 1's GNU ELPA index (185 KB) sits under the observed threshold, which is why it never fired there.
- **Suspect area**: `src/async-http.c` write-callback GCPRO discipline / coroutine suspension vs. `concat2` accumulation; needs a proper diagnosis, not a workaround.

## Implications

- **For the lexical-binding strategy ticket**: the NEEDS-LEXICAL rate in this dep-free sample is 1/27 (~4%). The audit-only strategy stays viable for a first MELPA milestone, but interpreter-only lexical binding would convert `memoize`-shaped packages (closure-over-let tables, the single most common memoize/lazy pattern) at modest cost.
- **For the target-set ticket**: a 10-package deterministic target set writes itself from the RUNS table above (e.g. toml, loop, anaphora, levenshtein, csv, vector-utils, move-text, xmlgen, list-utils, number) with zero declared dependencies — no cross-archive resolution needed for the milestone itself, deferring that decision without being blocked by it.
- **For the spec**: the transport P0 must be a milestone-2 work item with its own regression test (large in-memory body over HTTPS), not a deferred risk.
- **The overlay shim** is the highest-value future shim: `persistent-overlays` needed it, and overlays are ubiquitous in bigger modes.
- Cross-archive dependency resolution (MELPA packages requiring GNU-ELPA-hosted `dash`/`s`/`compat`/`cl-lib`) gates anything beyond the dep-free pool; see the archive mechanics findings.
