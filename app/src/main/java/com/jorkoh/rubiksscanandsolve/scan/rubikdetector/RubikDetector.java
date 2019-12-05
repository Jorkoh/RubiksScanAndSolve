package com.jorkoh.rubiksscanandsolve.scan.rubikdetector;

import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.ColorUtils;

import com.jorkoh.rubiksscanandsolve.model.CubeState;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

@SuppressWarnings("unused")
public class RubikDetector {

    private static final String TAG = RubikDetector.class.getSimpleName();

    private static final int NATIVE_DETECTOR_RELEASED = -1;

    private static final int DATA_SIZE = 6;

    private long nativeProcessorRef = NATIVE_DETECTOR_RELEASED;

    private int scanWidth;
    private int scanHeight;

    private int photoWidth;
    private int photoHeight;

    private int scanRotation;
    private int photoRotation;

    private boolean isSecondPhase;

    private int requiredMemory;

    private int requiredMemoryColors;

    private int inputFrameByteCount;

    private int inputFrameBufferOffset;

    private int resultFrameByteCount;

    private int resultFrameBufferOffset;

    static {
        //load the native code
        System.loadLibrary("rubikdetector_native");
    }

    private RubikDetector(@NonNull ImageProperties scanProperties, @NonNull ImageProperties photoProperties) {
        this(scanProperties, photoProperties, null);
    }

    private RubikDetector(@NonNull ImageProperties scanProperties, @NonNull ImageProperties photoProperties, @Nullable String storagePath) {
        this.nativeProcessorRef = createNativeDetector(scanProperties, photoProperties, storagePath);
        this.scanWidth = scanProperties.width;
        this.scanHeight = scanProperties.height;
        this.scanRotation = scanProperties.rotationDegrees;
        this.photoWidth = photoProperties.width;
        this.photoHeight = photoProperties.height;
        this.photoRotation = photoProperties.rotationDegrees;
        this.isSecondPhase = false;
        syncWithNativeObject();
    }

    public void updateScanProperties(int rotation, int width, int height) {
        applyScanProperties(rotation, width, height);
    }

    public void updatePhotoProperties(int rotation, int width, int height) {
        applyPhotoProperties(rotation, width, height);
    }

    public void updateScanPhase(boolean isSecondPhase) {
        applyScanPhase(isSecondPhase);
    }

    public void releaseResources() {
        Log.d(TAG, "#releaseResources. this:" + hashCode());
        if (isActive()) {
            Log.d(TAG, "#releaseResources - handle not -1, calling native release.");
            nativeReleaseCubeDetector(nativeProcessorRef);
            nativeProcessorRef = NATIVE_DETECTOR_RELEASED;
        }
    }

    @Nullable
    public boolean scanCube(@NonNull byte[] imageData) {
        if (isActive() && imageData.length >= requiredMemory) {
            return nativeScanCube(nativeProcessorRef, imageData);
        }
        return false;
    }

    public boolean scanCube(@NonNull ByteBuffer scanDataBuffer) {
        if (!scanDataBuffer.isDirect()) {
            throw new IllegalArgumentException("The scan data buffer needs to be a direct buffer.");
        }
        if (isActive() && scanDataBuffer.capacity() >= requiredMemory) {
            return nativeScanCubeDataBuffer(nativeProcessorRef, scanDataBuffer);
        }
        return false;
    }

    public boolean extractFacelets(@NonNull ByteBuffer scanDataBuffer, @NonNull byte[] photoDataBuffer) {
        if (!scanDataBuffer.isDirect()) {
            throw new IllegalArgumentException("Both th scan and the photo data buffers need to be a direct buffers.");
        }
        if (isActive() && scanDataBuffer.capacity() >= requiredMemory) {
            return nativeExtractFaceletsDataBuffer(nativeProcessorRef, scanDataBuffer, photoDataBuffer);
        }
        return false;
    }

    public CubeState analyzeColors(@NonNull ByteBuffer imageDataBuffer) {
        if (!imageDataBuffer.isDirect()) {
            throw new IllegalArgumentException("The image data buffer needs to be a direct buffer.");
        }
        if (isActive() && imageDataBuffer.capacity() >= requiredMemoryColors) {
            return decodeResult(nativeAnalyzeColorsDataBuffer(nativeProcessorRef, imageDataBuffer));
        }
        return null;
    }

    public boolean isActive() {
        return nativeProcessorRef != NATIVE_DETECTOR_RELEASED;
    }


    public int getRequiredMemory() {
        return requiredMemory;
    }

    public int getRequiredMemoryColors() {
        return requiredMemoryColors;
    }


    public int getInputFrameBufferOffset() {
        return inputFrameBufferOffset;
    }


    public int getInputFrameByteCount() {
        return inputFrameByteCount;
    }


    public int getResultBufferOffset() {
        return resultFrameBufferOffset;
    }


    public int getResultByteCount() {
        return resultFrameByteCount;
    }


    public int getScanWidth() {
        return scanWidth;
    }

    public int getScanHeight() {
        return scanHeight;
    }

    public int getPhotoWidth() {
        return photoWidth;
    }

    public int getPhotoHeight() {
        return photoHeight;
    }

    public int getScanRotation() {
        return scanRotation;
    }

    public int getPhotoRotation() {
        return photoRotation;
    }

    private long createNativeDetector(ImageProperties scanProperties, ImageProperties photoProperties, String storagePath) {

        return nativeCreateRubikDetector(
                scanProperties.rotationDegrees,
                scanProperties.width,
                scanProperties.height,
                photoProperties.rotationDegrees,
                photoProperties.width,
                photoProperties.height,
                storagePath);
    }

    private void applyScanPhase(boolean isSecondPhase) {
        if (isActive()) {
            nativeSetScanPhase(nativeProcessorRef, isSecondPhase);
            this.isSecondPhase = isSecondPhase;
        }
    }

    private void applyScanProperties(int rotation, int width, int height) {
        if (isActive()) {
            nativeSetScanProperties(nativeProcessorRef, rotation, width, height);
            this.scanWidth = width;
            this.scanHeight = height;
            this.scanRotation = rotation;
            syncWithNativeObject();
        }
    }

    private void applyPhotoProperties(int rotation, int width, int height) {
        if (isActive()) {
            nativeSetScanProperties(nativeProcessorRef, rotation, width, height);
            this.photoWidth = width;
            this.photoHeight = height;
            this.photoRotation = rotation;
            syncWithNativeObject();
        }
    }

    private void syncWithNativeObject() {
        if (isActive()) {
            this.requiredMemory = nativeGetRequiredMemory(nativeProcessorRef);
            this.resultFrameBufferOffset = nativeGetResultImageOffset(nativeProcessorRef);
            this.resultFrameByteCount = nativeGetResultImageSize(nativeProcessorRef);
            this.requiredMemoryColors = this.resultFrameByteCount * 2;
            this.inputFrameByteCount = nativeGetInputImageSize(nativeProcessorRef);
            this.inputFrameBufferOffset = nativeGetInputImageOffset(nativeProcessorRef);
        }
    }

    @Nullable
    private CubeState decodeResult(int[] detectionResult) {
        if (detectionResult == null) {
            return null;
        }

        ArrayList<CubeState.Face> facelets = new ArrayList<>();
        CubeState.Face[] values = CubeState.Face.values();
        for (int i = 0; i < 54; i++) {
            facelets.add(values[detectionResult[i]]);
        }
        List<Integer> colors = new ArrayList<>();
        for (int i = 0; i < 6; i++) {
            colors.add(ColorUtils.LABToColor(
                    ((detectionResult[54 + i * 3] / 10000f) / 255f) * 100f,
                    detectionResult[54 + i * 3 + 1] / 10000f - 128,
                    detectionResult[54 + i * 3 + 2] / 10000f - 128
            ));
            Log.d("TESTING", "Color #" + i + " has HEX: " + Integer.toHexString(colors.get(i)));
        }
        return new CubeState(facelets, colors);
    }

    /*
     * #############################################################################################
     * #############################################################################################
     * ##################################     NATIVE METHODS     ###################################
     * #############################################################################################
     * #############################################################################################
     */

    private native long nativeCreateRubikDetector(int scanRotation,
                                                  int scanWidth,
                                                  int scanHeight,
                                                  int photoRotation,
                                                  int photoWidth,
                                                  int photoHeight,
                                                  @Nullable String storagePath);

    private native boolean nativeScanCube(long nativeProcessorRef, byte[] scanData);

    private native boolean nativeScanCubeDataBuffer(long nativeProcessorRef, ByteBuffer scanDataBuffer);

    private native boolean nativeExtractFaceletsDataBuffer(long nativeProcessorRef, ByteBuffer scanDataBuffer, byte[] photoData);

    private native int[] nativeAnalyzeColorsDataBuffer(long nativeProcessorRef, ByteBuffer imageDataBuffer);

    private native void nativeReleaseCubeDetector(long nativeProcessorRef);

    private native void nativeSetScanPhase(long nativeProcessorRef, boolean isSecondPhase);

    private native void nativeSetScanProperties(long nativeProcessorRef, int rotation, int width, int height);

    private native int nativeGetRequiredMemory(long nativeProcessorRef);

    private native int nativeGetResultImageSize(long nativeProcessorRef);

    private native int nativeGetResultImageOffset(long nativeProcessorRef);

    private native int nativeGetInputImageSize(long nativeProcessorRef);

    private native int nativeGetInputImageOffset(long nativeProcessorRef);

    /*
     * #############################################################################################
     * #############################################################################################
     * ################################     END NATIVE METHODS     #################################
     * #############################################################################################
     * #############################################################################################
     */

    public static class ImageProperties {

        public final int rotationDegrees;

        public final int width;

        public final int height;

        public ImageProperties(int rotationDegrees, int width, int height) {
            this.rotationDegrees = rotationDegrees;
            this.width = width;
            this.height = height;
        }
    }

    public static class Builder {
        private String imageSavePath = null;
        private int scanWidth = 640;
        private int scanHeight = 480;
        private int photoWidth = 4032;
        private int photoHeight = 3024;
        private int scanRotation = 90;
        private int photoRotation = 90;

        public Builder scanRotation(int rotationDegrees) {
            this.scanRotation = rotationDegrees;
            return this;
        }

        public Builder photoRotation(int rotationDegrees) {
            this.photoRotation = rotationDegrees;
            return this;
        }

        public Builder scanSize(int width, int height) {
            this.scanWidth = width;
            this.scanHeight = height;
            return this;
        }

        public Builder photoSize(int width, int height) {
            this.photoWidth = width;
            this.photoHeight = height;
            return this;
        }

        public Builder imageSavePath(@Nullable String imageSavePath) {
            this.imageSavePath = imageSavePath;
            return this;
        }

        public RubikDetector build() {
            ImageProperties scanProperties = new ImageProperties(scanRotation, scanWidth, scanHeight);
            ImageProperties photoProperties = new ImageProperties(photoRotation, photoWidth, photoHeight);
            return new RubikDetector(scanProperties, photoProperties, imageSavePath);
        }
    }
}
