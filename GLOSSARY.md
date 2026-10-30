# XEmacs ELPA Client

Vocabulary for XEmacs's Emacs Lisp package client (`lisp/elpa.el`): installing prebuilt packages from ELPA-style repositories (GNU ELPA, MELPA).

## Language

**Allowlist**:
The explicit list of packages the client may install; together with HTTPS transport, it is the entire trust boundary. Trust by name, spanning all configured archives.
_Avoid_: whitelist, trusted packages, package pinning

**Unsigned-by-design archive**:
An archive that publishes no signatures at all, as a property of the archive (MELPA). Allowing it is a posture, not a deferred feature.
_Avoid_: temporarily unsigned, insecure archive

**Verification-deferred archive**:
An archive that does sign its index and packages (GNU ELPA), whose checks this client does not perform yet.
_Avoid_: unsupported archive

**Target set**:
The packages a milestone promises to install-and-run out of the box; what the success test enforces. A support contract, not a user-facing restriction — users may extend the allowlist beyond it.
_Avoid_: supported packages, package list

**Success test**:
A milestone gate of two halves: a deterministic offline fixture that pins exact versions, plus a live HTTPS smoke test that pins nothing.
_Avoid_: acceptance test, integration test

**Shim**:
New compatibility code written by this effort to satisfy a package's GNU-only require on XEmacs. Distinct from consuming a prebuilt compat package. Milestone 1's target set is zero-shim; the milestone-2 compatibility layer is core provision, not a shim (see **Compatibility layer**).
_Avoid_: alias file, polyfill

**Compatibility layer**:
The milestone-2 core provision that lets unmodified GNU-oriented packages run on XEmacs. Core, not a package, and governed by the support contract.
_Avoid_: compat package, shim

**Support contract**:
The exact set of APIs a milestone guarantees for its targets, established by compiling, loading, and exercising them. Anything outside the contract is unsupported even if it happens to work.
_Avoid_: supported API, best effort

**Cross-archive dependency resolution**:
Resolving each dependency requirement against all configured archives in `elpa-archives` order and taking the first entry that satisfies the version floor; the dependent's own archive does not limit the search. Every dependency name that is not core-provided (and is not the `emacs` pseudo-dependency) is allowlisted at every depth.
_Avoid_: multi-archive install, cross-repo dependency

**Core-provided package**:
A requirement name that the XEmacs core satisfies without a download, from a private list that is not a Customize option.
_Avoid_: builtin package, ignored dependency

## Font configuration

Vocabulary for selecting fonts on XEmacs, where there is no fontset API and a font is chosen **per charset**.

**Per-charset selection**:
How XEmacs decides which font renders a character: the face's font specifier is resolved by the character's charset, not by a fontset entry or a codepoint range. The charset is the only key.
_Avoid_: fontset, font fallback chain

**Font descriptor**:
The device-independent description of a font — family, weight, style, size, registry, encoding. What a user means by "this family at this size, bold"; each backend turns it into its own name format.
_Avoid_: font spec, font name

**XLFD**:
The legacy X font name format — fourteen dash-separated fields naming a core X server font. Modern XEmacs builds speak fontconfig instead, but the format still appears wherever a name is parsed as fields rather than read as a whole.
_Avoid_: font name, font descriptor

**Font backend**:
The per-device machinery that turns a font descriptor into a name and lists the fonts a device has. XEmacs has four: X11 (whose modern Xft mode speaks fontconfig), GTK (Pango), MS Windows, and the terminal.
_Avoid_: window system, display

**Tag-set**:
The tag list attached to a font instantiator inside a font specifier, selecting which charsets that font applies to. Matched in redisplay; the charset is the only key it can select by.
_Avoid_: fontset charset list, range table

**Specifier tag**:
A named tag usable in a tag-set. It may carry a charset predicate, deciding which charsets it matches, and a device predicate. XEmacs's built-in charset-class specifier tags — `one-dimensional`, `two-dimensional`, `initial`, `final`, `jit-charset` — are registered in every build, the JIT tag only where JIT charsets exist. A compatibility layer still registers its own names, so its tag-sets do not entangle another package's.
_Avoid_: charset tag, fontset charset list

**Charset tag**:
A name for a group of charsets, expandable to its members. Used in charset precedence lists. Distinct from a specifier tag, which attaches a charset predicate to a font instantiator.
_Avoid_: specifier tag, tag-set

**Stage**:
The two-pass ordering of font matching, expressed as the `initial` and `final` specifier tags. `initial` means the font matches the charset's own registries; `final` means a general fallback — any font whose repertoire covers the character, rendered through `iso10646-1`. A specifier tag whose charset predicate takes only a charset matches at both stages.
_Avoid_: priority, rank

**Final-stage fallback**:
A font attached under the `final` stage: it renders only characters that no `initial`-stage font covered. The XEmacs shape of a last-resort CJK font.
_Avoid_: default font, fallback chain

**JIT charset**:
An internal charset holding Unicode codepoints with no encodable XEmacs charset equivalent (rare CJK, e.g. Extension B). Being 2D, it matches any dimension-2 specifier tag like any other CJK charset.
_Avoid_: unicode charset, ext-b charset
