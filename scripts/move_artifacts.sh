#!/bin/bash
set -eu -o pipefail

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
cd $SCRIPT_DIR/../build/stage

find . -mindepth 1 -type f -exec mv -i -- {} . \;
find . -mindepth 1 -depth -type d -empty -exec rmdir {} \;
