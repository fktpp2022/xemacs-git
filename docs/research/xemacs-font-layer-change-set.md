# XEmacs font layer: the change set, its callers, and its risks

Ticket: #85 (wayfinder map #84). Branch: `research/xemacs-font-layer`
(base: `origin/xemacs-cmake` @ `4271de16d`). Date: 2026-10-29.

This is decision-support research, not a shipping proposal. It answers the
ticket's question — *what exactly must change in XEmacs's font layer to give
one device-independent spelling for a font, and what does each piece break?* —
with `file:line` citations against this tree.

Everything below was read out of the tree. Where a claim is about runtime
behaviour it was additionally confirmed against the working build
`./build/bin/xemacs` (21.5 beta36, Mule); that build is TTY-only, so the
runtime confirmation is limited to symbol exposure, device types and data
shapes, and the Xft claims are derived from source, not from execution. The
limits of that are spelled out in §9.

## Method

1. **Primary source reading** of the font layer: `lisp/font.el`,
   `lisp/fontcolor.el`, `lisp/faces.el`, `lisp/x-faces.el`, `lisp/gtk-faces.el`,
   `lisp/msw-faces.el`, `lisp/x-font-menu.el`, `lisp/gtk-font-menu.el`,
   `lisp/msw-font-menu.el`, `lisp/font-menu.el`, `lisp/fontconfig.el`,
   `lisp/font-mgr.el`, `lisp/cus-face.el`, `lisp/frame.el`;
   `src/fontcolor.c`, `src/fontcolor-x.c`, `src/fontcolor-gtk.c`,
   `src/fontcolor-msw.c`, `src/fontcolor-tty.c`, `src/fontcolor-xlike-inc.c`,
   `src/font-mgr.c`, `src/faces.c`, `src/specifier.c`, `src/console-*.c`,
   `src/console-xlike-inc.h`; `lwlib/lwlib-fonts.c`.
2. **Caller sweep** by exhaustive grep over `lisp/`, `src/`, `modules/`,
   `docs/user-config/`, `man/` for `font-list`, `font-create-name`,
   `font-create-object`, `set-face-font`, `font-set-face-font`,
   `try-font-name`, `make-font`, `make-font-specifier`.
3. **Empirical confirmation** against the TTY-only build for the claims that
   the TTY build can settle (device types, `font-list` on a stream device,
   autoload status of the font functions).
4. **Prior art in this repo**: `docs/user-config/xft-chinese-font-configuration.el`
   is a working per-charset font configuration on an Xft build and is used
   here as an independent witness to what "works" means in practice.

The facts handed down with the ticket were re-verified, not re-derived; §7.4
and §8 record the two places where verification *corrected* the handover.

---

## 1. The shape of the problem

### 1.1 There is no device-independent spelling today — only four of them

`make-font-specifier` documents the instantiators a font specifier may take
(`lisp/fontcolor.el:53-73`). The string form is explicitly *platform
dependent* (`lisp/fontcolor.el:60-67`):

| backend | spelling of "14pt upright medium-weight Courier" |
|---|---|
| X11 core | `-*-courier-medium-r-*-*-*-140-*-*-*-*-iso8859-*` |
| Xft / GTK2 | `Courier-14` |
| MS Windows | `Courier:14:Western` |

`set-face-font` accepts "an instantiator … a common instantiator is a
platform-dependent string naming the font" (`lisp/faces.el:504-523`). So the
*only* thing the public face API takes is a per-device name string. Every
higher layer in the tree therefore has to know which device it is talking to
before it can spell a font at all.

### 1.2 The device-independent spelling already exists — it is just not wired up

`make-font` builds a flat 12-element descriptor
(`lisp/font.el:361-377`):

```
[:family (:family) :weight :style :size :registry :encoding]
```

with accessors `font-family`/`font-weight`/`font-style`/`font-size`/
`font-registry`/`font-encoding` (`lisp/font.el:172-188`) and a style bitmask
(`lisp/font.el:190-235`). This is the tree's own device-independent spelling
of a font, and it is already what the *lowering* layer is built around:

* `font-create-name` (`lisp/font.el:379-383`) takes a descriptor and produces a
  device-appropriate **name**;
* `font-create-object` (`lisp/font.el:386-390`) takes a **name** and produces a
  descriptor.

Both dispatch through `font-window-system-mappings` (`lisp/font.el:108-120`),
which maps a *device type* to the two functions.

### 1.3 The gap: the descriptor is accepted by a function nobody calls

`font-set-face-font` (`lisp/font.el:991-1031`) is the only function in the tree
that accepts the 12-element descriptor as a face font:

* it recognises `(vectorp font)` and `(= (length font) 12)`
  (`lisp/font.el:993`);
* it lowers it with `font-create-name` (`lisp/font.el:994`);
* it stores the descriptor under the face property `font-specification`
  (`lisp/font.el:995`), which is what `font-update-device-fonts`
  (`lisp/font.el:1037-1049`) and `font-update-one-face`
  (`lisp/font.el:1051-1070`) replay onto new devices;
* it applies the style bits that a bare name cannot carry — underline
  (`lisp/font.el:1007`), smallcaps/bigcaps display table
  (`lisp/font.el:1008-1011`), strikethru (`lisp/font.el:1012-1015`).

But `font-set-face-font` has **no runtime callers**: the only references in the
tree are its own definition and two commented-out blocks in `lisp/cus-face.el`
(`lisp/cus-face.el:285`, `lisp/cus-face.el:311`). It is not an alias of
`set-face-font`, and nothing calls it.

There is also a mechanical reason it cannot be reached: its autoload cookie is
**malformed**. `lisp/font.el:990` reads `;;; ###autoload` — with a space —
whereas every other cookie in the file is `;;;###autoload`
(`lisp/font.el:385`, `:439`, `:446`, `:454`, `:458`, `:462`, `:466`, `:709`).
Confirmed against the build: `(fboundp 'font-set-face-font)` → `nil`,
`(fboundp 'font-create-object)` → `t`. So even a caller that wanted it would
have to `(require 'font)` first, and `font.el` is not dumped
(`lisp/cus-face.el:231-233` says so explicitly).

### 1.4 The second gap: the `x` row cannot distinguish core X from Xft

`font-window-system-mappings` keys on `(device-type device)`
(`lisp/font.el:381-382`, `lisp/font.el:388-389`). But Xft is not a device
type — it is a *compile-time sub-mode* of the `x` device
(`src/console-xlike-inc.h:103-115`):

```c
#if defined (THIS_IS_X) && defined (HAVE_XFT)
#define XEMACS_USE_XFT
```

So the `x` row (`lisp/font.el:109`) points at `x-font-create-name`
(`lisp/font.el:636`), which emits an **XLFD** — the core-X spelling — even on
an Xft build, where every font instance is in fact an Xft/fontconfig font
(§7.1). That mismatch is the root of most of what follows.

---

## 2. The change set, function by function

### 2.1 `set-face-font` — accept the descriptor

**Becomes:** `set-face-font` (`lisp/faces.el:504-523`) accepts the
12-element descriptor and lowers it through the same path `font-set-face-font`
uses today, instead of only accepting a name string.

**Two candidate shapes, and the trade-off between them:**

* **(a) Delegate.** Make `set-face-font` detect the 12-element vector and call
  the `font-set-face-font` logic. Cheap; reuses working code. But
  `lisp/faces.el` is dumped and `lisp/font.el` is not, so the delegation has
  to go through an autoload — which means fixing the `;;; ###autoload` cookie
  at `lisp/font.el:990` and paying an autoload of `font.el` the first time a
  descriptor is used. In a TTY-only build that autoload drags in the whole
  `font.el` module for a call that will then do nothing, because there is no
  `stream`/`tty` row that can lower a descriptor (`lisp/font.el:116` only
  covers `tty`).
* **(b) Move the lowering in.** Teach the *specifier* layer to accept the
  descriptor as a font instantiator, so the descriptor survives into the
  specifier and is lowered per device at instantiation time
  (`src/fontcolor.c:1610-1709`). This is the "right" model — it is what makes
  the spelling genuinely device-independent rather than "lowered once, at
  `set-face-font` call time, against whatever device happened to be selected"
  — but it touches the C instantiator path and the specifier's
  instantiator-validity check, and it is a much bigger change.

**Recommendation: (a) for the milestone, (b) as the follow-up.** Reason:
`font-set-face-font`'s existing implementation lowers the descriptor *once*,
against `(selected-device)` (it calls `font-create-name` with no device
argument, `lisp/font.el:994`), and then stores the resulting *name* into the
face. That is already device-dependent in exactly the way the milestone is
trying to remove — the descriptor is device-independent only at the moment it
is created.

**What it breaks:**

* `set-face-font` is public API documented at `man/lispref/faces.texi:409`,
  `man/xemacs/custom.texi:2179-2181`, `man/xemacs/new.texi:162`. Its
  docstring (`lisp/faces.el:505-521`) and `make-font-specifier`'s
  (`lisp/fontcolor.el:60-72`) both enumerate the accepted instantiators; all
  three lists need the descriptor added.
* The inheritance-vector form is *not* ambiguous: `set-face-font` already
  accepts "a vector of one element (a face to inherit from)"
  (`lisp/fontcolor.el:71`), and a 12-element vector is a different length, so
  `(= (length font) 12)` disambiguates cleanly.
* `font-set-face-font`'s own extra behaviour — the `font-specification`
  property, underline, smallcaps/bigcaps, strikethru — is *not* what
  `set-face-font` does today. If `set-face-font` grows the descriptor form,
  it must decide whether to also grow that behaviour (probably yes; it is the
  only reason to use a descriptor over a name).
* The `font-specification` property is read by `font-update-device-fonts`
  (`lisp/font.el:1047-1049`) and `font-update-one-face`
  (`lisp/font.el:1058-1070`) and replayed with `(set-face-font cur font-spec device)`
  — i.e. a *descriptor* is passed to `set-face-font` there. Today that call
  passes a descriptor to a `set-face-font` that cannot accept one, so the
  replay is already broken; wiring the descriptor into `set-face-font` fixes it
  as a side effect. Note both functions are themselves uncalled in this tree
  (no references outside `lisp/font.el`), so this is latent, not live.

### 2.2 `font-create-name` — lower the descriptor per device, correctly, on Xft

**Becomes:** `font-create-name` (`lisp/font.el:379-383`) must produce a
*fontconfig* name when the target device is an Xft device, and an XLFD when it
is a core-X device.

**How the dispatch should be done.** The tree already has a convention for
this: `x-font-create-object` (`lisp/font.el:566-578`) branches on
`(featurep 'xft-fonts)` — the feature provided by the `x` console at
`src/fontcolor-x.c:965-967` — and calls `xft-font-create-object`
(`lisp/font.el:786-808`) when the name matches `font-xft-font-regexp`
(`lisp/font.el:742-751`). The frobbers do the same
(`lisp/x-faces.el:178`, `:222`, `:252`, `:324`, `:505`, `:534`). So:

* `font-create-name` should route the `x` row to `xft-font-create-name`
  (`lisp/font.el:831-837`) when `(featurep 'xft-fonts)`, and to
  `x-font-create-name` (`lisp/font.el:636-705`) otherwise.

**What `xft-font-create-name` still has to be taught.** As written it is
(`lisp/font.el:831-837`):

```elisp
(defun xft-font-create-name (fontobj &optional device)
  (let* ((pattern (make-fc-pattern)))
    (if (font-family fontobj)
        (fc-pattern-add-family pattern (font-family fontobj)))
    (if (font-size fontobj)
        (fc-pattern-add-size pattern (font-size fontobj)))
    (fc-name-unparse pattern)))
```

It is **dead code today** — nothing calls it (the only references are its own
definition and the `fc` row's dead sibling). And it is not merely
unfinished, it is *wrong* in four ways against the descriptor it is supposed
to lower:

1. **The family is a list.** `make-font` wraps a string family in a list
   (`lisp/font.el:363-365`), so `(font-family fontobj)` is `("Noto Sans")`,
   not `"Noto Sans"`. `fc-pattern-add-family` wants a string. Compare
   `x-font-create-name`, which normalises with `(if (stringp family) (setq
   family (list family)))` in the other direction (`lisp/font.el:659-660`)
   and `mswindows-font-create-name`, which does the same
   (`lisp/font.el:942-943`).
2. **The size is a string.** `font-size` may be `"14pt"`, `"140px"` or a
   number; `fc-pattern-add-size` wants a number in points. The tree already
   has the converter: `font-spatial-to-canonical` (`lisp/font.el:294-355`),
   used by both `x-font-create-name` (`lisp/font.el:663-664`) and
   `mswindows-font-create-name` (`lisp/font.el:946-947`).
3. **Weight is dropped entirely.** The descriptor's `:weight`
   (`lisp/font.el:366-367`) and the `font-bold-p` style bit
   (`lisp/font.el:223`, `lisp/font.el:662`) are never consulted, so a bold
   descriptor would lower to a regular font. The tree has the vocabulary:
   `x-font-weight-mappings` (`lisp/font.el:122-134`),
   `fc-font-weight-translate-to-string` (`lisp/fontconfig.el:294-299`),
   `fc-font-weight-translate-from-constant`
   (`lisp/fontconfig.el:337-340`).
4. **Slant is dropped.** `font-italic-p` / `font-oblique-p`
   (`lisp/font.el:178-180`, `lisp/font.el:224-225`) are never consulted;
   `fc-font-slant-translate-to-string` (`lisp/fontconfig.el:287-292`) exists
   for this.

There is a template for the finished version in the same file:
`x-make-font-bold-xft` (`lisp/x-faces.el:184-211`) does
family → weight → size against a fontconfig pattern and calls
`fc-name-unparse`.

**What it breaks:**

* `font-create-name` is called from exactly four places, all of which would
  start receiving fontconfig names instead of XLFDs on an Xft build:
  `lisp/font.el:994` (the descriptor path), `lisp/faces.el:1349`
  (`make-face-family`), `lisp/faces.el:1569` (`make-face-size`),
  `lisp/faces.el:1634` and `lisp/faces.el:1654` (`make-face-height`). The
  `faces.el` three are the *frobbers*, and they are the dangerous ones: their
  output is fed straight back into the face's font specifier, and on an Xft
  build a fontconfig name is the *correct* spelling — but only if the
  descriptor→name lowering and the name→descriptor lowering are exact
  inverses. Today they are not, which is why the frobbers have separate
  `-xft` and `-core` variants that bypass `font-create-name` entirely
  (`lisp/x-faces.el:175-217`, `lisp/x-faces.el:249-296`).
* `font-create-name` is **not** autoloaded (no cookie at `lisp/font.el:379`;
  contrast `lisp/font.el:385`), and `font.el` is not dumped — so any caller
  outside `lisp/font.el` that wants the descriptor→name lowering must
  `(require 'font)` first. `lisp/faces.el` already depends on `font.el`
  (`lisp/faces.el:1344`, `:1564` say "keep the dependency on font.el for now").
* `font-window-system-mappings` is a `defconst` (`lisp/font.el:108`) that
  names functions defined *later* in the same file. If the `x` row is changed
  to a dispatching function rather than a plain name, the table stays valid;
  if instead a new row is added, note that `assq` on device type is
  first-match-wins (`lisp/font.el:382`), so a new `xft` row would be shadowed
  by the `x` row unless the `x` row is removed or reordered.

### 2.3 `font-create-object` — mostly already right, with one hole

`font-create-object` (`lisp/font.el:386-390`) → `x-font-create-object`
(`lisp/font.el:566-617`) already branches to `xft-font-create-object`
(`lisp/font.el:786-808`) when the *name* is not an XLFD but does match
`font-xft-font-regexp` (`lisp/font.el:566-578`). So the name→descriptor
direction is already Xft-aware.

The hole: `font-xft-font-regexp` (`lisp/font.el:742-751`) is extremely loose —
`(\-|\:|\,|[^:-])*` plus an optional size plus `(:[^:]*)*` — so it matches
essentially *any* colon-free string, including a plain family name with no
size. And `xft-font-create-object` (`lisp/font.el:786-808`) requires a device
(`(or device (default-device 'x))`, `lisp/font.el:791`) and calls
`fc-font-match` (`src/font-mgr.c:1003-1051`), which returns `Qnil` for a
non-`x` device (`src/font-mgr.c:1022-1023`) and errors on a non-`x` device
(`src/font-mgr.c:1024`). So `font-create-object` on a `tty`/`stream`/`gtk`
device with a non-XLFD name is a hard error, not a graceful nil.

Also: `font-create-object` **is** autoloaded (`lisp/font.el:385`) while
`font-create-name` is not (§2.2). That asymmetry should be fixed in the same
change.

### 2.4 `font-list` — list fontconfig fonts on an Xft build

**Becomes:** on an Xft `x` device, `font-list` returns fontconfig font names;
on a core-X `x` device it keeps returning core X server font names.

**The implementation already exists, in C.** `src/font-mgr.c:1054-1081`
(`fc-list-fonts-pattern-objects`) is a working `FcFontList` wrapper, and
`lisp/fontconfig.el:496-509` (`fc-find-available-font-families`) and
`lisp/fontconfig.el:511-524` (`fc-find-available-weights-for-family`) are
working Lisp wrappers over it. `lisp/font-mgr.el:197-237` contains *stubs* of
the same names that `error 'unimplemented` — so a new `font-list`
implementation must use the C versions (`src/font-mgr.c`), not the Lisp ones.
Note also that `lisp/font-mgr.el:40` does `(provide 'font-mgr)`
unconditionally, which shadows the C versions if that file is loaded; the
guard at `lisp/font-mgr.el:38` is a `####` comment, not code.

The existing `x_font_list` (`src/fontcolor-x.c:877-901`) calls `XListFonts`
unconditionally (`src/fontcolor-x.c:893-894`), so the change is a new
`xft_font_list` plus a dispatch. The console method table is at
`src/fontcolor-x.c:931-938`; `font_list` is registered there
(`src/fontcolor-x.c:936`).

**The cheapest correct change** is not in C at all: `font-list` is a Lisp-callable
C function (`src/fontcolor.c:1310-1322`) that dispatches to the device method.
Because `x_font_list` is *only* reached through the device method, the
dispatch could equally live in `lisp/font.el` — but `font-list` is a C symbol,
so the dispatch has to happen in C (`src/fontcolor.c:1310-1322` would need to
consult `XEMACS_USE_XFT`), or the `x` console method needs to become a
dispatcher.

**What it breaks — see §4 in full.** The short version: on an Xft build, the
only two in-tree consumers of `font-list` that parse the result as XLFDs are
`x-reset-device-font-menus-core` (`lisp/x-font-menu.el:159-201`) and
`x-available-font-sizes` (`lisp/x-faces.el:403-438`) — and *both are already
unreachable on an Xft build*, because their callers branch on
`(featurep 'xft-fonts)` first (`lisp/x-font-menu.el:101-105`,
`lisp/x-faces.el:500-509`, `lisp/x-faces.el:529-538`). The remaining three
consumers would *improve*.

### 2.5 `try-font-name` — the probe is vacuous on Xft

`try-font-name` (`lisp/faces.el:1031-1034`) is `(and name (make-font-instance
name device t) name)`. On a core-X build this is a real existence check.
On an Xft build it can never fail. §7.3 works this through.

**Becomes:** either `try-font-name` must be taught to distinguish "the font I
asked for" from "the font I got", or the callers that rely on it
(`x-font-create-name` `lisp/font.el:704`, `mswindows-font-create-name`
`lisp/font.el:982`, `x-make-font-bold-core` `lisp/x-faces.el:215-217`,
`mswindows-make-font-italic` `lisp/msw-faces.el:244-245`) must stop using it
on Xft builds. The tree already has the right tool: `fc-try-font`
(`lisp/fontconfig.el:478-488`), which actually asks fontconfig whether the
pattern matches anything, and which `x-make-font-bold-xft`
(`lisp/x-faces.el:197`, `:203`, `:209`) already uses.

---

## 3. Caller inventory

Sweep over `lisp/`, `src/`, `modules/`, `docs/user-config/` (plus `man/` for
documentation references). "Xft build" = what the call *would* see if
`font-list` returned fontconfig names and `font-create-name` returned
fontconfig names.

### 3.1 `font-list`

| # | site | what it does | on an Xft build |
|---|---|---|---|
| 1 | `lisp/font.el:713` — `x-font-build-cache` | `(mapcar 'x-font-create-object (font-list "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"))` | **Improves.** Today it maps XLFDs through `x-font-create-object`; with fontconfig names it would map them through `xft-font-create-object` (`lisp/font.el:786`), which is the correct parser. But note: `x-font-build-cache` has **no callers anywhere** (`lisp/font.el:710`; grep finds only its own definition) — it is dead code that happens to be autoloaded (`lisp/font.el:709`). |
| 2 | `lisp/frame.el:991` — `set-frame-font` interactive completion | `(font-list "*" frame)` supplies the completion list for the "Font name:" prompt | **Improves**, but only if the completion candidates are *valid* names for the device. Today `XListFonts(dpy, "*")` on a modern system with no core fonts installed returns an empty or near-empty list, so the prompt offers nothing. With fontconfig names it offers real choices. |
| 3 | `lisp/x-font-menu.el:179` — `x-reset-device-font-menus-core` | parses each name with `x-font-regexp` / `x-font-regexp-foundry-and-family` / `x-font-regexp-spacing` (`lisp/x-faces.el:110-137`) to build the Font/Size/Weight submenus | **Unreachable.** `x-reset-device-font-menus` (`lisp/x-font-menu.el:101-105`) dispatches to `x-reset-device-font-menus-xft` (`lisp/x-font-menu.el:124-157`) whenever `(featurep 'xft-fonts)`, and the Xft variant never calls `font-list` — it uses `fc-find-available-font-families` (`lisp/fontconfig.el:496`). **If** `font-list` changed and someone routed the core menu builder at it, the result would be an *empty* menu, not an error: `x-font-regexp` requires an XLFD (`lisp/x-faces.el:110-115`) and the `when` at `lisp/x-font-menu.el:183-184` would filter every name out. |
| 4 | `lisp/x-faces.el:435` — `x-available-font-sizes` | wildcard-substitutes the size fields of an XLFD, lists matching fonts, and extracts (pixelsize, pointsize, name) triples | **Unreachable on Xft.** Its only callers are `x-frob-font-size` (`lisp/x-faces.el:445-449`), reached from `x-find-smaller-font-core` (`lisp/x-faces.el:526-527`) / `x-find-larger-font-core` (`lisp/x-faces.el:543-544`), which are reached only when the name is an XLFD (`lisp/x-faces.el:505-509`, `:534-538`). |
| 5 | `lisp/gtk-font-menu.el:101` — `gtk-reset-device-font-menus` | parses names with `gtk-font-regexp` | **Already broken, and unaffected.** The `gtk` console has its own `font_list` method, `gtk_font_list` (`src/fontcolor-gtk.c:400-441`), which returns Pango descriptions — never XLFDs. So `gtk-font-regexp` (`lisp/gtk-faces.el:102`) never matches and the GTK font menu is empty today. This is a *separate* GTK bug, not an Xft one. |
| 6 | `lisp/msw-faces.el:273` — `mswindows-available-font-sizes` | wildcard-substitutes the pointsize of a Windows font name | **Unaffected** (Windows path). Uses `mswindows-font-regexp`, and the `mswindows` console has its own `font_list` (`src/fontcolor-msw.c:1313`). |
| 7 | `lisp/msw-font-menu.el:79` — `mswindows-reset-device-font-menus` | parses names with `mswindows-font-regexp` | **Unaffected** (Windows path). |
| 8 | `lisp/obsolete.el:222` | `(make-compatible 'list-fonts 'font-list)` | Unaffected — a compatibility alias, not a caller. |

Documented mentions (not callers): `lisp/fontconfig.el:453`, `:491-492`
(a comment sketching `font-list`'s fontconfig-shaped ideal),
`lisp/font-menu.el:114` (a comment), `man/internals/internals.texi:29170`
(`fc-font-list`, the *intended* future name of `fc-list-fonts-pattern-objects`).

**C-level:** `src/fontcolor.c:1310-1322` defines `font-list`;
`src/fontcolor-x.c:877-901` implements the `x` method;
`src/fontcolor-gtk.c:400-441` the `gtk` method;
`src/fontcolor-tty.c:466-470` the `tty` method;
`src/fontcolor-msw.c:1313` the `mswindows` method (inherited by `msprinter`,
`src/fontcolor-msw.c:1668`). There is **no `stream` method** — see §9.1.

### 3.2 `font-create-name`

| # | site | on an Xft build |
|---|---|---|
| 1 | `lisp/font.el:994` — `font-set-face-font` | The descriptor path. Would produce a fontconfig name — correct, provided §2.2's four fixes are made. |
| 2 | `lisp/faces.el:1349` — `make-face-family` | `(font-create-object f d)` then `(set-font-family fo family)` then `(font-create-name fo d)`. Currently `f` is the *instantiator string* of the face; `font-create-object` on an Xft build routes it to `xft-font-create-object` (`lisp/font.el:575`), giving a descriptor with a family/size/weight; `set-font-family` overwrites the family; `font-create-name` then lowers it back. With a fontconfig-aware `font-create-name` this round-trips. **Today it does not**: `x-font-create-name` emits an XLFD, so `make-face-family` on an Xft build produces an XLFD, which `xft_open_font_by_name` then hands to `XftFontOpenXlfd` (`lwlib/lwlib-fonts.c:84-88`) — which on a system without core fonts fails and falls back to `monospace` (§7.2). **This is a real, user-visible bug today: `make-face-family` does not work on an Xft build.** |
| 3 | `lisp/faces.el:1569` — `make-face-size` | Same shape, same conclusion. |
| 4 | `lisp/faces.el:1634` and `:1654` — `make-face-height` | Same shape, same conclusion. |

Note the `d` argument in all three: the frobbers pass the *device* through, so
a fontconfig-aware `font-create-name` can lower per device correctly. Only
`font-set-face-font` (`lisp/font.el:994`) omits the device and uses
`(selected-device)`.

### 3.3 `font-create-object`

| # | site | on an Xft build |
|---|---|---|
| 1 | `lisp/font.el:450` — `font-default-object-for-device` | `(font-create-object font)` — **note: no device argument**, so it uses `(selected-device)`. Caches the result in `font-default-cache` (`lisp/font.el:437`, `:449-451`). |
| 2 | `lisp/font.el:712` — `x-font-build-cache` | Dead code (§3.1 #1). |
| 3 | `lisp/faces.el:1347` — `make-face-family` | `(font-create-object f d)` — device passed. |
| 4 | `lisp/faces.el:1567` — `make-face-size` | device passed. |
| 5 | `lisp/faces.el:1618`, `:1627` — `make-face-height` | device passed. |
| 6 | `lisp/faces.el:1640`, `:1649` — `make-face-height` | device passed. |
| 7 | `lisp/cus-face.el:241` — `custom-face-bold` | `(font-create-object font)` — **no device**. Then `(font-bold-p fontobj)`. If `font-create-object` returns nil (unknown device type / unparseable name), `font-bold-p` signals on nil. |
| 8 | `lisp/cus-face.el:254` — `custom-face-italic` | same shape. |
| 9 | `lisp/cus-face.el:296` — `custom-face-font-size` | `(font-size fontobj)` — same. |
| 10 | `lisp/cus-face.el:322` — `custom-face-font-family` | `(font-family fontobj)` — same. |
| 11 | `lisp/gtk-faces.el:338` — GTK font-selection dialog | `(font-create-object (gtk-font-selection-dialog-get-font-name dialog))` — **no device**, and the name is a Pango description. On a `gtk` device this goes to `x-font-create-object` (`lisp/font.el:110` maps `gtk` → `x-font-create-name`/`x-font-create-object`), whose Xft branch (`lisp/font.el:573-575`) requires `(featurep 'xft-fonts)` — **which is false on a GTK build** (`src/fontcolor-x.c:965-967` provides it only from the `x` console). So on GTK the name falls through to `(make-font)` (an empty descriptor) at `lisp/font.el:578`, and `(font-family font)` is nil → `font-menu-set-font` gets a nil family. **The GTK font dialog is broken today.** |

### 3.4 `set-face-font`

| # | site | note |
|---|---|---|
| 1 | `lisp/font.el:1006` | inside `font-set-face-font` — the descriptor path's own recursion into the string path. |
| 2 | `lisp/font.el:1049` — `font-update-device-fonts` | `(set-face-font cur font-spec device)` where `font-spec` is the **`font-specification` property** — i.e. a *descriptor*. This call is currently **wrong**: it passes a 12-element vector to a `set-face-font` that only accepts names. Fixing §2.1 fixes this. The function has no callers. |
| 3 | `lisp/font.el:1070` — `font-update-one-face` | same. No callers. |
| 4 | `lisp/faces.el:504` | the definition. |
| 5 | `lisp/frame.el:1007` — `set-frame-font` | `(set-face-font face font-name frame)` for every face. `font-name` is a *string* from the user. |
| 6 | `lisp/font-menu.el:362` — `font-menu-set-font` | `(set-face-font 'default new-default-face-font …)`. `new-default-face-font` is built by the `font-menu-load-font` device method (`lisp/font-menu.el:446`, dispatched at `lisp/font-menu.el:329`; the `x` implementation is `lisp/x-font-menu.el:350`), which returns a **string**. |
| 7 | `lisp/font-menu.el:434` — `font-menu-set-font` | `(set-face-font face (font-instance-name (font-menu-load-font …)) …)`. A string. |
| 8 | `lisp/x-faces.el:800` | `(set-face-font face fn locale our-tag-set append)` — from the X resource fallback installation. `fn` is a string (an XLFD or a fontconfig name). |
| 9 | `lisp/msw-glyphs.el:77` | `(set-face-font 'border-glyph "WingDings:Regular:11::Symbol" …)` — a Windows name string. |
| 10 | `lisp/modeline.el:337-338`, `:699-703` | `(set-face-font 'modeline-mousable [bold] nil '(default mono win))` — a **one-element vector**, i.e. the *inheritance* form (inherit from the `bold` face). This is the form that must **not** be confused with the 12-element descriptor form. Length 1 ≠ 12, so the discrimination is safe. |
| 11 | `lisp/cl-macs.el:2049` | `(defsetf face-font (f &optional s) (x) (list 'set-face-font f x s))` — the `setf` setter for `face-font`. **This is a caller that must keep working with a descriptor**: `(setf (face-font 'default) descriptor)` would go through `set-face-font`. |
| 12 | `docs/user-config/xft-chinese-font-configuration.el:105-106`, `:174`, `:178`, `:182`, `:186`, `:194`, `:202`, `:206`, `:210`, `:214`, `:218`, `:222`, `:226`, `:230`, `:234`, `:238`, `:242`, `:246`, `:250`, `:332`, `:338`, `:344`, `:350`, `:360`, `:366`, `:372`, `:380`, `:385`, `:390`, `:395`, `:400`, `:406`, `:411`, `:524`, `:559` | The repo's own Xft configuration. **Every call passes a fontconfig *string*** built by `my-build-font-name` (`docs/user-config/xft-chinese-font-configuration.el:78-91`), e.g. `"Noto Sans Mono CJK SC-14"` or `"Noto Sans Mono CJK SC-14:bold"`. It also uses the per-charset tag-set form: `(set-face-font face font-spec locale tag-set)` (`docs/user-config/xft-chinese-font-configuration.el:104-105`). **This file is the repo's evidence that the "works" target is a fontconfig name, and it is the file that a descriptor-based API would have to keep working.** |
| 13 | `docs/adr/0002-cnfonts-xemacs-backend-via-specifier-tag-sets.md:5`, `:7`, `:30` | The ADR that motivates the change set. *(That ADR lives on the `docs/adr-0002-cnfonts-shim-surface` branch, not on this branch's base `origin/xemacs-cmake`.)* |

Documentation references (not callers): `man/lispref/faces.texi:409`,
`man/xemacs/custom.texi:2086`, `:2179-2181`, `:2582-2585`, `:3062`,
`man/xemacs/new.texi:162`, `:171`,
`man/xemacs-faq.texi:5576-5577`, `:5598`, `:5621`, `:5628`, `:5736`, `:5881-5882`,
`:6459`, `man/internals/internals.texi:28675`, `:28703-28706`, `:32740`.

### 3.5 `try-font-name` (for completeness — it is the probe §2.5 is about)

Live callers: `lisp/font.el:704` (`x-font-create-name`),
`lisp/font.el:982` (`mswindows-font-create-name`),
`lisp/x-faces.el:215-217` (`x-make-font-bold-core`),
`lisp/x-faces.el:238` (`x-make-font-unbold-core`),
`lisp/x-faces.el:298-301`, `:322` (`x-make-font-italic-core`,
`x-make-font-unitalic-core`), `lisp/x-faces.el:341-364`
(`x-make-font-bold-italic-core`), `lisp/msw-faces.el:244-245`,
`lisp/msw-faces.el:250-251`.

Commented out (dead): `lisp/gtk-faces.el:266-318` — the whole GTK frobber set
in Lisp is commented out; the GTK backend uses the C functions
`gtk-make-font-bold` (`src/fontcolor-gtk.c:483-506`), `gtk-make-font-italic`
etc. instead.

Dead: `lisp/font.el:77` — `; delete alternate defn of try-font-name`.

---

## 4. The compatibility risk of `font-list` returning fontconfig names on Xft

**Short answer: the risk is low, because almost nothing in the tree consumes
`font-list` on an Xft build.** The two consumers that *do* parse `font-list`
output as XLFDs are both already unreachable on Xft, because their callers
branch on `(featurep 'xft-fonts)` first.

What in the tree assumes core-X names or XLFDs:

1. **`x-font-regexp` and friends** (`lisp/x-faces.el:110-137`) — the XLFD
   regexp family. Used by:
   * `x-reset-device-font-menus-core` (`lisp/x-font-menu.el:183-201`) —
     **not reached on Xft** (`lisp/x-font-menu.el:103-105`);
   * `x-available-font-sizes` (`lisp/x-faces.el:405`, `:427`) —
     **not reached on Xft** (`lisp/x-faces.el:505-509`);
   * `x-frob-font-weight` / `x-frob-font-slant`
     (`lisp/x-faces.el:149-173`) — used only by the `-core` frobbers, which
     are reached only for XLFD-looking names (`lisp/x-faces.el:179-181`);
   * `x-font-create-object`'s XLFD branch (`lisp/font.el:568-617`);
   * `x-font-xlfd-font-name-p` (`lisp/x-faces.el:140-143`), which is the
     *discriminator* used to choose between the core and Xft frobbers.
2. **`x-font-regexp-ascii`** (`lisp/x-font-menu.el:169-173`) — the string
   `"iso8859-1"`, used as a filter on `font-list` output
   (`lisp/x-font-menu.el:183`). Same unreachability.
3. **`src/fontcolor-xlike-inc.c:219-259`** —
   `xlistfonts_checking_charset`, which calls `XListFonts` directly
   (`src/fontcolor-xlike-inc.c:234-235`) and matches each name against the
   charset's registry. This is the *core-X* `find_charset_font` fallback
   (`src/fontcolor-xlike-inc.c:691-...`). On Xft it is preceded by
   `xft_find_charset_font` (`src/fontcolor-xlike-inc.c:703-709`), which is
   tried first.
4. **`font_spec_matches_charset`** (`src/fontcolor-xlike-inc.c:122-215`) —
   shared by X and GTK. It matches a font spec against a charset by checking
   whether the spec string *ends with* one of the charset's registry strings
   (`src/fontcolor-xlike-inc.c:192-213`). **This is XLFD-shaped**: a
   fontconfig name like `"Noto Sans Mono CJK SC-14"` never ends with
   `"iso8859-1"`, so it does not match at `STAGE_INITIAL`. The one exception
   is the ASCII hack at `src/fontcolor-xlike-inc.c:168-173`: a short,
   star-free, fewer-than-5-hyphen name matches ASCII unconditionally. This is
   why the ASCII path works on Xft but a *charset-specific* font spec does not
   — and it is the mechanism behind §6.

**The real compatibility risk is not in `font-list` at all — it is in the
*shape* of the names.** Two separate hazards:

* **A fontconfig name is not an XLFD, and `xft_open_font_by_name` routes on
  hyphen count** (`lwlib/lwlib-fonts.c:74-88`). A name with 14 hyphens goes
  to `XftFontOpenXlfd`; a name with fewer than 14 hyphens *and* a `*` also
  goes to `XftFontOpenXlfd`; anything else goes to `XftFontOpenName`. So the
  *same* string can be interpreted two different ways depending on its
  punctuation. A family name that happens to contain 5+ hyphens ("DejaVu
  Sans Mono" has none, but "Noto-Sans-Mono" would) could flip the routing.
  Note the XLFD that `x-font-create-name` emits
  (`lisp/font.el:692-702`) has exactly **14** hyphens — verified by
  construction — so it *is* routed to `XftFontOpenXlfd`, **unless** the
  registry or encoding field contains a hyphen (`"iso8859-1"` would make it
  15), in which case it silently becomes `XftFontOpenName`.
* **`font-list` returning fontconfig names changes what `set-frame-font`'s
  completion offers** (`lisp/frame.el:989-993`), and those names are then
  passed to `set-face-font` (`lisp/frame.el:1007`). That is the *correct*
  spelling, so it improves; but a user who had a core-X name in their
  `.emacs` and upgrades to an Xft build will find that name no longer
  appears in the completion list, and `set-face-font` with that name will
  fail differently. This is a user-visible behaviour change, not a silent one.

---

## 5. The MS Windows path

### 5.1 What exists

* `mswindows-font-create-name` (`lisp/font.el:923-983`) — descriptor → Windows
  name (`"Family:Style:Size:Effects:Charset"`), with the same "probe with
  `try-font-name` until one family works" loop as the X version
  (`lisp/font.el:950-982`).
* `mswindows-font-create-object` (`lisp/font.el:879-921`) — Windows name →
  descriptor.
* The `mswindows` row of `font-window-system-mappings` (`lisp/font.el:113`).
* The `msprinter` console type exists (`src/console-msw.c:41`) and inherits
  all the font methods from `mswindows`
  (`src/fontcolor-msw.c:1664-1670`, including `font_list` at `:1668`).

### 5.2 Which frobbers exist per backend

`make-face-bold` and friends route through `Face-frob-property`
(`lisp/faces.el:1067-...`), whose `frob-mapping` is a plist keyed by device
type, looked up at `lisp/faces.el:1140-1143`
(`(plist-get frob-mapping devtype)`).

| frob operation | `tty` | `x` | `gtk` | `mswindows` | `msprinter` |
|---|---|---|---|---|---|
| bold (`lisp/faces.el:1462-1472`) | `(lambda (f d) t)` — sets the `highlight` flag | `x-make-font-bold` (`lisp/x-faces.el:175`) | `gtk-make-font-bold` (**C**, `src/fontcolor-gtk.c:483`) | `mswindows-make-font-bold` (`lisp/msw-faces.el:229`) | `mswindows-make-font-bold` (`lisp/faces.el:1468`) |
| italic (`lisp/faces.el:1481-1491`) | underline flag | `x-make-font-italic` (`lisp/x-faces.el:249`) | `gtk-make-font-italic` (C) | `mswindows-make-font-italic` (`lisp/msw-faces.el:241`) | `mswindows-make-font-italic` (`lisp/faces.el:1487`) |
| bold-italic (`lisp/faces.el:1500-1510`) | underline+highlight | `x-make-font-bold-italic` (`lisp/x-faces.el:324`) | `gtk-make-font-bold-italic` (C) | `mswindows-make-font-bold-italic` (`lisp/msw-faces.el:253`) | `mswindows-make-font-bold-italic` (`lisp/faces.el:1506`) |
| smaller (`lisp/faces.el:1582-1588`) | — | `x-find-smaller-font` (`lisp/x-faces.el:500`) | `gtk-find-smaller-font` | `mswindows-find-smaller-font` | `mswindows-find-smaller-font` (`lisp/faces.el:1587`) |
| larger (`lisp/faces.el:1594-1600`) | — | `x-find-larger-font` (`lisp/x-faces.el:529`) | `gtk-find-larger-font` | `mswindows-find-larger-font` | `mswindows-find-larger-font` (`lisp/faces.el:1599`) |
| family (`lisp/faces.el:1339-1351`) | — | generic lambda using `font-create-object`/`font-create-name` | *same* generic lambda | *same* generic lambda | *same* generic lambda |
| size (`lisp/faces.el:1559-1571`) | — | generic lambda | *same* | *same* | *same* |
| height (`lisp/faces.el:1611-1655`) | — | generic lambda | *same* | *same* | *same* |

So: **`msprinter` has a frobber row in every one of these mappings, but no row
in `font-window-system-mappings`** — see §8.

Also note `lisp/gtk-faces.el:266-318`: the *Lisp* GTK frobbers are commented
out; the GTK backend uses the C implementations.

### 5.3 What the MS Windows path needs

1. **A `msprinter` row in `font-window-system-mappings`** (see §8).
2. **`mswindows-font-create-name` needs the same four fixes as the Xft one**
   (§2.2), in milder form:
   * it uses `(face-font 'default)` as its "empty descriptor" answer
     (`lisp/font.el:931`), which is device-dependent and wrong when the
     descriptor is empty — but this is a rare path;
   * it does handle the family-list shape (`lisp/font.el:942-943`) and the
     size-string shape (`lisp/font.el:946-947`) correctly;
   * it does handle weight (`lisp/font.el:944-949`) but **not** the
     `font-bold-p` style bit the way `x-font-create-name` does
     (`lisp/font.el:661-662`) — actually it does
     (`lisp/font.el:944-945`), good;
   * it drops slant/italic: there is no `mswindows-frob-font-style` call in
     `mswindows-font-create-name`, so an italic descriptor lowers to a
     non-italic Windows name. `mswindows-construct-font-style`
     (`lisp/font.el:969-972`) is called with only the weight and an italic
     flag derived from `(font-italic-p fontobj)` — so italic *is* handled
     (`lisp/font.el:970-972`). Good.
   * the real gap: the "probe until one family works" loop
     (`lisp/font.el:954-982`) uses `try-font-name`, which on Windows *is* a
     real check (the Windows backend has no fallback-to-monospace), so this
     is fine.
3. **`font-create-object` for `mswindows`/`msprinter`** — the `msprinter`
   device type has no row in `font-window-system-mappings`, so
   `(font-create-object name printer-device)` returns `nil`
   (`lisp/font.el:386-390`) and the frobbers that call it
   (`lisp/faces.el:1347`, `:1567`, `:1618`) get nil.

---

## 6. GTK

### 6.1 What GTK needs

1. **A `gtk` row that is not the `x` row.** `font-window-system-mappings`
   (`lisp/font.el:110`) maps `gtk` to `x-font-create-name` /
   `x-font-create-object` — the *core-X* functions. That is wrong twice over:
   * `x-font-create-name` emits an XLFD (`lisp/font.el:692-702`), but GTK
     renders with Pango and wants a Pango description
     (`src/fontcolor-gtk.c:474`, `font_description_from_string`);
   * `x-font-create-object`'s Xft branch (`lisp/font.el:573-575`) is guarded
     by `(featurep 'xft-fonts)`, which is **false on a GTK build** —
     `xft-fonts` is provided only from the `x` console
     (`src/fontcolor-x.c:965-967`), and `HAVE_XFT` may still be defined in a
     GTK build (the tree says so explicitly at `src/console-xlike-inc.h:103-108`).
     So on GTK, `x-font-create-object` falls through to `(make-font)` — an
     empty descriptor (`lisp/font.el:576-578`).
   * Consequence, concretely: `lisp/gtk-faces.el:338` (the GTK font-selection
     dialog) calls `(font-create-object (gtk-font-selection-dialog-get-font-name dialog))`
     and then `(font-family font)` — which is `nil` on GTK. The GTK font
     dialog cannot work as written.
2. **A GTK `font-create-name`** that emits a Pango description. The C
   functions already exist and are the right shape:
   `gtk-set-font-weight` (`src/fontcolor-gtk.c:458-481`),
   `gtk-make-font-bold` (`src/fontcolor-gtk.c:483-506`),
   `gtk-make-font-unbold` (`src/fontcolor-gtk.c:508`),
   `gtk-make-font-italic` (`src/fontcolor-gtk.c:533`),
   `gtk-make-font-unitalic` (`src/fontcolor-gtk.c:558`),
   `gtk-make-font-bold-italic` (`src/fontcolor-gtk.c:579`). They all take and
   return *strings* (a Pango description), so a GTK lowering function would
   be a thin wrapper: parse the descriptor into a `PangoFontDescription`, set
   family/size/weight/style, and `pango_font_description_to_string`.
3. **A GTK `font-list` that returns Pango descriptions.** It already exists:
   `gtk_font_list` (`src/fontcolor-gtk.c:400-441`) returns
   `gtk_font_describe_face` strings. But the *consumer*
   (`lisp/gtk-font-menu.el:101-122`) parses them as XLFDs with
   `gtk-font-regexp` (`lisp/gtk-faces.el:102`), which never matches a Pango
   description — so the GTK font menu is empty. Either the regexps become
   Pango-shaped, or the menu is built from
   `pango_font_family_list_faces` directly.

### 6.2 Can GTK support per-charset fonts at all?

**Not today, and not with the current Pango API surface XEmacs uses.**

* `gtk_find_charset_font` (`src/fontcolor-gtk.c:449-456`) is a no-op:

  ```c
  static Lisp_Object
  gtk_find_charset_font (Lisp_Object UNUSED (device), Lisp_Object font,
                         Lisp_Object UNUSED (charset),
                         enum font_specifier_matchspec_stages UNUSED (stage))
  {
    /* Pango doesn't understand charset.  Maybe PangoCoverage? */
    return font;
  }
  ```

  It returns the font unchanged. In the instantiation path
  (`src/fontcolor.c:1687-1690`) this means the *same* font is used for every
  charset: the returned `matching_font` is the instantiator verbatim, and the
  charset is then carried into `Fmake_font_instance`
  (`src/fontcolor.c:1704`) as metadata only.

* The charset *matcher* is also XLFD-shaped. `font_spec_matches_charset`
  (`src/fontcolor-xlike-inc.c:122-215`) is compiled into the GTK backend too
  (`src/fontcolor-gtk.c:443-445` includes `fontcolor-xlike-inc.c`;
  `src/fontcolor-gtk.c:719` registers the method). It matches by checking
  whether the font spec string ends with one of the charset's registry
  strings (`src/fontcolor-xlike-inc.c:192-213`). A Pango description
  ("Noto Sans Mono CJK SC 14") never ends with "iso8859-1", so it fails at
  `STAGE_INITIAL` for every non-ASCII charset. The ASCII exception is
  `src/fontcolor-xlike-inc.c:168-173` (short, star-free, <5 hyphens → match).

* **Could it be done?** Yes, in principle — but it needs a different API.
  The comment at `src/fontcolor-gtk.c:454` names the right answer:
  `PangoCoverage`. A `PangoFontFace` obtained through `PangoFcFont` (the
  fontconfig-backed subclass) exposes a coverage set that *can* be tested
  against a charset. The tree never touches `PangoFcFont` (grep for
  `PangoCoverage`/`PangoFcFont` in `src/` → nothing), and `pango_font_map_*`
  is used only for listing families and loading fonts
  (`src/fontcolor-gtk.c:274`, `:417`). So implementing per-charset fonts on
  GTK means: (i) a coverage-based `gtk_find_charset_font` using `PangoFcFont`
  and `pango_font_get_coverage`; (ii) a coverage-based
  `gtk_font_spec_matches_charset`; (iii) a way to turn a charset into a
  coverage test (XEmacs already has `mule_to_fc_charset`
  (`src/fontcolor-xlike-inc.c:571`), which is fontconfig-specific).

  **Verdict: possible but a separate, substantial project.** It is *not* a
  small extension of the Xft work, and it is not needed for the milestone's
  target (an Xft/`x` device).

* **Contrast with Xft**, which *does* support per-charset fonts today:
  `xft_find_charset_font` (`src/fontcolor-xlike-inc.c:408-682`) parses the
  name with `FcNameParse`, matches with `FcFontMatch`
  (`src/fontcolor-xlike-inc.c:459-481`), then tests coverage with
  `FcCharSetIsSubset` (`src/fontcolor-xlike-inc.c:601`) or language with
  `FcLangSetHasLang` (`src/fontcolor-xlike-inc.c:646`), using a Mule-charset
  → RFC-3066-language table (`src/fontcolor-xlike-inc.c:356-395`). It
  returns `Qnil` at `STAGE_FINAL` (`src/fontcolor-xlike-inc.c:417-418`), so
  per-charset selection happens only at the initial stage.

---

## 7. `try-font-name` on Xft vs core X

### 7.1 On an Xft build, every font instance is an Xft font

`x_initialize_font_instance` (`src/fontcolor-x.c:214-...`) does **both**:

```c
#ifdef HAVE_XFT
  rf = xft_open_font_by_name (dpy, extname);      /* src/fontcolor-x.c:236 */
#endif
  fs = XLoadQueryFont (dpy, extname);              /* src/fontcolor-x.c:245 */
  if (!fs && !rf) { signal "Couldn't load font"; return 0; }   /* :247-253 */
  if (rf && fs) { XFreeFont (dpy, fs); fs = NULL; }           /* :255-259 */
```

So on an Xft build the core-X font is loaded and then immediately thrown
away, and the instance is an `XftFont` (`src/fontcolor-x.c:275-276`).

### 7.2 `xft_open_font_by_name` never fails

`lwlib/lwlib-fonts.c:67-111`:

```c
  if (count == 14 || (count < 14 && count >= 5 && strchr (name, '*')))
    res = XftFontOpenXlfd (dpy, DefaultScreen (dpy), name);   /* :88 */
  else
    res = XftFontOpenName (dpy, DefaultScreen (dpy), name);   /* :90 */
  if (!res)
    res = XftFontOpenName (dpy, DefaultScreen (dpy), "monospace");  /* :95 */
  if (!res)
    res = XftFontOpenName (dpy, DefaultScreen (dpy), "");           /* :98 */
  if (!res) { fprintf (stderr, "Unable to find any usable XFT font..."); abort (); }  /* :104-107 */
```

`rf` is therefore **never NULL** on an Xft build: the function falls back to
`monospace` and then to the empty pattern, and only aborts the process if even
those fail.

### 7.3 Consequence: `try-font-name` is vacuous on Xft

`try-font-name` (`lisp/faces.el:1031-1034`) → `make-font-instance`
(`src/fontcolor.c:1148-1197`) → `initialize_font_instance`
(`src/fontcolor.c:1190-1191`) → `x_initialize_font_instance`, which returns
failure only when *both* `fs` and `rf` are NULL (`src/fontcolor-x.c:247-253`).
On Xft, `rf` is never NULL. **So `try-font-name` returns the name for any
string whatsoever on an Xft build.**

Three things follow:

1. **`x-font-create-name`'s "did we find a good font yet?" loop always exits
   on the first candidate family.** `lisp/font.el:666-705`: the loop walks the
   family list, formats an XLFD for each, and probes it with
   `try-font-name` (`lisp/font.el:704`). On core X the probe filters out
   families that don't exist; on Xft it does not, so the *first* family always
   wins — even if it does not exist. So `x-font-create-name` on an Xft build
   returns an XLFD for the first family in the list, unconditionally.
2. **The XLFD is then routed to `XftFontOpenXlfd`** (14 hyphens,
   `lwlib/lwlib-fonts.c:84-88`), which looks up the *core X font path*. On a
   modern system with no core fonts installed, that fails, and
   `xft_open_font_by_name` falls back to `monospace`
   (`lwlib/lwlib-fonts.c:94-95`). **The requested family is silently lost.**
   This is the mechanism behind "the font you asked for is not the font you
   get" on an Xft build.
3. **`x-make-font-bold-core` (`lisp/x-faces.el:213-217`) always succeeds** with
   the "bold" candidate, because all three of its `try-font-name` probes
   succeed. It is only reached for XLFD-looking names
   (`lisp/x-faces.el:179-181`), which is the *correct* behaviour for the
   core-X path — but on an Xft build an XLFD-looking name is exactly the case
   where `xft_open_font_by_name` will fall back to monospace.

So: **`try-font-name` does behave differently on Xft than on core X.** On core
X it answers "does this font exist?"; on Xft it answers "yes", always. Any
change set that routes `font-create-name` through fontconfig on Xft must
*also* stop relying on `try-font-name` to validate the name it produces —
`fc-try-font` (`lisp/fontconfig.el:478-488`) is the correct probe, and the
`-xft` frobbers already use it (`lisp/x-faces.el:197`, `:203`, `:209`).

### 7.4 One handover fact needs correcting

The handover says "Xft accepts XLFDs: `xft_open_font_by_name` routes a
14-hyphen name to `XftFontOpenXlfd`". That is *literally* true
(`lwlib/lwlib-fonts.c:84-88`) but misleading in practice:

* The XLFD `x-font-create-name` emits has exactly 14 hyphens **only if**
  neither the registry nor the encoding field contains a hyphen. The default
  is `"*"` for both (`lisp/font.el:653-658`), which is fine; but a descriptor
  carrying `:encoding "iso8859-1"` produces 15 hyphens, and the name then
  silently takes the `XftFontOpenName` path instead (`lwlib/lwlib-fonts.c:84-90`).
* "Accepts" is too strong: `XftFontOpenXlfd` succeeds only if the *core X
  server* has a matching font. On a modern system it usually does not, and
  `xft_open_font_by_name` then substitutes `monospace`
  (`lwlib/lwlib-fonts.c:94-95`). So an XLFD is *parsed* by Xft but not
  *resolved* by it on a system without core fonts.

### 7.5 A second handover fact needs correcting (minor)

The handover says `font-list` on an `x` device "returns core X server font
names, which are absent on a modern system". That is right in substance. But
note that the *core-X* font-menu builder is not the thing that reads
`font-list` on an Xft build — `x-reset-device-font-menus`
(`lisp/x-font-menu.el:101-105`) switches to the fontconfig-based
`x-reset-device-font-menus-xft` (`lisp/x-font-menu.el:124-157`), which never
calls `font-list`. So the Xft menu is *already* fontconfig-based; the broken
`font-list` is invisible to it.

---

## 8. `font-window-system-mappings`: the `fc` and `pm` rows and the missing `msprinter` row

The table (`lisp/font.el:108-120`):

```elisp
(defconst font-window-system-mappings
  '((x         . (x-font-create-name x-font-create-object))
    (gtk       . (x-font-create-name x-font-create-object))
    ;; #### FIXME should this handle fontconfig font objects?
    (fc        . (fc-font-create-name fc-font-create-object))
    (mswindows . (mswindows-font-create-name mswindows-font-create-object))
    (pm        . (x-font-create-name x-font-create-object)) ; Change? FIXME
    ;; #### what is this bogosity?
    (tty       . (tty-font-create-plist tty-font-create-object)))
```

### 8.1 The `fc` row — **delete it**

* `fc-font-create-name` and `fc-font-create-object` are **not defined
  anywhere in the tree**. Grep for `\bfc-font-create-(name|object)\b` across
  `lisp/`, `src/`, `modules/`, `docs/` returns exactly one hit: the row
  itself (`lisp/font.el:112`).
* `fc` is **not a console type**. The console types defined in the tree are
  `dead` (`src/console.c:97`), `gtk` (`src/console-gtk.c:38`), `mswindows`
  (`src/console-msw.c:40`), `msprinter` (`src/console-msw.c:41`), `tty`
  (`src/console-tty.c:42`), `stream` (`src/console-stream.c:41`), and `x`
  (`src/console-x.c:40`).
* The row is therefore **unreachable**: `font-create-name`/`font-create-object`
  look up `(device-type device)` (`lisp/font.el:381-382`, `:388-389`), and no
  device can have type `fc`.
* The comment at `lisp/font.el:111` — "#### FIXME should this handle
  fontconfig font objects?" — is a note to a hypothetical future `fc` device
  type that was never built.

**Recommendation: delete the `fc` row** (and the FIXME comment). The
fontconfig objects it gestures at *do* exist — `fc-pattern-*` in
`src/font-mgr.c:231-371`, `fc-font-match` (`src/font-mgr.c:1003`),
`fc-list-fonts-pattern-objects` (`src/font-mgr.c:1054`) — but they are a
*different representation* from the 12-element descriptor, and the tree
already has a working bridge between the two:
`xft-font-create-object` (`lisp/font.el:786-808`) and `xft-font-create-name`
(`lisp/font.el:831-837`). Making the `x` row dispatch to those (per §2.2) is
the real fix; the `fc` row adds nothing.

### 8.2 The `pm` row — **delete it**

* `pm` is **not a console type** (same list as above). There is no `pm`
  console, no `pm` device, and no `pm` backend anywhere in the tree.
* The only other reference to `pm` as a device type is in
  `font-normalize-color` (`lisp/font.el:1264-1266`):

  ```elisp
  (case (device-type device)
   ((x pm)
    (apply 'format "#%02x%02x%02x" (font-color-rgb-components color)))
  ```

  — the same dead `pm`. (`ns`, on the next branch at `lisp/font.el:1274-1277`,
  is likewise not a console type: there is no NeXTSTEP backend in this tree.
  The handover's "no macOS/Cocoa/NeXTSTEP backend" is confirmed.)
* The row's own comment — `; Change? FIXME` (`lisp/font.el:114`) — is the
  author already flagging it.

**Recommendation: delete the `pm` row**, and delete the `pm` from the
`(x pm)` case in `font-normalize-color` (`lisp/font.el:1265`) at the same time,
so the two dead references go together. Leaving `pm` in `font-normalize-color`
is harmless (the `case` simply never matches) but it is a lie about what
device types exist.

### 8.3 The missing `msprinter` row — **add it**

* `msprinter` **is** a real console type (`src/console-msw.c:41`) and it
  inherits the whole font method set from `mswindows`
  (`src/fontcolor-msw.c:1664-1670`, including `font_list` at `:1668` and
  `find_charset_font` at `:1670`). It also has its own
  `initialize_font_instance` (`src/fontcolor-msw.c:1664`).
* The `make-face-*` frob mappings **already** have `msprinter` rows
  (`lisp/faces.el:1468`, `:1487`, `:1506`, `:1587`, `:1599`).
* But `font-window-system-mappings` has **no `msprinter` row**, so
  `(font-create-object name printer-device)` returns `nil`
  (`lisp/font.el:386-390`), and `(font-create-name descriptor printer-device)`
  returns `nil` (`lisp/font.el:379-383`).

  Concretely broken: `make-face-family` (`lisp/faces.el:1347`),
  `make-face-size` (`lisp/faces.el:1567`) and `make-face-height`
  (`lisp/faces.el:1618`, `:1640`) call `(font-create-object f d)` with `d`
  being the device. On an `msprinter` device that returns `nil`, and
  `(set-font-family nil family)` / `(font-size nil)` signal.

**Recommendation: add `(msprinter . (mswindows-font-create-name
mswindows-font-create-object))` to `font-window-system-mappings`**, next to
the `mswindows` row (`lisp/font.el:113`). This is a one-line change with no
downstream risk: `msprinter` currently gets `nil` from both functions, and
after the change it gets the same answers `mswindows` gets — which is
exactly what the C layer already does for `msprinter`
(`src/fontcolor-msw.c:1664-1670`).

### 8.4 Summary of the table's defects

| row | status | recommendation |
|---|---|---|
| `x` | live, but cannot distinguish core X from Xft | make it dispatch on `(featurep 'xft-fonts)` (§2.2) |
| `gtk` | live, but points at the *core-X* functions | give GTK its own row (Pango) (§6.1) |
| `fc` | **dead** — functions undefined, `fc` is not a console type | **delete** |
| `mswindows` | live | keep |
| `pm` | **dead** — `pm` is not a console type | **delete** (and drop `pm` from `lisp/font.el:1265`) |
| `tty` | live (`tty-font-create-plist` `lisp/font.el:481`, `tty-font-create-object` `lisp/font.el:477`) | keep |
| `msprinter` | **missing** — a real console type with a real font backend | **add** |
| `stream` | **missing** — a real console type (`src/console-stream.c:41`) with no font methods at all | out of scope here, but see §9.1 |

---

## 9. How the fix would be tested

### 9.1 In this repo's TTY-only build

The repo's build is TTY-only. `build/src/config.h` has `HAVE_X_WINDOWS`,
`HAVE_FONTCONFIG`, `HAVE_XFT`, `HAVE_GTK`, `HAVE_GTK2`, `HAVE_GTK3` all
`#undef`'d (`build/src/config.h:47-64`); `build/CMakeCache.txt` has
`XEMACS_WITH_X11:BOOL=OFF`, `XEMACS_WITH_GTK:BOOL=OFF`,
`XEMACS_WITH_FONTCONFIG:BOOL=OFF`. `XEMACS_WITH_XFT:BOOL=ON` is set but inert,
because `XEMACS_USE_XFT` requires `THIS_IS_X` as well as `HAVE_XFT`
(`src/console-xlike-inc.h:110`).

Confirmed against `./build/bin/xemacs --batch`:

```
device-list:        (#<stream-device on "stdio" 0x1>)
display-graphic-p:  nil
device-type:        stream
font-list "*":      nil
(featurep 'xft-fonts): nil     (featurep 'gtk): nil
(fboundp 'font-create-object): t
(fboundp 'font-set-face-font): nil
(fboundp 'font-create-name): nil
(fboundp 'make-font): nil
(fboundp 'set-face-font): t
```

Notes:

* **`font-list` returns `nil`, not `("normal")`.** The selected device's type
  is `stream`, and the `stream` console type has **no `font_list` method**
  (`src/console-stream.c:306-334` lists the stream methods; `font_list` is
  not among them), so `MAYBE_LISP_DEVMETH` returns `Qnil`
  (`src/fontcolor.c:1320-1321`). The `tty` console type *does* have one —
  `tty_font_list` returns `(list1 "normal")` (`src/fontcolor-tty.c:466-470`,
  registered at `src/fontcolor-tty.c:563`) — but the `tty` console is not
  the selected device in this build. So the ticket's "font-list → nil there"
  is right *for this build*, and for a slightly different reason than one
  might assume.
* **`font.el` is not loaded at startup** (`(featurep 'font)` → `nil`), so
  `font-window-system-mappings` is void until `(require 'font)`.
  `font-create-object` *is* autoloaded (`lisp/font.el:385`);
  `font-create-name`, `make-font` and `font-set-face-font` are **not**
  (no cookie, and the malformed cookie at `lisp/font.el:990`).

**What the TTY-only build can therefore test — and it is not nothing:**

* **Pure-Lisp logic**, which is where most of the change set lives:
  * the descriptor→name lowering is a pure function of the descriptor and the
    device type, so it can be unit-tested with a *fake* device — but the
    device must have type `x`/`gtk`/`mswindows` for the lookup to resolve,
    and in a TTY build there is no such device. **The lowering functions
    cannot be exercised in a TTY-only build as they stand**, because they
    all take a device.
  * what *can* be tested in a TTY-only build: that `font-create-name` /
    `font-create-object` return `nil` for an unknown device type
    (`lisp/font.el:383`, `:390`); that the descriptor accessors round-trip
    (`lisp/font.el:172-188`); that `font-combine-fonts` behaves
    (`lisp/font.el:392-435`); that `font-window-system-mappings` has no
    dangling rows (i.e. every function named in the table is `fboundp`) —
    **this is a genuinely useful regression test for §8** and it needs no
    device at all;
  * that `font-set-face-font` is reachable and its autoload cookie is well
    formed (§2.1);
  * that `set-face-font` accepts a 12-element descriptor and does *not*
    confuse it with the 1-element inheritance vector
    (`lisp/modeline.el:337` relies on the latter) — this can be tested on a
    TTY device, because the descriptor is recognised *before* any device
    lowering happens;
  * that `(font-list "*")` is `nil` on a stream device and `("normal")` on a
    tty device — i.e. that the change does not disturb the TTY path.
* **Anything that requires a device** — `font-list` returning fontconfig
  names, `font-create-name` producing a fontconfig name for an `x` device,
  `try-font-name` failing/succeeding on an Xft device — **cannot be tested in
  this build at all**. `fc-*` is unavailable (`HAVE_FONTCONFIG` undefined), so
  even the Lisp-level fontconfig helpers cannot run.

### 9.2 In a GUI build

The GUI build is where the change set is actually exercised. Two variants
matter, and they are *different* code paths:

* **Core-X build** (`XEMACS_WITH_X11=ON`, `XEMACS_WITH_XFT=OFF`):
  `xft-fonts` is not provided, `x-font-create-name` still emits XLFDs,
  `font-list` still returns core X server names, `try-font-name` is a real
  existence check. **Every existing behaviour must be unchanged.** This is
  the compatibility gate.
* **Xft build** (`XEMACS_WITH_X11=ON`, `XEMACS_WITH_XFT=ON`): the change set
  applies. The tests that matter:
  * `(font-list "*" x-device)` returns fontconfig names — and *no* name in the
    result should be an XLFD, and every name should be loadable by
    `(make-font-instance name device t)`.
  * `(font-create-name (make-font :family "Noto Sans Mono CJK SC" :size 14)
    x-device)` returns a fontconfig name (e.g. `"Noto Sans Mono CJK SC-14"`),
    and round-trips: `(font-family (font-create-object that-name device))`
    → `("Noto Sans Mono CJK SC")`.
  * `(font-create-name (make-font :family "…" :weight :bold :size 14) x-device)`
    returns a name with a bold weight, and
    `(font-weight (font-create-object name device))` → `:bold`. **This is the
    test that currently fails**, because `xft-font-create-name` drops weight
    (`lisp/font.el:831-837`).
  * `(font-create-name (make-font :family "…" :style <italic bit>) x-device)`
    keeps the slant.
  * `(font-create-name (make-font :family "…" :size "14pt") x-device)` — the
    size-as-string form must lower to 14 points, not to the literal string.
  * `try-font-name` must go back to meaning something: after the change,
    `(try-font-name "definitely-not-a-font" x-device)` should be `nil`, not
    the name back.
  * `make-face-family` / `make-face-size` / `make-face-height` must change
    the *rendered* font on an Xft device — today they produce an XLFD that
    silently becomes `monospace` (§7.3).
  * per-charset selection still works: set a face's font with a
    `two-dimensional`/`final` tag-set and check that a CJK character renders
    with the CJK font and ASCII with the Latin one. The repo's
    `docs/user-config/xft-chinese-font-configuration.el` is a ready-made
    end-to-end scenario (it already sets per-charset fonts through
    `set-face-font` with a tag-set,
    `docs/user-config/xft-chinese-font-configuration.el:104-105`).
  * the **font menu** must still populate: `x-reset-device-font-menus-xft`
    (`lisp/x-font-menu.el:124-157`) is already fontconfig-based and must not
    be disturbed by the `font-list` change.

* **GTK build** (`XEMACS_WITH_GTK=ON`): the `gtk` row of
  `font-window-system-mappings` (§6.1) and the GTK font menu
  (`lisp/gtk-font-menu.el:101-122`) need their own build; the Xft work does
  not touch them but will expose the `gtk` row's brokenness if the two are
  done in one change.

### 9.3 A practical testing shape for this repo

Given that the milestone's gate runs against a **GUI** build (ADR 0002,
`docs/adr/0002-cnfonts-xemacs-backend-via-specifier-tag-sets.md`, on the
`docs/adr-0002-cnfonts-shim-surface` branch), the natural
split is:

* **Pure-Lisp unit checks, runnable in the TTY-only build** — the
  `font-window-system-mappings` table is well-formed (every named function is
  `fboundp`; no row names a non-existent console type), the descriptor
  accessors round-trip, the descriptor form of `set-face-font` is accepted and
  distinguished from the inheritance-vector form, `font-set-face-font` is
  autoloadable. These need no device and no fontconfig.
* **Device-level checks, runnable only in the GUI build** — everything in
  §9.2.

---

## 10. What the change set breaks, in one table

| # | change | breaks | severity |
|---|---|---|---|
| 1 | `set-face-font` accepts the 12-element descriptor | `set-face-font` docstring (`lisp/faces.el:505-521`), `make-font-specifier` docstring (`lisp/fontcolor.el:60-72`), `man/lispref/faces.texi:409`, `man/xemacs/custom.texi:2179-2181`; `font-set-face-font` must be reachable (autoload cookie `lisp/font.el:990` is malformed) | low — docs + one character |
| 2 | `font-create-name` lowers to fontconfig on Xft | `make-face-family`/`-size`/`-height` (`lisp/faces.el:1349`, `:1569`, `:1634`, `:1654`) get fontconfig names — **correct**, but only if §2.2's four fixes land; `font-set-face-font` (`lisp/font.el:994`) | medium — the four fixes are mandatory, not optional |
| 3 | `xft-font-create-name` completed (`lisp/font.el:831-837`) | nothing today (it is dead code); it becomes load-bearing | low |
| 4 | `font-list` returns fontconfig names on Xft | nothing reachable: the two XLFD-parsing consumers are already unreachable on Xft (`lisp/x-font-menu.el:103-105`, `lisp/x-faces.el:505-509`) | **low** |
| 5 | `font-list` returns fontconfig names on Xft | *user-visible*: `set-frame-font` completion (`lisp/frame.el:989-993`) now offers fontconfig names; a user config with a core-X name in it will no longer be offered | low, and the right direction |
| 6 | `try-font-name` must mean something again on Xft | `x-font-create-name` (`lisp/font.el:704`), `x-make-font-bold-core` (`lisp/x-faces.el:215-217`) — must switch to `fc-try-font` (`lisp/fontconfig.el:478`) on Xft | medium — silently wrong fonts otherwise |
| 7 | delete the `fc` row (`lisp/font.el:112`) | nothing — the functions are undefined and `fc` is not a console type | none |
| 8 | delete the `pm` row (`lisp/font.el:114`) and `pm` from `lisp/font.el:1265` | nothing — `pm` is not a console type | none |
| 9 | add the `msprinter` row (`lisp/font.el:113`) | nothing — `msprinter` currently gets `nil` from both functions; after the change it gets the `mswindows` answers, matching the C layer (`src/fontcolor-msw.c:1664-1670`) | none |
| 10 | GTK gets its own `font-window-system-mappings` row | the `gtk` row currently points at the core-X functions (`lisp/font.el:110`), which is wrong for Pango; `lisp/gtk-faces.el:338` is broken as a result | medium — but separable from the Xft change |
| 11 | GTK per-charset fonts | requires `PangoFcFont` + `PangoCoverage` work that does not exist in the tree (`src/fontcolor-gtk.c:449-456`) | out of scope for the milestone |

## 11. The five-line gist

1. **The device-independent spelling already exists** — the 12-element
   `make-font` descriptor (`lisp/font.el:361-377`) — but the only function
   that accepts it (`font-set-face-font`, `lisp/font.el:991-1031`) is dead
   *and* unreachable (malformed autoload cookie, `lisp/font.el:990`), so the
   change set is mostly about **wiring, not inventing**.
2. **`font-create-name` is the piece that must change**: the `x` row of
   `font-window-system-mappings` (`lisp/font.el:109`) cannot distinguish core
   X from Xft, so on an Xft build it emits an XLFD that
   `xft_open_font_by_name` cannot resolve (`lwlib/lwlib-fonts.c:88`,
   `:94-95`) — the family is silently replaced by `monospace`.
3. **`xft-font-create-name` (`lisp/font.el:831-837`) is dead code and must
   be finished** — it currently drops the family list shape, the size string
   shape, the weight and the slant, so all four must be fixed before it can
   be the `x` row's implementation.
4. **The biggest compatibility risk is `try-font-name`, not `font-list`**:
   on Xft, `xft_open_font_by_name` never returns NULL
   (`lwlib/lwlib-fonts.c:94-98`), so `try-font-name` always succeeds and
   every probe that depends on it is vacuous; `font-list` itself is low-risk
   because its only XLFD-parsing consumers are already unreachable on Xft.
5. **GTK cannot do per-charset fonts today** —
   `gtk_find_charset_font` is a no-op (`src/fontcolor-gtk.c:449-456`) and the
   charset matcher is XLFD-shaped (`src/fontcolor-xlike-inc.c:122-215`); it
   would need `PangoFcFont`/`PangoCoverage` work, so it is a separate project
   and should not gate the Xft milestone.
