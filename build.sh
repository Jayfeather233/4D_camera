#!/usr/bin/env bash
# Build and run. Extra arguments are forwarded to the binary, e.g.
#   ./build.sh --mode solid --shape spring4d
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}"
cmake --build build -j"$(nproc)"

exec ./build/4DCam "$@"
