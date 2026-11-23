#!/bin/sh
# Is an issue's work actually merged?
#
#   scripts/verify-merged.sh ISSUE DELIVERABLE-BRANCH [INTEGRATION]
#
# Run before closing a ticket.  A closed ticket is not evidence: #157 was
# marked COMPLETED while its branch lexbind-p2-cconv was unmerged and did not
# work, and every later step inherited that.
#
# Checks that DELIVERABLE-BRANCH and every commit SHA the issue mentions are
# ancestors of INTEGRATION (default lexbind-p2).
#
#   exit 0  everything is merged
#   exit 1  something is not -- do not close the ticket
#   exit 2  the issue names no commit, so there is nothing to verify
set -eu

issue=${1:?usage: $0 ISSUE DELIVERABLE-BRANCH [INTEGRATION]}
deliverable=${2:?usage: $0 ISSUE DELIVERABLE-BRANCH [INTEGRATION]}
integration=${3:-lexbind-p2}

if ! git rev-parse --verify -q "refs/heads/$deliverable" >/dev/null; then
	echo "no such branch: $deliverable" >&2
	exit 2
fi

text=$(gh issue view "$issue" --json body,comments -q '.body, (.comments[]?.body)')

# Only tokens git actually knows: issue comment IDs are all-digit and match a
# hex regex, so a plain grep is not enough.
shas=
for tok in $(printf '%s' "$text" | grep -oE '\b[0-9a-f]{7,40}\b' | sort -u); do
	git rev-parse --verify -q "$tok^{commit}" >/dev/null && shas="$shas $tok"
done

if [ -z "$shas" ]; then
	echo "issue #$issue names no commit; cannot verify it is merged" >&2
	exit 2
fi

rc=0
for rev in "$deliverable" $shas; do
	if git merge-base --is-ancestor "$rev" "$integration" 2>/dev/null; then
		printf 'merged     %s\n' "$rev"
	else
		printf 'NOT MERGED %s\n' "$rev"
		rc=1
	fi
done

[ "$rc" -eq 0 ] || echo "issue #$issue is not fully contained in $integration" >&2
exit "$rc"
