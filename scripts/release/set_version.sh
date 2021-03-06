#!/bin/sh
set -eu -o pipefail
IFS=$'\n\t'

if [[ $# -ne 1 ]] ; then
    >&2 echo "Usage: $0 <new_version>"
    exit 1
fi

INPUT_VERSION=$1; shift

MAJOR_VERSION=${INPUT_VERSION%%.*}
WITHOUT_MAJOR_VERSION=${INPUT_VERSION#${MAJOR_VERSION}.}
MINOR_VERSION=${WITHOUT_MAJOR_VERSION%%.*}
WITHOUT_MINOR_VERSION=${INPUT_VERSION#${MAJOR_VERSION}.${MINOR_VERSION}.}
PATCH_VERSION=${WITHOUT_MINOR_VERSION%%.*}

XY_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}"
XYZ_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"

if [[ "${INPUT_VERSION}" == *-* ]] ; then
    TAGS_VERSION=${INPUT_VERSION#*-}
else
    TAGS_VERSION=
fi

if [[ -n "${TAGS_VERSION}" ]] ; then
    FULL_XYZ_VERSION="${XYZ_VERSION}-${TAGS_VERSION}"
    REMOTE_PATH_INFIX="nightly"
else
    FULL_XYZ_VERSION="${XYZ_VERSION}"
    REMOTE_PATH_INFIX="${XY_VERSION}/{version}"
fi

cd $(dirname -- $0)
cd ${PWD}/../..

# "version": "2.1.0-nightly.5",
sed -i -e 's/"version": "[^"]*"/"version": "'"${FULL_XYZ_VERSION}"'"/' package.json
# "remote_path": "./{name}/3.1/{version}/api/nodejs"
sed -i -e 's|"remote_path": "[^"]*"|"remote_path": "./{name}/'${REMOTE_PATH_INFIX}'/api/nodejs"|' package.json
