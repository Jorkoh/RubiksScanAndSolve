package com.jorkoh.rubiksscanandsolve.scan

import android.os.Environment
import androidx.camera.core.ImageProxy
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import com.jorkoh.rubiksscanandsolve.scan.ScanViewModel.ScanStages.*
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

    val scanResult: LiveData<String>
        get() = _scanResult
    private val _scanResult = MutableLiveData<String>("Not scanned yet")

    val flashEnabled: LiveData<Boolean>
        get() = _flashEnabled
    private val _flashEnabled = MutableLiveData<Boolean>(false)

    private val rubikDetector: RubikDetector = RubikDetector.Builder()
        .inputFrameRotation(90)
        .inputFrameSize(640, 480)
        .imageSavePath("${Environment.getExternalStorageDirectory()}/Rubik/")
        .build()

    private var imageDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)

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
                    _scanStage.postValue(PRE_SECOND_SCAN)
                }
                SECOND_SCAN -> {
                    _scanResult.postValue(rubikDetector.analyzeColors(imageDataBuffer))
                    _scanStage.postValue(DONE)
                }
                else -> {
                    // Do nothing
                }
            }
        }
    }

    fun startStopScanning() {
        when (scanStage.value) {
            PRE_FIRST_SCAN -> {
                _scanStage.postValue(FIRST_SCAN)
            }
            PRE_SECOND_SCAN -> {
                rubikDetector.updateScanPhase(true)
                _scanStage.postValue(SECOND_SCAN)
            }
            FIRST_SCAN -> {
                _scanStage.postValue(PRE_FIRST_SCAN)
            }
            SECOND_SCAN -> {
                _scanStage.postValue(PRE_SECOND_SCAN)
            }
            else -> {
                // Do nothing
            }
        }
    }

    fun switchFlash() {
        _flashEnabled.postValue(!(flashEnabled.value ?: false))
    }

    override fun onCleared() {
        super.onCleared()
        rubikDetector.releaseResources()
    }

}