#!/bin/bash
# Portable Linux build (glibc 2.31 baseline, SDL2 linked statically).
#
# Usage: platform/linux/portable/build.sh [output-dir]
#   output-dir defaults to <source>/build-portable
#
# Uses docker, or podman when docker is not available (CONTAINER=... overrides).
# Result: <output-dir>/dist/ with skeldal_bin and skeldal.sh

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
SRC_DIR=$(cd "$SCRIPT_DIR/../../.." && pwd)
OUT_DIR=${1:-$SRC_DIR/build-portable}
IMAGE=skeldal-portable-build

if [ -z "${CONTAINER:-}" ]; then
    if docker info >/dev/null 2>&1; then CONTAINER=docker
    elif command -v podman >/dev/null; then CONTAINER=podman
    else echo "docker or podman is required" >&2; exit 1
    fi
fi

mkdir -p "$OUT_DIR"
OUT_DIR=$(cd "$OUT_DIR" && pwd)

"$CONTAINER" build -t "$IMAGE" "$SCRIPT_DIR"

# rootless podman maps host uid to root in the container -> files stay owned by
# the host user; docker needs explicit --user
USER_ARGS=()
if [ "$CONTAINER" = docker ]; then
    USER_ARGS=(--user "$(id -u):$(id -g)")
fi

"$CONTAINER" run --rm "${USER_ARGS[@]}" \
    -v "$SRC_DIR:/src:ro" -v "$OUT_DIR:/out" \
    "$IMAGE" bash -euo pipefail -c '
        cmake -S /src -B /out/build \
            -DCMAKE_BUILD_TYPE=Release \
            -DACHIEVEMENTS_BACKEND=none \
            -DSKELDAL_SDL2_STATIC=ON \
            "-DCMAKE_EXE_LINKER_FLAGS=-static-libstdc++ -static-libgcc"
        cmake --build /out/build -j"$(nproc)" --target skeldal_bin
        rm -rf /out/dist && mkdir -p /out/dist
        cp /out/build/bin/skeldal_bin /out/build/bin/skeldal.sh /out/dist/
        strip /out/dist/skeldal_bin
        echo "--- dynamic dependencies:"
        objdump -p /out/dist/skeldal_bin | grep NEEDED
        echo "--- max required glibc:"
        objdump -T /out/dist/skeldal_bin | grep -o "GLIBC_[0-9.]*" | sort -uV | tail -1
    '

echo "Done: $OUT_DIR/dist"
