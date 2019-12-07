package com.jorkoh.rubiksscanandsolve.scan

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.PorterDuff
import android.hardware.display.DisplayManager
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.camera.core.*
import androidx.fragment.app.Fragment
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import androidx.navigation.fragment.findNavController
import com.jorkoh.rubiksscanandsolve.R
import com.jorkoh.rubiksscanandsolve.model.Solution
import com.jorkoh.rubiksscanandsolve.scan.ScanViewModel.ScanStages.*
import com.jorkoh.rubiksscanandsolve.scan.utils.AutoFitPreviewBuilder
import kotlinx.android.synthetic.main.fragment_scan.*
import permissions.dispatcher.*
import java.util.concurrent.Executors


@RuntimePermissions
class ScanFragment : Fragment() {

    private lateinit var scanVM: ScanViewModel

    private val executor = Executors.newCachedThreadPool()

    private var displayId = -1
    private var preview: Preview? = null
    private var imageAnalyzer: ImageAnalysis? = null
    private var imageCapture: ImageCapture? = null

    private lateinit var displayManager: DisplayManager
    private val displayListener = object : DisplayManager.DisplayListener {
        override fun onDisplayAdded(displayId: Int) = Unit
        override fun onDisplayRemoved(displayId: Int) = Unit
        override fun onDisplayChanged(displayId: Int) = view?.let { view ->
            if (displayId == this@ScanFragment.displayId) {
                preview?.setTargetRotation(view.display.rotation)
                imageAnalyzer?.setTargetRotation(view.display.rotation)
                imageCapture?.setTargetRotation(view.display.rotation)
            }
        } ?: Unit
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        retainInstance = true
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.fragment_scan, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        button_scan.setOnClickListener {
            scanVM.startStopScanning()
        }
        button_switch_flash.setOnClickListener {
            scanVM.switchFlash()
        }

        displayManager = view_finder.context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
        displayManager.registerDisplayListener(displayListener, null)

        view_finder.post {
            displayId = view_finder.display.displayId

            enableScannerWithPermissionCheck()
        }
    }

    @Suppress("WHEN_ENUM_CAN_BE_NULL_IN_JAVA")
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        scanVM = ViewModelProviders.of(this)[ScanViewModel::class.java]
        scanVM.scanStage.observe(viewLifecycleOwner, Observer { stage ->
            text_view_stage.text = when (stage) {
                PRE_FIRST_SCAN -> getString(R.string.stage_pre_first_scan)
                PRE_SECOND_SCAN -> getString(R.string.stage_pre_second_scan)
                FIRST_SCAN -> getString(R.string.stage_first_scan)
                SECOND_SCAN -> getString(R.string.stage_second_scan)
                FIRST_PHOTO -> getString(R.string.stage_first_photo)
                SECOND_PHOTO -> getString(R.string.stage_second_photo)
                FINISHED -> getString(R.string.stage_finished)
            }

            text_view_step_explanation.text = when(stage){
                PRE_FIRST_SCAN, FIRST_SCAN, FIRST_PHOTO -> getString(R.string.explanation_first_step)
                PRE_SECOND_SCAN, SECOND_SCAN, SECOND_PHOTO-> getString(R.string.explanation_second_step)
                FINISHED -> getString(R.string.explanation_finished)
            }

            when (stage) {
                PRE_FIRST_SCAN, PRE_SECOND_SCAN -> {
                    view_finder_overlay.background.setColorFilter(Color.GRAY, PorterDuff.Mode.SRC_ATOP)
                    button_scan.text = getString(R.string.start_scanner)
                }
                FIRST_SCAN, SECOND_SCAN -> {
                    view_finder_overlay.background.setColorFilter(Color.RED, PorterDuff.Mode.SRC_ATOP)
                    button_scan.text = getString(R.string.stop_scanner)
                }
                FIRST_PHOTO, SECOND_PHOTO -> {
                    view_finder_overlay.background.setColorFilter(Color.BLUE, PorterDuff.Mode.SRC_ATOP)
                    imageCapture?.takePicture(executor, object : ImageCapture.OnImageCapturedListener() {
                        override fun onCaptureSuccess(image: ImageProxy, rotationDegrees: Int) {
                            scanVM.processPhoto(image, rotationDegrees)
                            super.onCaptureSuccess(image, rotationDegrees)
                        }
                    })
                }
                FINISHED -> {
                    view_finder_overlay.background.setColorFilter(Color.GREEN, PorterDuff.Mode.SRC_ATOP)
                    button_scan.text = getString(R.string.disabled_scanner)
                }
            }

            button_scan.isEnabled = stage != FINISHED
        })
        scanVM.solution.observe(viewLifecycleOwner, Observer { solution ->
            findNavController().navigate(
                ScanFragmentDirections.actionScanFragmentToSolveFragment(solution)
            )
        })
        scanVM.flashEnabled.observe(viewLifecycleOwner, Observer { flashEnabled ->
            preview?.enableTorch(flashEnabled)
            button_switch_flash.setBackgroundResource(
                if (flashEnabled) {
                    R.drawable.ic_flash_off_black_24dp
                } else {
                    R.drawable.ic_flash_on_black_24dp
                }
            )
        })
    }

    @SuppressLint("RestrictedApi")
    @NeedsPermission(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun enableScanner() {
        // ViewFinder
        val viewFinderConfig = PreviewConfig.Builder().apply {
            setLensFacing(CameraX.LensFacing.BACK)
            setTargetAspectRatio(AspectRatio.RATIO_4_3)
            setTargetRotation(view_finder.display.rotation)
        }.build()
        preview = AutoFitPreviewBuilder.build(viewFinderConfig, view_finder)

        // Analyzer
        val analyzerConfig = ImageAnalysisConfig.Builder().apply {
            setLensFacing(CameraX.LensFacing.BACK)
            setTargetAspectRatio(AspectRatio.RATIO_4_3)
            setTargetRotation(view_finder.display.rotation)
            setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
        }.build()
        imageAnalyzer = ImageAnalysis(analyzerConfig).apply {
            setAnalyzer(executor, ImageAnalysis.Analyzer { image, rotation ->
                scanVM.processScanFrame(image, rotation)
            })
        }

        // Set up the capture use case to allow users to take photos
        val imageCaptureConfig = ImageCaptureConfig.Builder().apply {
            setLensFacing(CameraX.LensFacing.BACK)
            setBufferFormat(ImageFormat.YUV_420_888)
            setCaptureMode(ImageCapture.CaptureMode.MAX_QUALITY)
            setTargetAspectRatio(AspectRatio.RATIO_4_3)
            setTargetRotation(view_finder.display.rotation)
        }.build()

        imageCapture = ImageCapture(imageCaptureConfig)

        CameraX.bindToLifecycle(viewLifecycleOwner, preview, imageAnalyzer, imageCapture)
    }

    override fun onDestroyView() {
        super.onDestroyView()

        displayManager.unregisterDisplayListener(displayListener)
    }

    // Permissions stuff down here

    @OnShowRationale(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun showRationaleForCamera(request: PermissionRequest) {
        AlertDialog.Builder(requireContext())
            .setPositiveButton("positive") { _, _ -> request.proceed() }
            .setNegativeButton("negative") { _, _ -> request.cancel() }
            .setCancelable(false)
            .setMessage("this is a rationale")
            .show()
    }

    @OnPermissionDenied(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun onCameraDenied() {
        Toast.makeText(requireContext(), "permission has been denied", Toast.LENGTH_LONG).show()
    }

    @OnNeverAskAgain(Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun onCameraNeverAskAgain() {
        Toast.makeText(requireContext(), "never ask again", Toast.LENGTH_LONG).show()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        onRequestPermissionsResult(requestCode, grantResults)
    }
}
