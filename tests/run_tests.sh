#!/bin/sh
set -eu

cd "$(dirname "$0")"
trap 'rm -f test_edf_recordsize' EXIT

sysroot_args=""
if command -v xcrun >/dev/null 2>&1; then
  sysroot_args="-isysroot $(xcrun --sdk macosx --show-sdk-path)"
fi

# shellcheck disable=SC2086
cc -std=c99 -Wall -Wextra -Werror $sysroot_args -I.. -o test_edf_recordsize test_edf_recordsize.c ../edf_recordsize.c
./test_edf_recordsize
