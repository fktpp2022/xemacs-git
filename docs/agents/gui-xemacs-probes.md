# Driving a GUI XEmacs from a script

Facts about pointing this build at a window system (`DISPLAY=:0` under WSLg)
from a non-interactive driver: a probe file loaded with `-l`, or a gate run
under ctest. Each heading is what to do; the shape that fails is given as the
symptom to recognise.

## Log to a file, in XEmacs's own argument shape

Interactive XEmacs sends `princ` and `message` to the echo area, and `-batch`
has no frame to log to. Write the file yourself:

```elisp
(write-region (format "%s\n" value) nil "/tmp/probe.log" t)
```

The text argument is a bare string with `nil` for its partner. GNU's list form
`(write-region (list "text") nil FILE t)` signals
`wrong-type-argument integer-or-marker-p`. Keep
`(setq standard-output 'external-debugging-output)` at the top of the driver
when you also want the transcript on stdout.

## End every `map-extents` callback with `nil`

`map-extents` returns the first non-nil result its function produces and stops
mapping there. `push` returns the new list, so a callback whose last form is a
`push` visits exactly one extent and the scan silently returns a singleton:

```elisp
(map-extents (lambda (extent ignore)
               (when (interesting-p extent) (push extent out))
               nil)                    ; keeps the scan running
             nil from to)
```

## Give each driver form its own `condition-case`, and log the step first

A top-level error while `-l` loads drops interactive XEmacs into its event
loop, and `timeout` reports that as exit 124 with no output at all. Write the
step's name to the log *before* evaluating it, so the log names the form that
stopped, wrap each form in `condition-case`, and end the file with
`(kill-emacs 0)`.

## Bind the window in its own `let`

`let` evaluates every initialiser before establishing any of its bindings, so
one initialiser cannot read a variable another one defines: the answer is
`void-variable`. Use a nested `let`, or `let*` where the order is the point.

## Stop a probe by PID

`pkill -f xemacs` also matches the command line of the shell running it, and
takes down the session with the probe. Read the list, then kill the number:

```sh
pgrep -ax xemacs
```

## The gates read the build tree's bytecode

`build/bin/xemacs` loads `build/lisp/*.elc`, not `lisp/*.el`, so a gate run
straight after editing a shim grades the previous build. Refresh it with

```sh
cmake --build <build-dir> --target build-all
```

ctest's `xemacs_build` fixture now runs that itself. The three GUI gates are
registered there too, so they run by selection rather than by memory:

```sh
ctest -L gui --output-on-failure          # all three
DISPLAY=:0 <build>/bin/xemacs -no-init-file -no-site-file -l tests/cnfonts-xemacs-gui.el
```

A host with no display reachable reports them SKIP: XEmacs takes its terminal
path and exits on the non-tty stdin ctest hands it, before any Lisp runs.
