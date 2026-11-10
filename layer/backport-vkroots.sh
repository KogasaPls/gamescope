#!/bin/sh
# Emit the pinned header with vkroots PR 17 applied. Meson captures stdout
# only on success and tracks the header, this script and the diff as inputs.
set -eu
header=$1
diff=$2
set -- --silent --force --fuzz=0 "$header" "$diff"

if patch --dry-run "$@" >/dev/null; then
    exec patch --output=- "$@"
fi
if patch --dry-run --reverse "$@" >/dev/null; then
    exec cat "$header"
fi

echo "vkroots flagged-queue backport no longer matches $header. Review vkroots PR 17 against the new submodule revision." >&2
exit 1
