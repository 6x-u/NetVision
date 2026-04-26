/*
 * NetVision - NVCore.java
 * JNI wrapper for native NetVision core.
 * Developer: MERO:TG@QP4RM
 */

package com.netvision.app;

public class NVCore {

    static {
        System.loadLibrary("nvcore");
        System.loadLibrary("netvision_jni");
    }

    public native boolean init();
    public native void shutdown();
    public native int getNodeCount();
    public native int getEdgeCount();
    public native int getSessionCount();
    public native int getActiveSessionCount();
    public native void updateLayout(float width, float height);
    public native void resetGraph();
    public native float[] getNodePositions();
    public native boolean startRecording();
    public native boolean stopRecording();
    public native boolean saveReplay(String path);
    public native boolean loadReplay(String path);
    public native boolean startCapture(String device);
    public native void stopCapture();
    public native int processPackets(int maxCount);
}
