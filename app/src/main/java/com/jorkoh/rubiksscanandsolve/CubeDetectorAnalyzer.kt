package com.jorkoh.rubiksscanandsolve

import android.graphics.Bitmap
import android.graphics.ImageFormat
import android.os.Environment
import android.util.Log
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import com.jorkoh.rubiksscanandsolve.rubikdetector.config.DrawConfig
import java.nio.ByteBuffer

typealias DetectorListener = (detected: Boolean, lastImageWithDetection: Bitmap) -> Unit

class CubeDetectorAnalyzer(listener: DetectorListener? = null) : ImageAnalysis.Analyzer {

    private val listeners = ArrayList<DetectorListener>().apply { listener?.let { add(it) } }
    private var lastAnalyzedTimestamp = 0L
    private var areFaceletsDetected = false

    fun onFrameAnalyzed(listener: DetectorListener) = listeners.add(listener)

    // TODO this stuff should be initialized once we know the size of the images
    private val rubikDetector: RubikDetector = RubikDetector.Builder()
        .inputFrameRotation(90)
        // the input image will have a resolution of 640x480
        .inputFrameSize(640, 480)
        // the input image data will be stored in the YUV NV21 format
        .inputFrameFormat(RubikDetectorUtils.convertAndroidImageFormat(ImageFormat.NV21))
        // draw found facelets as colored rectangles
        .drawConfig(DrawConfig.FilledCircles())
        // mark as debuggable
        .debuggable(true)
        // save images of the process
        .imageSavePath("${Environment.getExternalStorageDirectory()}/Rubik/")
        // builds the RubikDetector for the given params.
        .build()
    // allocate a ByteBuffer of the capacity required by the RubikDetector
    private val imageDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)

    // allocate a buffer of sufficient capacity to store the output frame.
    private val drawingBuffer = ByteBuffer.allocate(rubikDetector.resultFrameByteCount)

    // create a bitmap for the output frame
    private val drawingBitmap = Bitmap.createBitmap(640, 480, Bitmap.Config.ARGB_8888)

    override fun analyze(image: ImageProxy, rotationDegrees: Int) {
        // If there are no listeners attached, we don't need to perform analysis
        if (listeners.isEmpty()) return

        // Try to detect no more often that every quarter of a second
        val currentTimestamp = System.currentTimeMillis()
        if (currentTimestamp - lastAnalyzedTimestamp >= 5000) {
            lastAnalyzedTimestamp = currentTimestamp

            Log.d("RESULTS", "Analyzer image rotation degrees: $rotationDegrees")
            Log.d("RESULTS", "Analyzer image width: ${image.width}, analyzer image height ${image.height}")

            // Couldn't make YUV_420_888 work with native so let's use NV21
            RubikDetectorUtils.YUV_420_888toNV21(image.image).also {
                imageDataBuffer.clear()
                imageDataBuffer.put(it, 0, it.size)
            }

            val imageHolderArray = imageDataBuffer.array()

            // call findCube passing the backing array of the ByteBuffer as a parameter to the RubikDetector
            val facelets = rubikDetector.findCube(imageHolderArray)

            val newAreFaceletsDetected = facelets != null
            if (newAreFaceletsDetected != areFaceletsDetected) {
                areFaceletsDetected = newAreFaceletsDetected
            }

            if (areFaceletsDetected) {
                Log.d("RESULTS", "Found facelets!")

                //copy the output frame data into the buffer
                drawingBuffer.put(
                    imageHolderArray,
                    rubikDetector.resultFrameBufferOffset,
                    rubikDetector.resultFrameByteCount
                )
                drawingBuffer.rewind()

                // write the output frame data to the bitmap
                drawingBitmap.copyPixelsFromBuffer(drawingBuffer)
                drawingBuffer.clear()
            }

            listeners.forEach { listener ->
                listener(areFaceletsDetected, drawingBitmap)
            }
        }
    }
}