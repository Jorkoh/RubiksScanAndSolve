package com.jorkoh.rubiksscanandsolve.scan

import android.os.Environment
import androidx.camera.core.ImageProxy
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetector
import com.jorkoh.rubiksscanandsolve.rubikdetector.RubikDetectorUtils
import com.jorkoh.rubiksscanandsolve.rubikdetector.model.CubeState
import com.jorkoh.rubiksscanandsolve.rubikdetector.model.toSolverScramble
import com.jorkoh.rubiksscanandsolve.rubiksolver.Search
import com.jorkoh.rubiksscanandsolve.scan.ScanViewModel.ScanStages.*
import java.nio.ByteBuffer

class ScanViewModel : ViewModel() {

    enum class ScanStages {
        PRE_FIRST_SCAN,
        FIRST_SCAN,
        FIRST_PHOTO,
        PRE_SECOND_SCAN,
        SECOND_SCAN,
        SECOND_PHOTO,
        DONE
    }

    val scanStage: LiveData<ScanStages>
        get() = _scanStage
    private val _scanStage = MutableLiveData<ScanStages>(PRE_FIRST_SCAN)

    val scanResult: LiveData<CubeState>
        get() = _scanResult
    private val _scanResult = MutableLiveData<CubeState>()

    val flashEnabled: LiveData<Boolean>
        get() = _flashEnabled
    private val _flashEnabled = MutableLiveData<Boolean>(false)

    private val rubikDetector: RubikDetector = RubikDetector.Builder()
        .scanRotation(90)
        .scanSize(640, 480)
        .photoRotation(90)
        .photoSize(4032, 3024)
        .imageSavePath("${Environment.getExternalStorageDirectory()}/Rubik/")
        .build()

    private var scanDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)

    fun processScanFrame(image: ImageProxy, rotation: Int) {
        if (scanStage.value != FIRST_SCAN && scanStage.value != SECOND_SCAN) {
            // Not actively scanning, ignore the frame
            return
        }

        if (image.width != rubikDetector.scanWidth || image.height != rubikDetector.scanHeight || rotation != rubikDetector.scanRotation) {
            rubikDetector.updateScanProperties(rotation, image.width, image.height)
            scanDataBuffer = ByteBuffer.allocateDirect(rubikDetector.requiredMemory)
        }

        val imageData = RubikDetectorUtils.YUV_420_888toNV21(image.image)
        scanDataBuffer.clear()
        scanDataBuffer.put(imageData, 0, imageData.size)

        if (rubikDetector.scanCube(scanDataBuffer)) {
            when (scanStage.value) {
                FIRST_SCAN -> {
                    _scanStage.postValue(FIRST_PHOTO)
                }
                SECOND_SCAN -> {
                    _scanStage.postValue(SECOND_PHOTO)
                }
                else -> {
                    // Do nothing
                }
            }
        }
    }

    fun processPhoto(image: ImageProxy, rotation: Int) {
        if (scanStage.value != FIRST_PHOTO && scanStage.value != SECOND_PHOTO) {
            // Not taking a photo, ignore it
            return
        }

        if (image.width != rubikDetector.photoWidth || image.height != rubikDetector.photoHeight || rotation != rubikDetector.photoRotation) {
            rubikDetector.updatePhotoProperties(rotation, image.width, image.height)
        }

        if (rubikDetector.extractFacelets(scanDataBuffer, RubikDetectorUtils.YUV_420_888toNV21(image.image))) {
            when (scanStage.value) {
                FIRST_PHOTO -> {
                    _scanStage.postValue(PRE_SECOND_SCAN)
                }
                SECOND_PHOTO -> {
                    _scanResult.postValue(rubikDetector.analyzeColors(scanDataBuffer))
                    _scanStage.postValue(DONE)
                }
                else -> {
                    // Do nothing
                }
            }
        } else {
            when (scanStage.value) {
                FIRST_PHOTO -> {
                    _scanStage.postValue(FIRST_SCAN)
                }
                SECOND_PHOTO -> {
                    _scanStage.postValue(SECOND_SCAN)
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
                rubikDetector.updateScanPhase(false)
                _scanStage.postValue(FIRST_SCAN)
            }
            PRE_SECOND_SCAN -> {
                rubikDetector.updateScanPhase(true)
                _scanStage.postValue(SECOND_SCAN)
            }
            FIRST_SCAN, FIRST_PHOTO -> {
                _scanStage.postValue(PRE_FIRST_SCAN)
            }
            SECOND_SCAN, SECOND_PHOTO -> {
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