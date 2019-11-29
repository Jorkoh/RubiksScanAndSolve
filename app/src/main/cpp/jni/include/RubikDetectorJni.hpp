//
// Created by catalin on 15.07.2017.
//
#include <jni.h>
#include <cstdlib>
#include "../../rubikdetectorcore/include/rubikdetector/data/processing/RubikFacelet.hpp"

#ifndef RUBIKDETECTORJNI_RUBIKDETECTORJNI_HPP
#define RUBIKDETECTORJNI_RUBIKDETECTORJNI_HPP

#ifdef __cplusplus
extern "C" {
#endif

jint JNI_OnLoad(JavaVM *vm, void *reserved);

JNIEXPORT jlong JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeCreateRubikDetector(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jint scanRotation,
                                                                                         jint scanWidth,
                                                                                         jint scanHeight,
                                                                                         jint photoRotation,
                                                                                         jint photoWidth,
                                                                                         jint photoHeight,
                                                                                         jstring storagePath_);

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeReleaseCubeDetector(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jlong cubeDetectorHandle);

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetDrawFoundFacelets(JNIEnv *env,
                                                                                          jobject instance,
                                                                                          jlong cubeDetectorHandle,
                                                                                          jboolean shouldDrawFoundFacelets);

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeScanCube(JNIEnv *env,
                                                                              jobject instance,
                                                                              jlong cubeDetectorHandle,
                                                                              jbyteArray imageByteData);

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeScanCubeDataBuffer(JNIEnv *env,
                                                                                        jobject instance,
                                                                                        jlong cubeDetectorHandle,
                                                                                        jobject scanDataDirectBuffer);

JNIEXPORT jboolean JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeExtractFaceletsDataBuffer(JNIEnv *env,
                                                                                               jobject instance,
                                                                                               jlong cubeDetectorHandle,
                                                                                               jobject scanDataDirectBuffer,
                                                                                               jbyteArray photoData);

JNIEXPORT jstring JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeAnalyzeColorsDataBuffer(JNIEnv *env,
                                                                                             jobject instance,
                                                                                             jlong cubeDetectorHandle,
                                                                                             jobject imageDataDirectBuffer);

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetScanPhase(JNIEnv *env,
                                                                                  jobject instance,
                                                                                  jlong cubeDetectorHandle,
                                                                                  jboolean isSecondPhase);

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetScanProperties(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle,
                                                                                       jint rotation,
                                                                                       jint width,
                                                                                       jint height);

JNIEXPORT void JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeSetDrawConfig(JNIEnv *env,
                                                                                   jobject instance,
                                                                                   jlong cubeDetectorHandle,
                                                                                   jint drawMode,
                                                                                   jint strokeWidth,
                                                                                   jboolean fillShape);

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetRequiredMemory(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle);

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetResultImageOffset(JNIEnv *env,
                                                                                          jobject instance,
                                                                                          jlong cubeDetectorHandle);

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetResultImageSize(JNIEnv *env,
                                                                                        jobject instance,
                                                                                        jlong cubeDetectorHandle);

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetInputImageSize(JNIEnv *env,
                                                                                       jobject instance,
                                                                                       jlong cubeDetectorHandle);

JNIEXPORT jint JNICALL
Java_com_jorkoh_rubiksscanandsolve_rubikdetector_RubikDetector_nativeGetInputImageOffset(JNIEnv *env,
                                                                                         jobject instance,
                                                                                         jlong cubeDetectorHandle);

#ifdef __cplusplus
}
#endif

jintArray processResult(const std::vector<std::vector<rbdt::RubikFacelet>> &result, _JNIEnv *env);

#endif //RUBIKDETECTORJNI_RUBIKDETECTORJNI_HPP
