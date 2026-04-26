/*
 * NetVision - MainActivity.java
 * Developer: MERO:TG@QP4RM
 */

package com.netvision.app;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {

    private NVCore nvCore;
    private NetworkCanvas canvas;
    private TextView statusText;
    private Button btnCapture;
    private Button btnRecord;
    private Button btnPause;
    private Handler handler;
    private boolean capturing = false;
    private boolean recording = false;
    private boolean paused = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        nvCore = new NVCore();
        nvCore.init();

        canvas = findViewById(R.id.network_canvas);
        statusText = findViewById(R.id.status_text);
        btnCapture = findViewById(R.id.btn_capture);
        btnRecord = findViewById(R.id.btn_record);
        btnPause = findViewById(R.id.btn_pause);

        canvas.setNVCore(nvCore);
        handler = new Handler(Looper.getMainLooper());

        btnCapture.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleCapture();
            }
        });

        btnRecord.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleRecording();
            }
        });

        btnPause.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                togglePause();
            }
        });

        statusText.setText("NetVision Ready - MERO:TG@QP4RM");
        startUpdateLoop();
    }

    private void toggleCapture() {
        if (!capturing) {
            if (nvCore.startCapture(null)) {
                capturing = true;
                btnCapture.setText("STOP");
                statusText.setText("Capturing...");
            } else {
                statusText.setText("Capture failed");
            }
        } else {
            nvCore.stopCapture();
            capturing = false;
            btnCapture.setText("START");
            statusText.setText("Capture stopped");
        }
    }

    private void toggleRecording() {
        if (!recording) {
            nvCore.startRecording();
            recording = true;
            btnRecord.setText("STOP REC");
            statusText.setText("Recording...");
        } else {
            nvCore.stopRecording();
            recording = false;
            btnRecord.setText("RECORD");
            statusText.setText("Recording saved");
        }
    }

    private void togglePause() {
        paused = !paused;
        btnPause.setText(paused ? "RESUME" : "PAUSE");
    }

    private void startUpdateLoop() {
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (capturing && !paused) {
                    nvCore.processPackets(128);
                    int w = canvas.getWidth();
                    int h = canvas.getHeight();
                    if (w > 0 && h > 0)
                        nvCore.updateLayout(w, h);
                }
                canvas.invalidate();
                updateStatus();
                handler.postDelayed(this, 33);
            }
        }, 33);
    }

    private void updateStatus() {
        int nodes = nvCore.getNodeCount();
        int edges = nvCore.getEdgeCount();
        int sessions = nvCore.getActiveSessionCount();
        String status = "Nodes: " + nodes + " | Edges: " + edges + " | Sessions: " + sessions;
        if (capturing) status = "[LIVE] " + status;
        if (recording) status = "[REC] " + status;
        if (paused) status = "[PAUSED] " + status;
        statusText.setText(status);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (capturing)
            nvCore.stopCapture();
        nvCore.shutdown();
    }
}
