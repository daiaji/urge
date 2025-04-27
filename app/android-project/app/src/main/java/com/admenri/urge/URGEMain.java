package com.admenri.urge;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.net.Uri;
import android.provider.Settings;
import android.util.Log;

import org.libsdl.app.SDLActivity;

public class URGEMain extends SDLActivity {
private static final String TAG = "URGEActivity";
    public static String GAME_PATH;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // External storage dirs
        java.io.File data_dir = getContext().getExternalFilesDir("");
        if (data_dir != null) {
            GAME_PATH = data_dir.toString();
            Log.i(TAG, GAME_PATH);
        }
    }
}
