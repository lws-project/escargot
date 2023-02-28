package com.samsung.lwe.escargot.shell;

import androidx.appcompat.app.AppCompatActivity;

import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;

import com.samsung.lwe.escargot.Bridge;
import com.samsung.lwe.escargot.Context;
import com.samsung.lwe.escargot.Evaluator;
import com.samsung.lwe.escargot.Globals;
import com.samsung.lwe.escargot.JavaScriptValue;
import com.samsung.lwe.escargot.Memory;
import com.samsung.lwe.escargot.VMInstance;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.Optional;

public class MainActivity extends AppCompatActivity {

    private void copyAssets() {
        AssetManager assetManager = getAssets();
        String[] files = null;
        try {
            files = assetManager.list("");
        } catch (IOException e) {
            Log.e("tag", "Failed to get asset file list.", e);
        }
        for(String filename : files) {
            InputStream in = null;
            OutputStream out = null;
            try {
                in = assetManager.open(filename);
                File outFile = new File(getApplicationContext().getFilesDir(), filename);
                out = new FileOutputStream(outFile);
                copyFile(in, out);
                in.close();
                in = null;
                out.flush();
                out.close();
                out = null;
            } catch(IOException e) {
                Log.e("tag", "Failed to copy asset file: " + filename, e);
            }
        }

        Log.e("Escargot shell", getApplicationContext().getFilesDir().getAbsolutePath());
    }

    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while((read = in.read(buffer)) != -1){
            out.write(buffer, 0, read);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // copy assets to internal storage (for copying js files which are used by test)
        copyAssets();

        Globals.initializeGlobals();

        Log.i("Escargot", Globals.buildDate());
        Log.i("Escargot", Globals.version());

        Memory.setGCFrequency(24);

        VMInstance vmInstance = VMInstance.create(Optional.of("en-US"), Optional.empty());
        Context context = Context.create(vmInstance);

        Globals.finalizeGlobals();
    }
}