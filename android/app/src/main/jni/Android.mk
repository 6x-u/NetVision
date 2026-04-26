# NetVision - Android.mk
# Developer: MERO:TG@QP4RM

LOCAL_PATH := $(call my-dir)

# ============================================================
# nvcore - Core C library
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE    := nvcore
LOCAL_SRC_FILES := \
    ../../../../../src/core/nv_capture_stub.c \
    ../../../../../src/core/nv_graph.c \
    ../../../../../src/core/nv_session.c \
    ../../../../../src/core/nv_replay.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../include
LOCAL_CFLAGS     := -Wall -O2
LOCAL_LDLIBS     := -llog
include $(BUILD_SHARED_LIBRARY)

# ============================================================
# netvision_jni - JNI bridge
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE    := netvision_jni
LOCAL_SRC_FILES := nv_jni_bridge.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../include
LOCAL_CFLAGS     := -Wall -O2
LOCAL_SHARED_LIBRARIES := nvcore
LOCAL_LDLIBS     := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
