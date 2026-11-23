package com.arut.engine;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.DialogInterface;
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
                    .setMessage("Choose one isolated runtime profile. You can return here by restarting the app.")
                    .setItems(PROFILES, new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            nativeSelectProfile(which);
                        }
                    })
                    .setCancelable(false)
                    .show();
            }
        });
    }

    private static native void nativeSelectProfile(int profileId);
}
