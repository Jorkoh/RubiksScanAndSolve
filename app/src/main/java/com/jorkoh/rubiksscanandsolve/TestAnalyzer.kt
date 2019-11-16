package com.jorkoh.rubiksscanandsolve

import android.graphics.ImageFormat
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import com.jorkoh.rubiksscanandsolve.rubikdetector.config.DrawConfig
import org.opencv.android.Utils
import java.nio.ByteBuffer
import java.util.*
import java.util.concurrent.TimeUnit
import kotlin.collections.ArrayList

typealias LumaListener = (luma: Double, fps: Double) -> Unit

class TestAnalyzer(listener: LumaListener? = null) : ImageAnalysis.Analyzer {
    private val listeners = ArrayList<LumaListener>().apply { listener?.let { add(it) } }
//    fun onFrameAnalyzed(listener: LumaListener) = listeners.add(listener)

    private var lastAnalyzedTimestamp = 0L


    override fun analyze(image: ImageProxy, rotationDegrees: Int) {
        // If there are no listeners attached, we don't need to perform analysis
        if (listeners.isEmpty()) return

        // Calculate the average luma no more often than every second
        val currentTimestamp = System.currentTimeMillis()
        if (currentTimestamp - lastAnalyzedTimestamp >= TimeUnit.SECONDS.toMillis(1)) {
            lastAnalyzedTimestamp = currentTimestamp

            listeners.forEach { it(0.0, 0.0) }
        }
    }

    private external fun adaptiveThresholdFromJNI(matAddr: Long)
}