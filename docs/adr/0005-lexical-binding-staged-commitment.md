# Build lexical binding: staged commitment, GNU-shaped, byte-compiled

XEmacs has no lexical binding at any layer, and that blocks vendoring real GNU packages
(`ctable`, `fix-word`, `memoize`; the twelve `lisp/` files that already declare
`-*- lexical-binding: t -*-`). We decided: **build real, byte-compiled lexical binding**,
committing **stage by stage with risk gates** rather than as one up-front bet — and we
rejected staying deferred, an interpreter-only subset, and a shim/translator, each on
measured grounds (map [#128](https://github.com/fktpp2022/xemacs-git/issues/128)).

## The rule (ticket [#138](https://github.com/fktpp2022/xemacs-git/issues/138))

**Staged commitment with risk gates.** GO starts P1 unconditionally: the interpreted
GNU-complete subset (one atomic changeset per [#144](https://github.com/fktpp2022/xemacs-git/issues/144),
per-file opt-in via `load-lexical-files`, blast radius opt-in-tiny, measured floor
~350–450 lines across ~10 files from the working spike on branch
`prototype/lexbind-capture-spike`, ticket [#136](https://github.com/fktpp2022/xemacs-git/issues/136)).
P2 (cconv rewrite + compiler env slot + `;ELC` bump) commits only at its named decision
point **if P1's gates stayed green**; P2→P3 has the same structure. Not an up-front
absolute ceiling (undiscriminating at this floor size), not an up-front perf budget
(unmeasurable today).

**Risk gates**, folded into the P2/P3 decision points: every phase stays within proven
bug classes (GC-protection only — the one class the spike found, a real segfault in
`Flet`'s varlist); no phase requires an undumpable environment; bootstrap gates stay
green. **Stay-deferred trip-wires**, re-decided only at the P2 gate, never re-litigated
from scratch: (i) P1 bootstrap fails repeatedly on the same out-of-class bug; (ii) P2's
cconv-rewrite floor exceeds 2× the interpreted floor. Performance: a benchmark ticket
runs inside P2 **before** the `;ELC` bump ships; go-if-within-budget is a P2-gate check.

## Considered options

- **Stay deferred** (the standing decision of [#33](https://github.com/fktpp2022/xemacs-git/issues/33)
  and [#51](https://github.com/fktpp2022/xemacs-git/issues/51), both now citing this ADR):
  rejected — the vendoring need is real and measured, not speculative, and the route is
  now priced at a few hundred lines, not a rewrite.
- **Interpreter-only lexical binding**: rejected on four measured grounds (map Out-of-scope
  section). This fork byte-compiles everything and prefers `.elc`
  (`lisp/update-elc-2.el:96`, `lisp/elpa.el:1007-1022`, `src/lread.c:566-569`) with no
  `no-byte-compile` opt-out; the acceptance set needs **mutable** capture
  (`lisp/subr-x.el:20`, ctable's `rest-rows` and recursive `get`); three of the twelve
  lexbound files are dumped (`lisp/dumped-lisp.el:309-311`), so the interpreter still pays
  the closure-object and `Feval`-environment cost while forcing source loads that trip
  `load-warn-when-source-only` (`lisp/loadup.el:104-105`); and no released GNU ever
  shipped the split. Note ([#132](https://github.com/fktpp2022/xemacs-git/issues/132)):
  the interpreter half is much cheaper than first assumed, so the rejection rests on the
  compiled-half evidence, not the runtime tax.
- **No-op `lexical-binding` shim or source translator**: rejected — false compatibility
  that misbehaves only at the user's runtime; Neubauer's mechanical-conversion effort was
  never finished.
- **Port GNU's code** (`cconv.el` as-is): rejected — it is bound to GNU's `internal-` IR
  and opset; the *algorithm* transfers, the *code* does not
  ([#131](https://github.com/fktpp2022/xemacs-git/issues/131)). We port GNU's
  *architecture*: separate closure conversion, environment carried in the compiled
  function, GNU's `internal-interpreter-environment` shape — re-implemented against
  XEmacs's own compiler and VM.

## Consequences

- Standing constraints, not to be reopened: no backwards `.elc` compatibility — bump
  `byte-compile-version` / the `;ELC` cookie (`src/lread.c:700-707`) and require a full
  rebuild exactly at P2, the only layout-changing phase; vendored/already-declaring files
  only, **core XEmacs lisp is not converted**; `load-lexical-files` is a documented
  XEmacs-only deviation with a cconv-tied sunset ([#144](https://github.com/fktpp2022/xemacs-git/issues/144)).
- The strategy is not settled once and for all: it is settled **per gate**. The gate
  checklist that fires the trip-wires lives in
  [`docs/planning/lexbinding-milestone.md`](../planning/lexbinding-milestone.md), which is
  the phase-by-phase spec and the implementation hand-off.
- Predecessor decisions stay settled: [#33](https://github.com/fktpp2022/xemacs-git/issues/33)
  deferred lexical binding as its own effort and [#51](https://github.com/fktpp2022/xemacs-git/issues/51)
  set the behavioural boundary (`ctable`, `fix-word` the named exclusions); this ADR
  supersedes the *deferral*, not the boundary.
