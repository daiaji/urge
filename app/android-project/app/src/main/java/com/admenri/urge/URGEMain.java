package com.admenri.urge;

import android.os.Bundle;
import android.util.Log;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import org.libsdl.app.SDLActivity;

public class URGEMain extends SDLActivity {
    private static final String TAG = "URGEActivity";
    public static String GAME_PATH;
    public static String APP_MD5;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Setup external storage dirs
        java.io.File data_dir = getContext().getExternalFilesDir("");
        if (data_dir != null) {
            GAME_PATH = data_dir.toString();
            Log.i(TAG, GAME_PATH);
        }

        // Setup apk md5
        APP_MD5 = getApkMd5(getContext());
    }

    public static String getApkMd5(Context context) {
        try {
            // 1. Apk file path
            PackageInfo packageInfo = context.getPackageManager()
                    .getPackageInfo(context.getPackageName(), 0);
            String apkPath = packageInfo.applicationInfo.sourceDir;
            File apkFile = new File(apkPath);

            // 2. Calculate md5
            MessageDigest md = MessageDigest.getInstance("MD5");
            try (FileInputStream fis = new FileInputStream(apkFile)) {
                byte[] buffer = new byte[10 << 16];
                int length;
                while ((length = fis.read(buffer)) != -1) {
                    md.update(buffer, 0, length);
                }
            }

            // 3. To hex string
            byte[] digest = md.digest();
            StringBuilder sb = new StringBuilder();
            for (byte b : digest) {
                sb.append(String.format("%02x", b & 0xff));
            }
            return sb.toString();
        } catch (PackageManager.NameNotFoundException | NoSuchAlgorithmException | IOException e) {
            e.printStackTrace();
            return null;
        }
    }
}
