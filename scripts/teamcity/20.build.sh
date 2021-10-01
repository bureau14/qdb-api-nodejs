#!/bin/bash

set -eux

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
source "$SCRIPT_DIR/configure.sh"

$NPM cache clean --force
$NPM install --build-from-source --copy_c_api=yes
