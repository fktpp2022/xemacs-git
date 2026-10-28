# cnfonts on XEmacs: the breakage catalogue

Ticket: [#79](https://github.com/fktpp2022/xemacs-git/issues/79) (wayfinder map [#77](https://github.com/fktpp2022/xemacs-git/issues/77)).
Branch: `prototype/cnfonts-load-79`. Date: 2026-10-28.

Question: *does unmodified upstream `cnfonts.el` load and run on XEmacs at all, and what breaks?*

## Verdict

1. **`(require 'cnfonts)` loads with zero stubs** — confirmed again (it was already known from #80).
2. **`(cnfonts-enable)` runs with zero stubs and reaches no font code at all.** It installs two hooks and prints a message. Milestone (B) as the map words it — "the elpa client fetches cnfonts and `cnfonts-enable` runs without error" — is **already satisfied**. #80's "the remaining breakage is at `cnfonts-enable` (run)" is wrong: the breakage is one level down, in `cnfonts--set-font-1`, which is milestone (C)'s work.
3. The font path runs once the missing GNU APIs are supplied. The call surface is **seven** functions, not the five ADR 0002 names.
4. Two of cnfonts.el's assumptions about XEmacs are wrong. The dynamic-scope one is harmless; the specifier-tag one breaks ADR 0002's stated lowering.

## What was run

- Build: `./build/bin/xemacs` — XEmacs 21.5 beta36 "leeks" [Lucid], Mule. Headless: `(device-list)` → `(#<stream-device on "stdio">)`, `(display-graphic-p)` → `nil`. Compiled **without** X11/Xft/GTK (`HAVE_X_WINDOWS`, `HAVE_XFT` undefined in `build/src/config.h`).
- Package: `cnfonts-20251223.636` from MELPA, installed by the repo's own elpa client (`lisp/elpa.el`) against a local mirror of `melpa.org/packages` (the sandbox's proxy route made the live fetch flaky). Files: `cnfonts.el`, `cnfonts-ui.el`, `cnfonts-pkg.el`, plus generated autoloads.
- Prototype (this branch): `prototype/cnfonts-load-79.el` (the driver), `prototype/xemacs-shim.el` (recording stubs), `prototype/probe-tags.el` (the specifier-tag probe).

```
CNFONTS_PKG_DIR=<elpa-user-dir>/cnfonts-20251223.636 \
  build/bin/xemacs --batch -l prototype/cnfonts-load-79.el
```

## Phase-by-phase catalogue

| Phase | Call | Result |
|---|---|---|
| 1 | symbol availability | the ten GNU APIs below are absent; `set-face-font`, `face-property`, `font-list`, `display-graphic-p`, `display-multi-font-p` present |
| 2 | `(require 'cnfonts)`, no shim | **OK** |
| 3 | `(cnfonts-enable)`, no shim | **OK** — sets `after-make-frame-functions` and `window-setup-hook` to `(cnfonts-set-font)`, prints the mode message, touches no font API |
| 4 | `(cnfonts-set-font)`, shim loaded | **OK but a no-op** — headless, `display-graphic-p` is `nil`, so the `(when (display-graphic-p frame) …)` guard skips the whole font path |
| 4 | `(cnfonts--get-valid-fonts)` | **OK** → `("Monaco" "微软雅黑" "HanaMinB" "Segoe UI Symbol" "NanumGothic")` |
| 4 | `(cnfonts--font-exists-p "Consolas")` | **OK** → `"Consolas"` |
| 4 | `(cnfonts--set-font-1 (12 12 12 12 12))` | **OK** once the stubs lower `font-spec` to a name string; 53 stub calls recorded (below) |
| 6 | dynamic-scope check | compiled and interpreted `cnfonts--font-exists-p` agree; **no closure misbehaviour** |
| 7 | faces after the run | `bold`/`italic`/`bold-italic` got `global="Monaco-12.0"` (cnfonts calls `set-face-font` directly — XEmacs has it); `default` did **not** (it goes through the `set-face-attribute` stub) |

## The shim surface, as cnfonts actually asks for it

Recorded from the run (53 calls):

| Function | Calls | Load-bearing? | What the shim must answer |
|---|---|---|---|
| `font-spec` | 8 | yes | a font XEmacs' `set-face-font` accepts (name string or font object), from `:name`/`:size`/`:weight`/`:slant` |
| `list-fonts` | 9 | yes | "is this font available?" — cnfonts uses the answer to pick a font |
| `x-list-fonts` | 11 | yes | a font list cnfonts can parse (it slices the XLFD: `(cl-subseq lst 2 (- (length lst) 12))`) |
| `font-family-list` | 0 in this run | yes | the installed families, so `cl-find-if` can match the parsed name — reached only when `x-list-fonts` finds nothing, i.e. for every *missing* font, so a real shim hits it constantly |
| `string-join` | 11 | no | `(mapconcat #'identity …)` |
| `set-face-attribute` | 1 | sink | lower to `set-face-font` (cnfonts only ever passes `:font`) |
| `set-fontset-font` | 13 | sink | lower to `set-face-font … 'global TAG-SET` |

`set-fontset-font`'s 13 calls carry three shapes:

```
("fontset-default" kana    "微软雅黑-12.0" nil nil)          ; 5 CJK charsets
("fontset-default" hangul  "HanaMinB-12.0"  nil append)       ; ext-B fallback for hangul
("fontset-default" nil     "HanaMinB-12.0"  nil prepend)      ; ext-B default (charset nil)
("fontset-default" symbol  "Segoe UI Symbol-12.0" nil prepend); 2 symbol charsets
("fontset-default" (10102 . 10131) "NanumGothic-12.0" nil prepend) ; 4 ornament RANGES
```

The four ornament calls pass **codepoint ranges** (`(10102 . 10131)`), which XEmacs has no key for — confirming ADR 0002's "no codepoint-range key" from the other direction.

## The stub-compat policy, decided by the evidence

A blanket no-op stub is wrong, and the catalogue says exactly why — the ten missing functions split into two kinds:

- **Load-bearing** (`font-spec`, `list-fonts`, `x-list-fonts`, `font-family-list`): cnfonts uses their *return values* to decide which fonts to use. A constant or no-op answer makes cnfonts silently pick the wrong font (or none) — the failure is invisible and the profile looks configured.
- **Sink** (`set-face-attribute`, `set-fontset-font`, and `string-join`): the return value is unused, but the *effect* is the whole point. A silent no-op here yields "cnfonts-enable ran without error and configured nothing" — the exact false green the milestone (B) success test must not be able to report.

So the policy is: **sink calls must actually lower, and where the lowering is approximate, say so** (the EXT-B and ornament-range fallbacks are approximations and belong in a warning or a documented degradation, not in silence); **load-bearing calls must answer with real XEmacs data**, and if XEmacs cannot answer, the shim must say so rather than guess.

## What is *not* broken

- `cl-lib` is core-provided (milestone 2) and supplies every `cl-*` cnfonts uses (`cl-subseq`, `cl-remove-if`, `cl-find-if`, `cl-loop`, `cl-incf`, `cl-remove-duplicates`, `cl-letf`, `cl-position`, `setf`).
- `when-let*`, `delete-dups`, `float`, `display-graphic-p`, `display-multi-font-p`, `with-selected-frame`, `declare-function`, `run-hook-with-args`, `define-minor-mode` all exist.
- `mwheel` loads; only `mouse-wheel-text-scale` is missing (XEmacs' `mwheel` has a different API). `touch-screen` is soft-required and absent; `touch-screen-pinch` is missing with it. Neither is on the font path — both are in the scroll/zoom commands.
- **Dynamic scope is not a problem here.** cnfonts.el is `-*- lexical-binding: t -*-` and XEmacs ignores that, but the only closure in cnfonts.el that captures a variable is the `(lambda (x) …)` inside `cnfonts--font-exists-p`, and it is invoked while its `let`/parameter binding is still live. Compiled and interpreted runs return the same answers. (The byte compiler's "variable `non-num-p` bound but not referenced" warning is cnfonts' own dead binding, not a scope bug.)

## Two conflicts with the plan

### 1. The shim surface is seven functions, not five

ADR 0002 says the compat layer "overrides **exactly those five** functions … the research found only five calls with no XEmacs equivalent, so five is the whole surface". The run contradicts that:

- **Three more GNU APIs are called and absent**: `list-fonts` (9 calls), `font-family-list` (11), `string-join` (11). They are load-bearing (see above).
- **`face-attribute` is never called by cnfonts.el.** It appears only in `cnfonts-ui.el`, which the map puts out of scope. So of ADR 0002's five, four are needed for cnfonts.el.

The byte compiler is not a reliable detector here: `lisp/obsolete.el` has `(make-compatible 'list-fonts 'font-list)`, so the compiler knows `list-fonts` while the runtime does not have it (`fboundp` → `nil`, with or without `(require 'obsolete)`). #80's byte-compile warning list therefore understates the surface by `list-fonts`.

### 2. ADR 0002's `set-fontset-font` lowering errors in a TTY-only build

ADR 0002 lowers `set-fontset-font` to `(set-face-font FACE FONT 'global '(two-dimensional final))`. In this build that call **signals `Invalid argument: Invalid tag set, (two-dimensional final)`**.

Root cause: XEmacs registers the charset-class specifier tags (`two-dimensional`, `one-dimensional`, `initial`, `final`, `jit-charset`, `x-coverage-instantiator`) inside

```c
#if defined (HAVE_X_WINDOWS) || defined (HAVE_GTK)     /* src/faces.c:2286 … 2453 */
    define_specifier_tag (Qtwo_dimensional, Qnil,
                         intern ("specifier-tag-two-dimensional-p"));
```

so a build with neither X11 nor GTK has **none** of them. `(specifier-tag-list)` in this build returns only the device types (`stream tty color … printer`); `(valid-specifier-tag-set-p '(two-dimensional final))` → `nil`. This also means the repo's own `docs/user-config/xft-chinese-font-configuration.el` — which sets the Chinese font with `'(two-dimensional final)` inside a `condition-case` — silently does nothing in such a build.

The cheap fix works and is one line:

```elisp
(define-specifier-tag 'cnfonts-2d nil 'specifier-tag-two-dimensional-p)
(set-face-font 'default "Noto Sans Mono CJK SC-14" 'global '(cnfonts-2d) 'prepend)
;; => default's font specifier: global=((cnfonts-2d) . "Noto Sans Mono CJK SC-14")
```

`specifier-tag-two-dimensional-p` exists in every build, so re-registering it under a Lisp tag reproduces the built-in's charset semantics without a charset inventory. A **stage** tag cannot be re-registered from Lisp: `define-specifier-tag` requires a one-argument charset predicate, and the stage predicates take two (`specifier-tag-final-stage-p`), and the C source says "We only allow the stage argument to be specified from C."

## Consequences for the map

- Milestone (B) as written is already met; the remaining work is (C) plus a shim that answers seven functions instead of five.
- The headless success gate cannot use the public path: headless, `display-graphic-p` and `display-multi-font-p` are both `nil`, so `cnfonts-set-font` skips the font work entirely. The gate has to reach `cnfonts--set-font-1` (or fake the display check).
- ADR 0002 needs an amendment on two points: the shim surface (four of its five, plus three) and the tag-set lowering (a Lisp-defined tag, not the built-in `two-dimensional`).

## Reproduction

```bash
# 1. mirror MELPA (the sandbox proxy made the live fetch flaky)
mkdir -p /tmp/melpa-mirror-79
curl -sS -o /tmp/melpa-mirror-79/archive-contents https://melpa.org/packages/archive-contents
curl -sS -o /tmp/melpa-mirror-79/cnfonts-20251223.636.tar \
     https://melpa.org/packages/cnfonts-20251223.636.tar

# 2. install it with the repo's elpa client
cat > /tmp/install.el <<'EOF'
(require 'elpa)
(setq elpa-archives '(("melpa" . "/tmp/melpa-mirror-79/")))
(setq elpa-installable-packages (append '(cnfonts) elpa-installable-packages))
(setq elpa-user-dir (file-name-as-directory (make-temp-file "elpa-cnfonts79-" t)))
(elpa-install "cnfonts")
(elpa-activate-all)
(message "DIR: %s" (cdr (elpa--newest-installed-build 'cnfonts)))
EOF
build/bin/xemacs --batch -l /tmp/install.el

# 3. run the catalogue
CNFONTS_PKG_DIR=<elpa-user-dir>/cnfonts-20251223.636 \
  build/bin/xemacs --batch -l prototype/cnfonts-load-79.el

# 4. the specifier-tag probe
build/bin/xemacs --batch -l prototype/probe-tags.el
```
