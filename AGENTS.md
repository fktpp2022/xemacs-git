## Agent skills

### Issue tracker

Issues live as GitHub issues in `fktpp2022/xemacs-git`, managed with the `gh` CLI. See `docs/agents/issue-tracker.md`.

### Triage labels

Default five-role vocabulary: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: root `GLOSSARY.md` and `docs/adr/`. See `docs/agents/domain.md`.

### Coding standards

`CODING_STANDARDS.md` is read at review, not while implementing.

### GUI probes

Driving a GUI XEmacs from a script -- a `-l` probe, or a gate under `ctest -L gui`: see `docs/agents/gui-xemacs-probes.md`.

### Shell

Commands run under zsh here: quote a glob (`'*.elc'`), because one that matches nothing aborts the whole command with `no matches found`.
