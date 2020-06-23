#!/bin/bash

set -eux

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
source "$SCRIPT_DIR/configure.sh"

$NPM install mocha-teamcity-reporter
$NPM test -- --reporter mocha-teamcity-reporter
