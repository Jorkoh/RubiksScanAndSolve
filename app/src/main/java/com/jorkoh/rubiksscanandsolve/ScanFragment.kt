package com.jorkoh.rubiksscanandsolve

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.graphics.Color
import android.graphics.ImageFormat
import android.graphics.PorterDuff
import android.hardware.display.DisplayManager
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.camera.core.*
import androidx.fragment.app.Fragment
import com.jorkoh.rubiksscanandsolve.utils.AutoFitPreviewBuilder
import kotlinx.android.synthetic.main.fragment_scan.*
import kotlinx.android.synthetic.main.fragment_scan.view.*
import permissions.dispatcher.*
import java.util.concurrent.Executors


@RuntimePermissions
class ScanFragment : Fragment() {

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

    private val detectorListener: DetectorListener = { detected  ->
        view_finder_overlay.post {
            view_finder_overlay.background.setColorFilter(
                if (detected) {
                    Color.GREEN
                } else {
                    Color.RED
                },
                PorterDuff.Mode.SRC_ATOP
            )
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        retainInstance = true
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.fragment_scan, container, false).apply {
            button_scan.setOnClickListener {
                //TODO temporary stuff
                (imageAnalyzer?.analyzer as CubeDetectorAnalyzer).detectFrame = true
            }
            button_photo.setOnClickListener {
                imageCapture?.takePicture(Executors.newCachedThreadPool(), object : ImageCapture.OnImageCapturedListener() {
                    override fun onCaptureSuccess(image: ImageProxy?, rotationDegrees: Int) {
                        // It does return YUV! Won't be used tho..
                        Log.d("TEST", "Image format: ${image?.format}")
                        super.onCaptureSuccess(image, rotationDegrees)
                    }
                })
            }
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        displayManager = view_finder.context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
        displayManager.registerDisplayListener(displayListener, null)

        view_finder.post {
            displayId = view_finder.display.displayId

            updateCameraUI()
            enableScannerWithPermissionCheck()
        }
    }

    private fun updateCameraUI() {
        //TODO build overlay controls here if needed
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
            setAnalyzer(Executors.newCachedThreadPool(), CubeDetectorAnalyzer(detectorListener))
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
