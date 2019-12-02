//// Created by catalin on 12.07.2017.


#include <math.h>
#include "../../include/rubikdetector/rubikprocessor/internal/RubikProcessorImpl.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/imgproc/types_c.h>
#include <future>
#include "../../include/rubikdetector/utils/Utils.hpp"
#include "../../include/rubikdetector/utils/CrossLog.hpp"
#include "../../include/rubikdetector/data/config/ImageProperties.hpp"
#include "../../include/rubikdetector/utils/CIEDE2000.h"

namespace rbdt {

/**##### PUBLIC API #####**/
    RubikProcessorImpl::RubikProcessorImpl(const ImageProperties scanProperties,
                                           const ImageProperties photoProperties,
                                           std::unique_ptr<RubikFaceletsDetector> faceletsDetector,
                                           std::unique_ptr<RubikColorDetector> colorDetector,
                                           std::shared_ptr<ImageSaver> imageSaver) :
            faceletsDetector(std::move(faceletsDetector)),
            colorDetector(std::move(colorDetector)),
            imageSaver(imageSaver) {
        applyScanProperties(scanProperties);
        applyPhotoProperties(photoProperties);
    }

    RubikProcessorImpl::~RubikProcessorImpl() {
        LOG_DEBUG("NativeRubikProcessor", "RubikProcessorBehavior - destructor.");
    }

    bool RubikProcessorImpl::processScan(const uint8_t *scanData) {
        return scanCubeInternal(scanData);
    }

    bool RubikProcessorImpl::processPhoto(const uint8_t *scanData, const uint8_t *photoData) {
        return extractFaceletsInternal(scanData, photoData);
    }

    CubeState RubikProcessorImpl::processColors(const uint8_t *imageData) {
        return analyzeColorsInternal(imageData);
    }

    void RubikProcessorImpl::updateScanPhase(const bool &isSecondPhase) {
        applyScanPhase(isSecondPhase);
    }

    void RubikProcessorImpl::updateImageProperties(const ImageProperties &newProperties) {
        applyScanProperties(newProperties);
    }

    int RubikProcessorImpl::getRequiredMemory() {
        return totalRequiredMemory;
    }

    int RubikProcessorImpl::getFrameRGBABufferOffset() {
        return firstFaceletOffset;
    }

    int RubikProcessorImpl::getFaceletsByteCount() {
        return (27 * faceletByteCount);
    }

    int RubikProcessorImpl::getFrameYUVByteCount() {
        return frameYUVByteCount;
    }

    int RubikProcessorImpl::getFrameYUVBufferOffset() {
        return frameYUVOffset;
    }

/**##### END PUBLIC API #####**/
/**##### PRIVATE MEMBERS FROM HERE #####**/

    void RubikProcessorImpl::applyScanPhase(const bool &isSecondPhase) {
        this->isSecondPhase = isSecondPhase;
    }

    void RubikProcessorImpl::applyScanProperties(const ImageProperties &properties) {
        scanWidth = properties.width;
        scanHeight = properties.height;

        // Take shortest side as the dimension since the image will be square cropped
        scanDimension = scanWidth < scanHeight ? scanWidth : scanHeight;

        scanNeedsCrop = scanWidth != scanHeight;
        if (scanNeedsCrop) {
            if (scanWidth > scanHeight) {
                scanCroppingRegion = cv::Rect((scanWidth - scanHeight) / 2, 0, scanDimension, scanDimension);
            } else {
                scanCroppingRegion = cv::Rect(0, (scanHeight - scanWidth) / 2, scanDimension, scanDimension);
            }
        } else {
            scanCroppingRegion = cv::Rect();
        }

        scanRotation = properties.rotation;

        scanScalingRatio = (float) DEFAULT_DIMENSION / scanDimension;
        scanNeedsResize = scanScalingRatio != 1;

        // Calculate offsets
        frameYUVByteCount = scanWidth * (scanHeight + scanHeight / 2);
        frameYUVOffset = RubikProcessorImpl::NO_OFFSET;

        frameGrayByteCount = scanWidth * scanHeight;
        frameGrayOffset = frameYUVOffset + frameYUVByteCount;

        faceGrayByteCount = DEFAULT_FACE_DIMENSION * DEFAULT_FACE_DIMENSION;
        firstFaceGrayOffset = frameGrayOffset + frameGrayByteCount;

        faceletByteCount = DEFAULT_FACELET_DIMENSION * DEFAULT_FACELET_DIMENSION * 3;
        firstFaceletOffset = firstFaceGrayOffset + (3 * faceGrayByteCount);

        totalRequiredMemory = frameYUVByteCount +
                              frameGrayByteCount +
                              (3 * faceGrayByteCount) +
                              (54 * faceletByteCount);

        faceletsDetector->onFrameSizeSelected(DEFAULT_FACE_DIMENSION);
    }

    void RubikProcessorImpl::applyPhotoProperties(const ImageProperties &properties) {
        photoWidth = properties.width;
        photoHeight = properties.height;

        // Take shortest side as the dimension since the image will be square cropped
        photoDimension = photoWidth < photoHeight ? photoWidth : photoHeight;

        photoNeedsCrop = photoWidth != photoHeight;
        if (photoNeedsCrop) {
            if (photoWidth > photoHeight) {
                photoCroppingRegion = cv::Rect((photoWidth - photoHeight) / 2, 0, photoDimension, photoDimension);
            } else {
                photoCroppingRegion = cv::Rect(0, (photoHeight - photoWidth) / 2, photoDimension, photoDimension);
            }
        } else {
            photoCroppingRegion = cv::Rect();
        }

        photoRotation = properties.rotation;

        photoScalingRatio = (float) DEFAULT_DIMENSION / photoDimension;
        photoNeedsResize = photoScalingRatio != 1;

        faceletsDetector->onFrameSizeSelected(DEFAULT_FACE_DIMENSION);
    }

    bool RubikProcessorImpl::scanCubeInternal(const uint8_t *scanData) {
        /* Frame rate stuff */
        frameNumber++;
        double processingStart = rbdt::getCurrentTimeMillis();
        /* Frame rate stuff */

        // Allocate the mats
        cv::Mat frameYUV(scanHeight + scanHeight / 2, scanWidth, CV_8UC1, (uchar *) scanData);
        cv::Mat frameGray(scanHeight, scanWidth, CV_8UC1, (uchar *) scanData + frameGrayOffset);

        cv::Mat topFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                            (uchar *) scanData + firstFaceGrayOffset);
        cv::Mat leftFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                             (uchar *) scanData + firstFaceGrayOffset + faceGrayByteCount);
        cv::Mat rightFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                              (uchar *) scanData + firstFaceGrayOffset + (2 * faceGrayByteCount));

        // Gray
        cv::cvtColor(frameYUV, frameGray, cv::COLOR_YUV2GRAY_NV21);

        // Crop, resize and rotate
        cropResizeAndRotate(frameGray, scanNeedsCrop, scanCroppingRegion, scanDimension, scanNeedsResize, scanRotation);

        // Perspective transform to extract the faces
        extractFaces(frameGray, topFaceGray, leftFaceGray, rightFaceGray);

        //TODO parallelize this
        LOG_DEBUG("NativeRubikProcessor", "DETECTING SCAN FACES.");
        std::vector<std::vector<RubikFacelet>> topFacelets = faceletsDetector->detect(topFaceGray, "top_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> leftFacelets = faceletsDetector->detect(leftFaceGray, "left_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> rightFacelets = faceletsDetector->detect(rightFaceGray, "right_face_", frameNumber);

        bool cubeFound = !topFacelets.empty() && !leftFacelets.empty() && !rightFacelets.empty();

        /* Frame rate stuff */
        double processingEnd = rbdt::getCurrentTimeMillis();
        double delta = processingEnd - processingStart;
        double fps = 1000 / delta;
        frameRateSum += fps;
        float frameRateAverage = (float) frameRateSum / frameNumber;
        LOG_DEBUG("NativeRubikProcessor",
                  "Done processing in this frame.\nframeNumber: %d,\nframeRate current frame: %.2f,\nframeRateAverage: %.2f,\nframeRate: %.2f. \nRubikProcessor - Returning the found facelets",
                  frameNumber, fps, frameRateAverage, fps);
        /* Frame rate stuff */

        return cubeFound;
    }

    bool RubikProcessorImpl::extractFaceletsInternal(const uint8_t *scanData, const uint8_t *photoData) {
        /* Frame rate stuff */
        frameNumber++;
        double processingStart = rbdt::getCurrentTimeMillis();
        /* Frame rate stuff */

        // Allocate the mats
        cv::Mat frameYUV(photoHeight + photoHeight / 2, photoWidth, CV_8UC1, (uchar *) photoData);
        cv::Mat frameGray;

        cv::Mat topFaceGray;
        cv::Mat leftFaceGray;
        cv::Mat rightFaceGray;

        // Gray
        cv::cvtColor(frameYUV, frameGray, cv::COLOR_YUV2GRAY_NV21);

        imageSaver->saveImage(frameGray, 0, "_photo");

        // Crop, resize and rotate
        cropResizeAndRotate(frameGray, photoNeedsCrop, photoCroppingRegion, photoDimension, photoNeedsResize, photoRotation);

        imageSaver->saveImage(frameGray, 0, "_photo_crop_resize_rotate");

        // Perspective transform to extract the faces
        extractFaces(frameGray, topFaceGray, leftFaceGray, rightFaceGray);

        //TODO parallelize this
        LOG_DEBUG("NativeRubikProcessor", "DETECTING PHOTO FACES.");
        imageSaver->saveImage(topFaceGray, 0, "top_face_photo");
        imageSaver->saveImage(leftFaceGray, 0, "left_face_photo");
        imageSaver->saveImage(rightFaceGray, 0, "right_face_photo");
        std::vector<std::vector<RubikFacelet>> topFacelets = faceletsDetector->detect(topFaceGray, "top_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> leftFacelets = faceletsDetector->detect(leftFaceGray, "left_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> rightFacelets = faceletsDetector->detect(rightFaceGray, "right_face_", frameNumber);

        bool cubeFound = !topFacelets.empty() && !leftFacelets.empty() && !rightFacelets.empty();
        if (cubeFound) {
            LOG_DEBUG("NativeRubikProcessor", "CUBE FOUND!.");

            // Repeat the process the gray image went through with HSV. Since this is only done once when the cube
            // is actually found it's cheaper than doing it every frame
            cv::Mat frame;
            cv::cvtColor(frameYUV, frame, cv::COLOR_YUV2BGR_NV21);
            /**/
            if (!isSecondPhase) {
                imageSaver->saveImage(frame, 0, "complete_first");
            } else {
                imageSaver->saveImage(frame, 0, "complete_second");
            }
            /**/
            cropResizeAndRotate(frame, photoNeedsCrop, photoCroppingRegion, photoDimension, photoNeedsResize, photoRotation);

            cv::Mat topFace(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);
            cv::Mat leftFace(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);
            cv::Mat rightFace(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);

            extractFaces(frame, topFace, leftFace, rightFace);

            /**/
            if (!isSecondPhase) {
                imageSaver->saveImage(topFace, 0, "top_first");
                imageSaver->saveImage(leftFace, 0, "left_first");
                imageSaver->saveImage(rightFace, 0, "right_first");
            } else {
                imageSaver->saveImage(topFace, 0, "top_second");
                imageSaver->saveImage(leftFace, 0, "left_second");
                imageSaver->saveImage(rightFace, 0, "right_second");
            }
            /**/

            // Write the facelets to the
            saveFacelets(topFacelets, topFace, leftFacelets, leftFace, rightFacelets, rightFace, scanData);
        }

        /* Frame rate stuff */
        double processingEnd = rbdt::getCurrentTimeMillis();
        double delta = processingEnd - processingStart;
        double fps = 1000 / delta;
        frameRateSum += fps;
        float frameRateAverage = (float) frameRateSum / frameNumber;
        LOG_DEBUG("NativeRubikProcessor",
                  "Done processing in this frame.\nframeNumber: %d,\nframeRate current frame: %.2f,\nframeRateAverage: %.2f,\nframeRate: %.2f. \nRubikProcessor - Returning the found facelets",
                  frameNumber, fps, frameRateAverage, fps);
        /* Frame rate stuff */

        return cubeFound;
    }

    void RubikProcessorImpl::rotateMat(cv::Mat &matImage, int rotFlag) {
        if (rotFlag != 0 && rotFlag != 360) {
            if (rotFlag == 90) {
                cv::transpose(matImage, matImage);
                cv::flip(matImage, matImage, 1);
            } else if (rotFlag == 270 || rotFlag == -90) {
                cv::transpose(matImage, matImage);
                cv::flip(matImage, matImage, 0);
            } else if (rotFlag == 180) {
                cv::flip(matImage, matImage, -1);
            }
        }
    }

    void RubikProcessorImpl::cropResizeAndRotate(cv::Mat &matImage, bool needsCrop, const cv::Rect &croppingRegion, int frameDimension,
                                                 bool needsResize, int rotation) {
        // Crop
        if (needsCrop) {
            LOG_DEBUG("NativeRubikProcessor",
                      "Cropping frame of width %d and height %d into a square of width %d, height %d, x %d and y %d.",
                      matImage.cols, matImage.rows, croppingRegion.width, croppingRegion.height, croppingRegion.x, croppingRegion.y);
            matImage = matImage(croppingRegion);
            matImage.cols = frameDimension;
            matImage.rows = frameDimension;
        } else {
            LOG_DEBUG("NativeRubikProcessor", "Frame already at square, no cropping needed.");
        }

        // Resize
        if (needsResize) {
            LOG_DEBUG("NativeRubikProcessor", "Resizing frame to processing size. Image dimension: %d. Processing dimension: %d.",
                      frameDimension, DEFAULT_DIMENSION);
            cv::resize(matImage, matImage, cv::Size(DEFAULT_DIMENSION, DEFAULT_DIMENSION));
        } else {
            LOG_DEBUG("NativeRubikProcessor", "Frame already at processing size, no resize needed.");
        }

        // Rotate
        rotateMat(matImage, rotation);
    }

    void RubikProcessorImpl::extractFaces(cv::Mat &matImage, cv::Mat &topFace, cv::Mat &leftFace, cv::Mat &rightFace) {
        auto lensOffset = static_cast<float>(DEFAULT_DIMENSION * 0.112);
        // Top face: top, right, left, bottom
        std::vector<cv::Point2f> topFaceCorners;
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.5),
                static_cast<float>(DEFAULT_DIMENSION * 0.0151 + lensOffset))
        );
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.9899),
                static_cast<float>(DEFAULT_DIMENSION * 0.2979 ))
        );
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.0101),
                static_cast<float>(DEFAULT_DIMENSION * 0.2979 ))
        );
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.5),
                static_cast<float>(DEFAULT_DIMENSION * 0.5808))
        );
        applyPerspectiveTransform(matImage, topFace, topFaceCorners, cv::Size(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION));

        // Left face: top, right, left, bottom
        std::vector<cv::Point2f> leftFaceCorners;
        leftFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.0801),
                static_cast<float>(DEFAULT_DIMENSION * 0.1768))
        );
        leftFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.57),
                static_cast<float>(DEFAULT_DIMENSION * 0.4596))
        );
        // Apply lens offset scaled to each vector
        leftFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.0801 + lensOffset * cos(30 * CV_PI / 180)),
                static_cast<float>(DEFAULT_DIMENSION * 0.7425 - lensOffset * sin(30 * CV_PI / 180)))
        );
        leftFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.57),
                static_cast<float>(DEFAULT_DIMENSION * 1.0253))
        );
        applyPerspectiveTransform(matImage, leftFace, leftFaceCorners, cv::Size(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION));

        // Right face: top, right, left, bottom
        std::vector<cv::Point2f> rightFaceCorners;
        rightFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.43),
                static_cast<float>(DEFAULT_DIMENSION * 0.4596))
        );
        rightFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.9199),
                static_cast<float>(DEFAULT_DIMENSION * 0.1768 ))
        );
        rightFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.43),
                static_cast<float>(DEFAULT_DIMENSION * 1.0253))
        );
        // Apply lens offset scaled to each vector
        rightFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(DEFAULT_DIMENSION * 0.9199 - lensOffset * cos(30 * CV_PI / 180)),
                static_cast<float>(DEFAULT_DIMENSION * 0.7425 - lensOffset * sin(30 * CV_PI / 180)))
        );
        applyPerspectiveTransform(matImage, rightFace, rightFaceCorners, cv::Size(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION));
    }

    void RubikProcessorImpl::applyPerspectiveTransform(const cv::Mat &inputImage, cv::Mat &outputImage,
                                                       const std::vector<cv::Point2f> &inputPoints,
                                                       const cv::Size &outputSize) {
        std::vector<cv::Point2f> outputPoints;
        outputPoints.emplace_back(cv::Point2f(0, 0));
        outputPoints.emplace_back(cv::Point2f(outputSize.width - 1, 0));
        outputPoints.emplace_back(cv::Point2f(0, outputSize.height - 1));
        outputPoints.emplace_back(cv::Point2f(outputSize.width - 1, outputSize.height - 1));

        //Apply the perspective transformation
        auto perspectiveMatrix = cv::getPerspectiveTransform(inputPoints, outputPoints);
        cv::warpPerspective(inputImage, outputImage, perspectiveMatrix, outputSize);
    }

    void RubikProcessorImpl::saveFacelets(std::vector<std::vector<RubikFacelet>> &topFacelets, cv::Mat &topFaceHSV,
                                          std::vector<std::vector<RubikFacelet>> &leftFacelets, cv::Mat &leftFaceHSV,
                                          std::vector<std::vector<RubikFacelet>> &rightFacelets, cv::Mat &rightFaceHSV,
                                          const uint8_t *data) {
        int savedFacelets = 0;
        int phaseOffset = 0;
        if (isSecondPhase) {
            phaseOffset = 27 * faceletByteCount;
        }
        // Top
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = topFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 3;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletOffset + phaseOffset + (savedFacelets * faceletByteCount));
                LOG_DEBUG("TESTING", "Original size: %d x %d, resized to: %d x %d",
                          roi.width, roi.height, DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION);
                cv::resize(topFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION),
                           cv::InterpolationFlags::INTER_CUBIC);
                savedFacelets++;
            }
        }
        // Left
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = leftFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 3;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletOffset + phaseOffset + (savedFacelets * faceletByteCount));
                LOG_DEBUG("TESTING", "Original size: %d x %d, resized to: %d x %d",
                          roi.width, roi.height, DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION);
                cv::resize(leftFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION),
                           cv::InterpolationFlags::INTER_CUBIC);
                savedFacelets++;
            }
        }
        // Right
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = rightFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 3;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletOffset + phaseOffset + (savedFacelets * faceletByteCount));
                LOG_DEBUG("TESTING", "Original size: %d x %d, resized to: %d x %d",
                          roi.width, roi.height, DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION);
                cv::resize(rightFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION),
                           cv::InterpolationFlags::INTER_CUBIC);
                savedFacelets++;
            }
        }
    }

    bool compareUp(std::vector<double> i, std::vector<double> j) {
        return i[0] < j[0];
    }

    bool compareFront(std::vector<double> i, std::vector<double> j) {
        return i[1] < j[1];
    }

    bool compareRight(std::vector<double> i, std::vector<double> j) {
        return i[2] < j[2];
    }

    bool compareDown(std::vector<double> i, std::vector<double> j) {
        return i[3] < j[3];
    }

    bool compareLeft(std::vector<double> i, std::vector<double> j) {
        return i[4] < j[4];
    }

    bool compareBack(std::vector<double> i, std::vector<double> j) {
        return i[5] < j[5];
    }

    CubeState RubikProcessorImpl::analyzeColorsInternal(const uint8_t *data) {
        int processedFacelets = 0;
        for (int i = 0; i < 54; i++) {
            cv::Mat facelet(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                            (uchar *) data + firstFaceletOffset + (processedFacelets * faceletByteCount));
            imageSaver->saveImage(facelet, i + 1, "");
            processedFacelets++;
        }
        /// Finished saving for debug

        /*

        // Calculate the average CIELabs of the facelets ignoring the lightness channel
        std::vector<CIEDE2000::LAB> meanLabs(0);
        for (int i = 0; i < 54; i++) {
            cv::Mat facelet(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                            (uchar *) data + firstFaceletOffset + (i * faceletByteCount));
            cv::Mat faceletLab;
            cv::cvtColor(facelet, faceletLab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> labChannels(3);
            cv::split(faceletLab, labChannels);
            double lValue = cv::mean(labChannels[0])[0] * 100.0 / 255.0;
            double aValue = cv::mean(labChannels[1])[0] * 100.0 / 255.0;
            double bValue = cv::mean(labChannels[2])[0] * 100.0 / 255.0;
            meanLabs.emplace_back(CIEDE2000::LAB{lValue, aValue, bValue});
        }

        CIEDE2000::LAB upCenter = meanLabs[4];
        CIEDE2000::LAB frontLabel = meanLabs[13];
        CIEDE2000::LAB rightLabel = meanLabs[22];
        CIEDE2000::LAB downLabel = meanLabs[31];
        CIEDE2000::LAB leftLabel = meanLabs[40];
        CIEDE2000::LAB backLabel = meanLabs[49];

        // Each facelet has distances to every center piece calculated
        std::vector<std::vector<double>> ciede2000ToFaceCenters(54, std::vector<double>(9));
        for (int i = 0; i < 54; i++) {
            ciede2000ToFaceCenters[i] = {
                    CIEDE2000::CIEDE2000(upCenter, meanLabs[i]),
                    CIEDE2000::CIEDE2000(frontLabel, meanLabs[i]),
                    CIEDE2000::CIEDE2000(rightLabel, meanLabs[i]),
                    CIEDE2000::CIEDE2000(downLabel, meanLabs[i]),
                    CIEDE2000::CIEDE2000(leftLabel, meanLabs[i]),
                    CIEDE2000::CIEDE2000(backLabel, meanLabs[i])
            };
        }

        std::vector<double> distancePlaceholder{
                99999,
                99999,
                99999,
                99999,
                99999,
                99999,
        };
        std::vector<int> faceletsFaces(54);

        // Pick closest pieces to centers alternating between centers and assign the corresponding facelet to that face
        for (int i = 0; i < 54; i++) {
            switch (i % 6) {
                case 0: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareUp);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 0;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 0", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                case 1: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareFront);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 1;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 1", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                case 2: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareRight);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 2;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 2", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                case 3: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareDown);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 3;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 3", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                case 4: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareLeft);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 4;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 4", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                case 5: {
                    auto minDistance = std::min_element(ciede2000ToFaceCenters.begin(), ciede2000ToFaceCenters.end(), compareBack);
                    int closestFaceletIndex = std::distance(ciede2000ToFaceCenters.begin(), minDistance);
                    ciede2000ToFaceCenters[closestFaceletIndex] = distancePlaceholder;
                    faceletsFaces[closestFaceletIndex] = 5;
                    LOG_DEBUG("TESTING", "Facelet %d at face %d identified as 5", (closestFaceletIndex % 9) + 1,
                              (closestFaceletIndex / 9) + 1);
                    break;
                }
                default:
                    break;
            }
        }

        // 0 white, green 1, red 2, yellow 3, orange 4, blue 5
        int correctColors[54] = {0, 5, 4, 3, 0, 5, 4, 0, 5,
                                 1, 2, 3, 4, 1, 2, 3, 3, 1,
                                 4, 0, 0, 3, 2, 1, 4, 4, 2,
                                 0, 1, 0, 1, 3, 2, 2, 4, 2,
                                 5, 0, 1, 3, 4, 2, 3, 5, 5,
                                 3, 5, 1, 1, 5, 0, 3, 4, 5};
        for (int i = 0; i < 54; i++) {
            if (faceletsFaces[i] != correctColors[i]) {
                LOG_DEBUG("TESTING", "Facelet %d at face %d identified as %d when it was %d",
                          (i % 9) + 1, (i / 9) + 1, faceletsFaces[i], correctColors[i]);
            }
        }


        // Form a cube state string with the order the solver algorithm requests
        std::string result = "";
        int order[54] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                         18, 19, 20, 21, 22, 23, 24, 25, 26,
                         9, 10, 11, 12, 13, 14, 15, 16, 17,
                         33, 30, 27, 34, 31, 28, 35, 32, 29,
                         44, 43, 42, 41, 40, 39, 38, 37, 36,
                         53, 52, 51, 50, 49, 48, 47, 46, 45};
        for (int i = 0; i < 54; i++) {
            int faceletFace = faceletsFaces[order[i]];

            if (faceletFace == 0) {
                result += "U";
            } else if (faceletFace == 1) {
                result += "F";
            } else if (faceletFace == 2) {
                result += "R";
            } else if (faceletFace == 3) {
                result += "D";
            } else if (faceletFace == 4) {
                result += "L";
            } else if (faceletFace == 5) {
                result += "B";
            }
        }

        return result;
    */

        /**/
        std::vector<cv::Scalar_<float>> meanValues(0);
        for (int i = 0; i < 54; i++) {
            cv::Mat facelet(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                            (uchar *) data + firstFaceletOffset + (i * faceletByteCount));

            cv::Mat faceletLab;
            cv::cvtColor(facelet, faceletLab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> labChannels(3);
            cv::split(faceletLab, labChannels);
            meanValues.emplace_back(cv::Scalar_<float>(
                    static_cast<float>(cv::mean(labChannels[0])[0]),
                    static_cast<float>(cv::mean(labChannels[1])[0]),
                    static_cast<float>(cv::mean(labChannels[2])[0])));
        }

        std::vector<int> labels;
        std::vector<cv::Scalar> centers;
        int expectedDifferentColors = 6;

        cv::TermCriteria criteria(CV_TERMCRIT_ITER, 100, 0.5);
        cv::kmeans(meanValues, expectedDifferentColors, labels, criteria, 100, cv::KmeansFlags::KMEANS_PP_CENTERS, centers);

        int upLabel = labels[4];
        int frontLabel = labels[13];
        int rightLabel = labels[22];
        int downLabel = labels[31];
        int leftLabel = labels[40];
        int backLabel = labels[49];


        //Asume it's being tested in white top, green front, red right -> yellow top, orange front, blue left
        LOG_DEBUG("TESTING", "Labels: white %d, green %d, red %d, yellow %d, orange %d and blue %d",
                  upLabel, frontLabel, rightLabel, downLabel, leftLabel, backLabel);
        for (int i = 0; i < 54; i++) {
            std::string color;
            if (labels[i] == upLabel) {
                color += "W";
            } else if (labels[i] == frontLabel) {
                color += "G";
            } else if (labels[i] == rightLabel) {
                color += "R";
            } else if (labels[i] == downLabel) {
                color += "Y";
            } else if (labels[i] == leftLabel) {
                color += "O";
            } else if (labels[i] == backLabel) {
                color += "B";
            }
            LOG_DEBUG("TESTING",
                      "Facelet %d with mean LAB (%.2f, %.2f, %.2f) has been labeled as part of group %d (%s) which has center on (%.2f, %.2f, %.2f)",
                      i + 1,
                      meanValues[i][0], meanValues[i][1], meanValues[i][2],
                      labels[i],
                      color.c_str(),
                      centers[labels[i]][0], centers[labels[i]][1], centers[labels[i]][2]);
        }


        std::vector<CubeState::Face> facelets(0);
        for (int i = 0; i < 54; i++) {
            if (labels[i] == upLabel) {
                facelets.emplace_back(CubeState::Face::UP);
            } else if (labels[i] == frontLabel) {
                facelets.emplace_back(CubeState::Face::FRONT);
            } else if (labels[i] == rightLabel) {
                facelets.emplace_back(CubeState::Face::RIGHT);
            } else if (labels[i] == downLabel) {
                facelets.emplace_back(CubeState::Face::DOWN);
            } else if (labels[i] == leftLabel) {
                facelets.emplace_back(CubeState::Face::LEFT);
            } else if (labels[i] == backLabel) {
                facelets.emplace_back(CubeState::Face::BACK);
            }
        }
        std::vector<cv::Scalar> colors(0);
        colors.emplace_back(centers[upLabel]);
        colors.emplace_back(centers[frontLabel]);
        colors.emplace_back(centers[rightLabel]);
        colors.emplace_back(centers[downLabel]);
        colors.emplace_back(centers[leftLabel]);
        colors.emplace_back(centers[backLabel]);

        return CubeState(facelets, colors);
        /**/
    }
} //namespace rbdt