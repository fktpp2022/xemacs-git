# XEmacs font / fontset surface and the cnfonts call mapping

Ticket: #78 (wayfinder map #77). Branch: `research/xemacs-font-surface`.
Date: 2026-05-10.

This is decision-support research, not a shipping proposal. It answers, for each
GNU font API that `cnfonts` calls, what XEmacs 21.5 actually provides and how
faithfully it can be mapped. It also settles the two open map questions:
(a) the backend design (`font-spec` → XEmacs font object, `set-fontset-font` →
per-charset application, which cnfonts internals to override), and (b)
per-charset feasibility — whether XEmacs can select a font per char range /
charset / language at all.

## Method

1. **Primary source reading.** The XEmacs C and Lisp font/face/specifier code in
   this tree: `lisp/font.el`, `lisp/faces.el`, `lisp/fontcolor.el`,
   `lisp/specifier.el`, `lisp/frame.el`, `lisp/mule/mule-charset.el`,
   `lisp/mule/mule-cmds.el`, `src/faces.c`, `src/specifier.c`, `src/fontcolor.c`,
   `src/fontcolor-x.c`, `src/fontcolor-xlike-inc.c`, `src/font-mgr.c`,
   `src/device-x.c`, `src/input-method-xlib.c`. Every claim below cites
   `file:line`.
2. **Empirical confirmation** against the working build
   `./build/bin/xemacs` (21.5 beta36 "leeks" [Lucid], Mule). The build is
   headless (`(device-list)` → `(#<stream-device on "stdio">)`,
   `(display-graphic-p)` → `nil`), so font *instantiation* cannot be exercised,
   but symbol exposure (`fboundp`), charset inventory, and data shapes were
   confirmed by batch evaluation.
3. **Call-site reading.** The GNU API surface `cnfonts` actually touches was read
   from the shipped `cnfonts` source (`/tmp/cnfonts.el`, 834 lines), not from
   memory, so the verdicts below are against real call forms.
4. **Prior art.** The in-tree working config
   `docs/user-config/xft-chinese-font-configuration.el` already drives XEmacs's
   per-charset font mechanism; it is used here as an independent confirmation
   that the mechanism is reachable from user Lisp.

The source is authoritative for *how it works*; the running build confirms *what
is exposed*.

## 1. Face font assignment

XEmacs has a first-class face system with a **font specifier** per face, and no
single "the font attribute" setter. The relevant Lisp surface:

- `(set-face-font FACE FONT &optional LOCALE TAG-SET HOW-TO-ADD)` —
  `lisp/faces.el:504`. Thin wrapper over `set-face-property` for the `font`
  property: `(set-face-property face 'font font locale tag-set how-to-add)`
  (`lisp/faces.el:523`; `set-face-property` at `lisp/faces.el:261`).
- `(set-face-property FACE PROPERTY VALUE &optional LOCALE TAG-SET HOW-TO-ADD)` —
  `lisp/faces.el:261`. For a built-in property it calls `set-specifier`
  (`lisp/faces.el:388`); for a user-defined property it `put`s or
  `add-spec-to-specifier`s (`lisp/faces.el:391-396`).
- `(face-property FACE PROPERTY &optional LOCALE TAG-SET EXACT-P)` —
  `lisp/faces.el:96`. Returns the specifier (no LOCALE) or the spec list for a
  locale.
- `(face-font FACE &optional LOCALE TAG-SET EXACT-P)` — `lisp/faces.el:455`,
  `(face-font-name FACE &optional DOMAIN CHARSET)` — `lisp/faces.el:447`, and
  `(face-font-instance FACE &optional DOMAIN CHARSET)` — `lisp/faces.el:473`.

**What a "font" argument may be.** From the `set-face-font` docstring
(`lisp/faces.el:505-522`) and `make-font-specifier` (`lisp/fontcolor.el:53-73`),
a font argument (instantiator) is one of:

- a **string naming a font** — platform-dependent syntax. X11/GTK1 XLFD
  (`"-*-courier-medium-r-*-*-*-140-*-*-*-*-iso8859-*"`), Xft/GTK2
  (`"Courier-14"`), MS-Windows (`"Courier:14:Western"`) — `lisp/fontcolor.el:62-67`;
- a **font instance** object (`lisp/fontcolor.el:68-69`);
- a **vector of no elements** (TTY: no font) (`lisp/fontcolor.el:70-71`);
- a **vector of one element** = a face to inherit from (`lisp/fontcolor.el:72`);
- a **list of instantiators**, an **alist of locale → instantiator list**, or a
  **font specifier object** (`lisp/faces.el:516-522`);
- and, via the `font.el` override `font-set-face-font`
  (`lisp/font.el:991-1031`), a **12-element font descriptor vector** (see §2),
  which is converted to a name by `font-create-name` and stored under the
  `font-specification` property (`lisp/font.el:993-995`).

Note there is **no fontset name** accepted here — a fontset name is meaningless
to `set-face-font` (see §3).

## 2. Font objects — the XEmacs equivalent of `font-spec`

XEmacs's font *descriptor* is a plain 12-element vector built by **`make-font`**:

```
(defun make-font (&rest args) ...)          ; lisp/font.el:361
;; => [:family F :weight W :style S :size SZ :registry R :encoding E]
```

(`lisp/font.el:361-376`.) Accessors `font-family`, `font-weight`, `font-style`,
`font-size`, `font-registry`, `font-encoding` (`lisp/font.el:172-187`). This is
the closest structural analogue of GNU's `font-spec`: a flat property bag with
`:family`, `:weight` (keyword), `:style` (bitmask built by `define-new-mask`,
`lisp/font.el:191-235`), `:size`, `:registry`, `:encoding`.

Two conversions exist, keyed by device type through
`font-window-system-mappings` (`lisp/font.el:108-116`):

- **font object → name:** `(font-create-name FONTOBJ &optional DEVICE)`
  (`lisp/font.el:379-384`). Per-device: `x-font-create-name`
  (`lisp/font.el:636`), `xft-font-create-name` (`lisp/font.el:831`),
  `mswindows-font-create-name` (`lisp/font.el:923`), `tty-font-create-plist`.
  (The mapping table also lists an `fc-font-create-name` at `lisp/font.el:112`,
  but no such function is defined anywhere in the tree — a dangling forward
  reference; the Xft route is `x`-device-typed and uses `xft-font-create-name`.)
- **name → font object:** `(font-create-object FONTNAME &optional DEVICE)`
  (`lisp/font.el:386-390`). Per-device: `x-font-create-object`
  (`lisp/font.el:566`), `xft-font-create-object` (`lisp/font.el:786`),
  `mswindows-font-create-object` (`lisp/font.el:879`).

Empirically, `(require 'font)` then
`(make-font :family "Noto Sans Mono CJK SC" :size 14)` returns
`[:family ("Noto Sans Mono CJK SC") :weight nil :style 0 :size 14 :registry nil :encoding nil]`.
The fontconfig/Xft path (`fc-font-create-name`, `fc-font-create-object`,
`lisp/fontconfig.el`) is the modern Xft route.

**Gaps vs GNU `font-spec`.** XEmacs's descriptor has no `:name`, `:foundry`,
`:slant` (slant is a style bitmask, not a keyword), `:width`, `:adstyle`, or
`:font-entity`/`:font-object` fields, and no `:extra` (OpenType features). GNU's
`(font-spec :name "Fam-14" :size 12 :weight 'bold :slant 'italic)` must be
lowered to either a plain Xft name string or a `make-font` call with the
corresponding keyword/bitmask. This is mechanical (see §5).

## 3. Per-charset / CJK font selection — the crux

**Verdict: XEmacs *can* select a font per charset, natively, but through its
specifier tag-set + charset-predicate mechanism — not through a fontset. The
map's claim "fontset exists only as a C/X-resource concept, no Lisp fontset API
at all" is CONFIRMED, but the *underlying per-charset capability* exists and is
used by redisplay.**

### 3a. No Lisp fontset API — confirmed

`fontsetp`, `get-fontset-info`, `get-font-info` appear only in a
`globally-declare-fboundp` forward-declaration block
(`lisp/font.el:47-48`) — a byte-compiler hint, **not a definition**. Empirical
check on the build: `fontsetp`, `get-fontset-info`, `get-font-info`,
`make-fontset`, `fontset` are all `fboundp = nil`, and
`(apropos-internal "fontset" 'fboundp)` is empty. The only "fontset" concepts in
the tree are:

- the **`-fontset` X resource** (`*FontSet`), used to seed the XIM input-method
  fontset — `src/device-x.c:101`;
- the **Xlib `XFontSet`** created for XIM in `x_init_fontset`
  (`src/input-method-xlib.c:1096`, described at `:865`);
- fontconfig's **`FcFontSet`**, a list of matching patterns returned by
  `FcFontList`/`FcFontSort` (`src/font-mgr.c:588` `fontset_to_list`,
  `:1076` `FcFontList`).

None of these is a Lisp-visible per-charset fontset object.

### 3b. Per-charset font selection does exist — via specifier tags

The face `font` property is a *font specifier*. A specifier maps
`(LOCALE, TAG-SET) → instantiator list`. The tag-set may include tags with a
**charset predicate**:

- `(define-specifier-tag TAG &optional DEVICE-PREDICATE CHARSET-PREDICATE)` —
  `src/specifier.c:1230`. Its docstring states the charset predicate "is
  primarily used to determine what font to use for a given (set of) charset(s)
  when that tag is used in a `set-face-font` call" (`src/specifier.c:1238-1241`).
- `(add-spec-to-specifier SPECIFIER INSTANTIATOR &optional LOCALE TAG-SET
  HOW-TO-ADD)` — `src/specifier.c:2237`; `TAG-SET` is the tag list attached to
  the instantiator. `(set-specifier ...)` (`lisp/specifier.el:318`) is the
  higher-level form.

At redisplay, the face cachel resolves the font **per charset** by calling
`face_property_matching_instance (face, Qfont, charset, …)` in an
initial-stage / final-stage search — `src/faces.c:1134, 1156, 1175, 1197`, with
`face_property_matching_instance` defined at `src/faces.c:577`. The match walks
the specifier's tag-sets through `charset_matches_specifier_tag_set_p`
(`src/specifier.c:1008-1061`), which consults the per-charset
`Vcharset_tag_lists` table (`src/specifier.c:49-57`). Font instantiation itself
lives in `src/fontcolor.c` (`font_spec_matches_charset` at `:1520`;
`find_charset_font` dispatch at `:1688`), and the X/Xft implementations are
`fontcolor-xlike-inc.c` (`font_spec_matches_charset` at `:123`,
`find_charset_font` at `:691`, `xft_find_charset_font` at `:409`). For a finally
unsatisfiable charset, X11 rewrites the XLFD to the charset's registries and
falls back to `iso10646-1` (`src/fontcolor-xlike-inc.c:691-770`).

**Built-in charset tags.** XEmacs pre-defines specifier tags for *classes* of
charsets (`src/faces.c:2334-2346`):

- `two-dimensional` → `specifier-tag-two-dimensional-p` (matches 2D charsets:
  Chinese, Japanese, Korean, Bopomofo) — `src/faces.c:2050`;
- `one-dimensional` → `specifier-tag-one-dimensional-p` — `src/faces.c:2036`;
- `initial` / `final` → stage predicates — `src/faces.c:2067, 2077`;
- `jit-charset` (for JIT/Unicode charsets) — `src/faces.c:2094`.

The built-in tag definitions are at `src/faces.c:2334-2348`. The default face's
font specifier is seeded with instances keyed on exactly these tag-sets, e.g.
`(x two-dimensional initial)`, `(x two-dimensional final
x-coverage-instantiator)`, `(x one-dimensional final x-coverage-instantiator)` —
`src/faces.c:2370-2430`.

**This is reachable from user Lisp today.** The in-tree
`docs/user-config/xft-chinese-font-configuration.el` sets the Chinese font with

```elisp
(set-face-font 'default "WenQuanYi Zen Hei Mono-18" 'global '(two-dimensional final))
```

i.e. a font instantiator tagged for 2D CJK charsets at the final stage. Because
`TAG-SET` accepts any tag defined with a charset predicate, a compat layer can
define **per-charset or per-script tags** (`(define-specifier-tag 'cn-hand
nil (lambda (cs) (memq cs '(chinese-gb2312 …))))`) and attach a font to exactly
those charsets — the finest granularity XEmacs supports is the individual
charset, not an arbitrary codepoint range.

### 3c. `set-language-environment` does *not* select fonts

`(set-language-environment LANGUAGE-NAME)` (`lisp/mule/mule-cmds.el:667-696`)
sets coding-system priority, the default input method, the locale, and charset
precedence; `finish-set-language-environment` (`lisp/mule/mule-cmds.el:698`)
handles input methods. Neither touches face fonts. There is no
language-keyed font lookup in redisplay: the only key is the **charset** (and
its derived class tags). Consequently there is no `set-language-environment` →
font mapping to lean on.

### 3d. No codepoint-range → font mapping

GNU `set-fontset-font` accepts a charset symbol, a **cons `(FROM . TO)` codepoint
range**, or a script symbol. XEmacs's matching key is the *charset* only; there
is no Lisp-visible "font for codepoints 0x4E00–0x9FFF" mechanism. Ranges must be
approximated by the charsets that cover them.

## 4. Font listing — XEmacs's `x-list-fonts`

There is **no `x-list-fonts`** (`fboundp = nil`; `(apropos-internal "x-list" …)`
empty). XEmacs's equivalents:

- `(font-list PATTERN &optional DEVICE MAXNUMBER)` — `src/fontcolor.c:1310`. On X
  it calls `x_font_list` (`src/fontcolor-x.c:878-900`), which is a direct wrapper
  over **`XListFonts`** — the same C call GNU's `x-list-fonts` uses.
- Fontconfig family listing: `fc-find-available-font-families`
  (`lisp/fontconfig.el:496`), `fc-font-match` (`lisp/font-mgr.el:197`),
  `fc-list-fonts-pattern-objects` (`lisp/font-mgr.el:208`, C DEFUN at
  `src/font-mgr.c:1054`), backed by `FcFontList` (`src/font-mgr.c:1076`).
- `x-font-families-for-device` (`lisp/font.el:619`) and
  `xft-font-families-for-device` (`lisp/font.el:812`) for UI use.

## 5. Per-call verdict

| GNU call (`cnfonts` use) | XEmacs equivalent | Verdict | Difficulty |
|---|---|---|---|
| `font-spec :name F :size N [:weight 'bold] [:slant 'italic]` (`cnfonts.el:509-541`) | `make-font :family/:size/:weight/:style` (`lisp/font.el:361`); or a plain Xft/XLFD name string built by `xft-font-create-name` (`lisp/font.el:831`) / the fontconfig `fc-*` names (`lisp/fontconfig.el:496`, `lisp/font-mgr.el:197`) | **mappable** — no `font-spec`; must lower keywords (`:slant` → style bitmask via `set-font-italic-p`, `lisp/font.el:191-235`) and `:name` → string | Low–medium |
| `set-face-attribute 'default nil :font SPEC` (`cnfonts.el:546`) | `(set-face-font 'default SPEC-INSTANTIATOR)` (`lisp/faces.el:504`); generic form `set-face-property` (`lisp/faces.el:261`) | **mappable** — no generic attribute dispatcher; only the `:font` keyword is used here | Low |
| `set-face-font 'bold/:italic/:bold-italic SPEC` (`cnfonts.el:550-572`) | `set-face-font` (same name) (`lisp/faces.el:504`); also accepts a 12-elem descriptor via `font-set-face-font` (`lisp/font.el:991`) | **direct equivalent** (FONT must be a string/instance; a GNU `font-spec` must first be lowered) | Low |
| `set-fontset-font "fontset-default" CHARSET SPEC [nil HOW-TO-ADD]` (`cnfonts.el:581-611`; charsets `kana han cjk-misc bopomofo hangul symbol phonetic`, `nil`=EXT-B prepend) | No fontset API. Map to `(set-face-font FACE SPEC 'global TAG-SET)` (`lisp/faces.el:504`) with a specifier tag-set whose charset predicate matches the target — built-in `two-dimensional` covers `han`/`kana`/`hangul`/`bopomofo`/`cjk-misc` (`src/faces.c:2050, 2334`); custom `(define-specifier-tag … CHARSET-PREDICATE)` (`src/specifier.c:1230`) for per-script/per-charset; matched in redisplay at `src/faces.c:1134` + `src/specifier.c:1008` | **mappable** — per-charset-class natively; per-individual-charset via custom tags; **no codepoint-range key** | Medium (the core of the backend) |
| `x-list-fonts FONT nil nil 1` (`cnfonts.el:464`) | `(font-list PATTERN)` (`src/fontcolor.c:1310`, `src/fontcolor-x.c:878` = `XListFonts`); `fc-list-fonts-pattern-objects` / `fc-find-available-font-families` (`lisp/fontconfig.el:496`) | **mappable** — same underlying `XListFonts`/`FcFontList`; returns name strings | Low |
| `face-attribute FACE ATTR [FRAME INHERIT]` | `face-property-instance` (`lisp/faces.el:187`), `face-property` (`lisp/faces.el:96`); for fonts `face-font` / `face-font-name` / `face-font-instance` (`lisp/faces.el:455/447/473`) | **mappable** — no generic dispatcher; needs an attribute→accessor table (cnfonts mainly reads back font/size) | Low–medium |
| `display-graphic-p [DISPLAY]` (`cnfonts.el:256, 705, 811`) | `display-graphic-p` (same name) (`lisp/frame.el:1226`), delegating to `device-on-window-system-p` | **direct equivalent** | None |

Empirical `fboundp` snapshot (build `./build/bin/xemacs -batch`):
`font-spec` ✗, `set-fontset-font` ✗, `set-face-attribute` ✗, `face-attribute` ✗,
`x-list-fonts` ✗, `fontsetp`/`get-fontset-info`/`get-font-info` ✗;
`set-face-font` ✓, `set-face-property` ✓, `face-property` ✓,
`face-property-instance` ✓, `font-list` ✓, `make-font-specifier` ✓,
`font-specifier-p` ✓, `font-instance-name` ✓, `make-font`/`font-create-name`/
`font-create-object` ✓ after `(require 'font)`, `set-language-environment` ✓,
`display-graphic-p` ✓, `define-specifier-tag` ✓, `add-spec-to-specifier` ✓.

## Summary / verdicts

### (a) Backend design sketch

1. **`font-spec` shim.** Implement `font-spec` as a lowerer, not a new type:
   accept `:name`/`:family`/`:size`/`:weight`/`:slant` and emit either (i) a plain
   XEmacs font-name string (`"Family-Size:bold:italic"` for Xft, or an XLFD) for
   direct use as a face-font instantiator, or (ii) a `make-font` descriptor
   (`lisp/font.el:361`) when structured access is needed. `:slant`/`:weight`
   keywords lower to the style bitmask / weight keyword XEmacs understands
   (`lisp/font.el:161-208`, `x-font-weight-mappings` at `lisp/font.el:118`).
2. **`set-face-attribute` shim.** Dispatch `:font` → `set-face-font`
   (`lisp/faces.el:504`); provide `face-attribute` by dispatching the handful of
   attributes cnfonts reads back to `face-font`/`face-font-name`/`face-height`
   (`lisp/faces.el:455/447/1687`).  Note there is no single generic attribute
   accessor to fall back on — only per-property accessors.
3. **`set-fontset-font` shim — the real work.** Treat the GNU *fontset* as a
   virtual object. Maintain a per-(face,charset-class) registry; on each call,
   translate `(NAME, TARGET, FONTSPEC, HOW-TO-ADD)` into
   `(set-face-font FACE lowered-fontspec 'global TAG-SET)`:
   - map script symbols `han`/`kana`/`hangul`/`bopomofo`/`cjk-misc` to the
     built-in `two-dimensional` tag (`src/faces.c:2050`);
   - for per-script distinction, `define-specifier-tag` once per script/charset
     group with a `CHARSET-PREDICATE` (`src/specifier.c:1230`) and reuse it;
   - `HOW-TO-ADD` `prepend`/`append` map to the `how-to-add` argument
     `prepend`/`append` (`src/specifier.c:2237`, `lisp/faces.el:504`);
   - target `nil` (cnfonts's EXT-B whole-range prepend, `cnfonts.el:594`) has no
     exact equivalent — approximate with a `two-dimensional`/`final`-stage
     fallback or a broad custom tag.
   Consider shadowing *only* `font-spec`, `set-face-attribute`, `face-attribute`,
   `x-list-fonts`, `set-fontset-font` and leaving cnfonts's own face-selection
   and profile logic running; the English-face path
   (`set-face-font 'bold …`, `cnfonts.el:550-572`) already works unchanged.

### (b) Per-charset feasibility verdict

**XEmacs *can* choose a font per charset — this is the make-or-break fact, and
it is positive.** The capability is real, native, and used by redisplay: the face
font specifier is resolved per charset through tag-sets carrying charset
predicates (`src/faces.c:1134`; `src/specifier.c:1008, 1230`; `src/fontcolor.c:1520`).
It is reachable from Lisp today — the repo's own Chinese Xft config already does
`(set-face-font 'default FONT 'global '(two-dimensional final))`.

**Granularity limit:** the match key is the **charset** (and its dimension
class), never an arbitrary codepoint range. XEmacs has no fontset object and no
Lisp fontset API (verified), so "cnfonts works" degrades to:

- **Full** for the 1D-vs-2D split (English vs CJK) — the exact thing cnfonts
  wants for the common case (monospace Latin + a CJK fallback), including bold/
  italic variants.
- **Good** for per-script/per-charset selection (`han`, `kana`, `hangul`, …) by
  defining custom specifier tags with charset predicates — because XEmacs
  charsets correspond closely to the scripts cnfonts names.
- **Approximate** only where cnfonts keys on Unicode codepoint ranges or on the
  fontset's ordering (`prepend`/`append` semantics across overlapping ranges,
  and the `nil`/EXT-B range): these cannot be expressed exactly and must collapse
  to charset-class tags, with the stage tags (`initial`/`final`) providing the
  fallback ordering.

So per-charset feasibility is **not** the blocker the map feared: a compat layer
mapping `set-fontset-font` → per-charset specifier tags is viable. The residual
risk is *fidelity of per-range and per-script ordering*, not the existence of
per-charset font selection.
