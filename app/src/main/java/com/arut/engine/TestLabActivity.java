package com.arut.engine;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;

/** Java-only profile chooser; the native runtime is launched only after selection. */
public final class TestLabActivity extends Activity {
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
        new AlertDialog.Builder(this)
            .setTitle("ARUT-HARDT Test Laboratory — Select a profile")
            .setItems(PROFILES, (dialog, which) -> {
                Intent intent = new Intent(TestLabActivity.this, EngineNativeActivity.class);
                intent.putExtra(EngineNativeActivity.EXTRA_PROFILE_ID, which);
                startActivity(intent);
                finish();
            })
            .setCancelable(false)
            .show();
    }
}
