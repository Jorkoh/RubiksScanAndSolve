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
                                                                                         jint rotationDegrees,
                                                                                         jint frameWidth,
                                                                                         jint frameHeight,
                                                                                         jint drawMode,
                                                                                         jint strokeWidth,
                                                                                         jboolean fillShape,
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
            .rotation((int) rotationDegrees)
            .inputFrameSize((int) frameWidth, (int) frameHeight)
            .drawConfig(rbdt::DrawConfig(rbdt_jni::drawModeFromInt((int) drawMode),
                                         (int) strokeWidth,
                                         (bool) fillShape))
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

JNIEXPORT jintArray  JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeFindCube(JNIEnv *env,
                                                                              jobject instance,
                                                                              jlong cubeDetectorHandle,
                                                                              jbyteArray imageByteData) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    jboolean isCopy = 3; //some arbitrary value
    void *ptr = env->GetPrimitiveArrayCritical(imageByteData, &isCopy);
    if (ptr) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        env->ReleasePrimitiveArrayCritical(imageByteData, ptr, JNI_ABORT);
        std::vector<std::vector<rbdt::RubikFacelet>> result = cubeDetector.process(ptrAsInt);
        return processResult(result, env);
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp",
                 "Could not obtain image byte array. No processing performed.");
        return NULL;
    }
}

JNIEXPORT jintArray JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeFindCubeDataBuffer(JNIEnv *env,
                                                                                        jobject instance,
                                                                                        jlong cubeDetectorHandle,
                                                                                        jobject imageDataDirectBuffer) {
    rbdt::RubikProcessor &cubeDetector = *reinterpret_cast<rbdt::RubikProcessor *>(cubeDetectorHandle);

    void *ptr = env->GetDirectBufferAddress(imageDataDirectBuffer);
    if (ptr) {
        uint8_t *ptrAsInt = reinterpret_cast<uint8_t *>(ptr);
        std::vector<std::vector<rbdt::RubikFacelet>> result = cubeDetector.process(ptrAsInt);
        return processResult(result, env);
    } else {
        LOG_WARN("RUBIK_JNI_PART.cpp",
                 "Could not obtain image byte array. No processing performed.");
        return NULL;
    }
}

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetImageProperties(JNIEnv *env,
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
    return cubeDetector.getFrameRGBAByteCount();
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