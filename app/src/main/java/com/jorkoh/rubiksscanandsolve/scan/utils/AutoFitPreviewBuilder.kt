package com.jorkoh.rubiksscanandsolve.scan.utils

import android.graphics.Matrix
import android.util.Log
import android.util.Size
import android.view.Display
import android.view.Surface
import android.view.TextureView
import android.view.ViewGroup
import androidx.camera.core.Preview
import androidx.camera.core.PreviewConfig
import java.lang.IllegalArgumentException
import java.lang.ref.WeakReference


class AutoFitPreviewBuilder private constructor(config: PreviewConfig, viewFinderRef: WeakReference<TextureView>) {

    val useCase: Preview

    private var bufferRotation: Int = 0
    private var viewFinderRotation: Int? = null

    private var bufferDimens: Size = Size(0, 0)
    private var viewFinderDimens: Size = Size(0, 0)

    private var viewFinderDisplay: Int = -1


    init {
        // Make sure that the view finder reference is valid
        val viewFinder = viewFinderRef.get() ?: throw IllegalArgumentException("Invalid reference to view finder used")

        // Initialize the display and rotation from texture view information
        viewFinderDisplay = viewFinder.display.displayId
        viewFinderRotation = getDisplaySurfaceRotation(
            viewFinder.display
        )
            ?: 0

        // Initialize public use-case with the given config
        useCase = Preview(config)

        // Every time the view finder is updated, recompute layout
        useCase.setOnPreviewOutputUpdateListener {
            val viewFinder = viewFinderRef.get() ?: return@setOnPreviewOutputUpdateListener

            // To update the SurfaceTexture, we have to remove it and re-add it
            val parent = viewFinder.parent as ViewGroup
            parent.removeView(viewFinder)
            parent.addView(viewFinder, 0)

            // Update internal texture
            viewFinder.surfaceTexture = it.surfaceTexture

            // Apply relevant transformations
            bufferRotation = it.rotationDegrees
            val rotation =
                getDisplaySurfaceRotation(
                    viewFinder.display
                )
            updateTransform(viewFinder, rotation, it.textureSize, viewFinderDimens)
        }

        // Every time the provided texture view changes, recompute layout
        viewFinder.addOnLayoutChangeListener { view, left, top, right, bottom, _, _, _, _ ->
            val viewFinder = view as TextureView
            val newViewFinderDimens = Size(right - left, bottom - top)
            val rotation =
                getDisplaySurfaceRotation(
                    viewFinder.display
                )
            updateTransform(viewFinder, rotation, bufferDimens, newViewFinderDimens)
        }
    }

    private fun updateTransform(
        textureView: TextureView?,
        rotation: Int?,
        newBufferDimens: Size,
        newViewFinderDimens: Size
    ) {
        textureView ?: return

        if (rotation == viewFinderRotation && newBufferDimens == bufferDimens && newViewFinderDimens == viewFinderDimens) {
            return
        }

        if (rotation == null) {
            return
        } else {
            viewFinderRotation = rotation
        }

        if (newBufferDimens.width == 0 || newBufferDimens.height == 0) {
            return
        } else {
            bufferDimens = newBufferDimens
        }

        if (newViewFinderDimens.width == 0 || newViewFinderDimens.height == 0) {
            return
        } else {
            viewFinderDimens = newViewFinderDimens
        }

        Log.d("RESULTS", "Transform image width: ${bufferDimens.width}, transform image height ${bufferDimens.height}")

        val bufferWidth = bufferDimens.width.toFloat()
        val bufferHeight = bufferDimens.height.toFloat()
        val viewFinderWidth = viewFinderDimens.width.toFloat()
        val viewFinderHeight = viewFinderDimens.height.toFloat()

        var scaleX = 1.0f
        var scaleY = 1.0f
        val centerX = viewFinderDimens.width / 2f
        val centerY = viewFinderDimens.height / 2f

        val viewRatio = bufferWidth / bufferHeight
        val videoRatio = viewFinderWidth / viewFinderHeight
        if (viewRatio > videoRatio) {
            scaleY = viewFinderHeight / viewFinderWidth * viewRatio
        } else {
            scaleX = viewFinderWidth / viewFinderHeight / viewRatio
        }

        val matrix = Matrix().apply {
            setScale(scaleX, scaleY, centerX, centerY)
            postRotate(-viewFinderRotation!!.toFloat(), centerX, centerY)
        }

        textureView.setTransform(matrix)

    }

    companion object {

        fun getDisplaySurfaceRotation(display: Display?) = when (display?.rotation) {
            Surface.ROTATION_0 -> 0
            Surface.ROTATION_90 -> 90
            Surface.ROTATION_180 -> 180
            Surface.ROTATION_270 -> 270
            else -> null
        }

        fun build(config: PreviewConfig, viewFinder: TextureView) =
            AutoFitPreviewBuilder(config, WeakReference(viewFinder)).useCase
    }
}