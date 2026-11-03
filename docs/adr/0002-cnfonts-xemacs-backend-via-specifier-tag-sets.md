# cnfonts-xemacs backend: shim seven GNU font calls onto specifier tag-sets

*Amended 2026-10-29 by [#82](https://github.com/fktpp2022/xemacs-git/issues/82): the shim surface is **seven** functions, not five, and the tag-set is the shim's **own** tag rather than the built-in `two-dimensional`/`final`.*

*Amended 2026-10-30 by [#95](https://github.com/fktpp2022/xemacs-git/issues/95), following [ADR 0003](0003-font-layer-one-descriptor-per-device-names.md): the built-in charset-class tags are now registered in **every** build, not only under `HAVE_X_WINDOWS || HAVE_GTK` (`src/faces.c:2283-2302`), so the availability argument below is superseded. The decision to register the layer's own tags stands: their names are the layer's contract, and a tag-set shared with `two-dimensional` would entangle cnfonts with any other configuration under that name.*

*Amended 2026-11-02 by [#114](https://github.com/fktpp2022/xemacs-git/issues/114): the shim's dimension-2 tag now excludes JIT charsets, so `cnfonts-jit-charset` is the only tag-level font for `jit-ucs-charset-0` and the Chinese font is no longer its fallback. The "Per-codepoint fidelity" paragraph, the EXT-B consequence below, and the "Cutting per-codepoint EXT-B out of scope" option are rewritten accordingly.*

*Amended 2026-11-02: `symbol`, `phonetic`, and the ornament codepoint-range requests are no longer dropped; they lower into a codepoint override map on Xft ([ADR 0004](0004-cnfonts-codepoint-overrides.md)). The last ceiling bullet and the "approximate lowering" sentence are updated.*

*Amended 2026-11-03 by [#104](https://github.com/fktpp2022/xemacs-git/issues/104), following the inventory in `docs/research/cnfonts-interactive-surface.md` -- on the `research/cnfonts-interactive-surface` branch, which wayfinder research tickets keep unmerged, so read it there (`git show research/cnfonts-interactive-surface:docs/research/cnfonts-interactive-surface.md`): the layer now also answers the GNU spellings `cnfonts-ui` needs to open and navigate the profile editor (see "Editor surface"). The rejection of widening below still holds where it was aimed: no cnfonts internal -- profile loading, face selection, its own `set-face-font` calls -- is shadowed.*

*Amended 2026-11-03 by [#118](https://github.com/fktpp2022/xemacs-git/issues/118), following [#117](https://github.com/fktpp2022/xemacs-git/issues/117): `posn-at-point` no longer answers nil for a position outside the window's displayed range; it estimates from the `window-start` the last redisplay left, and nothing in the layer forces a redisplay. That is what lets the align page's pointer warp fire after the page rebuilds under the pointer. The "Approximate" bullet below is rewritten accordingly; the approximation itself, and the manual's caveat about it, stand.*

XEmacs has no Lisp fontset API and no `font-spec`/`set-face-attribute`/`x-list-fonts`/`set-fontset-font`/`list-fonts`/`font-family-list`/`string-join`; it selects a font **per charset** through face-font specifier tag-sets carrying charset predicates (`src/specifier.c:1230`, matched at `src/faces.c:1134`). The `cnfonts-xemacs` compat layer overrides **exactly those seven functions** and leaves every other cnfonts internal (profile loading, face selection, the English path that already works via `set-face-font 'bold …`) running untouched. `face-attribute` is **not** among them: cnfonts.el never calls it — only `cnfonts-ui.el` does, which this map puts out of scope — so it is added back when that scope changes.

`set-fontset-font` lowers to `(set-face-font FACE FONT 'global TAG-SET)`, with the shim registering **its own** specifier tags from Lisp rather than using XEmacs's built-in charset-class tags: one for dimension-2 charsets (the CJK font) and one for the JIT charset only (the EXT-B font). The built-in `two-dimensional`/`one-dimensional`/`initial`/`final`/`jit-charset` tags are registered only inside `#if defined (HAVE_X_WINDOWS) || defined (HAVE_GTK)` (`src/faces.c:2286…2453`), so a TTY-only build has none of them and `(set-face-font … '(two-dimensional final))` signals *Invalid tag set*; a shim that used them would work in a GUI build and fail in the headless build the milestone's gate runs on. The shim's own tags are registered with `define-specifier-tag`, whose charset predicate is re-evaluated for every existing charset (`src/specifier.c:1195…1225`), so they are live rather than merely accepted by `set-face-font`.

Per-codepoint fidelity is the one accepted approximation: cnfonts's `(set-fontset-font "fontset-default" nil extb-fontspec nil 'prepend)` (EXT-B, `cnfonts.el:594`) has no XEmacs equivalent — no codepoint-range key, no fontset default bucket — so it collapses to the JIT-charset bucket. That bucket is now the only tag-level font for `jit-ucs-charset-0`: the shim's dimension-2 tag excludes JIT charsets, so the Chinese font is no longer its fallback. Per-character fallback in redisplay supplies the glyphs the EXT-B font lacks, and codepoints that map to a real 2D charset (e.g. U+20109 → `chinese-cns11643-7`) still get the Chinese font through the dimension-2 tag.

## Editor surface

`cnfonts-edit-profile` requires `cnfonts-ui`, whose load-time and navigation
calls need eleven more GNU spellings. The layer answers them, same rule as the
font half: upstream files stay byte-identical.

- **Faithful**: `make-composed-keymap` (a sparse map with MAPS then PARENT as
  its ordered `set-keymap-parents`, which is how XEmacs searches), `widget-at`
  (GNU's optional POS, delegated to the core function), `char-to-string` (the
  core function behind a raw-codepoint branch), and
  `custom--initialize-widget-variables` (the faces and prefixes XEmacs's own
  `custom-mode` sets).
- **Exact only at its call site**: `face-valid-attribute-values` answers nil,
  because `cnfonts-ui` only compares the answer with
  `(("unspecified" . unspecified))` to choose raised buttons, and XEmacs has
  no `:box` attribute to list values for.
- **Approximate**: `widget-move`, replaced by GNU's character-wise scan so the
  start page's tab stop at `point-min` is reachable -- a shared function, so
  this is the widest blast radius in the layer and every widget buffer gets
  GNU's navigation. Accepted (decided in #102) on two conditions: what counts
  as tabable stays XEmacs's own rule (`:tab-order` sequences and inactive
  widgets are still passed over), and the replacement is named in the user
  manual. The alternative -- a layer-local navigation command bound in
  `cnfonts-ui-mode-map` -- keeps core intact but copies `cnfonts-ui-forward`'s
  loop, which would have to be re-audited against every cnfonts release.
- **Approximate**: `posn-at-point`, `posn-x-y`, `default-font-width`,
  `default-font-height` and `set-mouse-pixel-position`, which compute
  window-relative pixels from the default face's metrics (XEmacs counts from
  the window's text area, not from a frame, and the answer comes from the
  display's current state -- no redisplay is forced, so a position the display
  has not reached gets an estimate that can fall outside its window; decided in
  #117 after the align page's warp was found to be vetoed by a displayed-range
  guard reading the `window-end` the page's own `erase-buffer` left behind).
  XEmacs has no `frame-char-width`, `window-font-width` or equivalent, so the
  face's font metrics are the only pixel route there is; the align page's warp
  is honest for a monospace default font in a full-width window and the manual
  says so.
- **Not shimmed**: `after-make-frame-functions` (no frame-creation hook in
  XEmacs, so frames made after `cnfonts-enable` keep the old fonts) and
  `face-font-rescale-alist` (the option's effect is per-charset size
  arithmetic, not a variable). The mouse-wheel and touch-pinch commands already
  degrade to cnfonts's own message through its `functionp` guards, and that is
  the accepted contract (#102): XEmacs's `mwheel.el` is not wired, because the
  destination says a command with no XEmacs equivalent degrades loudly rather
  than getting an invented backend.
- **Outside the layer**: `lisp/wid-edit.el` takes a push button's GUI image
  branch only when the console provides a `button` image instantiator. A
  `--with-widgets=no` build registers none, so every cnfonts button warned and
  drew nothing; that is a core bug, not a cnfonts shim.
  **The scope rule is about upstream cnfonts, not about XEmacs's own Lisp**
  (decided in #102): fixing a core bug that reaches every widget user belongs
  in core, and the layer stays the only place a cnfonts gap is answered. The
  alternative -- setting `widget-push-button-gui` from the layer -- would have
  made cnfonts's editor look right while leaving the bug for Custom, Calc and
  every other button.

## Considered options

- **Per-script custom tags** (`define-specifier-tag` with a charset predicate per script, distinguishing han / kana / hangul / bopomofo / cjk-misc): deferred, not rejected. The cost is now known to be small — XEmacs already ships the grouping (`charset-tag-to-charset-list` over `chinese/language`, `japanese-kanji/list`, `korean/language`, `katakana/script`, `hangul/script`, `bopomofo/script`), so each tag is one line — and the fidelity it buys is real but not yet needed: the milestone's success test is Latin-plus-CJK, and the coarse tag renders that correctly. The trigger to upgrade is a rendering check that shows half-width katakana or Zhuyin coming out wrong, since those two scripts live in dimension-1 charsets the coarse tag cannot match.
- **Use the built-in `two-dimensional`/`final` tags where they exist**: rejected. It means two code paths — a GUI build and a non-GUI build behave differently — and the non-GUI path is exactly where the milestone's gate runs.
- **Define the CJK font at the `final` stage only**: not expressible. `define-specifier-tag` requires a one-argument charset predicate and the stage predicates take two (`src/specifier.c:1262…1272`, "We only allow the stage argument to be specified from C"), so a stage tag cannot be registered from Lisp. A one-argument predicate is evaluated for both stages (`src/specifier.c:1085…1115`), which means the CJK font becomes an *initial*-stage match.
- **Widen the shim** beyond the seven (e.g. shadow cnfonts's face-selection or profile logic): rejected. Every shadowed function is a divergence from upstream that must be re-audited on each cnfonts release; the measured call surface is seven, so seven is the whole surface.
- **Native reimplementation of cnfonts (β)** — the map's escape hatch if per-charset mapping proved infeasible: not taken. Per-charset selection is native and reachable from user Lisp, so upstream cnfonts can run behind a thin shim.
- **Cutting per-codepoint EXT-B out of scope**: rejected. Declaring it out of scope would delete a fallback that mostly works. Ext-B codepoints land in the 2D JIT charset `jit-ucs-charset-0`, and the JIT-bucket entry is now that charset's only tag-level font (the shim's dimension-2 tag excludes JIT charsets), so they render with the EXT-B font; per-character fallback in redisplay supplies any glyph it lacks. This is still not a codepoint-pinned extb font, and codepoints with a real 2D charset keep the Chinese font.

## Consequences

- The CJK font is selected at the **initial** stage, so it renders in its native charset encoding instead of through the `iso10646-1` translation the `final` stage implies. A divergence from GNU, in the CJK font's favour.
- Known ceilings, to be marked in code:
  - **EXT-B is a JIT-bucket entry, not a codepoint-pinned default.** The JIT charset is excluded from the dimension-2 tag, so the EXT-B font is its only tag-level font; it is prepended, so it wins the JIT charset when it covers it, and per-character fallback in redisplay supplies what it lacks. The scope is broader than GNU's (any unencodable 2D codepoint falls to ext-B, not just Ext-B). Codepoints that map to a real 2D charset (e.g. U+20109 → `chinese-cns11643-7`) still get the Chinese font.
  - **`kana` and `bopomofo` do not get the CJK font.** Their XEmacs charsets (`katakana-jisx0201`, `chinese-sisheng`) are dimension 1, so a dimension-2 tag never matches them; half-width katakana and Zhuyin fall to the Latin font. This is the gap per-script tags would close.
  - **The coarse tag also offers the CJK font to non-CJK 2D charsets** (`indian-1-column`, `indian-2-column`, `ethiopic`, `tibetan`, `tibetan-1-column`); the per-charset coverage check usually rejects the font, so this is a latent rather than an observed effect.
  - **cnfonts's separate `cjk-misc` bucket collapses into the 2D tag**, which is where its font already sits; its `symbol` and `phonetic` requests and its four ornament codepoint-range requests become **codepoint overrides** on Xft ([ADR 0004](0004-cnfonts-codepoint-overrides.md)) — XEmacs has no `symbol` charset, no `phonetic` charset (its `ipa` definition is commented out at `lisp/mule/misc-lang.el:34`), and no range key.
  - Under the single coarse tag, cnfonts's `'hangul … 'append` fallback (`cnfonts.el:590`) is a no-op: a Chinese font lacking Korean glyphs has no per-script escape hatch. Same trigger as the per-script-tags upgrade above.
- The shim's answers must be real, not shaped to pass: **load-bearing** calls (`font-spec`, `list-fonts`, `x-list-fonts`, `font-family-list`) are consulted by cnfonts to choose fonts, so they must answer with XEmacs data; **sink** calls (`set-face-attribute`, `set-fontset-font`) must actually lower, and where a lowering is approximate — a codepoint override the device cannot apply, a request with no tag — they must say so rather than succeed silently.
- `HOW-TO-ADD` maps cleanly: `prepend`/`append` pass through to `set-face-font`'s `how-to-add` argument (`src/specifier.c:2237`).
- The shim is re-specified against the fixed font layer (decided in [#88](https://github.com/fktpp2022/xemacs-git/issues/88), recorded in [ADR 0003](0003-font-layer-one-descriptor-per-device-names.md)): `font-spec` returns the **12-element descriptor**, which cnfonts hands straight to XEmacs's own `set-face-font` (`cnfonts.el:550`, `:559`, `:569`); `x-list-fonts` returns a **synthetic XLFD** (`-*-FAMILY-*-*-*-*-*-*-*-*-*-*-*-*`), because cnfonts parses the lister's answer as `(cl-subseq lst 2 (- (length lst) 12))` (`cnfonts.el:464-470`) and a raw fontconfig name makes that **signal `Args out of range`** — cnfonts errors out rather than misparses; existence is decided through the descriptor route (lower with `font-create-name`, probe with `try-font-name`), so the platform's own matcher decides; `font-family-list` answers with the layer's new device-neutral family list.
- The milestone's success test is a **GUI** gate, not a headless one (decided in [#82](https://github.com/fktpp2022/xemacs-git/issues/83)): it runs upstream cnfonts through its own public `cnfonts-set-font` path against a GUI-enabled build of this tree, with no faked display checks. It asserts that the CJK font sits under the dimension-2 tag and the EXT-B font under the JIT-charset tag, that the CJK tag's charset predicate fires for the Chinese charsets and not for ASCII, and that the codepoint override map carries the symbol and ornament families and sizes and updates them in place when an edited profile is re-applied ([ADR 0004](0004-cnfonts-codepoint-overrides.md)). The gate stubs the platform's font listing with fixture font names, so it needs no CJK fonts installed and asserts configuration rather than rendering; whether CJK text actually renders aligned, and the symbol and ornament rows at the profile's sizes, stays a manual GUI check.
