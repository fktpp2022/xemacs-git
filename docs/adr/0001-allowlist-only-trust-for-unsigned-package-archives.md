# Allowlist-only trust for unsigned package archives

The milestone-1 ELPA client (`lisp/elpa.el`) must consume MELPA, which publishes **no signatures at all** (index and artifacts verified unsigned; melpa#1749), alongside GNU ELPA, whose signature checking we do not implement yet. We decided: HTTPS transport plus the explicit per-package install allowlist are the **only** trust controls for milestone 1 — no signature verification, no checksum pinning, no TLS pinning — and the dead `elpa-check-signature` defcustom (a boolean no code reads) is removed rather than reworded.

## Considered options

- **GNU-shaped GPG verification now** (as `package.el` does: `package-check-signature` multi-valued with `allow-unsigned` default, `package-unsigned-archives` exemptions): rejected for milestone 1 — MELPA signs nothing, so the machinery verifies nothing on the archive we are adding; and GNU's own keyring story (`gnu-elpa-keyring-update`) shows ongoing operational cost (key expiry rots verification until users disable it).
- **Keep + reword the knob**: rejected — a `defcustom` no code reads ships an inert control into Customize "as if functional" (Elisp manual on defcustom); Abseil: knobs beyond real need "are almost always haunted". GNU shipped the knob *with* its verifier (24.4); nobody ships the knob alone.
- **Per-archive allowlist scoping** (`(package . archive)` pairs): rejected — trust and source selection are different concepts; GNU's `package-pinned-packages` is explicitly version disambiguation, not security. If a same-name collision ever matters, add pinning as its own concept.

Byte-level client pinning (pip hash-checking, cargo lock, npm integrity, Homebrew formula sha256) is the industry pattern for untrusted channels, but every one pins specific bytes — impossible against a rolling archive that deletes old artifacts (they 404; measured in [Map MELPA's archive mechanics](https://github.com/fktpp2022/xemacs-git/issues/31)). The surviving pattern is apt's: explicit opt-in, fail closed, and say so ("authenticity must be ensured in another way by the user itself"). TUF-style server-side frameworks verify signed metadata that a plaintext index does not offer.

## Consequences

- Residual risk, accepted for milestone 1: if an archive is compromised or the connection is tampered with, **any allowlisted package can run arbitrary code**. The allowlist is not a cast-iron guarantee.
- Spec work items for the implementation hand-off: delete the `elpa-check-signature` defcustom; replace the header comment's deferred-features line with the trust-model statement (see ticket resolution wording); give the milestone spec a "Security & trust" section.
- When verification eventually lands, it lands **GNU-shaped**: multi-valued check knob (`allow-unsigned` default), bad signature never suppressed (only a missing one tolerated), per-archive exemption list. Recorded in the spec as a named future core effort, deliberately out of scope for the MELPA milestone map.
