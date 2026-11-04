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

## A widget is a cons, not an extent

XEmacs's `wid-edit` hands out widgets as `(<type> . plist)` conses, so the
GNU-shaped scan `map-extents` + `(widget-get EXTENT :tag)` signals
`wrong-type-argument listp #<extent ...>` -- and `widget-type`, which is
`(car widget)`, does the same. Take the widgets from wherever the owning code
keeps them (`cnfonts-ui--widgets-alist` for the editor) or from
`(widget-at POS)`, and press one with `(widget-button-press (widget-get w :from))`.

## Frame geometry has no GNU spellings

`frame-position` and `set-frame-name` are void here. Position, size and title
all go through one call, and `width`/`height` are in characters:

```elisp
(modify-frame-parameters (selected-frame)
                         '((left . 0) (top . 0) (user-position . t)
                           (width . 100) (height . 33) (name . "*probe*")))
```

`user-position` is what keeps the window manager from placing the frame off
the screen, which matters because a screenshot only captures what is visible.

## Screenshots come from the Windows side, not from X

There is no `import`, `xwd`, `scrot` or PIL here, and WSLg draws each X window
as a Windows window, so capture the desktop from PowerShell and write it
straight back into the WSL filesystem over its UNC path:

```powershell
# shot.ps1 -- param([string]$Out)
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
$b = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
```

```sh
/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe \
    -NoProfile -ExecutionPolicy Bypass -File '\\wsl.localhost\Ubuntu\tmp\shot.ps1' \
    -Out '\\wsl.localhost\Ubuntu\tmp\walk\1.png' </dev/null
```

The `</dev/null` is load-bearing: without it the interop call hangs the shell.
`wslpath -w /tmp` gives the UNC path for any other file. What this captures is
the whole desktop, so raise and position the frame first (above), and note
that the Windows desktop is scaled: a 1123x786 px frame lands on a 1280x720
screen, so anything below ~700 px tall is off the bottom of the shot.

Driving the frame while taking shots needs a handshake, not sleeps: have the
probe write a marker file and spin on `(while (not (file-exists-p NEXT))
(sit-for 0.2))`, and let the shell wait for the marker, shoot, then touch the
file it waits on.
