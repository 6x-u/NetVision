/*
 * NetVision - nv_jni_bridge.c
 * JNI bridge for Android native access to NetVision core.
 * Developer: MERO:TG@QP4RM
 */

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include "nv_types.h"
#include "nv_graph.h"
#include "nv_session.h"
#include "nv_replay.h"
#include "nv_capture.h"

#define TAG "NetVision"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static nv_graph_t       *g_graph   = NULL;
static nv_session_mgr_t *g_sess    = NULL;
static nv_replay_t      *g_replay  = NULL;
static nv_capture_ctx_t *g_capture = NULL;

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_init(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    g_graph  = nv_graph_create();
    g_sess   = nv_session_mgr_create();
    g_replay = nv_replay_create(100000);

    if (!g_graph || !g_sess || !g_replay) {
        LOGE("Failed to initialize NetVision core");
        return JNI_FALSE;
    }

    LOGI("NetVision core initialized");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_netvision_app_NVCore_shutdown(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;

    if (g_capture) {
        nv_capture_stop(g_capture);
        nv_capture_destroy(g_capture);
        g_capture = NULL;
    }
    if (g_graph)  { nv_graph_destroy(g_graph);         g_graph = NULL; }
    if (g_sess)   { nv_session_mgr_destroy(g_sess);     g_sess = NULL; }
    if (g_replay) { nv_replay_destroy(g_replay);       g_replay = NULL; }

    LOGI("NetVision core shutdown");
}

JNIEXPORT jint JNICALL
Java_com_netvision_app_NVCore_getNodeCount(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_graph ? (jint)nv_graph_node_count(g_graph) : 0;
}

JNIEXPORT jint JNICALL
Java_com_netvision_app_NVCore_getEdgeCount(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_graph ? (jint)nv_graph_edge_count(g_graph) : 0;
}

JNIEXPORT jint JNICALL
Java_com_netvision_app_NVCore_getSessionCount(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_sess ? (jint)nv_session_count(g_sess) : 0;
}

JNIEXPORT jint JNICALL
Java_com_netvision_app_NVCore_getActiveSessionCount(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_sess ? (jint)nv_session_active_count(g_sess) : 0;
}

JNIEXPORT void JNICALL
Java_com_netvision_app_NVCore_updateLayout(JNIEnv *env, jobject thiz, jfloat w, jfloat h)
{
    (void)env;
    (void)thiz;
    if (g_graph)
        nv_graph_update_layout(g_graph, w, h);
}

JNIEXPORT void JNICALL
Java_com_netvision_app_NVCore_resetGraph(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    if (g_graph)
        nv_graph_reset(g_graph);
}

JNIEXPORT jfloatArray JNICALL
Java_com_netvision_app_NVCore_getNodePositions(JNIEnv *env, jobject thiz)
{
    (void)thiz;
    if (!g_graph)
        return NULL;

    uint32_t count = nv_graph_node_count(g_graph);
    nv_node_t *nodes = nv_graph_get_nodes(g_graph);

    jfloatArray result = (*env)->NewFloatArray(env, count * 4);
    if (!result)
        return NULL;

    jfloat *buf = (*env)->GetFloatArrayElements(env, result, NULL);
    uint32_t i;
    for (i = 0; i < count; i++) {
        buf[i * 4 + 0] = nodes[i].pos_x;
        buf[i * 4 + 1] = nodes[i].pos_y;
        buf[i * 4 + 2] = (float)nodes[i].type;
        buf[i * 4 + 3] = (float)nodes[i].active;
    }
    (*env)->ReleaseFloatArrayElements(env, result, buf, 0);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_startRecording(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_replay ? (nv_replay_start_recording(g_replay) == 0 ? JNI_TRUE : JNI_FALSE) : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_stopRecording(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return g_replay ? (nv_replay_stop_recording(g_replay) == 0 ? JNI_TRUE : JNI_FALSE) : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_saveReplay(JNIEnv *env, jobject thiz, jstring path)
{
    (void)thiz;
    if (!g_replay || !path)
        return JNI_FALSE;
    const char *cpath = (*env)->GetStringUTFChars(env, path, NULL);
    int ret = nv_replay_save(g_replay, cpath);
    (*env)->ReleaseStringUTFChars(env, path, cpath);
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_loadReplay(JNIEnv *env, jobject thiz, jstring path)
{
    (void)thiz;
    if (!g_replay || !path)
        return JNI_FALSE;
    const char *cpath = (*env)->GetStringUTFChars(env, path, NULL);
    int ret = nv_replay_load(g_replay, cpath);
    (*env)->ReleaseStringUTFChars(env, path, cpath);
    return ret == 0 ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_netvision_app_NVCore_startCapture(JNIEnv *env, jobject thiz, jstring device)
{
    (void)thiz;
    const char *dev = NULL;

    if (g_capture)
        return JNI_FALSE;

    if (device)
        dev = (*env)->GetStringUTFChars(env, device, NULL);

    g_capture = nv_capture_create(dev, 65535, 0);

    if (device && dev)
        (*env)->ReleaseStringUTFChars(env, device, dev);

    if (!g_capture)
        return JNI_FALSE;

    if (nv_capture_start(g_capture) != 0) {
        nv_capture_destroy(g_capture);
        g_capture = NULL;
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_netvision_app_NVCore_stopCapture(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    if (g_capture) {
        nv_capture_stop(g_capture);
        nv_capture_destroy(g_capture);
        g_capture = NULL;
    }
}

JNIEXPORT jint JNICALL
Java_com_netvision_app_NVCore_processPackets(JNIEnv *env, jobject thiz, jint max_count)
{
    (void)env;
    (void)thiz;
    nv_packet_t pkt;
    int processed = 0;

    if (!g_capture || !g_graph)
        return 0;

    while (processed < max_count) {
        if (nv_capture_next_packet(g_capture, &pkt) != 0)
            break;
        nv_graph_process_packet(g_graph, &pkt);
        if (g_sess)
            nv_session_track(g_sess, &pkt);
        if (g_replay && nv_replay_is_recording(g_replay))
            nv_replay_record_packet(g_replay, &pkt);
        processed++;
    }

    return processed;
}
