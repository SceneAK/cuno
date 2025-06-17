package com.sceneak.cunoandr;

import com.google.androidgamesdk.GameActivity;

public class GameAppActivity extends GameActivity {
    static {
        System.loadLibrary("cuno"); 
    }

    // override callbacks if needed, e.g. onPause, onResume, etc.
}
