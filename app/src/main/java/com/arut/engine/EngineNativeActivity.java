package com.arut.engine;

import android.app.NativeActivity;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

/** Native game surface with lightweight, touch-safe gameplay test controls. */
public final class EngineNativeActivity extends NativeActivity {
    private static final String TAG = "ARUT-HARDT-UI";
    private static native void nativeSetCombatAction(int action, boolean pressed);
    private static native void nativeSetMovementInput(float x, float y);

    private LinearLayout combatPanel;
    private LinearLayout movementPanel;
    private boolean combatPanelAttached;
    private boolean movementPanelAttached;
    private JoystickView movementJoystick;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "EngineNativeActivity created; profile="
            + getIntent().getIntExtra("arut.profile_id", -1));
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (!isCombatProfile()) return;
        if (hasFocus) {
            getWindow().getDecorView().post(() -> {
                attachMovementControls();
                attachCombatControls();
            });
        } else {
            nativeSetMovementInput(0.0F, 0.0F);
            if (movementJoystick != null) movementJoystick.resetFromSystem();
        }
    }

    @Override
    protected void onDestroy() {
        nativeSetMovementInput(0.0F, 0.0F);
        removePanel(combatPanel, combatPanelAttached);
        removePanel(movementPanel, movementPanelAttached);
        combatPanelAttached = false;
        movementPanelAttached = false;
        super.onDestroy();
    }

    private void removePanel(View panel, boolean attached) {
        if (!attached || panel == null) return;
        try {
            getWindowManager().removeView(panel);
        } catch (IllegalArgumentException ignored) {
            // The application panel may already have been removed by Android.
        }
    }

    private boolean isCombatProfile() {
        final int profile = getIntent().getIntExtra("arut.profile_id", 0);
        return profile == 1 || profile == 2;
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private GradientDrawable roundedBackground(int color, float radiusDp,
                                                int strokeColor) {
        final GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setCornerRadius(dp(radiusDp));
        if (strokeColor != Color.TRANSPARENT) {
            drawable.setStroke(dp(1), strokeColor);
        }
        return drawable;
    }

    private WindowManager.LayoutParams panelLayout(int gravity, int x, int y,
                                                    String title) {
        final View decor = getWindow().getDecorView();
        final WindowManager.LayoutParams params = new WindowManager.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT);
        params.token = decor.getWindowToken();
        params.gravity = gravity;
        params.x = dp(x);
        params.y = dp(y);
        params.setTitle(title);
        return params;
    }

    private TextView panelLabel(String value, float sizeSp) {
        final TextView label = new TextView(this);
        label.setText(value);
        label.setTextColor(Color.WHITE);
        label.setTextSize(TypedValue.COMPLEX_UNIT_SP, sizeSp);
        label.setGravity(Gravity.CENTER);
        return label;
    }

    private Button actionButton(String label, int action, boolean accent) {
        final Button button = new Button(this);
        button.setText(label);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.5F);
        button.setAllCaps(false);
        button.setTextColor(Color.WHITE);
        button.setMinWidth(0);
        button.setMinHeight(dp(42));
        button.setPadding(dp(3), 0, dp(3), 0);
        button.setBackground(roundedBackground(
            accent ? Color.rgb(36, 91, 112) : Color.rgb(45, 57, 74),
            9, Color.argb(70, 220, 235, 250)));
        button.setOnClickListener(view -> {
            Log.i(TAG, "combat action=" + action + " label=" + label);
            nativeSetCombatAction(action, true);
        });
        return button;
    }

    private Button guardButton() {
        final Button button = actionButton("GUARD", 3, false);
        button.setOnClickListener(view -> { });
        button.setOnTouchListener((view, event) -> {
            final int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN) {
                nativeSetCombatAction(3, true);
                view.setPressed(true);
                return true;
            }
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                nativeSetCombatAction(3, false);
                view.setPressed(false);
                if (action == MotionEvent.ACTION_UP) view.performClick();
                return true;
            }
            return true;
        });
        return button;
    }

    private void addRow(LinearLayout panel, Button... buttons) {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        for (Button button : buttons) {
            final LinearLayout.LayoutParams params =
                new LinearLayout.LayoutParams(dp(82), dp(44));
            params.setMargins(dp(2), dp(2), dp(2), dp(2));
            row.addView(button, params);
        }
        panel.addView(row, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    private void attachMovementControls() {
        if (movementPanelAttached || isFinishing()) return;
        final View decor = getWindow().getDecorView();
        if (decor.getWindowToken() == null) {
            decor.post(this::attachMovementControls);
            return;
        }

        movementPanel = new LinearLayout(this);
        movementPanel.setOrientation(LinearLayout.VERTICAL);
        movementPanel.setGravity(Gravity.CENTER);
        movementPanel.setPadding(dp(8), dp(5), dp(8), dp(7));
        movementPanel.setBackground(roundedBackground(
            Color.argb(168, 11, 20, 32), 18, Color.argb(90, 220, 235, 250)));
        movementPanel.setElevation(dp(10));

        movementPanel.addView(panelLabel("MOVE", 10.5F),
            new LinearLayout.LayoutParams(dp(156), dp(19)));
        movementJoystick = new JoystickView();
        movementPanel.addView(movementJoystick,
            new LinearLayout.LayoutParams(dp(156), dp(156)));
        movementPanel.addView(panelLabel("UP  ·  DOWN  ·  LEFT  ·  RIGHT", 8.5F),
            new LinearLayout.LayoutParams(dp(156), dp(17)));

        try {
            getWindowManager().addView(movementPanel, panelLayout(
                Gravity.BOTTOM | Gravity.START, 12, 12,
                "ARUT-HARDT movement joystick"));
            movementPanelAttached = true;
            Log.i(TAG, "Visible movement joystick attached; downward input maps to backward movement");
        } catch (WindowManager.BadTokenException | IllegalStateException error) {
            Log.e(TAG, "Could not attach movement joystick", error);
        }
    }

    private void attachCombatControls() {
        if (combatPanelAttached || isFinishing()) return;
        final View decor = getWindow().getDecorView();
        if (decor.getWindowToken() == null) {
            decor.post(this::attachCombatControls);
            return;
        }

        combatPanel = new LinearLayout(this);
        combatPanel.setOrientation(LinearLayout.VERTICAL);
        combatPanel.setPadding(dp(6), dp(5), dp(6), dp(6));
        combatPanel.setElevation(dp(12));
        combatPanel.setBackground(roundedBackground(
            Color.argb(218, 11, 20, 32), 14, Color.argb(90, 220, 235, 250)));

        combatPanel.addView(panelLabel("COMBAT TEST", 10.5F),
            new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(20)));
        addRow(combatPanel, actionButton("JAB", 0, true),
            actionButton("HEAVY", 1, false), actionButton("KICK", 2, false));
        addRow(combatPanel, guardButton(), actionButton("DODGE", 4, false),
            actionButton("JUMP", 7, false));
        addRow(combatPanel, actionButton("HIT*", 5, false),
            actionButton("RECOVER*", 6, false));

        final TextView note = panelLabel("* diagnostic actions", 8.5F);
        note.setTextColor(Color.LTGRAY);
        combatPanel.addView(note, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(17)));

        try {
            getWindowManager().addView(combatPanel, panelLayout(
                Gravity.BOTTOM | Gravity.END, 10, 12,
                "ARUT-HARDT combat test controls"));
            combatPanelAttached = true;
            Log.i(TAG, "Compact combat test panel attached");
        } catch (WindowManager.BadTokenException | IllegalStateException error) {
            Log.e(TAG, "Could not attach combat test panel", error);
        }
    }

    /** Visible analog stick. Screen-down is negative local Y (backward). */
    private final class JoystickView extends View {
        private final Paint basePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint ringPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint knobPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint centerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private float axisX;
        private float axisY;
        private int activePointerId = MotionEvent.INVALID_POINTER_ID;

        JoystickView() {
            super(EngineNativeActivity.this);
            setContentDescription("Movement joystick. Drag up to move forward and down to move backward.");
            basePaint.setColor(Color.argb(105, 105, 145, 174));
            ringPaint.setColor(Color.argb(190, 220, 235, 248));
            ringPaint.setStyle(Paint.Style.STROKE);
            ringPaint.setStrokeWidth(dp(2));
            knobPaint.setColor(Color.argb(235, 235, 245, 252));
            centerPaint.setColor(Color.rgb(36, 91, 112));
            setClickable(true);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            final float cx = getWidth() * 0.5F;
            final float cy = getHeight() * 0.5F;
            final float radius = Math.min(getWidth(), getHeight()) * 0.39F;
            final float travel = radius * 0.62F;
            canvas.drawCircle(cx, cy, radius, basePaint);
            canvas.drawCircle(cx, cy, radius, ringPaint);
            canvas.drawCircle(cx, cy, radius * 0.58F, ringPaint);
            canvas.drawLine(cx - radius * 0.74F, cy, cx + radius * 0.74F, cy, ringPaint);
            canvas.drawLine(cx, cy - radius * 0.74F, cx, cy + radius * 0.74F, ringPaint);
            canvas.drawCircle(cx + axisX * travel, cy - axisY * travel,
                radius * 0.37F, knobPaint);
            canvas.drawCircle(cx + axisX * travel, cy - axisY * travel,
                radius * 0.13F, centerPaint);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            final int action = event.getActionMasked();
            if (action == MotionEvent.ACTION_DOWN) {
                activePointerId = event.getPointerId(0);
                updateAxis(event.getX(0), event.getY(0));
                return true;
            }
            if (action == MotionEvent.ACTION_MOVE && activePointerId != MotionEvent.INVALID_POINTER_ID) {
                final int index = event.findPointerIndex(activePointerId);
                if (index >= 0) updateAxis(event.getX(index), event.getY(index));
                return true;
            }
            if (action == MotionEvent.ACTION_POINTER_UP &&
                event.getPointerId(event.getActionIndex()) == activePointerId) {
                releaseStick();
                return true;
            }
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL) {
                releaseStick();
                performClick();
                return true;
            }
            return activePointerId != MotionEvent.INVALID_POINTER_ID;
        }

        private void updateAxis(float x, float y) {
            final float cx = getWidth() * 0.5F;
            final float cy = getHeight() * 0.5F;
            final float radius = Math.max(1.0F, Math.min(getWidth(), getHeight()) * 0.39F);
            float dx = (x - cx) / radius;
            float dy = (y - cy) / radius;
            final float length = (float) Math.sqrt(dx * dx + dy * dy);
            if (length > 1.0F) {
                dx /= length;
                dy /= length;
            }
            final float magnitude = Math.min(1.0F, length);
            if (magnitude <= 0.08F) {
                axisX = axisY = 0.0F;
            } else {
                final float remapped = (magnitude - 0.08F) / 0.92F;
                axisX = dx / Math.max(length, 0.0001F) * remapped;
                axisY = -dy / Math.max(length, 0.0001F) * remapped;
            }
            nativeSetMovementInput(axisX, axisY);
            invalidate();
        }

        private void releaseStick() {
            activePointerId = MotionEvent.INVALID_POINTER_ID;
            axisX = axisY = 0.0F;
            nativeSetMovementInput(0.0F, 0.0F);
            invalidate();
        }

        void resetFromSystem() {
            releaseStick();
        }

        @Override
        public boolean performClick() {
            super.performClick();
            return true;
        }
    }
}
