package com.arut.engine;

import android.app.NativeActivity;
import android.os.Bundle;

/** Owns the native engine thread; no Java dialog or overlay is shown on this activity. */
public final class EngineNativeActivity extends NativeActivity {
    static final String EXTRA_PROFILE_ID = "com.arut.engine.EXTRA_PROFILE_ID";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        int profileId = getIntent().getIntExtra(EXTRA_PROFILE_ID, -1);
        nativeSelectProfile(profileId);
    }

    private static native void nativeSelectProfile(int profileId);
}
