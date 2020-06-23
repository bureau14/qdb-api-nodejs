#!/bin/bash

set -eux

SCRIPT_DIR="$(cd "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null && pwd)"
source "$SCRIPT_DIR/configure.sh"

QDB_DIR=`pwd`/qdb
cd examples/tutorial
$NPM install --save bureau14/qdb-api-nodejs#${BUILD_BRANCH} --build-from-source --copy_c_api=yes --c_api_path=$QDB_DIR
$NPM start
