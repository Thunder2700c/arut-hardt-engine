package com.arut.engine;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.os.Bundle;

/** Selects the profile before NativeActivity starts its native engine thread. */
public final class TestLabActivity extends Activity {
    static {
        System.loadLibrary("arut_android");
    }

    private static final CharSequence[] PROFILES = {
        "ECS Stress Test — 5,000 entities and renderer load",
        "Humanoid Test — mannequin and character systems",
        "Combined Integration — stress scene plus humanoid"
    };

    private static native void nativeSelectProfile(int profileId);
    private static native void nativeSetImportedCharacterEnabled(boolean enabled);

    private int selectedProfileId;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        showTestLaboratory();
    }

    private void showTestLaboratory() {
        new AlertDialog.Builder(this)
            .setTitle("ARUT-HARDT Test Laboratory — Select a profile")
            .setItems(PROFILES, (dialog, which) -> {
                // This must run BEFORE NativeActivity.onCreate starts android_main.
                nativeSelectProfile(which);
                selectedProfileId = which;
                if (which == 1 || which == 2) {
                    showCharacterSourceChoice();
                } else {
                    launchEngine(false);
                }
            })
            .setCancelable(false)
            .show();
    }

    private void showCharacterSourceChoice() {
        final CharSequence[] sources = {
            "Procedural robot mannequin (default)",
            "Optional imported GLB character"
        };
        new AlertDialog.Builder(this)
            .setTitle("Choose the humanoid character")
            .setItems(sources, (dialog, which) -> launchEngine(which == 1))
            .setCancelable(false)
            .show();
    }

    private void launchEngine(boolean useImportedCharacter) {
        nativeSetImportedCharacterEnabled(useImportedCharacter);
        Intent intent = new Intent(TestLabActivity.this, EngineNativeActivity.class);
        intent.putExtra("arut.profile_id", selectedProfileId);
        startActivity(intent);
        finish();
    }
}
