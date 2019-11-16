package com.jorkoh.rubiksscanandsolve

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.hardware.display.DisplayManager
import android.os.Bundle
import android.util.DisplayMetrics
import android.util.Log
import android.util.Rational
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.camera.core.*
import com.jorkoh.rubiksscanandsolve.utils.AutoFitPreviewBuilder
import kotlinx.android.synthetic.main.fragment_scan.*
import permissions.dispatcher.*
import java.util.concurrent.Executors

@RuntimePermissions
class ScanFragment : Fragment() {

    private var displayId = -1
    private var preview: Preview? = null
    private var imageAnalyzer: ImageAnalysis? = null

    private lateinit var displayManager: DisplayManager
    private val displayListener = object : DisplayManager.DisplayListener {
        override fun onDisplayAdded(displayId: Int) = Unit
        override fun onDisplayRemoved(displayId: Int) = Unit
        override fun onDisplayChanged(displayId: Int) = view?.let { view ->
            if (displayId == this@ScanFragment.displayId) {
                preview?.setTargetRotation(view.display.rotation)
                imageAnalyzer?.setTargetRotation(view.display.rotation)
            }
        } ?: Unit
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Mark this as a retain fragment, so the lifecycle does not get restarted on config change
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

        // Every time the orientation of device changes, recompute layout
        displayManager = view_finder.context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
        displayManager.registerDisplayListener(displayListener, null)

        // Wait for the views to be properly laid out
        view_finder.post {
            // Keep track of the display in which this view is attached
            displayId = view_finder.display.displayId

            // Build UI controls and bind all camera use cases
            updateCameraUI()
            enableScannerWithPermissionCheck()
        }
    }

    private fun updateCameraUI() {
        //TODO build overlay controls here if needed
    }

    @SuppressLint("RestrictedApi")
    @NeedsPermission(Manifest.permission.CAMERA)
    fun enableScanner() {
        // Set up the view finder use case to display camera preview
        val viewFinderConfig = PreviewConfig.Builder().apply {
            setLensFacing(CameraX.LensFacing.BACK)
            // We request aspect ratio but no resolution to let CameraX optimize our use cases
            setTargetAspectRatio(AspectRatio.RATIO_4_3)
            // Set initial target rotation, we will have to call this again if rotation changes
            // during the lifecycle of this use case
            setTargetRotation(view_finder.display.rotation)
        }.build()

        // Use the auto-fit preview builder to automatically handle size and orientation changes
        preview = AutoFitPreviewBuilder.build(viewFinderConfig, view_finder)

        // Setup image analysis pipeline that computes average pixel luminance in real time
        val analyzerConfig = ImageAnalysisConfig.Builder().apply {
            setLensFacing(CameraX.LensFacing.BACK)
            // In our analysis, we care more about the latest image than analyzing *every* image
            setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
            // Set initial target rotation, we will have to call this again if rotation changes
            // during the lifecycle of this use case
            setTargetRotation(view_finder.display.rotation)
        }.build()

        imageAnalyzer = ImageAnalysis(analyzerConfig).apply {
            setAnalyzer(Executors.newCachedThreadPool(), TestAnalyzer { luma, fps ->
                // Values returned from our analyzer are passed to the attached listener
                // We log image analysis results here -- you should do something useful instead!
                Log.d("RESULTS", "Average luminosity: $luma." + " Frames per second: ${"%.01f".format(fps)}")
            })
        }

        // Apply declared configs to CameraX using the same lifecycle owner
        CameraX.bindToLifecycle(viewLifecycleOwner, preview, imageAnalyzer)
    }

    override fun onDestroyView() {
        super.onDestroyView()

        displayManager.unregisterDisplayListener(displayListener)
    }

    @OnShowRationale(Manifest.permission.CAMERA)
    fun showRationaleForCamera(request: PermissionRequest) {
        AlertDialog.Builder(requireContext())
            .setPositiveButton("positive") { _, _ -> request.proceed() }
            .setNegativeButton("negative") { _, _ -> request.cancel() }
            .setCancelable(false)
            .setMessage("this is a rationale")
            .show()
    }

    @OnPermissionDenied(Manifest.permission.CAMERA)
    fun onCameraDenied() {
        Toast.makeText(requireContext(), "permission has been denied", Toast.LENGTH_LONG).show()
    }

    @OnNeverAskAgain(Manifest.permission.CAMERA)
    fun onCameraNeverAskAgain() {
        Toast.makeText(requireContext(), "never ask again", Toast.LENGTH_LONG).show()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        onRequestPermissionsResult(requestCode, grantResults)
    }
}
