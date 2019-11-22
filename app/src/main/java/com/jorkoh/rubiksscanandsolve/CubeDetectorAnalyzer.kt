package com.jorkoh.rubiksscanandsolve

import android.os.Environment
import android.util.Log
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import java.nio.ByteBuffer

typealias DetectorListener = (detected: Boolean) -> Unit

class CubeDetectorAnalyzer(listener: DetectorListener? = null) : ImageAnalysis.Analyzer {

    //TODO temporary
    var detectFrame = false

    private val listeners = ArrayList<DetectorListener>().apply { listener?.let { add(it) } }
    private var lastAnalyzedTimestamp = 0L

    fun onFrameAnalyzed(listener: DetectorListener) = listeners.add(listener)

    // TODO this stuff should be initialized once we know the size of the images
    private val rubikDetector: RubikDetector = RubikDetector.Builder()
        .inputFrameRotation(90)
        // the input image will have a resolution of 640x480
        .inputFrameSize(640, 480)
        // save images of the process
        .imageSavePath("${Environment.getExternalStorageDirectory()}/Rubik/")
        // builds the RubikDetector for the given params.
        .build()
    // allocate a ByteBuffer of the capacity required by the RubikDetector
    private val imageDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)

    override fun analyze(image: ImageProxy, rotationDegrees: Int) {
        // If there are no listeners attached, we don't need to perform analysis
        if (listeners.isEmpty()) return

//        // Try to detect no more often that every quarter of a second
//        val currentTimestamp = System.currentTimeMillis()
//        if (currentTimestamp - lastAnalyzedTimestamp >= 200) {
//            lastAnalyzedTimestamp = currentTimestamp

//        if (detectFrame) {
//            detectFrame = false

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

            listeners.forEach { listener ->
                listener(facelets != null)
            }
//        }
    }
}