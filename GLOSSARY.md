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

**Tag-set**:
The tag list attached to a font instantiator inside a font specifier, selecting which charsets that font applies to. Tags carry a **charset predicate** and are matched in redisplay; the built-in ones are `one-dimensional`, `two-dimensional`, `initial`, `final`, `jit-charset`.
_Avoid_: fontset charset list, range table

**Stage**:
The two-pass ordering of font matching, expressed as the `initial` and `final` tags. `initial` means the font matches the charset's own registries; `final` means a general fallback — any font whose repertoire covers the character, rendered through `iso10646-1`.
_Avoid_: priority, rank

**Final-stage fallback**:
A font attached under the `final` stage: it renders only characters that no `initial`-stage font covered. The XEmacs shape of a last-resort CJK font.
_Avoid_: default font, fallback chain

**JIT charset**:
An internal charset holding Unicode codepoints with no encodable XEmacs charset equivalent (rare CJK, e.g. Extension B). Being 2D, it matches the `two-dimensional` tag like any other CJK charset.
_Avoid_: unicode charset, ext-b charset
