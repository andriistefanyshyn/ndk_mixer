package com.restingrobots.ndkmixer

import android.content.res.AssetManager
import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        createMixer()
        btn.setOnClickListener {
            addTrack(assets, "cartoon001.wav", 0)
            addTrack(assets, "cartoon002.wav", 1)
            addTrack(assets, "cartoon003.wav", 2)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        deleteMixer()
    }

    external fun deleteMixer()
    external fun createMixer()
    external fun addTrack(assetManager: AssetManager, fileName: String, slot: Int)

    companion object {
        // Used to load the 'native-lib' library on application startup.
        init {
            System.loadLibrary("mixer")
        }
    }
}
