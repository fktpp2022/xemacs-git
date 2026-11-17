# Coding standards

Read at review, not while implementing. The `code-review` skill carries its own
smell baseline; the rules here are this repo's, and a repo rule overrides it.

## Recording a reversal in an ADR

An ADR under `docs/adr/` is a record of what was decided, so changing a claim it
pins is a new decision, not an edit to prose. Give it the form the file already
uses: an italic header above the body, naming the ticket that reversed it, what
now holds, and which paragraph below is rewritten.

```markdown
*Amended 2026-11-03 by [#118](https://github.com/fktpp2022/xemacs-git/issues/118):
`posn-at-point` no longer answers nil outside the displayed range. The
"Approximate" bullet below is rewritten accordingly.*
```

A body-only edit leaves the claim looking as it always did to anyone reading the
ADR for the first time.

## Unrooted windows in C

A freshly created or freshly read Lisp object held only in a C local is not
rooted; any call that can allocate (consing, `make_vector`, `Fmake_byte_code`)
may GC and sweep it before the caller stores it. Root it for exactly the window
between creation and the store:

- scalar: `NGCDECL1;` + `NGCPRO1 (x);` ... `NUNGCPRO;` (or `GCPRO1` when the
  caller's `g` is free), placed immediately before the allocating call and
  popped immediately after the value is linked into a rooted structure.
- array/stack: `GCPRO1_ARRAY (array, count)` / `GCPRO2_ARRAY (...)` — never
  hand-patch `gcpro1.nvars`.
- clearing the local (setting it to `Qnil`) before rooting is required when the
  local still holds a value freed earlier in the function (`free_list`,
  `free_cons`), and set dangling struct slots to `Qnil` before `UNGCPRO`.

Error signals restore `gcprolist` via `UNWIND_GCPRO_TO` (`eval.c`), so a
`CHECK_*` inside a `NGCPRO` window is safe.

The reader windows (`read_list_conser`, `read_vector`,
`read_compiled_function`) were the #157 use-after-free; the regression test
`tests/automated/elc-load-gc-tests.el` drives `gc-cons-threshold` low to force a
GC inside each window.
