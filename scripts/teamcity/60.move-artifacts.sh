#!/bin/bash

set -eu -o pipefail

cd build/stage

find . -mindepth 1 -type f -exec mv -i -- {} . \;
find . -mindepth 1 -depth -type d -empty -exec rmdir {} \;