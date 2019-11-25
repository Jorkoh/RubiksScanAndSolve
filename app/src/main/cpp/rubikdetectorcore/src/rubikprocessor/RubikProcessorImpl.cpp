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

namespace rbdt {

/**##### PUBLIC API #####**/
    RubikProcessorImpl::RubikProcessorImpl(const ImageProperties imageProperties,
                                           std::unique_ptr<RubikFaceletsDetector> faceletsDetector,
                                           std::unique_ptr<RubikColorDetector> colorDetector,
                                           std::unique_ptr<FaceletsDrawController> faceletsDrawController,
                                           std::shared_ptr<ImageSaver> imageSaver) :
            faceletsDetector(std::move(faceletsDetector)),
            colorDetector(std::move(colorDetector)),
            faceletsDrawController(std::move(faceletsDrawController)),
            imageSaver(imageSaver) {
        applyImageProperties(imageProperties);
    }

    RubikProcessorImpl::~RubikProcessorImpl() {
        LOG_DEBUG("NativeRubikProcessor", "RubikProcessorBehavior - destructor.");
    }

    bool RubikProcessorImpl::process(const uint8_t *imageData) {
        return findCubeInternal(imageData);
    }

    std::string RubikProcessorImpl::processColors(const uint8_t *imageData) {
        return analyzeColorsInternal(imageData);
    }

    void RubikProcessorImpl::updateScanPhase(const bool &isSecondPhase) {
        applyScanPhase(isSecondPhase);
    }

    void RubikProcessorImpl::updateImageProperties(const ImageProperties &newProperties) {
        applyImageProperties(newProperties);
    }

    int RubikProcessorImpl::getRequiredMemory() {
        return totalRequiredMemory;
    }

    int RubikProcessorImpl::getFrameRGBABufferOffset() {
        return firstFaceletHSVOffset;
    }

    int RubikProcessorImpl::getFaceletsByteCount() {
        return (27 * faceletHSVByteCount);
    }

    int RubikProcessorImpl::getFrameYUVByteCount() {
        return frameYUVByteCount;
    }

    int RubikProcessorImpl::getFrameYUVBufferOffset() {
        return frameYUVOffset;
    }

    void RubikProcessorImpl::updateDrawConfig(DrawConfig drawConfig) {
        faceletsDrawController->updateDrawConfig(drawConfig);
    }
/**##### END PUBLIC API #####**/
/**##### PRIVATE MEMBERS FROM HERE #####**/

    void RubikProcessorImpl::applyScanPhase(const bool &isSecondPhase) {
        this->isSecondPhase = isSecondPhase;
    }

    void RubikProcessorImpl::applyImageProperties(const ImageProperties &properties) {
        originalWidth = properties.width;
        originalHeight = properties.height;

        // Take shortest side as the dimension since the image will be square cropped
        frameDimension = originalWidth < originalHeight ? originalWidth : originalHeight;

        needsCrop = originalWidth != originalHeight;
        if (needsCrop) {
            if (originalWidth > originalHeight) {
                croppingRegion = cv::Rect((originalWidth - originalHeight) / 2, 0, frameDimension, frameDimension);
            } else {
                croppingRegion = cv::Rect(0, (originalHeight - originalWidth) / 2, frameDimension, frameDimension);
            }
        } else {
            croppingRegion = cv::Rect();
        }

        rotation = properties.rotation;

        scalingRatio = (float) DEFAULT_DIMENSION / frameDimension;
        needsResize = scalingRatio != 1;

        // Calculate offsets
        frameYUVByteCount = originalWidth * (originalHeight + originalHeight / 2);
        frameYUVOffset = RubikProcessorImpl::NO_OFFSET;

        frameGrayByteCount = originalWidth * originalHeight;
        frameGrayOffset = frameYUVOffset + frameYUVByteCount;

        faceGrayByteCount = DEFAULT_FACE_DIMENSION * DEFAULT_FACE_DIMENSION;
        firstFaceGrayOffset = frameGrayOffset + frameGrayByteCount;

        faceletHSVByteCount = DEFAULT_FACELET_DIMENSION * DEFAULT_FACELET_DIMENSION * 3;
        firstFaceletHSVOffset = firstFaceGrayOffset + (3 * faceGrayByteCount);

        totalRequiredMemory = frameYUVByteCount +
                              frameGrayByteCount +
                              (3 * faceGrayByteCount) +
                              (54 * faceletHSVByteCount);

        faceletsDetector->onFrameSizeSelected(DEFAULT_FACE_DIMENSION);
    }

    // Gray, crop,  resize, rotate, transform, detect facelets, (if found) repeat with HSV, extract facelets from HSV
    bool RubikProcessorImpl::findCubeInternal(const uint8_t *data) {
        /* Frame rate stuff */
        frameNumber++;
        double processingStart = rbdt::getCurrentTimeMillis();
        /* Frame rate stuff */

        // Allocate the mats
        cv::Mat frameYUV(originalHeight + originalHeight / 2, originalWidth, CV_8UC1, (uchar *) data);
        cv::Mat frameGray(originalHeight, originalWidth, CV_8UC1, (uchar *) data + frameGrayOffset);

        cv::Mat topFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                            (uchar *) data + firstFaceGrayOffset);
        cv::Mat leftFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                             (uchar *) data + firstFaceGrayOffset + faceGrayByteCount);
        cv::Mat rightFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                              (uchar *) data + firstFaceGrayOffset + (2 * faceGrayByteCount));

        // Gray
        cv::cvtColor(frameYUV, frameGray, cv::COLOR_YUV2GRAY_NV21);

        cropResizeAndRotate(frameGray);

        // Perspective transform to extract the faces
        extractFaces(frameGray, topFaceGray, leftFaceGray, rightFaceGray);

        //TODO parallelize this
        std::vector<std::vector<RubikFacelet>> topFacelets = faceletsDetector->detect(topFaceGray, "top_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> leftFacelets = faceletsDetector->detect(leftFaceGray, "left_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> rightFacelets = faceletsDetector->detect(rightFaceGray, "right_face_", frameNumber);

        bool cubeFound = !topFacelets.empty() && !leftFacelets.empty() && !rightFacelets.empty();
        if (cubeFound) {
            LOG_DEBUG("NativeRubikProcessor", "CUBE FOUND!.");

            // Repeat the process the gray image went through with HSV. Since this is only done once when the cube
            // is actually found it's cheaper than doing it every frame
            cv::Mat frameHSV;
            cv::cvtColor(frameYUV, frameHSV, cv::COLOR_YUV2BGR_NV21);
            cv::cvtColor(frameHSV, frameHSV, cv::COLOR_BGR2Lab);

            cropResizeAndRotate(frameHSV);

            cv::Mat topFaceHSV(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);
            cv::Mat leftFaceHSV(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);
            cv::Mat rightFaceHSV(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC3);

            extractFaces(frameHSV, topFaceHSV, leftFaceHSV, rightFaceHSV);
            // Write the facelets to the
            saveFacelets(topFacelets, topFaceHSV, leftFacelets, leftFaceHSV, rightFacelets, rightFaceHSV, data);
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

    std::vector<std::vector<RubikFacelet::Color>> RubikProcessorImpl::detectFacetColors(
            const cv::Mat &currentFrame,
            const std::vector<std::vector<RubikFacelet>> facetModel) {
        std::vector<std::vector<RubikFacelet::Color>> colors(3, std::vector<RubikFacelet::Color>(3));
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = facetModel[i][j];
                float innerCircleRadius = facelet.innerCircleRadius();
                if ((facelet.center.x - innerCircleRadius) >= 0 &&
                    (facelet.center.x + innerCircleRadius) <= currentFrame.cols &&
                    (facelet.center.y - innerCircleRadius) >= 0 &&
                    (facelet.center.y + innerCircleRadius) <= currentFrame.rows
                    && innerCircleRadius >= 0) {
                    cv::Rect roi = cv::Rect(
                            cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                            cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                    );
                    cv::Mat stickerRoiHSV;
                    // Convert the image to HSV
                    cv::cvtColor(currentFrame(roi), stickerRoiHSV, CV_RGB2HSV);

                    float whiteMinRatio;
                    if (i == 1 && j == 1) {
                        whiteMinRatio = 0.44f;
                    } else {
                        whiteMinRatio = 0.5f;
                    }
                    colors[i][j] = colorDetector->detectColor(stickerRoiHSV, whiteMinRatio, i * 10 + j, frameNumber);
                } else {
                    colors[i][j] = RubikFacelet::Color::WHITE;
                    LOG_DEBUG("NativeRubikProcessor", "frameNumberOld: %d FOUND INVALID RECT WHEN DETECTING COLORS", frameNumber);
                }
            }
        }
        return colors;
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

    void RubikProcessorImpl::cropResizeAndRotate(cv::Mat &matImage) {
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
            phaseOffset = 27 * faceletHSVByteCount;
        }
        // Top
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = topFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 4;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletHSVOffset + phaseOffset + (savedFacelets * faceletHSVByteCount));
                cv::resize(topFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION));
                savedFacelets++;
            }
        }
        // Left
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = leftFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 4;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletHSVOffset + phaseOffset + (savedFacelets * faceletHSVByteCount));
                cv::resize(leftFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION));
                savedFacelets++;
            }
        }
        // Right
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = rightFacelets[i][j];
                float innerCircleRadius = facelet.innerCircleRadius() / 4;
                cv::Rect roi = cv::Rect(
                        cv::Point2f(facelet.center.x - innerCircleRadius, facelet.center.y - innerCircleRadius),
                        cv::Point2f(facelet.center.x + innerCircleRadius, facelet.center.y + innerCircleRadius)
                );
                cv::Mat stickerHSV(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                                   (uchar *) data + firstFaceletHSVOffset + phaseOffset + (savedFacelets * faceletHSVByteCount));
                cv::resize(rightFaceHSV(roi), stickerHSV, cv::Size(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION));
                savedFacelets++;
            }
        }
    }

    void RubikProcessorImpl::applyColorsToResult(std::vector<std::vector<RubikFacelet>> &facelets,
                                                 const std::vector<std::vector<RubikFacelet::Color>> colors) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                facelets[i][j].color = colors[i][j];
            }
        }
    }

    void RubikProcessorImpl::upscaleResult(std::vector<std::vector<RubikFacelet>> &facelets) {

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                facelets[i][j].center.x *= upscalingRatio;
                facelets[i][j].center.y *= upscalingRatio;
                facelets[i][j].width *= upscalingRatio;
                facelets[i][j].height *= upscalingRatio;
            }
        }
    }

    std::string RubikProcessorImpl::analyzeColorsInternal(const uint8_t *data) {
        int processedFacelets = 0;
        for (int i = 0; i < 54; i++) {
            cv::Mat facelet(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                            (uchar *) data + firstFaceletHSVOffset + (processedFacelets * faceletHSVByteCount));
            cv::Mat faceletRGB;
            cv::cvtColor(facelet, faceletRGB, cv::COLOR_Lab2BGR);
            imageSaver->saveImage(faceletRGB, i + 1, "");
            processedFacelets++;
        }

        std::vector<cv::Scalar_<float>> meanValues(0);
        for (int i = 0; i < 54; i++) {
            cv::Mat facelet(DEFAULT_FACELET_DIMENSION, DEFAULT_FACELET_DIMENSION, CV_8UC3,
                            (uchar *) data + firstFaceletHSVOffset + (i * faceletHSVByteCount));

            cv::Mat faceletDouble;
            facelet.convertTo(faceletDouble, CV_32F);
            cv::Scalar_<float> faceletMeans = cv::mean(faceletDouble);
            meanValues.emplace_back(faceletMeans);
        }

        std::vector<int> labels;
        std::vector<cv::Scalar> centers;
        int expectedDifferentColors = 6;

        cv::TermCriteria criteria(CV_TERMCRIT_ITER, 10, 1.0);
        cv::kmeans(meanValues, expectedDifferentColors, labels, criteria, 5, cv::KmeansFlags::KMEANS_PP_CENTERS, centers);

        for (int i = 0; i < 54; i++) {
            LOG_DEBUG("TESTING",
                      "Facelet %d with mean LAB (%.2f, %.2f, %.2f) has been labeled as part of group %d which has center on (%.2f, %.2f, %.2f)",
                      i + 1,
                      meanValues[i][0], meanValues[i][1], meanValues[i][2],
                      labels[i],
                      centers[labels[i]][0], centers[labels[i]][1], centers[labels[i]][2]);
        }
        //Asume it's being tested in white top, green front, red right -> yellow top, orange front, blue left
        int whiteLabel = labels[4];
        int greenLabel = labels[13];
        int redLabel = labels[22];
        int yellowLabel = labels[31];
        int orangeLabel = labels[40];
        int blueLabel = labels[49];

        LOG_DEBUG("TESTING", "Labels: white %d, green %d, red %d, yellow %d, orange %d and blue %d",
                  whiteLabel, greenLabel, redLabel, yellowLabel, orangeLabel, blueLabel);

        std::string result = "\n";
        for (int i = 0; i < 54; i++) {
            std::string labelColor;

            if (labels[i] == whiteLabel) {
                result += "W";
            } else if (labels[i] == greenLabel) {
                result += "G";
            } else if (labels[i] == redLabel) {
                result += "R";
            } else if (labels[i] == yellowLabel) {
                result += "Y";
            } else if (labels[i] == orangeLabel) {
                result += "O";
            } else if (labels[i] == blueLabel) {
                result += "B";
            }

            if ((i + 1) % 9 == 0) {
                result += "\n";
            } else if ((i + 1) % 3 == 0) {
                result += " ";
            }
        }

        return result;
    }

} //namespace rbdt