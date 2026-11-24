package com.arut.engine;

import android.app.NativeActivity;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

/** Starts the native engine and overlays native-backed touch controls on humanoid profiles. */
public final class EngineNativeActivity extends NativeActivity {
    private static native void nativeSetCombatAction(int action, boolean pressed);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final int profile = getIntent().getIntExtra("arut.profile_id", 0);
        if (profile == 1 || profile == 2) addCombatControls();
    }

    private int dp(float value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private Button actionButton(String label, int action) {
        final Button button = new Button(this);
        button.setText(label);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12.0F);
        button.setAllCaps(false);
        button.setTextColor(Color.WHITE);
        button.setMinWidth(dp(70));
        button.setMinHeight(dp(44));
        button.setBackgroundTintList(ColorStateList.valueOf(Color.rgb(47, 62, 82)));
        button.setOnClickListener(view -> nativeSetCombatAction(action, true));
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
                new LinearLayout.LayoutParams(dp(76), dp(46));
            params.setMargins(dp(2), dp(2), dp(2), dp(2));
            row.addView(button, params);
        }
        panel.addView(row, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    private void addCombatControls() {
        final LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setPadding(dp(6), dp(5), dp(6), dp(6));
        final GradientDrawable background = new GradientDrawable();
        background.setColor(Color.argb(205, 15, 23, 42));
        background.setCornerRadius(dp(10));
        panel.setBackground(background);

        final TextView title = new TextView(this);
        title.setText("COMBAT TEST");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 10.0F);
        title.setGravity(Gravity.CENTER);
        panel.addView(title, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(18)));

        addRow(panel, actionButton("Jab", 0), actionButton("Heavy", 1),
               actionButton("Kick", 2));
        addRow(panel, guardButton(), actionButton("Dodge", 4),
               actionButton("Jump", 7));
        addRow(panel, actionButton("Hit*", 5), actionButton("Recover*", 6));

        final TextView note = new TextView(this);
        note.setText("* diagnostic reactions");
        note.setTextColor(Color.LTGRAY);
        note.setTextSize(TypedValue.COMPLEX_UNIT_SP, 9.0F);
        note.setGravity(Gravity.CENTER);
        panel.addView(note, new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(16)));

        final FrameLayout.LayoutParams overlay = new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT,
            Gravity.BOTTOM | Gravity.END);
        overlay.setMargins(0, 0, dp(10), dp(12));
        addContentView(panel, overlay);
    }
}
