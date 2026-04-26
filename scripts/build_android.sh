#!/bin/bash
# NetVision - Android build script (using ndk-build)
# Developer: MERO:TG@QP4RM

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
JNI_DIR="$PROJECT_DIR/android/app/src/main/jni"
OUTPUT_DIR="$PROJECT_DIR/build/android"

if [ -z "$ANDROID_NDK" ]; then
    if [ -d "$HOME/android-ndk-r26d" ]; then
        export ANDROID_NDK="$HOME/android-ndk-r26d"
    else
        echo "Error: ANDROID_NDK not set. Set it to your NDK path."
        exit 1
    fi
fi

echo "Using NDK: $ANDROID_NDK"

mkdir -p "$OUTPUT_DIR"
cd "$JNI_DIR"

"$ANDROID_NDK/ndk-build" \
    NDK_PROJECT_PATH="$PROJECT_DIR/android/app/src/main" \
    NDK_LIBS_OUT="$OUTPUT_DIR/libs" \
    NDK_OUT="$OUTPUT_DIR/obj" \
    APP_BUILD_SCRIPT="$JNI_DIR/Android.mk" \
    -j$(nproc)

echo ""
echo "Android build complete:"
echo "  ARM32 .so: $OUTPUT_DIR/libs/armeabi-v7a/"
echo "  ARM64 .so: $OUTPUT_DIR/libs/arm64-v8a/"
echo ""
echo "For full APK, use: cd android && ./gradlew assembleRelease"
