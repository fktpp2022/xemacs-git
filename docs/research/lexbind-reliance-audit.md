# Classify the lexical reliance of the acceptance set

Ticket: #133. Date: 2026-11-05. Re-landed onto the mainline under wayfinder ticket [#141](https://github.com/fktpp2022/xemacs-git/issues/141); previously only on the throwaway branch `research/lexbind-reliance-audit` (commit `43b310b62`).

This is a measurement, not a plan: it records the minimum lexical semantics the
acceptance set *actually* relies on when run stock and byte-compiled. It
recommends no implementation.

Categories (from #133):

| | meaning |
|---|---|
| (a) | runtime variable capture — a lambda reads a binding of an enclosing form; harmless if the application is immediate, load-bearing if the closure escapes its binder |
| (b) | **mutable** capture — `setq`/`push`/`incf` inside a closure that mutates an enclosing binding the outer scope (or another closure) must see |
| (c) | local **function** binding — `cl-labels` / `cl-flet` |
| (d) | **expansion-time environment** — `&environment`, or macros that inspect the caller's bindings |
| (e) | declares the `-*- lexical-binding: t -*-` header but nothing in the file needs it |

Scope:

* the twelve `lisp/` files that declare the header (verified with
  `grep -l 'lexical-binding: t' lisp/*.el` — exactly the twelve counted during
  charting: `tabulated-list.el`, `macroexp.el`, `subr-x.el`, `cl-lib.el`,
  `bind-key.el`, `use-package.el`, `use-package-core.el`,
  `use-package-bind-key.el`, `use-package-delight.el`, `use-package-diminish.el`,
  `use-package-ensure.el`, `use-package-xemacs.el`);
* the three upstream packages excluded on closure grounds — `ctable` and
  `fix-word` by #51 (rule 5 names them with the observed errors
  `void-variable buffer` / `void-variable fnc`; see
  `docs/research/melpa-m2-audit.md` lines 91-92, 120-125 on branch
  `research/melpa-m2-audit`) — and `memoize`, the #33 witness;
* every guard in the tree that would behave differently once `lexical-binding`
  is a defined variable.

Upstream sources were fetched at the commits behind the pinned builds
(`gh`-visible versions from #51 and `tests/automated/elpa-tests.el:1312-1338`):

| package | pinned build | source |
|---|---|---|
| `ctable` 20210128.629 | #51 item 5 | github.com/kiwanami/emacs-ctable @ `48b7374` (2021-01-28, HEAD) |
| `fix-word` 20260802.938 | #51 item 5 | github.com/mrkkrp/fix-word @ `e29889f` (2026-08-02, HEAD) |
| `memoize` 20220718.1430 | elpa-tests.el:1312-1338 | github.com/skeeto/emacs-memoize @ `51b0759` (v1.1, HEAD) |

None of the three is vendored in this tree. The `memoize` test
(`tests/automated/elpa-tests.el:1312-1338`) only asserts that the allowlist
*refuses* `memoize` before any download; nobody has ever run `memoize` on this
XEmacs, so its classification below is source-audit evidence, extending the
executed evidence (#46's `void-variable buffer` / `void-variable fnc`) that the
other two have.

## Per-file classification, vendored core

### lisp/macroexp.el — (e)

Header at line 1. The file is one function, `macroexp-progn` (16-20), a pure
list operation with zero free-variable references. Nothing in it needs lexical
binding.

### lisp/tabulated-list.el — (e)

Header at line 1. The sorter was hand-refactored in PR #74 specifically to
avoid closures; the comment says so (`lisp/tabulated-list.el:79-83`, and PR #74:
"sorting goes through named functions instead of closures, because XEmacs
lambdas are dynamically scoped and cannot capture the sorter"). Instead of
captures it uses two global specials, `tabulated-list--sort-column-index` and
`tabulated-list--sorter` (84-85), set by `tabulated-list--get-sorter` (102-117)
and read by the named predicates `tabulated-list--default-sorter` /
`--selected-sorter` / `--sort-reverse` (87-100) during the immediately-following
`sort` (179). The only lambdas in the file are those named functions themselves,
which capture nothing. Zero (a-e) reliance.

### lisp/subr-x.el — one (b); rest (e)

Header at line 1. `hash-table-keys` (17-21) is a textbook mutable capture:
`(let (keys) (maphash (lambda (key _value) (push key keys)) hash-table)
(nreverse keys))` — the lambda at line 20 `push`es into the enclosing `let`
binding (19) and the outer scope reads the result (21). The application is
immediate (inside `maphash`), so the pattern happens to work under dynamic
semantics too; as a *classification* it is (b). Everything else in the file
operates in a single scope: `string-trim` (33-43) `setq`s its own parameter
(40, 42) with no closure involved, and `if-let*`/`when-let*` (48-58) are macro
wrappers that inspect nothing.

### lisp/cl-lib.el — (e) itself; hosts the set's only (c) machinery

Header at line 1. The file is almost entirely aliasing macros and functions that
capture nothing; its own runtime needs nothing lexical. Its significance is the
two local-function-binding shims:

* `cl-labels` (416-426) forwards to XEmacs's `labels` (defined at
  `lisp/cl-macs.el:4117` via `defmacro*` with `&environment`, expanding into a
  `lexical-let` + gensym closure machinery usable only in interpreted code; the
  byte compiler has its own `labels` in `byte-compile-initial-macro-environment`,
  per the comment at cl-macs.el:4153-4155). Before forwarding, cl-lib guards on
  `(and (boundp 'lexical-binding) lexical-binding)` (418) and `message`s "This
  `cl-labels' requires `lexical-binding' to be non-nil" (424).
* `cl-flet` (435-440) is missing from the vendored cl-lib 0.7.1 and is shimmed
  to cl-macs `flet` (lisp/cl-macs.el:4166-4197), i.e. dynamic `letf*` on
  `symbol-function` cells.

Nobody in the acceptance set calls `cl-labels`; only `bind-key.el` calls
`cl-flet` (below). No file in the set uses `&environment` — the only
`&environment` consumers in the tree are cl-macs internals
(`lisp/cl-macs.el:357, 1667, 1681, 1707, 4117`), i.e. the machinery the shims
forward into, not the set's own code. (d) is empty for the whole acceptance set.

### lisp/bind-key.el — (c) + (a) immediate; the rest capture-free

Header at line 1.

* (c): `cl-flet` at 366-374 binds the local function `wrap` inside
  `bind-keys-form`. `wrap`'s body reads `pkg` (368, 373) — an (a) capture
  riding inside the (c) form — and is applied immediately, non-recursively, only
  from the `flet` body. The dynamic `flet` shim satisfies this exactly (the
  comment at cl-lib.el:431-434 notes the divergence is confined to recursive or
  closed-over *function* references, which bind-key does not have).
* (a): the `cl-mapcan` lambda at 392-403 captures five lexicals — `prefix-map`
  (394, 395), `filter` (395, 402), `map` (396, 402), `repeat-map` (400, 401),
  `repeat-type` (400) — all read-only, applied immediately over `first` (404).
* capture-free escapes and no-capture sites: the keymap menu `:filter` lambda
  emitted at 209-211 escapes into a keymap but every outer name is
  comma-inserted, so it closes over nothing; likewise the emitted body at
  200-206. `bind-key--remove`'s two transients (230, 240) and the
  `describe-personal-keybindings` sort lambda (520-522) bind only their own
  parameters.

### lisp/use-package.el — (e)

Header at line 1. Loader glue; contains no lambda at all.

### lisp/use-package-core.el — (a) immediate + one (b); the charted "mutable capture" is capture-by-symbol

Header at line 1.

* (b): `use-package-normalize-pairs` (894-905) is a second subr-x-shaped
  mutable capture: `last-item` is `let`-bound at 895, read at 902 and `setq`
  inside the mapcar lambda at 904 — the cell must persist across successive
  applications of the closure within one call. Immediate application, so it
  works dynamically; as a shape it is (b).
* the charted memoize site is *not* a lexical capture. `use-package-memoize`
  (790-800) emits `(defvar ,next #'(lambda () (if ,loaded ,result (setq ,loaded
  t ,result ,arg))))` (798-799): the two "outer cells" the closure `setq`s are
  gensyms (793-795) that the backquote inserts **as symbols**, and 796-797 emit
  them as global `defvar`s. At runtime the closure references global specials,
  not an enclosing lexical frame — verified: nothing in the emitted form closes
  over a lexical binding. It contributes nothing to the lexical requirement;
  the chart's "mutable capture" label describes the idiom's intent, not its
  compiled semantics.
* (a) immediate: the normalization/handler helpers build closures over their
  own `let`/argument bindings and apply them in the same call, during
  macroexpansion — `use-package-handle-mode`'s lambda reads `alist` (981-991,
  lambda 986-990), the `:hook` handler's inner `sym` lambda reads `fun`
  (1313-1332, lambdas 1318/1323), the `:commands`/`:autoload` handlers' lambdas
  read `state` and `name-string` (1340-1356, 1362-1378, lambdas 1346/1368),
  `use-package-require-after-load`'s lambda reads `body` (1421-1437, lambda
  1430), and the `:custom` handler's lambdas read `name` (1472-1501, lambdas
  1486/1495) — all read-only, all applied immediately (`mapcar`/`cl-mapcan`)
  under the dynamic extent of their binders.
* parameter-only transients — lambda bodies that bind nothing from an enclosing
  form: 951, 967-973, 1293-1306 (normalize/autoloads helpers), 1460-1470
  (`use-package-normalize/:custom`), the normalization wrappers around
  819-872. The `:catch` warning lambda emitted at 1231-1246 is capture-free by
  construction: `name` is comma-quoted in (1232) and `use-package--form` is
  evaluated at expansion time (1239); the result is installed through a gensym
  `defvar` (1225, 1230), a global at runtime.
* capture-free: `use-package-defaults` (180-190) and `use-package-merge-key-alist`
  (221-224) lambdas bind only their parameters and read globals; the defcustom
  `:set` function (320-328) closes over nothing (outer names are comma-inserted
  at compile time); the keyword-sort lambda (593-594) and the statistics column
  sorters (1072-1093) bind only their arguments and read global
  `use-package-statistics` / `use-package-statistics-status-order` — note the
  1072-1093 lambdas *escape* into `tabulated-list-format` and are funcalled
  later (through tabulated-list's named sorter, lisp/tabulated-list.el:96), yet
  still capture nothing, so they are dynamically safe.
* no `cl-labels`, no `cl-flet`, no `&environment` anywhere in the file.

### lisp/use-package-bind-key.el — (a) immediate

Header at line 1. 148-152 and 169-170: mapcar lambdas capturing `bind-macro`+`name`
and `override`+`name` respectively, applied immediately during expansion. The
emitted keymap-autoload command (172-177) escapes but comma-inserts everything —
at runtime it references only `use-package-autoload-keymap` and constants.
61-63 binds only its parameter.

### lisp/use-package-delight.el — (e)

Header at line 1. No lambda in the file.

### lisp/use-package-diminish.el — (a) immediate

Header at line 1. 49-51: mapcar lambda capturing `name`+`label`, applied
immediately. 65-70 binds only its parameter.

### lisp/use-package-ensure.el — (e)

Header at line 1. The lambdas at 79-85 and 138-145 bind only their parameters
and are applied immediately; the `:ensure` default predicate appended at
200-203 closes over nothing (`use-package-always-ensure` is a global). Zero
free-lexical captures: the header is inert for this file.

### lisp/use-package-xemacs.el — (e)

Header at line 1; 94 lines; no lambda at all. Its commentary (line 10) names
`cl-flet` as compatibility provided "in the core files proper" — that is the
cl-lib.el:435-440 shim, confirming the fork treats the (c) need as a cl-lib
concern, not a use-package one.

## Per-file classification, upstream packages

### memoize.el — pure (a), escaping; the #33 witness confirmed

`memoize--wrap` (memoize.el:91-111): `(let ((table ...) (timeouts ...)) (lambda
(&rest args) ...))` — the lambda at 99-111 *is the return value* (installed via
`fset` at memoize.el:76 in the symbol branch, returned to the caller at 78 in
the function branch), so it outlives the `let` frame. Captures:
`table`, `timeouts` (97-98), `func`, `timeout` (91). All mutations go *through*
the captured hash-table objects (`puthash`/`remhash`); neither captured binding
is ever `setq` — so this is (a), not (b). The nested timer callback (110-111)
captures the *invocation's* `args` and `table` and escapes again.
`memoize-by-buffer-contents--wrap` (134-175) repeats the pattern over three
tables (164-166). `defmemoize` (113-120) is a plain macro emitting `defun` +
`memoize`; it inspects no environment — not (d). #33's "closes over a
`let`-bound hash table" is verified at 97-99. Non-lexical caveat: the timeout
arm calls `run-at-time` (107-110), absent from stock XEmacs 21.5 without the
milestone-1 `timer` shim (the kit listed in melpa-m2-audit's Method) — that is
an API gap, not a capture; with an infinite timeout (`memoize-default-timeout`
nil) the closure body never touches it.

### fix-word.el — (a), escaping

`fix-word` (fix-word.el:62) is a factory: it returns the lambda at 100-108,
which reads `fnc` — a *function argument* whose binding ends when `fix-word`
returns. `fix-word-define-command` (167-173) evaluates `(fix-word ,fnc)` at
load/compile time and `defalias`es the resulting command; when the command later
runs, only dynamic scope would answer `fnc` — hence #46's observed
`void-variable fnc` (melpa-m2-audit.md:92, 120-125). The helpers
`fix-word--fix-region` / `--fix-and-move` / `--fix-quickly` / `--transform-word`
(110-165) thread `fnc` as an explicit parameter — no capture. No (b)/(c)/(d).

### ctable.el — (a) dominant (escaping), two genuine (b), no (c)/(d)

* (a) escaping — the library's architecture is "build closures over local
  bindings and store them in structs/keymaps":
  `ctbl:dest-init-buffer` (344): `:clear-func` lambda (367-369) over `buffer`
  (356) — this is the executed `void-variable buffer` failure
  (melpa-m2-audit.md:91, 121-123); `ctbl:dest-init-region` (377): `:min-func`/
  `:max-func` (390-391) and `:clear-func` (396-397) over `mark-begin`/`mark-end`;
  `ctbl:dest-init-inline` (410): `:clear-func` (425-427) over `buffer` (413);
  `ctbl:cp-update` (572): response callback (592-597) over `cp`;
  `ctbl:render-column-header-keymap` (878): mouse handler (883-886) over
  `col-id`, installed into a keymap; `ctbl:render-get-formats` (1038): format
  closures (1044-1051) over `cw`; `ctbl:render-async-main` (1428): success/
  failure callbacks (1438-1470) over `buf`/`dest`/`model`/`param`/`cmodels`;
  `ctbl:render-async-continue` (1479): callbacks (1493-1520) over
  `cp`/`astate`/`begin-index`/`dest`/`model`; `ctbl:sort` (1688-1723): nested
  comparator factories over `ref`/`f`/`cp`/`fs`; `ctbl:create-table-component-region`
  (1810): `after-update` lambda (1826-1833) over `keymap`/`cp` (1825), stored in
  the dest (1834).
* (b) with escaped, cross-closure mutation — two sites where the shared binding
  itself is `setq` and the holders escape:
  `ctbl:async-model-wrapper` (1538-1565): `rest-rows` (1540) is `setq` by the
  `:request` closure (1562) and re-`setq`/read by the `:reset` closure (1564);
  the two closures are installed in the async-model struct and communicate only
  through that one mutable binding. `ctbl:cursor-to-nearest-cell` (738-761):
  `get` is `setq` to a lambda (750-755) that recursively calls **itself through
  the `get` binding** (`(funcall get (cdr cmds))`, 754) — a late-bound mutable
  self-reference. (The four navigation lambdas 744-747 capture nothing.)
* (a) immediate: `ctbl:define-keymap` (184-198): the mapc lambda reads `map`
  and `prefix`; `map` is mutated structurally by `define-key`, never `setq` —
  (a), not (b). `ctbl:dest-ol-selection-set` (437): callback lambda (445-452)
  captures `col-id` (439; `tcell-id`/`begin`/`end` are its own parameters),
  applied immediately inside `ctbl:find-all-by-row-id` (700).
* capture-free escapes: tooltip timer lambda (1604-1607) and the buffer
  component's `:after-update-func` in `ctbl:create-table-component-buffer`
  (1749; lambda 1761-1763) close over nothing.
* no `cl-labels`, no `cl-flet`, no `&environment` anywhere in the file (grepped)
  — ctable needs (a)+(b) only. (Four `cl-defun` keyword-lambda entry points at
  1740/1749/1810/1841 are a kit/API concern, not a lexical-capture one.)

## Guards that change behavior when `lexical-binding` becomes a defined variable

A whole-tree search for references to the *variable* `lexical-binding` (mode-line
headers aside) finds exactly one site: the cl-lib.el:418 guard
`(and (boundp 'lexical-binding) lexical-binding)`, whose else-branch is the
`message` at cl-lib.el:424. There is no `(default-value 'lexical-binding)`, no
loader or byte-compiler reference to the symbol anywhere in `lisp/` or `src/`
(grep of `lisp/`, `tests/`, `src/`: only the cl-lib lines), and no test asserts
on the variable; `lisp/files.el:918-919` only documents that `lexical-let`
closures interpret. Consequences for the guard itself:

* Today the symbol is unbound everywhere, so the guard always takes the warning
  branch — harmless, because nothing in the tree calls `cl-labels`.
* If `lexical-binding` becomes a *globally defined* variable with default `t`,
  the guard is permanently satisfied — including for dynamic callers of
  `cl-labels`, which then silently expand into the `labels` fallback with no
  warning. The guard's purpose (cl-lib.el:419-423's own comment admits "the
  value of `lexical-binding' here is unreliable") fails in the false-negative
  direction.
* If it becomes defined with default `nil`, the guard keeps warning unless
  whatever defines the feature *binds* the variable non-nil during compilation
  of header-declaring files — i.e. the guard depends on a per-file dynamic
  binding, not on mere definition.

No other file in the tree reads the variable, so cl-lib.el:418-424 is the only
measured break-on-definition site.

## Aggregate

**The smallest subset that runs the whole acceptance set byte-compiled is
(a) + (b): runtime variable capture — including closures that escape their
binder — with mutable cells for `setq`-inside-the-closure.**

* (a) is load-bearing in three graded forms, all present in the set:
  immediate application inside the dynamic extent (subr-x, bind-key, the whole
  use-package family, ctable's `define-keymap`/`dest-ol-selection-set`) — which today's
  dynamic semantics already satisfy; capture-free *escaping* lambdas
  (use-package-core.el:1072-1093, use-package-bind-key.el:172-177,
  bind-key.el:209-211) — also satisfied today; and genuinely **escaping
  captures** (memoize.el:99-111/167-175, fix-word.el:100-108, and the ctable
  sites listed above) — *not* satisfied today; these are the measured floor,
  exactly the `void-variable` failures #46 executed.
* (b) is required by exactly four measured sites: subr-x.el:20 (`push keys`),
  use-package-core.el:895-905 (`last-item`), ctable.el:1540+1562+1564
  (`rest-rows`) and ctable.el:750-755 (recursive `get`). The first two are
  immediate applications that dynamic semantics coincidentally serve, but under
  an enabled header they compile to closure-over-mutable-cell code, so a byte
  compiler that implements (a) lexically must implement (b) to compile
  subr-x.el and use-package-core.el at all. ctable's two are genuine
  cross-escaping-closure mutation and need real cells.
* (c) is **not required**: no acceptance-set file calls `cl-labels`, and
  bind-key.el:366-374's `cl-flet` is served by the existing dynamic shim
  (cl-lib.el:435-440 → cl-macs.el:4166), whose documented divergence
  (recursion / closed-over function references) this set never exercises.
  (c) joins the floor only for packages *outside* the set that use `cl-labels`.
* (d) is **empty**: no file in the set (vendored or upstream) uses
  `&environment` or inspects caller bindings; every "looks like (d)" site —
  `use-package-memoize`, the emitted commands, the menu `:filter` lambda — turns
  out to comma-insert its context and close over nothing.
* (e) covers six of the twelve headers outright — `macroexp.el`,
  `tabulated-list.el`, `use-package.el`, `use-package-delight.el`,
  `use-package-ensure.el`, `use-package-xemacs.el` — plus `cl-lib.el`'s own
  runtime code. Half the acceptance "declares" lexical binding and needs
  nothing from it.

**Which files still fail (a)+(b):** none of the acceptance set fails on lexical
grounds once (a)+(b) exist — every reliance measured here is inside those two
categories, and the three executed/recorded closure failures (ctable's escaping
`buffer`, fix-word's escaping `fnc`, memoize's `let`-bound hash table) are the
canonical (a) shapes (ctable additionally the two (b) shapes). Caveats: (i)
`memoize` has never been run anywhere on this tree (elpa-tests.el:1312-1338
only tests refusal at install), so its pass is a source-audit conclusion; (ii)
non-lexical gaps (ctable's `header-line`, XEmacs display/API differences) are
outside this ticket — #51 rule 3 excluded these packages *on closure grounds*,
and only closure grounds are measured here.

**Reverting tabulated-list.el to upstream's closure changes the aggregate not
at all.** Upstream GNU's `tabulated-list--get-sorter`
(emacs-mirror/emacs master, lisp/emacs-lisp/tabulated-list.el:413-432) builds
two escaping (a) closures — the default sorter closing over the column index
`n` (lines 423-428) and the flip wrapper closing over `sorter` (line 431) — and
hands them to `sort`. Reverting therefore *adds* two escaping-capture sites
(lisp/tabulated-list.el:84-117 would shrink to match) to the already-required
category (a): no new (b)/(c)/(d) appears, so the minimum subset stays (a)+(b).
What flips is which files need it: as vendored, the file is (e) and its compiled
code is correct under today's dynamic semantics; reverted, it becomes the only
*dumped core* file (preloaded by `lisp/dumped-lisp.el`, per PR #74) whose
byte-compiled code is runtime-wrong without (a) — `tabulated-list--get-sorter`'s
returned lambda reads `n`/`sorter` after that `let*` frame is gone (the `sort`
runs in `tabulated-list-print`'s frame, lisp/tabulated-list.el:177-179), yielding
`void-variable n` — precisely the failure PR #74's refactor (the comment at
lisp/tabulated-list.el:79-83) exists to avoid.
