package com.arut.engine;

import android.app.NativeActivity;
import android.graphics.Color;
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

/**
 * Starts the native engine and displays combat test controls in an application
 * panel. NativeActivity renders through its own native window, so an ordinary
 * addContentView() child can be composited underneath the Vulkan scene.
 */
public final class EngineNativeActivity extends NativeActivity {
    private static final String TAG = "ARUT-HARDT-UI";
    private static native void nativeSetCombatAction(int action, boolean pressed);

    private LinearLayout combatPanel;
    private boolean combatPanelAttached;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "EngineNativeActivity created; profile="
            + getIntent().getIntExtra("arut.profile_id", -1));
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus && isCombatProfile() && !combatPanelAttached) {
            // Wait until the Activity window has a valid token before adding
            // the application-panel window above the native rendering window.
            getWindow().getDecorView().post(this::attachCombatControls);
        }
    }

    @Override
    protected void onDestroy() {
        if (combatPanelAttached && combatPanel != null) {
            try {
                getWindowManager().removeView(combatPanel);
            } catch (IllegalArgumentException ignored) {
                // The panel may already have been removed during shutdown.
            }
            combatPanelAttached = false;
        }
        super.onDestroy();
    }

    private boolean isCombatProfile() {
        final int profile = getIntent().getIntExtra("arut.profile_id", 0);
        return profile == 1 || profile == 2;
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private Button actionButton(String label, int action) {
        final Button button = new Button(this);
        button.setText(label);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.0F);
        button.setAllCaps(false);
        button.setTextColor(Color.WHITE);
        button.setMinWidth(dp(70));
        button.setMinHeight(dp(44));
        button.setBackgroundTintList(android.content.res.ColorStateList.valueOf(
            Color.rgb(47, 62, 82)));
        button.setOnClickListener(view -> {
            Log.i(TAG, "combat button pressed: " + label + " action=" + action);
            nativeSetCombatAction(action, true);
        });
        return button;
    }

    private Button guardButton() {
        final Button button = actionButton("Guard", 3);
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
                new LinearLayout.LayoutParams(dp(88), dp(48));
            params.setMargins(dp(2), dp(2), dp(2), dp(2));
            row.addView(button, params);
        }
        panel.addView(row, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    private void attachCombatControls() {
        if (combatPanelAttached || isFinishing()) return;

        final View decor = getWindow().getDecorView();
        final android.os.IBinder token = decor.getWindowToken();
        if (token == null) {
            Log.w(TAG, "Combat controls deferred: Activity window token is not ready");
            decor.post(this::attachCombatControls);
            return;
        }

        combatPanel = new LinearLayout(this);
        combatPanel.setOrientation(LinearLayout.VERTICAL);
        combatPanel.setPadding(dp(6), dp(5), dp(6), dp(6));
        combatPanel.setElevation(dp(12));

        final GradientDrawable background = new GradientDrawable();
        background.setColor(Color.argb(225, 15, 23, 42));
        background.setCornerRadius(dp(10));
        combatPanel.setBackground(background);

        final TextView title = new TextView(this);
        title.setText("TEMPORARY A / B / C ATTACK TEST");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.0F);
        title.setGravity(Gravity.CENTER);
        combatPanel.addView(title, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(20)));

        addRow(combatPanel, actionButton("A · JAB", 0), actionButton("B · HEAVY", 1),
               actionButton("C · KICK", 2));
        addRow(combatPanel, guardButton(), actionButton("Dodge", 4),
               actionButton("Jump", 7));
        addRow(combatPanel, actionButton("Hit*", 5), actionButton("Recover*", 6));

        final TextView note = new TextView(this);
        note.setText("A = Jab  ·  B = Heavy Punch  ·  C = Kick  |  * diagnostic");
        note.setTextColor(Color.LTGRAY);
        note.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9.0F);
        note.setGravity(Gravity.CENTER);
        combatPanel.addView(note, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(18)));

        final WindowManager.LayoutParams overlay = new WindowManager.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
            PixelFormat.TRANSLUCENT);
        overlay.token = token;
        overlay.gravity = Gravity.BOTTOM | Gravity.END;
        overlay.x = dp(10);
        overlay.y = dp(12);
        overlay.setTitle("ARUT-HARDT combat test controls");

        try {
            getWindowManager().addView(combatPanel, overlay);
            combatPanelAttached = true;
            Log.i(TAG, "Combat test panel attached above native window; profile="
                + getIntent().getIntExtra("arut.profile_id", -1));
        } catch (WindowManager.BadTokenException | IllegalStateException error) {
            Log.e(TAG, "Could not attach combat test panel", error);
        }
    }
}
