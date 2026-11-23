package com.arut.engine;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.DialogInterface;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.os.Bundle;

/** Native Android profile picker for the engine's isolated development scenes. */
public final class TestLabActivity extends NativeActivity {
    private static final CharSequence[] PROFILES = {
        "ECS Stress Test — 5,000 entities and renderer load",
        "Humanoid Test — mannequin and character systems",
        "Combined Integration — stress scene plus humanoid"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        showTestLaboratory();
    }

    private void showTestLaboratory() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                new AlertDialog.Builder(TestLabActivity.this)
                    .setTitle("ARUT-HARDT Test Laboratory")
                    .setMessage("Choose one isolated runtime profile. Humanoid controls: left stick to move, drag on the right to look, tap the lower-right area to jump. Restart the app to choose another profile.")
                    .setItems(PROFILES, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            nativeSelectProfile(which);
                            showControlOverlay(which != 0);
                        }
                    })
                    .setCancelable(false)
                    .show();
            }
        });
    }


    private float dp(float value) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, value,
            getResources().getDisplayMetrics());
    }

    /** Draws touch guidance without consuming events; NativeActivity still receives touch input. */
    private void showControlOverlay(boolean humanoid) {
        final float density = getResources().getDisplayMetrics().density;
        View overlay = new View(this) {
            private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);

            private void text(Canvas canvas, String value, float x, float y, float size) {
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(Color.WHITE);
                paint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
                paint.setTextSize(size);
                paint.setTextAlign(Paint.Align.CENTER);
                canvas.drawText(value, x, y, paint);
            }

            @Override
            protected void onDraw(Canvas canvas) {
                super.onDraw(canvas);
                final float width = getWidth();
                final float height = getHeight();
                final float leftX = width * 0.18f;
                final float leftY = height * 0.82f;
                final float radius = 54.0f * density;

                paint.setStyle(Paint.Style.FILL);
                paint.setColor(0x33202A3A);
                canvas.drawCircle(leftX, leftY, radius, paint);
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(2.0f * density);
                paint.setColor(0xBFFFFFFF);
                canvas.drawCircle(leftX, leftY, radius, paint);
                paint.setColor(0x99FFFFFF);
                canvas.drawCircle(leftX, leftY, 22.0f * density, paint);
                text(canvas, "MOVE", leftX, leftY + radius + 22.0f * density,
                    12.0f * density);

                text(canvas, "DRAG TO LOOK", width * 0.78f, height * 0.16f,
                    12.0f * density);
                if (humanoid) {
                    final float jumpX = width * 0.87f;
                    final float jumpY = height * 0.82f;
                    paint.setStyle(Paint.Style.FILL);
                    paint.setColor(0x663B82F6);
                    canvas.drawCircle(jumpX, jumpY, 38.0f * density, paint);
                    paint.setStyle(Paint.Style.STROKE);
                    paint.setStrokeWidth(2.0f * density);
                    paint.setColor(0xDFFFFFFF);
                    canvas.drawCircle(jumpX, jumpY, 38.0f * density, paint);
                    text(canvas, "JUMP", jumpX, jumpY + 5.0f * density,
                        12.0f * density);
                }
            }
        };
        overlay.setBackgroundColor(Color.TRANSPARENT);
        overlay.setClickable(false);
        overlay.setFocusable(false);
        overlay.setFocusableInTouchMode(false);
        addContentView(overlay, new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT));
        overlay.bringToFront();
    }

    private static native void nativeSelectProfile(int profileId);
}
