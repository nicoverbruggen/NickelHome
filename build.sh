#!/usr/bin/env bash
set -euo pipefail

image="${NICKELTC_IMAGE:-ghcr.io/pgaskin/nickeltc:1.0}"
workdir="${PWD}"

if [ "$#" -eq 0 ]; then
    set -- clean all koboroot
fi

exec podman run --rm -it \
    -v "${workdir}:${workdir}:Z" \
    -w "${workdir}" \
    --userns=keep-id \
    -e HOME \
    --entrypoint make \
    "${image}" \
    "$@"
