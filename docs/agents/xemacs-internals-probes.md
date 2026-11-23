# Checking which XEmacs you are actually running

Facts about trusting a measurement in this tree. Every entry below is a
confound that produced a wrong conclusion at least once; each heading is the
check to run, and the misreading to recognise.

## Which compiler is loaded

`-l bytecomp` resolves against `load-path`, and XEmacs prefers a `.elc` **only
when it is not older than its `.el`**. In a build directory that means a stale
`.elc` silently loses to the source, or the *dumped* compiler wins because the
feature is already provided. You then measure the old compiler and believe you
measured the new one.

Ask the running image, never the file you passed:

```elisp
(symbol-file 'byte-compile-lambda 'defun)   ; -> "bytecomp.elc" or "bytecomp"
(symbol-file 'byte-compile-lexbind-special-p 'defun)
```

`"bytecomp"` (no extension) means it loaded the *source*. Fix the build
directory rather than adding `-l` somewhere else.

## Loading a specific compiler without the trap

Copy the candidate next to a `.elc` built from it, so freshness cannot flip:

```
xemacs -batch -q -l /path/to/candidate.elc -f batch-byte-compile TARGET.el
```

A `candidate.el` sitting beside `candidate.elc` re-introduces the trap the
moment the `.elc` is older.

## Installed packages shadowing local sources

`xemacs-packages` sits on `load-path` ahead of the build's `lisp/`, so
`fsf-compat/cl-lib.elc` (or any packaged `.elc`) can win over the source file
under test. When a test depends on a file being loaded from *this* tree, pin the
build `lisp/` directory to the front of `load-path` in the test setup
(`tests/automated/lexbind-p17-twelve-tests.el` does this).

## Stale `.elc` across a branch switch

`build/lisp/*.elc` survives `git checkout`. Files that were byte-compiled on the
previous branch keep being loaded, so a test can fail (or pass) on a tree whose
sources say otherwise. After any branch switch that changes what compiles:

```
rm -f build/lisp/*.elc-for-the-files-you-changed build/lisp/.copied
cmake --build build
```

Symptom to recognise: a failure whose assertion mentions a *loading* property
(closures, interpreted-vs-compiled) rather than the behaviour you changed.

## Does a measurement actually reach the string in question?

Before trusting a fail/pass on file content, confirm the bytes you think are
there. Two cheap swerves caught a whole misdiagnosis:

- **Mutation both ways.** Blank the suspected trigger and re-measure; restore it
  and re-measure. A trigger that does not make the result appear and disappear
  is not the trigger. (The real one needed an ISO-2022 designation *and* a
  ≥3-byte UTF-8 character *and* prior reader activity.)
- **Truncated-prefix bisect.** Load a growing prefix of the file with the
  truncation error swallowed, and watch the symptom appear:

```elisp
(condition-case nil (load "/tmp/prefix.elc" nil t) (error nil))
```

Then binary-search the byte offset. This found the trigger inside a docstring
without a single rebuild.

## Instrument with one tagged probe, then delete it

`#ifdef DEBUG_xxxx` around the probe, the tag in every line it prints, and the
first thing it prints is the *previous* value — stale-buffer bugs are invisible
unless you log what was there before. `[DEBUG-cc1]` in `read_atom_0` printed
`len`, the width walk, the rewind return code, the bytes, and the previous atom,
and named the cause in one run after hours of file-level experiments.

Delete it afterwards (`grep` the tag) or keep it as a `.patch` outside the tree.

## A loop beats a theory

A 0.4 s load-and-check command is worth more than any amount of reading: it
converts every hypothesis into a one-line experiment. If a bug resists a second
look and you do not yet have such a command, build it first — the two wrong
diagnoses in the #157/#164 session both happened before the loop existed.
