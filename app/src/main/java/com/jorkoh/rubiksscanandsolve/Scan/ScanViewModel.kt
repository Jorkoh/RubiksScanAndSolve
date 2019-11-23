package com.jorkoh.rubiksscanandsolve.Scan

import android.os.Environment
import androidx.camera.core.ImageProxy
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.jorkoh.rubiksscanandsolve.Scan.ScanViewModel.ScanStages.*
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import java.nio.ByteBuffer

class ScanViewModel : ViewModel() {

    enum class ScanStages {
        PRE_FIRST_SCAN,
        FIRST_SCAN,
        PRE_SECOND_SCAN,
        SECOND_SCAN,
        DONE
    }

    val scanStage: LiveData<ScanStages>
        get() = _scanStage

    private val _scanStage = MutableLiveData<ScanStages>(PRE_FIRST_SCAN)

    private val rubikDetector: RubikDetector = RubikDetector.Builder()
        .inputFrameRotation(90)
        // the input image will have a resolution of 640x480
        .inputFrameSize(640, 480)
        // save images of the process
        .imageSavePath("${Environment.getExternalStorageDirectory()}/Rubik/")
        // builds the RubikDetector for the given params.
        .build()

    private var imageDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)
    private val faceletsBuffer = ByteBuffer.allocate(rubikDetector.resultByteCount * 2)

    fun processFrame(image: ImageProxy, rotation: Int) {
        if (scanStage.value != FIRST_SCAN && scanStage.value != SECOND_SCAN) {
            // Not actively scanning, ignore the frame
            return
        }

        if (image.width != rubikDetector.frameWidth || image.height != rubikDetector.frameHeight || rotation != rubikDetector.frameRotation) {
            rubikDetector.updateImageProperties(rotation, image.width, image.height)
            imageDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)
        }

        val imageData = RubikDetectorUtils.YUV_420_888toNV21(image.image)
        imageDataBuffer.clear()
        imageDataBuffer.put(imageData, 0, imageData.size)

        if (rubikDetector.findCube(imageDataBuffer)) {
            when (scanStage.value) {
                FIRST_SCAN -> {
                    faceletsBuffer.put(
                        imageDataBuffer.array(),
                        rubikDetector.resultBufferOffset,
                        rubikDetector.resultByteCount
                    )
                    _scanStage.postValue(PRE_SECOND_SCAN)
                }
                SECOND_SCAN -> {
                    faceletsBuffer.put(
                        imageDataBuffer.array(),
                        rubikDetector.resultBufferOffset + rubikDetector.resultByteCount,
                        rubikDetector.resultByteCount
                    )
                    _scanStage.postValue(DONE)
                    //TODO Get the colors, maybe add an extra stage before or instead of done?
                }
                else -> {
                    // Do nothing
                }
            }
        }
    }

    fun startScanning(){
        when(scanStage.value){
            PRE_FIRST_SCAN -> _scanStage.postValue(FIRST_SCAN)
            PRE_SECOND_SCAN -> _scanStage.postValue(SECOND_SCAN)
            else -> {
                // Do nothing
            }
        }
    }

}