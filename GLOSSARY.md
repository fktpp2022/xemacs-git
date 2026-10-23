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
New compatibility code written by this effort to satisfy a package's GNU-only require on XEmacs. Distinct from consuming a prebuilt compat package; the target set is zero-shim.
_Avoid_: alias file, polyfill
