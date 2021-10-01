#!/bin/bash

set -eux

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
source "$SCRIPT_DIR/configure.sh"

rm -rf ~/.npm/_cacache/
$NPM install --build-from-source --copy_c_api=yes
