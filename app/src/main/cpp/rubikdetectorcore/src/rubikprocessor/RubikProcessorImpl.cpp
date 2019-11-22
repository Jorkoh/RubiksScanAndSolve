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

    std::vector<std::vector<RubikFacelet>> RubikProcessorImpl::process(const uint8_t *imageData) {
        return findCubeInternal(imageData);
    }

    void RubikProcessorImpl::updateImageProperties(const ImageProperties &newProperties) {
        applyImageProperties(newProperties);
    }

    int RubikProcessorImpl::getRequiredMemory() {
        return totalRequiredMemory;
    }

    int RubikProcessorImpl::getFrameRGBABufferOffset() {
        return frameRGBAOffset;
    }

    int RubikProcessorImpl::getFrameRGBAByteCount() {
        return frameRGBAByteCount;
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

        // Calculate offsets: YUV -> RGBA -> Resized RGBA -> TOP RGBA -> LEFT RGBA -> RIGHT RGBA -> TOP GRAY -> LEFT GRAY -> RIGHT GRAY
        frameYUVByteCount = originalWidth * (originalHeight + originalHeight / 2);
        frameYUVOffset = RubikProcessorImpl::NO_OFFSET;

        frameRGBAByteCount = originalWidth * originalHeight * 4;
        frameRGBAOffset = frameYUVOffset + frameYUVByteCount;

        faceRGBAByteCount = DEFAULT_FACE_DIMENSION * 8;
        firstFaceRGBAOffset = frameRGBAOffset + frameRGBAByteCount;

        faceGrayByteCount = DEFAULT_FACE_DIMENSION * 2;
        firstFaceGrayOffset = firstFaceRGBAOffset + (3 * faceRGBAByteCount);

        totalRequiredMemory = frameYUVByteCount + frameRGBAByteCount + (3 * faceRGBAByteCount) + (3 * faceGrayByteCount);

        faceletsDetector->onFrameSizeSelected(DEFAULT_FACE_DIMENSION);
    }

    // Color, crop,  resize, rotate, transform, gray, detect facelets, (if found) extract facelets from color, (later on) analyze colors
    std::vector<std::vector<RubikFacelet>> RubikProcessorImpl::findCubeInternal(const uint8_t *data) {
        /* Frame rate stuff */
        frameNumber++;
        double processingStart = rbdt::getCurrentTimeMillis();
        /* Frame rate stuff */

        // Allocate the mats
        // IMPORTANT, processing color frames are in RGBA, if saved directly to disk for debugging convert with CV_RGB2BGR
        cv::Mat frameYUV(originalHeight + originalHeight / 2, originalWidth, CV_8UC1,
                (uchar *) data);
        cv::Mat frameRGBA(originalHeight, originalWidth, CV_8UC4,
                (uchar *) data + frameRGBAOffset);
        cv::Mat topFaceRgba(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC4,
                (uchar *) data + firstFaceRGBAOffset);
        cv::Mat leftFaceRgba(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC4,
                             (uchar *) data + firstFaceRGBAOffset + faceRGBAByteCount);
        cv::Mat rightFaceRgba(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC4,
                              (uchar *) data + firstFaceRGBAOffset + (2 * faceRGBAByteCount));
        cv::Mat topFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                (uchar *) data + firstFaceGrayOffset);
        cv::Mat leftFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                             (uchar *) data + firstFaceGrayOffset + faceGrayByteCount);
        cv::Mat rightFaceGray(DEFAULT_FACE_DIMENSION, DEFAULT_FACE_DIMENSION, CV_8UC1,
                              (uchar *) data + firstFaceGrayOffset + (2 * faceGrayByteCount));

        // Color
        cv::cvtColor(frameYUV, frameRGBA, cv::COLOR_YUV2RGBA_NV21);
        /**/
//        imageSaver->saveImage(frameRGBA, frameNumber, "_color");
        /**/

        // Crop
        if (needsCrop) {
            LOG_DEBUG("NativeRubikProcessor",
                      "Cropping frame of width %d and height %d into a square of width %d, height %d, x %d and y %d.",
                      frameRGBA.cols, frameRGBA.rows, croppingRegion.width, croppingRegion.height, croppingRegion.x, croppingRegion.y);
            frameRGBA = frameRGBA(croppingRegion);
            frameRGBA.cols = frameDimension;
            frameRGBA.rows = frameDimension;
        } else {
            LOG_DEBUG("NativeRubikProcessor", "Frame already at square, no cropping needed.");
        }

        // Resize
        if (needsResize) {
            LOG_DEBUG("NativeRubikProcessor", "Resizing frame to processing size. Image dimension: %d. Processing dimension: %d.",
                      frameDimension, DEFAULT_DIMENSION);
            cv::resize(frameRGBA, frameRGBA, cv::Size(DEFAULT_DIMENSION, DEFAULT_DIMENSION));
        } else {
            LOG_DEBUG("NativeRubikProcessor", "Frame already at processing size, no resize needed.");
        }

        // Rotate
        rotateMat(frameRGBA, rotation);

        /**/
//        imageSaver->saveImage(frameRGBA, frameNumber, "_crop_resize_rotate");
        /**/

        // Perspective transform to extract the faces
        extractFaces(frameRGBA, topFaceRgba, leftFaceRgba, rightFaceRgba);

        // Get the gray frames
        cv::cvtColor(topFaceRgba, topFaceGray, CV_RGBA2GRAY);
        cv::cvtColor(leftFaceRgba, leftFaceGray, CV_RGBA2GRAY);
        cv::cvtColor(rightFaceRgba, rightFaceGray, CV_RGBA2GRAY);

        /**/
//        imageSaver->saveImage(topFaceGray, frameNumber, "_gray_perspective_top");
//        imageSaver->saveImage(leftFaceGray, frameNumber, "_gray_perspective_left");
//        imageSaver->saveImage(rightFaceGray, frameNumber, "_gray_perspective_right");
        /**/

        std::vector<std::vector<RubikFacelet>> topFacelets = faceletsDetector->detect(topFaceGray, "top_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> leftFacelets = faceletsDetector->detect(leftFaceGray, "left_face_", frameNumber);
        std::vector<std::vector<RubikFacelet>> rightFacelets = faceletsDetector->detect(rightFaceGray, "right_face_", frameNumber);

        std::vector<std::vector<RubikFacelet>> facelets(0);
        if (!topFacelets.empty() && !leftFacelets.empty() && !rightFacelets.empty()) {
            LOG_DEBUG("NativeRubikProcessor", "CUBE FOUND!.");
            facelets = topFacelets;
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

        return facelets;
    }

    std::vector<std::vector<RubikFacelet::Color>> RubikProcessorImpl::detectFacetColors(
            const cv::Mat &currentFrame,
            const std::vector<std::vector<RubikFacelet>> facetModel) {
        std::vector<std::vector<RubikFacelet::Color>> colors(3, std::vector<RubikFacelet::Color>(3));
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet faceletRect = facetModel[i][j];
                float innerCircleRadius = faceletRect.innerCircleRadius();
                if (0 <= (faceletRect.center.x - innerCircleRadius) &&
                    0 <= (2 * innerCircleRadius) &&
                    (faceletRect.center.x + innerCircleRadius) <= currentFrame.cols &&
                    0 <= (faceletRect.center.y - innerCircleRadius) &&
                    (faceletRect.center.y + innerCircleRadius) <= currentFrame.rows) {
                    cv::Rect roi = cv::Rect(
                            cv::Point2f(faceletRect.center.x - innerCircleRadius,
                                        faceletRect.center.y - innerCircleRadius),
                            cv::Point2f(faceletRect.center.x + innerCircleRadius,
                                        faceletRect.center.y + innerCircleRadius)
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
        applyPerspectiveTransform(matImage, topFace, topFaceCorners, cv::Size(DEFAULT_DIMENSION, DEFAULT_DIMENSION));

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
        applyPerspectiveTransform(matImage, leftFace, leftFaceCorners, cv::Size(DEFAULT_DIMENSION, DEFAULT_DIMENSION));

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
        applyPerspectiveTransform(matImage, rightFace, rightFaceCorners, cv::Size(DEFAULT_DIMENSION, DEFAULT_DIMENSION));

        /**/
//        imageSaver->saveImage(topFace, frameNumber, "_perspective_top");
//        imageSaver->saveImage(leftFace, frameNumber, "_perspective_left");
//        imageSaver->saveImage(rightFace, frameNumber, "_perspective_right");
        /**/
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
// #EndAdded

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

} //namespace rbdt