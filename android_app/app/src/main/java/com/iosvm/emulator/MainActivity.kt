package com.iosvm.emulator

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.View
import android.widget.ImageView
import android.widget.TextView

class MainActivity : Activity() {

    private lateinit var displayView: ImageView
    private lateinit var logView: TextView
    private lateinit var screenBitmap: Bitmap
    private val handler = Handler(Looper.getMainLooper())
    private val frameWidth = 390
    private val frameHeight = 844

    companion object {
        init {
            System.loadLibrary("ios_vm")
        }
    }

    // Native JNI functions
    external fun nativeInit(width: Int, height: Int)
    external fun nativeLoadApp(path: String): Boolean
    external fun nativeRenderFrame(bitmap: Bitmap)
    external fun nativeTouchEvent(x: Float, y: Float, action: Int)
    external fun nativeGetLogs(): String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        displayView = findViewById(R.id.ios_display_view)
        logView = findViewById(R.id.console_log_view)

        // Initialize iOS virtual screen bitmap
        screenBitmap = Bitmap.createBitmap(frameWidth, frameHeight, Bitmap.Config.ARGB_8888)
        displayView.setImageBitmap(screenBitmap)

        // Initialize Darwin and Compositor engine
        nativeInit(frameWidth, frameHeight)

        // Setup touch handling forwarded to iOS responder chain
        displayView.setOnTouchListener { _, event ->
            val scaleX = frameWidth.toFloat() / displayView.width.toFloat()
            val scaleY = frameHeight.toFloat() / displayView.height.toFloat()
            val iosX = event.x * scaleX
            val iosY = event.y * scaleY

            nativeTouchEvent(iosX, iosY, event.actionMasked)
            true
        }

        // Start 60 FPS Render Loop
        startDisplayRefreshLoop()
    }

    private fun startDisplayRefreshLoop() {
        handler.post(object : Runnable {
            override fun run() {
                // Render current iOS frame buffer
                nativeRenderFrame(screenBitmap)
                displayView.invalidate()

                // Update console output
                logView.text = nativeGetLogs()

                // 16ms = ~60 FPS
                handler.postDelayed(this, 16)
            }
        })
    }
}
