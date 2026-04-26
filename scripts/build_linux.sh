#!/bin/bash
# NetVision - Linux build script
# Developer: MERO:TG@QP4RM

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/linux"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNV_BUILD_SHARED=ON \
    -DNV_BUILD_TESTS=ON

make -j$(nproc)

echo ""
echo "Build complete:"
echo "  Executable: $BUILD_DIR/netvision"
echo "  Library:    $BUILD_DIR/libnvcore.so"
echo ""
echo "Run tests:"
echo "  LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/nv_test_graph"
echo "  LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/nv_test_session"
echo "  LD_LIBRARY_PATH=$BUILD_DIR $BUILD_DIR/nv_test_replay"
