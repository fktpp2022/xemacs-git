# cnfonts's interactive surface on XEmacs

Research ticket: *which functions, variables and macros do upstream
`cnfonts.el` and `cnfonts-ui.el` (20251223.636) reference that this XEmacs
build lacks?* This note records the answer, the call sites, the data flow,
and how far a shim can carry each gap. It changes no code.

- Package under test: `/home/fankai/.emacs.d/elpa/cnfonts-20251223.636/`
  (`cnfonts.el` 835 lines, `cnfonts-ui.el` 672 lines), also vendored as
  `tests/elpa/melpa-fixture/cnfonts-20251223.636.tar`.
- Build under test: `/home/fankai/projects/xemacs/install/bin/xemacs`,
  `21.5 (beta36) "leeks" XEmacs Lucid`, GUI on `:0`, core Lisp taken from
  this tree's `lisp/`.
- The font half of cnfonts already has a shim: `lisp/cnfonts-xemacs.el`
  (`font-spec`, `x-list-fonts`, `list-fonts`, `font-family-list`,
  `string-join`, `set-face-attribute`, `set-fontset-font`, and a
  `gbk`→`cp936` coding alias). This ticket is about everything *above*
  that layer.

## Method

1. Read both files in full and collected every referenced symbol with a
   reader-based walker (`read` + recursive descent, tracking `defun`,
   `defvar`, `let`, `lambda` and `condition-case` binders) so locally
   bound names are not reported as dependencies. Run in the target build
   with `fboundp`/`boundp`.
2. Probed each candidate symbol individually after loading the libraries
   cnfonts actually loads (`cl-lib` from `fsf-compat`, `cus-edit`,
   `wid-edit`, `cnfonts-xemacs`), because a bare `-q` session has not yet
   loaded them and would report false gaps.
3. Ran a live GUI probe (`-q -l scratch.el`) that requires
   `cnfonts-xemacs`, `cnfonts`, then `cnfonts-ui`, and then calls
   `(cnfonts-ui)`, stubbing the newly missing symbol at each failure. The
   stubs were then exercised command by command, with `widget-move`
   wrapped in a counting guard so an unbounded loop reports instead of
   hanging.
4. Compared the GNU definitions against this tree's: GNU `subr.el`,
   `cus-edit.el`, `faces.el`, `wid-edit.el` from the `emacs-29` mirror;
   XEmacs `lisp/wid-edit.el`, `lisp/cus-edit.el`, `lisp/frame.el`,
   `lisp/subr.el`, `lisp/unicode.el`, `src/frame.c`, `src/keymap.c`,
   `src/eval.c`.

### The first-error chain (observed)

```
require cnfonts-ui        -> (void-function make-composed-keymap)        cnfonts-ui.el:93
+ make-composed-keymap    -> require OK; (cnfonts-ui) ->
                             (void-function face-valid-attribute-values) cnfonts-ui.el:198
+ face-valid-attribute-values ->
                             (void-function custom--initialize-widget-variables)
                                                                         cnfonts-ui.el:160
+ custom--initialize-widget-variables ->
                             (wrong-type-argument characterp 10102)      cnfonts-ui.el:75
+ char-to-string (Unicode) -> require OK; (cnfonts-ui) OK
```

Four shims carry the whole of `require cnfonts-ui` plus `(cnfonts-ui)`.
The remaining gaps are reached only from individual commands; the chain
continues:

```
+ widget-at arity         -> TAB/backtab, SPC and =/- work, but TAB on the
                             start page calls widget-move >60 times without
                             reaching the page's leading :tab-stop-point widget
+ widget-move semantics   -> forward navigation terminates
+ posn-at-point/-x-y, default-font-width/-height,
  set-mouse-pixel-position ->
                             the align page's [+]/[−] buttons complete
```

## The table

`LB` = load-bearing (on a failing path of a command that a user invokes),
`CO` = cosmetic (guarded, inert, or optional). Line numbers are
grep-verified against the installed package.

### Absent at load time or on `(cnfonts-ui)`

| Symbol | Kind | Call sites | XEmacs today | Failure | Class | XEmacs provision | Shim fidelity |
|---|---|---|---|---|---|---|---|
| `make-composed-keymap` | function | `cnfonts-ui.el:93` (`defvar cnfonts-ui-mode-map`, evaluated at load) | absent (`fboundp` nil) | `void-function`; `cnfonts-ui` never loads, so `cnfonts-edit-profile` (`cnfonts.el:818-819`) and all 19 UI commands are blocked | **LB** | `make-sparse-keymap` + `set-keymap-parents` (`src/keymap.c:854`), whose PARENTS list is searched in order | **faithful**: GNU 29 `subr.el:1041-1043` returns `(keymap MAPS...)`; a sparse map with those maps as ordered parents has the same lookup. Verified: `TAB`→`cnfonts-ui-forward`, `backtab`→`cnfonts-ui-backward`, `[mouse-1]`→`ignore` |
| `face-valid-attribute-values` | function | `cnfonts-ui.el:198` | absent; `:box` is not an XEmacs face attribute (`face-property 'default 'box` ⇒ nil), and neither `internal-lisp-face-attribute-values` nor `defined-colors` exists | `void-function` on entry to `(cnfonts-ui)`, before any page is built | **LB** | none | **approximate, exact at this call site**: the result is compared only with `'(("unspecified" . unspecified))` to choose raised vs bracketed buttons (`cus-edit.el:1683-1684` in GNU). Returning nil yields `custom-raised-buttons` = t, the GUI branch |
| `custom--initialize-widget-variables` | function | `cnfonts-ui.el:160`, body of `cnfonts-ui-mode`, i.e. once per page buffer | absent | `void-function` on the first `(cnfonts-ui-mode)`, so every page creation dies | **LB** | no function; XEmacs's `Custom-mode` does the same work inline (`lisp/cus-edit.el:3973-3976`). `widget-documentation-face`, `widget-button-face`, `widget-push-button-prefix/suffix`, `custom-button-face`, `custom-documentation-face` exist; `widget-button-pressed-face`, `custom-button-pressed-face`, `widget-button-click-moves-point` do not | **approximate** (visual only): 8 lines setting the buffer-local faces to the `-face`-suffixed XEmacs faces and clearing the push-button prefix under `custom-raised-buttons` |
| `char-to-string` | function, narrower domain | `cnfonts-ui.el:75-76` (ornament page `:note` lambda), `cnfonts-ui.el:422-423` (align page ornament row) | present (`lisp/subr.el:2263`) but it is `(string CHARACTER)`, and XEmacs characters are charset-encoded, not Unicode codepoints: `(char-to-string 10102)` and even `(char-to-string #x4E2D)` signal | `wrong-type-argument characterp 10102` while building the ornament page; `(cnfonts-ui)` pre-builds all pages, so the whole UI dies | **LB** | `unicode-to-char` (`lisp/unicode.el:79-87`, an FSF-compat wrapper) returns the character; `(char-to-unicode c)` is the inverse | **faithful** for 0..`#x10FFFF`: wrap the core function, converting integers > 255 through `unicode-to-char` and leaving characters alone (`integerp` is nil for XEmacs characters, so the test is safe). Range-guard invalid input, which GNU signals on and `unicode-to-char` may return nil for |

### Present but incompatible (the ticket's list called these missing)

| Symbol | Kind | Call sites | XEmacs definition | Failure | Class | Provision | Shim fidelity |
|---|---|---|---|---|---|---|---|
| `widget-at` | function, arity | `cnfonts-ui.el:351`, `:510`, `:622` | `lisp/wid-edit.el:1472` `(widget-at POS)` — POS required; GNU `wid-edit.el:1340` `(&optional pos)` | `wrong-number-of-arguments` for every zero-arg call: `cnfonts-ui-toggle-select-font` from `SPC`, `cnfonts-ui-increase/decrease-align` from `=`/`-`/`C-up`/`C-down`, and `cnfonts-ui-forward`/`-backward` | **LB** | GNU's own default is point | **faithful**: `(widget-at (or pos (point)))`. GNU also falls back to `widget-field-at`; XEmacs reads the `field` property directly, equivalent here (cnfonts uses no editable fields) |
| `widget-move` | function, algorithm | `cnfonts-ui.el:620`, `:624` | `lisp/wid-edit.el:1322` walks `widget-next-button-or-field`; at the end it wraps to `point-max`/`point-min` and then **restores `opoint` and gives up** when nothing tabable is ahead. GNU `wid-edit.el:1207-1231` advances one character at a time across the wrap and re-tests `widget-tabable-at` | no error. `cnfonts-ui-forward`'s `while` loop (`:621-624`) never sees the start page's only `:tab-stop-point` widget, which sits at `point-min` (`widget-at` scan: `1:TAB 2:no 4:no ...`), and calls `widget-move` indefinitely — observed 61 calls, stopped by the probe guard. `backward` terminates (the same page's trailing tab-stop is reachable), and every other page has a tab-stop ahead, so only forward-from-start hangs | **LB** (TAB on the start page) | `widget-tabable-at` exists and accepts the same optional POS; `next-overlay-change` does not (XEmacs has extents) | **approximate and risky**: either reimplement GNU's character-wise wrap scan on top of `widget-tabable-at` (same semantics, O(n) instead of overlay jumps) or add an explicit give-up in the caller. Either way it is a global widget function, not a cnfonts-local shim |
| `set-mouse-pixel-position` | function, first argument | `cnfonts-ui.el:187` (called with `(window-frame)`) | present, but `src/frame.c:2310` takes a **window** and window-relative pixels; GNU takes a frame | `wrong-type-argument windowp #<x-frame ...>` — reached only after the three geometry shims below | **LB** for the align page's button path | `(frame-selected-window frame)`; `window-pixel-edges` is the frame-relative anchor | **approximate**: accept a frame and map it to its selected window. The coordinates cnfonts computes from `posn-x-y` are window-relative, which is what XEmacs wants — so the wrapper is closer to correct than the upstream call, which mixes GNU's frame-relative contract with window-relative numbers |

### Absent, reached only from the align page's mouse repositioning

`cnfonts-ui--move-mouse` (`cnfonts-ui.el:183-190`) runs only when a page
rebuild was triggered by the align page's `[+]`/`[−]` buttons
(`:434-437`, `:445-448` set `cnfonts-ui--move-mouse`), via the macro tail
at `:153-154`. The font change and profile save have already happened by
then, so these gaps cost the mouse warp and the command's clean
completion, not the font.

| Symbol | Call site | XEmacs provision | Failure | Class | Shim fidelity |
|---|---|---|---|---|---|
| `posn-at-point` | `cnfonts-ui.el:185` | no `posn-*` at all (only `event-*`); nothing in `lisp/` or `src/` | `void-function` | **LB** (that action) | **approximate**: `window-pixel-edges` + `current-column` + `count-lines` from `window-start`, minus `window-hscroll` |
| `posn-x-y` | `cnfonts-ui.el:185` | absent | `void-function` | **LB** (that action) | **approximate**: the same computation returns `(cons x y)` |
| `default-font-width` | `cnfonts-ui.el:189` | absent; `font-width` on `(face-property 'default 'font)` answers 10 px in this build | `void-function` | **LB** (that action) | **approximate**: GNU measures the frame default font's space; the face's font is the frame default here |
| `default-font-height` | `cnfonts-ui.el:190` | absent; `font-height` answers 19 px | `void-function` | **LB** (that action) | **approximate**: same route |

### Absent but guarded, inert, or optional

| Symbol | Call sites | State | Class | Note |
|---|---|---|---|---|
| `after-make-frame-functions` | `cnfonts.el:235`, `:239` | absent; explicitly commented out as an XEmacs omission at `lisp/frame.el:667-672` | **CO** (silent degradation) | `add-hook` creates the variable and nothing ever runs it, so frames created after startup never get cnfonts fonts. Startup still works when the mode is enabled in init, because `window-setup-hook` is run once (`lisp/startup.el:599-601`) — but that code also `setq`s it to nil afterwards, so a later `cnfonts-mode` gets no hook either. XEmacs has no frame-creation hook; the nearest is `after-setting-font-hook` |
| `face-font-rescale-alist` | `cnfonts.el:441` (`setq`), option at `:93-98` | absent; `setq` creates it, nothing in XEmacs reads it | **CO** (option off by default) | XEmacs has no face rescale. With `cnfonts-use-face-font-rescale` non-nil, `cnfonts--set-font` (`:425-431`) shrinks the list to the Latin size and sets only that font, so the CJK size silently stops tracking. Not expressible as a variable shim; would need per-charset size arithmetic |
| `mouse-wheel-text-scale`, `text-scale-increase`, `text-scale-decrease` | `cnfonts.el:741-746`; the `cl-letf` at `:742-745` is only reached if the first exists | absent | **CO** | `functionp` guard falls through to a message. `mwheel-scroll` exists; no text-scale layer does |
| `touch-screen-pinch`, `touch-screen-aux-tool`, `(require 'touch-screen nil t)` | `cnfonts.el:46`, `:758`, `:759-772`, `:773` | absent | **CO** | noerror require and `functionp` guard; the key `(kbd "<touchscreen-pinch>")` at `:110` never fires |
| `widget-forward-hook` | `cnfonts-ui.el:617` | unbound — and also unbound in GNU 29, whose `wid-edit.el:1260` only runs it | **CO** | `run-hooks` on an unbound symbol is a no-op in XEmacs (`src/eval.c:5065-5091`, verified) |
| `insert-text-button`, `widget-forward`, `widget-backward` | — | present | — | **not referenced by either file** (see contradictions) |
| `widget-value`, `widget-value-set`, `widget-toggle-action`, `widget-apply`, `widget-get`, `widget-create`, `widget-insert`, `widget-setup`, `widget-keymap` | throughout `cnfonts-ui.el` | present with compatible signatures (`widget-keymap` is a variable) | — | verified in the live run: font toggling and alignment through the widget `:action` path complete |
| `cl-lib` and the `cl-*` macros (`cl-loop`, `cl-incf`, `cl-letf`, `cl-find-if`, `cl-remove-if`, `cl-remove-duplicates`, `cl-subseq`, `cl-position`) | `cnfonts.el:44` and throughout | present via the `fsf-compat` package (`locate-library` → `.../lisp/fsf-compat/cl-lib.elc`), all fbound after `(require 'cl-lib)` | — | the bare `-q` probe reports them missing only because the library is not yet loaded |
| `special-mode`, `special-mode-map`, `cus-edit`, `mwheel`, `tabulated-list-mode`, `string-join` | `cnfonts-ui.el:34`, `:156`; `cnfonts.el:45` | present | — | `special-mode` derives in `lisp/special-mode.el:45` |
| `font-spec`, `x-list-fonts`, `list-fonts`, `font-family-list`, `string-join`, `set-face-attribute`, `set-fontset-font`, `gbk` | `cnfonts.el:465-479`, `:547-611` | supplied by `lisp/cnfonts-xemacs.el` | — | the existing backend layer; `gbk` is aliased to `cp936` at `:50-53` |

## Commands and what blocks them

`cnfonts.el` defines 11 commands (`set-font`, `increase`, `decrease`,
`reset`, `mouse-wheel`, `touch-screen-pinch`, `switch-profile`,
`next-profile`, `edit-profile`, `regenerate-profile`, and `cnfonts-mode`);
`cnfonts-ui.el` defines 19 (10 explicit plus the 9 page commands generated
by `cnfonts-ui-create-page`). 30 command symbols in all.

| Command | Group | Blocked by |
|---|---|---|
| `cnfonts-edit-profile` | cnfonts | `make-composed-keymap` (via `require cnfonts-ui`), then the whole UI chain |
| `cnfonts-set-font`, `cnfonts-increase-fontsize`, `cnfonts-decrease-fontsize`, `cnfonts-reset-fontsize`, `cnfonts-switch-profile`, `cnfonts-next-profile`, `cnfonts-regenerate-profile` | cnfonts | nothing new — they run through the existing backend shim (verified) |
| `cnfonts-mode` / `cnfonts-enable` / `cnfonts-disable` | cnfonts | nothing fatal; `after-make-frame-functions` makes the per-frame half inert |
| `cnfonts-mouse-wheel` | cnfonts | `mouse-wheel-text-scale` — guarded message |
| `cnfonts-touch-screen-pinch` | cnfonts | `touch-screen-pinch` — guarded message |
| `cnfonts-ui`, `cnfonts-ui-restart`, `cnfonts-ui-next-page`, `cnfonts-ui-previous-page`, the 9 `cnfonts-ui-page-*` | UI | `make-composed-keymap`; `face-valid-attribute-values`; `custom--initialize-widget-variables`; `char-to-string` (ornament page, and the align page's ornament row) |
| `cnfonts-ui-mode` | UI | `custom--initialize-widget-variables` |
| `cnfonts-ui-forward` | UI | `widget-at` (arity), then `widget-move` (unbounded loop on the start page) |
| `cnfonts-ui-backward` | UI | `widget-at` (arity) |
| `cnfonts-ui-toggle-select-font` | UI | `widget-at` (arity) on the `SPC` path; the click path passes its widget and works once the page builds |
| `cnfonts-ui-increase-align`, `cnfonts-ui-decrease-align` | UI | `widget-at` on the `=`/`-`/`C-up`/`C-down` path; on the button path, `posn-at-point`, `posn-x-y`, `default-font-width`, `default-font-height`, `set-mouse-pixel-position` |

## Data flow of the load-bearing gaps

**Keymap.** `cnfonts-ui-mode-map` is built at load: `make-keymap`, then
`set-keymap-parent` to a composed map of `widget-keymap` and
`special-mode-map`, then `suppress-keymap`, then a dozen `define-key`s
(`cnfonts-ui.el:90-113`). `cnfonts-ui-mode` (`:156-161`) only installs it.
A load-time failure here is what makes the whole file unloadable; with the
`set-keymap-parents` shim the resolved bindings match the source.

**Page note text.** `cnfonts-ui-create-page` expands to a `defun` per page
(`:122-154`); `cnfonts-ui` (`:192-205`) calls each with `create-buffer` t,
so pages are built before the start page is shown.
`cnfonts-ui--create-fonts-page` (`:281-337`) funcalls the page's `:note`
when it is a function (`:288-291`). The ornament page's note
(`cnfonts-ui.el:68-78`) maps over `cnfonts-ornaments`
(`cnfonts.el:202-213`), a list of integer pairs, and calls
`char-to-string` on each end. The align page does the same at
`:417-424`. In XEmacs the integers are not characters, so the first
ornament pair (`#x2776 . #x2793`) aborts page creation.

**Widget plist keywords.** `:tab-stop-point`, `:font-name`, `:index`,
`:flag`, `:key`, `:page-name` are not widget-type keywords; XEmacs's
`widget-create` stores them in the widget plist like GNU does (verified:
`(widget-get w :tab-stop-point)` ⇒ t), which is why the navigation logic
in `cnfonts-ui-forward` (`:613-629`) and `cnfonts-ui-toggle-select-font`
(`:349-370`) is meaningful at all.

**Mouse warp.** Align page `[+]`/`[−]` action → `cnfonts-ui--move-mouse` t →
`cnfonts-increase/decrease-fontsize` (font applied through
`cnfonts-xemacs`) → page rebuilt with `create-buffer` t → macro tail calls
`cnfonts-ui--move-mouse` → `(posn-at-point (+ (point) 1))`, `posn-x-y`,
`default-font-width/-height`, `set-mouse-pixel-position
(window-frame) x y`. The failure lands after the rebuild, so the page and
the font are already correct when the error is signalled.

## Shim feasibility, summarised

- **Faithful, small, cnfonts-local:** `make-composed-keymap` (parent
  list), `widget-at` (optional POS), `char-to-string` (Unicode domain),
  `set-mouse-pixel-position` (frame→window), and the
  `custom--initialize-widget-variables` body (visual, but the XEmacs faces
  and prefix variables exist).
- **Exact only because of the call site:** `face-valid-attribute-values`
  — no `:box` attribute exists, but cnfonts only asks whether `:box` has
  exactly one "unspecified" value, to choose raised vs bracketed buttons.
- **Approximate:** `posn-at-point`, `posn-x-y`, `default-font-width`,
  `default-font-height` (composed from window edges, column/row and font
  metrics; correct for a monospace default font and a full-width window),
  and `custom--initialize-widget-variables`' face choices (XEmacs has no
  `custom-button-pressed`/`mouse` faces).
- **Approximate and global:** `widget-move`. Faithfulness needs GNU's
  character-wise wrap scan; doing it by redefining a function shared with
  every widget user is a wider blast radius than the other shims, and the
  caller (`cnfonts-ui-forward`) still wants a termination guarantee.
- **Not expressible as a shim:** `face-font-rescale-alist` (the option's
  effect is font-size arithmetic, not a variable) and
  `after-make-frame-functions` (XEmacs has no frame-creation hook; the
  closest is `after-setting-font-hook`, a different event).
- **No shim needed:** the guarded input paths, `widget-forward-hook`,
  `cl-lib` and the `cl-*` family, and the whole font backend.

## Where the ticket's starting facts were wrong or incomplete

- `widget-at`, `widget-value`, `widget-value-set`, `widget-toggle-action`
  and `widget-move` are **present**. Two of them are still blockers, but
  for different reasons: `widget-at` requires POS where GNU makes it
  optional, and `widget-move` gives up on wrap instead of scanning, which
  turns `cnfonts-ui-forward` into an unbounded loop on the start page.
- `widget-forward`, `widget-backward` and `insert-text-button` are
  **not referenced** anywhere in either file. The forward hook that is
  referenced is the variable `widget-forward-hook` (`cnfonts-ui.el:617`),
  which is unbound in GNU 29 too.
- Missing beyond the listed set: `posn-at-point` and `posn-x-y` (both on
  the same line as `default-font-width`), `after-make-frame-functions`,
  and the `char-to-string` codepoint domain. `set-mouse-pixel-position` is
  present but takes a window, not a frame. `widget-keymap` is present as a
  variable, not a function.
- Command counts: 11 commands in `cnfonts.el` (not 13), 19 in
  `cnfonts-ui.el`, 30 command symbols in total.
- The build's `cl-lib` comes from the `fsf-compat` package and is
  perfectly usable; a bare `-q` probe that never requires it will
  misreport the whole `cl-*` family.

## Reproduction

Probe one symbol at a time (functions with `fboundp`, variables with
`boundp`), after loading the libraries cnfonts loads:

```sh
DISPLAY=:0 /home/fankai/projects/xemacs/install/bin/xemacs -batch -q \
  -eval '(progn (setq load-path (cons "/home/fankai/projects/xemacs/xemacs-git/lisp" load-path)) (require (quote cus-edit)) (require (quote wid-edit)) (require (quote cl-lib)) (require (quote cnfonts-xemacs)) (princ (format "%S %S %S\n" (quote make-composed-keymap) (fboundp (quote make-composed-keymap)) (boundp (quote make-composed-keymap)))) (princ (format "%S %S\n" (quote widget-at) (fboundp (quote widget-at)))))'
```

The live chain (a GUI session; keep `kill-emacs` and a `condition-case`
around everything, because an uncaught error in a `-l` file leaves an
interactive XEmacs idle instead of exiting):

```elisp
;; /tmp/cnf/scratch.el
(setq standard-output 'external-debugging-output)
(setq load-path (cons "/home/fankai/projects/xemacs/xemacs-git/lisp" load-path))
(setq load-path (cons "/home/fankai/.emacs.d/elpa/cnfonts-20251223.636" load-path))
(condition-case err
    (progn (require 'cnfonts-xemacs) (require 'cnfonts)
           (require 'cnfonts-ui) (princ "REQUIRED-OK\n"))
  (error (princ (format "ERROR: %S\n" err))))
(kill-emacs 0)
```

```sh
DISPLAY=:0 timeout 60 /home/fankai/projects/xemacs/install/bin/xemacs \
  -q -l /tmp/cnf/scratch.el 2>&1 | grep -E "ERROR|REQUIRED"
# ERROR: (void-function make-composed-keymap)
```

Add the four shims from the first table one at a time and re-run; then
drive the commands. The probe harness used here is reproduced in
`/tmp/cnf/final.el` in this session and is short enough to rebuild from
the table: shims 1-4 plus `widget-at`, then `(cnfonts-ui)`, then each
`cnfonts-ui-page-*`, then `cnfonts-ui-forward` with a counting wrapper
around `widget-move`. Two details matter for a faithful probe:

- `standard-output` must be `'external-debugging-output`; in an
  interactive XEmacs `princ` goes to the echo area.
- A dynamic `let` around a `defun` does not close over the saved
  function in XEmacs; store the original in a `defvar` instead, e.g.
  `(defvar cnf--orig-widget-at (symbol-function 'widget-at))`.

## Limitations

No GNU Emacs binary is installed in this environment, so GNU behaviour is
taken from the `emacs-29` sources cited above rather than observed. In
particular, whether GNU's character-wise `widget-move` lands on the start
page's leading tab-stop widget (which XEmacs's does not) follows from the
source, not from a run. The XEmacs side is observed throughout.
