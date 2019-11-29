//
// Created by catalin on 15.07.2017.
//
#include <jni.h>
#include <vector>
#include "../include/RubikDetectorJni.hpp"
#include "../../rubikdetectorcore/include/rubikdetector/imagesaver/ImageSaver.hpp"
#include "../../rubikdetectorcore/include/rubikdetector/utils/CrossLog.hpp"
#include "../../rubikdetectorcore/include/rubikdetector/utils/Utils.hpp"
#include "../../rubikdetectorcore/include/rubikdetector/rubikprocessor/builder/RubikProcessorBuilder.hpp"
#include "../../rubikdetectorcore/include/rubikdetector/data/config/ImageProperties.hpp"
#include "../include/RubikDetectorJniUtils.hpp"

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeCreateRubikDetector(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jint scanRotation,
                                                                                         jint scanWidth,
                                                                                         jint scanHeight,
                                                                                         jint photoRotation,
                                                                                         jint photoWidth,
                                                                                         jint photoHeight,
                                                                                         jstring storagePath) {
    std::shared_ptr<rbdt::ImageSaver> imageSaver;
    if (storagePath != NULL) {
        const char *cppStoragePath = env->GetStringUTFChars(storagePath, 0);
        imageSaver = std::make_shared<rbdt::ImageSaver>(cppStoragePath);
        env->ReleaseStringUTFChars(storagePath, cppStoragePath);
    } else {
        imageSaver = nullptr;
    }

    rbdt::RubikProcessor *rubikDetector = rbdt::RubikProcessorBuilder()
            .scanRotation((int) scanRotation)
            .photoRotation((int) photoRotation)
            .scanSize((int) scanWidth, (int) scanHeight)
            .photoSize((int) photoWidth, (int) photoHeight)
            .imageSaver(imageSaver)
            .build();
    return reinterpret_cast<jlong>(rubikDetector);

}

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeReleaseCubeDetector(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jlong cubeDetectorHandle) {

    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    LOG_DEBUG("RUBIK_JNI_PART.cpp", "nativeReleaseCube");
    delete &cubeDetector;
}

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeScanCube(JNIEnv *env,
                                                                              jobject instance,
                                                                              jlong cubeDetectorHandle,
                                                                              jbyteArray imageByteData) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    jboolean isCopy = 3; //some arbitrary value
    void *ptr = env->GetPrimitiveArrayCritical(imageByteData, &isCopy);
    if (ptr) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        env->ReleasePrimitiveArrayCritical(imageByteData, ptr, JNI_ABORT);
        return static_cast<jboolean>(cubeDetector.processScan(ptrAsInt));
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp", "Could not obtain image byte array. No processing performed.");
        return static_cast<jboolean>(false);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeScanCubeDataBuffer(JNIEnv *env,
                                                                                        jobject instance,
                                                                                        jlong cubeDetectorHandle,
                                                                                        jobject scanDataDirectBuffer) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    void *ptr = env->GetDirectBufferAddress(scanDataDirectBuffer);
    if (ptr) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        return static_cast<jboolean>(cubeDetector.processScan(ptrAsInt));
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp",
                 "Could not obtain image byte array. No processing performed.");
        return static_cast<jboolean>(false);
    }
}

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeExtractFaceletsDataBuffer(JNIEnv *env,
                                                                                               jobject instance,
                                                                                               jlong cubeDetectorHandle,
                                                                                               jobject scanDataDirectBuffer,
                                                                                               jbyteArray photoData) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    void *ptr = env->GetDirectBufferAddress(scanDataDirectBuffer);
    jboolean isCopy = 3; //some arbitrary value
    void *ptr2 = env->GetPrimitiveArrayCritical(photoData, &isCopy);
    if (ptr && ptr2) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        uint8_t *ptrAsInt2 = reinterpret_cast<uint8_t *>(ptr2);
        env->ReleasePrimitiveArrayCritical(photoData, ptr2, JNI_ABORT);
        return static_cast<jboolean>(cubeDetector.processPhoto(ptrAsInt, ptrAsInt2));
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp",
                 "Could not obtain image byte array. No processing performed.");
        return static_cast<jboolean>(false);
    }
}

JNIEXPORT jstring JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeAnalyzeColorsDataBuffer(JNIEnv *env,
                                                                                             jobject instance,
                                                                                             jlong cubeDetectorHandle,
                                                                                             jobject imageDataDirectBuffer) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    void *ptr = env->GetDirectBufferAddress(imageDataDirectBuffer);
    if (ptr) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        return (env)->NewStringUTF(cubeDetector.processColors(ptrAsInt).c_str());
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp",
                 "Could not obtain image byte array. No color processing performed.");
        return (env)->NewStringUTF("");
    }
}

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetScanPhase(JNIEnv *env,
                                                                                  jobject instance,
                                                                                  jlong cubeDetectorHandle,
                                                                                  jboolean isSecondPhase) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    cubeDetector.updateScanPhase((bool) isSecondPhase);
}

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetScanProperties(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle,
                                                                                       jint rotation,
                                                                                       jint width,
                                                                                       jint height) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    cubeDetector.updateImageProperties(rbdt::ImageProperties((int) rotation, (int) width, (int) height));
}

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetDrawConfig(JNIEnv *env,
                                                                                   jobject instance,
                                                                                   jlong cubeDetectorHandle,
                                                                                   jint drawMode,
                                                                                   jint strokeWidth,
                                                                                   jboolean fillShape) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    cubeDetector.updateDrawConfig(rbdt::DrawConfig(rbdt_jni::drawModeFromInt((int) drawMode),
                                                   (int) strokeWidth,
                                                   (bool) fillShape));
}

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetRequiredMemory(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    return cubeDetector.getRequiredMemory();
}

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetResultImageOffset(JNIEnv *env,
                                                                                          jobject instance,
                                                                                          jlong cubeDetectorHandle) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    return cubeDetector.getFrameRGBABufferOffset();
}

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetResultImageSize(JNIEnv *env,
                                                                                        jobject instance,
                                                                                        jlong cubeDetectorHandle) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    return cubeDetector.getFaceletsByteCount();
}

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetInputImageSize(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    return cubeDetector.getFrameYUVByteCount();
}

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetInputImageOffset(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jlong cubeDetectorHandle) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);
    return cubeDetector.getFrameYUVBufferOffset();
}

#ifdef __cplusplus
}
#endif

jintArray processResult(const std::vector<std::vector<rbdt::RubikFacelet>> &result, _JNIEnv *env) {
    if (result.size() != 0) {
        size_t data_size = 9 * 6;
        jint flattenedResult[data_size];
        jint *currentPos = flattenedResult;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                *(currentPos++) = rbdt::asInt(result[i][j].color);
                *(currentPos++) = (int) (result[i][j].center.x * 100000);
                *(currentPos++) = (int) (result[i][j].center.y * 100000);
                *(currentPos++) = (int) (result[i][j].width * 100000);
                *(currentPos++) = (int) (result[i][j].height * 100000);
                *(currentPos++) = (int) (result[i][j].angle * 100000);
            }
        }

        jintArray retArray = env->NewIntArray(data_size);
        env->SetIntArrayRegion(retArray, 0, data_size, (jint *) flattenedResult);
        return retArray;
    } else {
        return NULL;
    }
}