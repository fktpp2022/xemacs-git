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
