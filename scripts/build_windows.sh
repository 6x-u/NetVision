#!/bin/bash
# NetVision - Windows cross-compilation build script
# Developer: MERO:TG@QP4RM

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/windows"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$PROJECT_DIR/cmake/mingw-w64-x86_64.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNV_HEADLESS=ON \
    -DNV_BUILD_TESTS=OFF

make -j$(nproc)

echo ""
echo "Windows build complete:"
echo "  DLL: $BUILD_DIR/nvcore.dll"
echo "  LIB: $BUILD_DIR/nvcore.lib"
echo ""
echo "Note: Link with npcap on Windows for full capture support."
echo "Download npcap SDK: https://npcap.com/dist/npcap-sdk-1.13.zip"
