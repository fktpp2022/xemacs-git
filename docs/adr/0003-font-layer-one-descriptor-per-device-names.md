# The font layer: one descriptor, per-device names, an honest probe

*Wayfinder map [#84](https://github.com/fktpp2022/xemacs-git/issues/84); decided in [#86](https://github.com/fktpp2022/xemacs-git/issues/86) and [#88](https://github.com/fktpp2022/xemacs-git/issues/88). Research: `docs/research/xemacs-font-layer-change-set.md` on the throwaway branch `research/xemacs-font-layer`.*

XEmacs has no device-independent spelling for a font. `set-face-font` takes "an instantiator … a common instantiator is a **platform-dependent string** naming the font" (`lisp/faces.el:504-523`), and the string differs per backend: an XLFD on core X, a fontconfig name under Xft, `Family:Style:Size` on MS Windows (`lisp/fontcolor.el:60-67`). Every layer above the font code must therefore know which device it is talking to before it can name a font at all.

The device-independent spelling already exists — the 12-element descriptor built by `make-font` (`lisp/font.el:361-377`) — but nothing accepts it: `font-set-face-font` (`lisp/font.el:991-1031`), the only function that takes a descriptor, has no runtime callers and a malformed autoload cookie (`;;; ###autoload`, `lisp/font.el:990`), so it is not even defined. This ADR decides to wire that route up, and to fix the two places where the Xft build is served the legacy X11 behaviour.

## Considered options

- **Keep the name string as the only spelling, and let each consumer dispatch per device.** Rejected: it is the status quo, and it is what forces the cnfonts shim to carry per-backend code.
- **Define the missing `fc` row's functions** (`fc-font-create-name`, `fc-font-create-object`) and make `fc` a device type. Rejected: `fc` is not a console type (`src/console.c:238-244`); Xft is a compile-time sub-mode of the `x` device, not a device of its own.
- **Dispatch on the device rather than the build.** Rejected: on an Xft build *every* `x` device ends up holding an `XftFont` (`src/fontcolor-x.c:236-259`), so a per-device distinction has nothing to distinguish. The tree's existing convention — `(featurep 'xft-fonts')`, used by `x-font-create-object` (`lisp/font.el:566-578`) and all six X frobbers — is used instead.
- **Keep the XLFD as the one name format for the `x` device.** Rejected: `XftFontOpenXlfd` succeeds only if the **core X server** has the font, so on a modern system the family is silently replaced by `monospace` (`lwlib/lwlib-fonts.c:88`, `:94-95`).
- **Teach `try-font-name` to distinguish "the font I asked for" from "the font I got", rather than fixing its callers.** Chosen: `xft_open_font_by_name` never returns NULL, so the probe answers "yes" to *any* string on an Xft build; fixing the probe once corrects every caller (`x-font-create-name`'s candidate loop, `x-make-font-bold-core`, the MS Windows producers) instead of patching them one by one.
- **Leave the built-in charset-class tags inside the GUI-only block.** Rejected: they are part of the per-charset mechanism, and leaving them out makes a TTY build's font behaviour differ from a GUI build's for no reason — a difference this repo already ships as a silent no-op (`docs/user-config/xft-chinese-font-configuration.el` sets the Chinese font under `'(two-dimensional final)` inside a `condition-case`, so it quietly does nothing there).

## Consequences

- `set-face-font` accepts the 12-element descriptor, and `font-set-face-font` becomes reachable (the autoload cookie is fixed). The descriptor's `:weight` is a keyword and `:style` a bitmask (`lisp/font.el:190-235`), the same vocabulary the existing producers already read.
- `font-create-name` dispatches on `(featurep 'xft-fonts)`: on Xft it produces the **fontconfig name** that `fc-name-unparse` yields, and `xft-font-create-name` (`lisp/font.el:831-837`) is completed to carry the family list shape, the size-string shape (`font-spatial-to-canonical`, `lisp/font.el:294-355`), the weight and the slant. The round-trip — `(font-family (font-create-object (font-create-name f)))` returns the input family, and likewise for weight and slant — is the property that makes the change safe, and it is the central assertion of the test plan.
- `try-font-name` answers via fontconfig under Xft (`fc-try-font`, `lisp/fontconfig.el:478-488`, which the `-xft` frobbers already use).
- Fontconfig hands weight and slant constants back as **floats** from a matched pattern (`200.0`, not `200`), so `fc-font-weight-translate-from-constant` and `fc-font-slant-translate-from-constant` truncate before the symbol lookup. Without that, the object side of the round-trip silently drops the weight.
- `font-list` returns **fontconfig names on Xft builds** and unchanged XLFDs on legacy X11. The compatibility risk is low and mislocated: the only two in-tree consumers that parse its output as XLFDs — `x-reset-device-font-menus-core` (`lisp/x-font-menu.el:159-201`) and `x-available-font-sizes` (`lisp/x-faces.el:403-438`) — are already unreachable on Xft, because their callers branch on `(featurep 'xft-fonts)` first. The one visible change is `set-frame-font` completion (`lisp/frame.el:989-993`), which starts offering the names the system actually has; a user config naming a core-X font will no longer see it offered. That is the intended direction, documented rather than shimmed.
- A **device-neutral `font-family-list`** is added. The layer already knows each backend's name format — it is what `font-create-name` produces — so it derives families centrally (XLFD → field 2; fontconfig → strip the size; MS Windows → field 1). The two existing family listers stay as they are: `x-font-families-for-device` and `xft-font-families-for-device` are stubs marked *"clearly bogus for Xft"* (`lisp/font.el:812-830`) that fall back to a hard-coded alias list, and `fc-find-available-font-families` is fontconfig-only.
- The built-in charset-class specifier tags (`two-dimensional`, `one-dimensional`, `initial`, `final`, `jit-charset`) are registered in **every** build, not only under `HAVE_X_WINDOWS || HAVE_GTK` (`src/faces.c:2286-2453`).
- The mapping table's dead rows go: `fc` and `pm` are deleted (their functions are undefined; neither is a console type), and the missing `msprinter` row is added, so `msprinter` gets the same answers as `mswindows` — matching the C layer, which already gives it all the font methods (`src/fontcolor-msw.c:1664-1670`).
- The change set is **additive per backend**: a core-X build keeps its XLFD answer for `font-list`, keeps `try-font-name`'s real existence semantics, and keeps every existing behaviour. Only Xft builds change, and there the change is toward the modern spelling.
- `x-font-create-object`'s Xft branch is guarded by the **device** being an `x` device, not by `(featurep 'xft-fonts)`. That feature is build-wide and provided by the `x` console (`src/fontcolor-x.c:965-967`), so in a build with both an Xft `x` console and a GTK console, a GTK device with a non-XLFD name would reach `xft-font-create-object` → `fc-font-match`, which **errors on a non-`x` device** (`src/font-mgr.c:1024`). The descriptor route must not hard-error off the `x` device.
- **GTK is out of scope for this ADR** ([#87](https://github.com/fktpp2022/xemacs-git/issues/87)), and carries a fresh map of its own: [Map: GTK font support in XEmacs (Pango)](https://github.com/fktpp2022/xemacs-git/issues/90). The reasons are worth recording, because they are properties of the tree rather than of taste: GTK **cannot be built in this repository at all** — `XEMACS_WITH_GTK=ON` only prints a message (`CMakeLists.txt:372-374`), nothing in the CMake tree defines `HAVE_GTK` or finds the libraries, so `src/CMakeLists.txt:353` never fires, and only the autoconf path defines it (`configure.ac:2894-2917`); GTK's per-charset support is **structurally absent**, not merely missing (`gtk_find_charset_font` returns the font unchanged, `src/fontcolor-gtk.c:449-456`, and the charset matcher is XLFD-shaped, `src/fontcolor-xlike-inc.c:122-215`); and GTK's naming fixes — its `gtk` row, the commented-out frobbers (`lisp/gtk-faces.el:265-268`), the font menu's XLFD-shaped parsing of Pango descriptions (`lisp/gtk-font-menu.el:101-122`) — are separable from the Xft change. Until that map lands, the cnfonts shim's GTK behaviour is the documented contract: **warn, and set the CJK font as the face's plain font** — not the Latin one, because with no charset fallback on GTK the Latin font would render Chinese as boxes.
- Known ceiling: this is a font-*name* layer. Per-charset selection still needs the tag mechanism, and on GTK it still cannot work — Pango has no charset concept, so `gtk_find_charset_font` remains a no-op (`src/fontcolor-gtk.c:449-456`) and the charset matcher remains XLFD-shaped (`src/fontcolor-xlike-inc.c:122-215`). GTK is a separate project, tracked in [#87](https://github.com/fktpp2022/xemacs-git/issues/87).

## Verification

Decided in [#89](https://github.com/fktpp2022/xemacs-git/issues/89). **`-batch` yields a `stream` device even in a GUI build**, and the ctest harness runs `-batch` (`tests/CMakeLists.txt`), so device-level font behaviour can never be exercised by `ctest` in any build. Verification therefore has two halves.

### Device-free: `tests/automated/font-tests.el`

Run by `ctest`, and listed in its `FAIL_REGULAR_EXPRESSION` set — a test not on that list cannot fail the suite (the harness exits 0 on assertion failures). It asserts the properties that hold with no device at all:

1. `font-set-face-font` is `fboundp` **before `font.el` is loaded** — the `;;;###autoload` cookie is a one-character regression risk whose only symptom is an unreachable descriptor route.
2. `font-window-system-mappings` is exactly `(x gtk mswindows msprinter tty)` and every named function is `fboundp` — the `fc`/`pm` deletion and the `msprinter` addition.
3. The descriptor's accessors round-trip.
4. `set-face-font` accepts a 12-element descriptor and records it as the `font-specification`.
5. A 1-element vector is still the inheritance form, not a descriptor.
6. `font-create-name` answers `nil` for a device type with no row.

### Device-level: `tests/font-layer-gui.el`

A standalone script, run against a GUI build (`xemacs -l tests/font-layer-gui.el`); it prints `PASS`/`FAIL`/`SKIP` lines and exits non-zero on failure. It is separate from the cnfonts gate, which **keeps its `font-list` stub**: the bare script owns the real lister, so the stub cannot mask the change this ADR makes.

On an **Xft** build it asserts: `font-list` returns a non-empty list with no XLFDs, and the listed names load; `font-family-list` returns families; the descriptor → name → object round-trip holds for family, weight, slant and a size given as `"14pt"` (the weight check skips when the probe family has no bold variant; the slant check tries a known italic family before skipping); `try-font-name` returns `nil` for a font that does not exist; `make-face-family` changes the rendered font; and per-charset selection still routes through a tag-set.

On a **core-X** build the same script is the compatibility gate: `font-list` still returns XLFDs and `try-font-name` is still a real existence check.

### Manual: MS Windows

The Windows path changes in this change set (`mswindows-font-create-name`'s probe, the new `msprinter` row), so it is not left unverified. Run the script's assertions by hand on a Windows build and record the result:

- [ ] `(font-create-name (make-font :family "Courier New" :size 14) device)` produces a `Family:Style:Size:Effects:Charset` name.
- [ ] The round-trip `(font-weight (font-create-object (font-create-name font device) device))` returns the input weight, `:bold` for a bold descriptor, and likewise for italic and for a `"14pt"` size.
- [ ] `(font-list "*" device)` returns Windows name strings, and `(font-family-list device)` returns their first fields.
- [ ] `(try-font-name "definitely-not-a-font" device)` returns `nil` — the probe is a real existence check on Windows.
- [ ] On an `msprinter` device, `font-create-object` and `font-create-name` return non-nil (they returned `nil` before the new row), and `make-face-family` / `make-face-size` / `make-face-height` do not signal.
- [ ] `make-face-family` / `make-face-size` / `make-face-height` change the rendered font.

GTK has no section: it is out of scope here and carries its own map.
