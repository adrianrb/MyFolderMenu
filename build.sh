#!/usr/bin/env bash
# Build cross-compilado (MinGW-w64) a partir do WSL/Linux.
set -euo pipefail

cd "$(dirname "$0")"

cmake -S . -B build-mingw -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake \
    -DMFM_BUILD_TESTS=ON

cmake --build build-mingw -j "${JOBS:-$(nproc)}"
echo
echo "OK: $(pwd)/build-mingw/MyFolderMenu.exe"
