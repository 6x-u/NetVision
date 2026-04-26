/*
 * NetVision - NetworkCanvas.java
 * Custom view for rendering the network graph.
 * Developer: MERO:TG@QP4RM
 */

package com.netvision.app;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

public class NetworkCanvas extends View {

    private NVCore nvCore;
    private Paint nodePaint;
    private Paint edgePaint;
    private Paint textPaint;
    private Paint bgPaint;
    private float zoom = 1.0f;
    private float panX = 0.0f;
    private float panY = 0.0f;
    private float lastTouchX;
    private float lastTouchY;
    private ScaleGestureDetector scaleDetector;

    private static final int COLOR_CLIENT  = Color.rgb(0, 200, 0);
    private static final int COLOR_SERVER  = Color.rgb(0, 120, 255);
    private static final int COLOR_API     = Color.rgb(180, 0, 255);
    private static final int COLOR_GATEWAY = Color.rgb(255, 165, 0);
    private static final int COLOR_DEFAULT = Color.rgb(128, 128, 128);

    public NetworkCanvas(Context context) {
        super(context);
        init();
    }

    public NetworkCanvas(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        nodePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        nodePaint.setStyle(Paint.Style.FILL);

        edgePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        edgePaint.setColor(Color.argb(100, 100, 150, 200));
        edgePaint.setStrokeWidth(2.0f);

        textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        textPaint.setColor(Color.rgb(200, 200, 200));
        textPaint.setTextSize(24.0f);
        textPaint.setTextAlign(Paint.Align.CENTER);

        bgPaint = new Paint();
        bgPaint.setColor(Color.rgb(13, 13, 20));

        scaleDetector = new ScaleGestureDetector(getContext(),
                new ScaleGestureDetector.SimpleOnScaleGestureListener() {
                    @Override
                    public boolean onScale(ScaleGestureDetector detector) {
                        zoom *= detector.getScaleFactor();
                        if (zoom < 0.1f) zoom = 0.1f;
                        if (zoom > 5.0f) zoom = 5.0f;
                        invalidate();
                        return true;
                    }
                });
    }

    public void setNVCore(NVCore core) {
        this.nvCore = core;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        canvas.drawRect(0, 0, getWidth(), getHeight(), bgPaint);

        if (nvCore == null)
            return;

        float[] positions = nvCore.getNodePositions();
        if (positions == null || positions.length < 4)
            return;

        int nodeCount = positions.length / 4;
        float radius = 24.0f * zoom;

        for (int i = 0; i < nodeCount; i++) {
            float x = positions[i * 4] * zoom + panX;
            float y = positions[i * 4 + 1] * zoom + panY;
            int type = (int) positions[i * 4 + 2];
            int active = (int) positions[i * 4 + 3];

            if (active == 0) continue;

            switch (type) {
                case 0: nodePaint.setColor(COLOR_CLIENT);  break;
                case 1: nodePaint.setColor(COLOR_SERVER);  break;
                case 2: nodePaint.setColor(COLOR_API);     break;
                case 3: nodePaint.setColor(COLOR_GATEWAY); break;
                default: nodePaint.setColor(COLOR_DEFAULT); break;
            }

            canvas.drawCircle(x, y, radius, nodePaint);
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        scaleDetector.onTouchEvent(event);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                lastTouchX = event.getX();
                lastTouchY = event.getY();
                break;
            case MotionEvent.ACTION_MOVE:
                if (!scaleDetector.isInProgress()) {
                    float dx = event.getX() - lastTouchX;
                    float dy = event.getY() - lastTouchY;
                    panX += dx;
                    panY += dy;
                    lastTouchX = event.getX();
                    lastTouchY = event.getY();
                    invalidate();
                }
                break;
        }
        return true;
    }
}
