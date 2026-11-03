# cnfonts's symbol, phonetic, and ornament requests become codepoint overrides

XEmacs selects a font per charset, and a specifier tag can only match charsets ([ADR 0002](0002-cnfonts-xemacs-backend-via-specifier-tag-sets.md)). Three of cnfonts's font requests have no charset to select: the GNU scripts `symbol` and `phonetic`, and its four ornament codepoint ranges. The shim used to drop them with a warning, so the align page's symbol and ornament glyphs fell to per-character fallback — which picks whatever installed font covers the codepoint, at the size of the charset's own font, not the profile's symbol or ornament size. Editing those sizes in the profile changed nothing on screen: the reported symptom.

## Decision

`set-fontset-font` records these requests in an ordered map, `xft-font-overrides`, as `(RANGE FAMILY . POINTS)` entries. The Xft per-character fallback (`src/redisplay-x.c`) consults the map *before* the charset font's coverage: for a covered codepoint it builds the search from the override family and points instead of the face family and size, so the override wins even where the charset's font has a glyph. Anything the named family lacks is drawn by any installed font that covers it, at the same points. Later ranges win, so cnfonts's ornament requests (made after `symbol`) take precedence over the symbol ranges where they overlap. The lookup and the map are exposed to Lisp as `xft-font-override`, and the map itself as the variable `xft-font-overrides`, so both the gates and users can inspect it.

The mapped ranges are cnfonts's own, narrowed to what its align page exercises:

- `symbol`: Greek and Coptic (U+0370–U+03FF), IPA Extensions (U+0250–U+02AF), Phonetic Extensions (U+1D00–U+1DBF).
- `phonetic`: the IPA and Phonetic Extensions ranges.
- ornaments: cnfonts's four ranges, U+2776–U+2793, U+24B6–U+24FE, U+2295–U+22A1, U+2190–U+2200.

## Considered options

- **Keep dropping the requests.** Rejected: this is the bug. Symbol and ornament glyphs then follow the fallback font's size, and no profile edit reaches them.
- **Materialise GNU's `symbol` script as ranges.** Rejected: GNU derives scripts from Unicode blocks (`admin/unidata/blocks.awk`), so its `symbol` script also claims punctuation, arrows, mathematical operators, and enclosed alphanumerics. Importing that turns ordinary prose punctuation — quotes, dashes, `→`, `①` — into the symbol font, which browsers and typesetting systems deliberately avoid: Unicode's `Common` characters inherit the primary text family, and only an author's explicit range (`@font-face unicode-range`, InDesign composite fonts, fontspec `ucharclasses`) moves them. The layer therefore maps the scripts the align page exercises rather than the block union.
- **Give the ranges a specifier tag.** Not expressible: a specifier tag's predicate takes a charset, never a codepoint, and adding a codepoint key to the tag mechanism is a font-layer change far larger than the need.
- **Pin the symbols to fontconfig `charset` patterns instead of a family.** Rejected: the profile names a family and a size; resolving the family through fontconfig's own search is the same route the face font takes, and it keeps the fallback (any covering font at the same size) in one place.

## Consequences

- The fallback cache is keyed by display, codepoint, the face font's pixel size, and the override's family and points. Re-applying an edited profile therefore reads new cache entries and re-renders at the new size without a restart; stale entries age out at the cache bound or when the display is closed. The cache entry owns a copy of the family string, because `FcPatternAddString` keeps the pointer ([`fc_intern`](../../src/font-mgr.c) comment); the copy is freed with the entry.
- The overrides are **Xft-only**. A GTK device has no per-character fallback (Pango, [ADR 0003](0003-font-layer-one-descriptor-per-device-names.md)), so the shim warns and changes nothing there; a core-X device has no Xft fallback either, and the map is inert. This is the device the shim already treats approximately, not a new divergence.
- A codepoint inside a mapped range that the named family cannot draw is rendered by any covering font at the *override* size. This is the intended meaning of a unicode-range rule: the size belongs to the range, not to the font that happened to supply the glyph.
- The `symbol` range does not claim every codepoint GNU's `symbol` script does. In particular `√` (U+221A) keeps the text font, while `→` (U+2190) is claimed by cnfonts's own ornament range. That asymmetry is the difference between the explicit ranges and GNU's block union, and it is recorded here rather than "fixed" by widening the map.
- The GUI gates assert the map's family and size for both rows, and that re-applying an edited profile updates them in place (`tests/cnfonts-xemacs-gui.el`); the Xft gate asserts the same through the primitive and that the requests create no face instantiator (`tests/xft-rendering-gui.el`). The rendered pixels remain a manual check.
