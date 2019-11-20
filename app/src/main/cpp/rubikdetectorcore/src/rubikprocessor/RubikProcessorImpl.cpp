//// Created by catalin on 12.07.2017.


#include <math.h>
#include "../../include/rubikdetector/rubikprocessor/internal/RubikProcessorImpl.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/imgproc/types_c.h>
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
        if (debuggable) {
            LOG_DEBUG("NativeRubikProcessor", "RubikProcessorBehavior - destructor.");
        }
    }

    std::vector<std::vector<RubikFacelet>> RubikProcessorImpl::process(
            const uint8_t *imageData) {
        return findCubeInternal(imageData);
    }

    void RubikProcessorImpl::updateImageProperties(const ImageProperties &newProperties) {
        applyImageProperties(newProperties);
    }

    void RubikProcessorImpl::overrideInputFrameWithOutputFrame(const uint8_t *imageData) {
        if (inputImageFormat == RubikProcessor::ImageFormat::RGBA8888) {
            //input frame is equal to output frame already, this would have no effect.
            return;
        }
        cv::Mat yuvFrame = cv::Mat(imageHeight + imageHeight / 2, imageWidth, CV_8UC1,
                                   (uchar *) imageData + inputImageOffset);
        cv::Mat rgbaFrame = cv::Mat(imageHeight, imageWidth, CV_8UC4,
                                    (uchar *) imageData + outputRgbaImageOffset);
        int reverseColorConversionCode;
        switch (inputImageFormat) {
            case RubikProcessor::ImageFormat::YUV_I420:
                reverseColorConversionCode = cv::COLOR_RGBA2YUV_I420;
                break;
            case RubikProcessor::ImageFormat::YUV_YV12:
                reverseColorConversionCode = cv::COLOR_RGBA2YUV_YV12;
                break;
            default:
                reverseColorConversionCode = NO_CONVERSION_NEEDED;
        }

        if (reverseColorConversionCode != NO_CONVERSION_NEEDED) {
            cv::cvtColor(rgbaFrame, yuvFrame, reverseColorConversionCode);
        } else {
            if (inputImageFormat == RubikProcessor::ImageFormat::YUV_NV21) {
                rbdt::encodeNV21(rgbaFrame, yuvFrame, imageWidth, imageHeight);
            } else if (inputImageFormat == RubikProcessor::ImageFormat::YUV_NV12) {
                rbdt::encodeNV12(rgbaFrame, yuvFrame, imageWidth, imageHeight);
            }
        }
    }

    void RubikProcessorImpl::setDebuggable(const bool isDebuggable) {
        if (debuggable) {
            LOG_DEBUG("NativeRubikProcessor", "setDebuggable. current:%d, new: %d, frameNumber: %d",
                      debuggable, isDebuggable, frameNumber);
        }
        debuggable = isDebuggable;
        colorDetector->setDebuggable(isDebuggable);
        if (imageSaver != nullptr) {
            imageSaver->setDebuggable(isDebuggable);
        }
        faceletsDetector->setDebuggable(isDebuggable);
        faceletsDrawController->setDebuggable(isDebuggable);
    }

    bool RubikProcessorImpl::isDebuggable() const {
        return debuggable;
    }

    int RubikProcessorImpl::getRequiredMemory() {
        return totalRequiredMemory;
    }

    int RubikProcessorImpl::getOutputFrameBufferOffset() {
        return outputRgbaImageOffset;
    }

    int RubikProcessorImpl::getOutputFrameByteCount() {
        return outputRgbaImageByteCount;
    }

    int RubikProcessorImpl::getInputFrameByteCount() {
        return inputImageByteCount;
    }

    int RubikProcessorImpl::getInputFrameBufferOffset() {
        return inputImageOffset;
    }

    void RubikProcessorImpl::updateDrawConfig(DrawConfig drawConfig) {
        faceletsDrawController->updateDrawConfig(drawConfig);
    }
/**##### END PUBLIC API #####**/
/**##### PRIVATE MEMBERS FROM HERE #####**/

    void RubikProcessorImpl::applyImageProperties(const ImageProperties &properties) {
        RubikProcessorImpl::inputImageFormat = properties.inputImageFormat;
        switch (inputImageFormat) {
            case RubikProcessor::ImageFormat::YUV_NV21:
                cvColorConversionCode = cv::COLOR_YUV2RGBA_NV21;
                break;
            case RubikProcessor::ImageFormat::YUV_NV12:
                cvColorConversionCode = cv::COLOR_YUV2RGBA_NV12;
                break;
            case RubikProcessor::ImageFormat::YUV_I420:
                cvColorConversionCode = cv::COLOR_YUV2RGBA_I420;
                break;
            case RubikProcessor::ImageFormat::YUV_YV12:
                cvColorConversionCode = cv::COLOR_YUV2RGBA_YV12;
                break;
            case RubikProcessor::ImageFormat::RGBA8888:
                cvColorConversionCode = RubikProcessorImpl::NO_CONVERSION_NEEDED;
                break;
        }
        rotation = properties.rotation;
        imageWidth = properties.width;
        imageHeight = properties.height;
        // Rotated stuff
        if (rotation == -90 || rotation == 90 || rotation == 270) {
            rotatedImageHeight = imageWidth;
            rotatedImageWidth = imageHeight;
        } else {
            rotatedImageHeight = imageHeight;
            rotatedImageWidth = imageWidth;
        }

        // Largest side of original image
        largestDimension = rotatedImageWidth > rotatedImageHeight ? rotatedImageWidth : rotatedImageHeight;
        if (largestDimension > DEFAULT_DIMENSION) {
            // Frame larger than expected, needs downscaling
            upscalingRatio = (float) largestDimension / DEFAULT_DIMENSION;
            downscalingRatio = (float) DEFAULT_DIMENSION / largestDimension;
            if (largestDimension == rotatedImageHeight) {
                // Height is the largest side
                processingWidth = (int) round(rotatedImageHeight * downscalingRatio * ((float) rotatedImageWidth / rotatedImageHeight));
                processingHeight = (int) round(rotatedImageHeight * downscalingRatio);
            } else {
                // Width is the largest side
                processingWidth = (int) round(rotatedImageWidth * downscalingRatio);
                processingHeight = (int) round(rotatedImageWidth * downscalingRatio * ((float) rotatedImageHeight / rotatedImageWidth));
            }
        } else if (largestDimension < DEFAULT_DIMENSION) {
            // Frame smaller than expected, processing dimensions will be 320x240 or 240x320
            upscalingRatio = 1.0f;
            downscalingRatio = 1.0f;
            processingHeight = largestDimension == rotatedImageHeight ? 320 : 240;
            processingWidth = largestDimension == rotatedImageWidth ? 320 : 240;
        } else {
            upscalingRatio = 1.0f;
            downscalingRatio = 1.0f;
            processingHeight = rotatedImageHeight;
            processingWidth = rotatedImageWidth;
        }
        needsResize = rotatedImageHeight != processingHeight || rotatedImageWidth != processingWidth;

        // Calculate offsets
        if (inputImageFormat != RubikProcessor::ImageFormat::RGBA8888) {
            // image is in one of the supported YUV formats
            // IMPORTANT, when calculating offsets of YUV don't use rotated values because rotation will only happen after RGBA transformation
            outputRgbaImageByteCount = imageWidth * imageHeight * 4;
            outputRgbaImageOffset = imageWidth * (imageHeight + imageHeight / 2);
            inputImageByteCount = imageWidth * (imageHeight + imageHeight / 2);
            inputImageOffset = RubikProcessorImpl::NO_OFFSET;

            if (needsResize) {
                processingRgbaImageOffset = outputRgbaImageByteCount + inputImageByteCount;
                processingRgbaImageByteCount = processingHeight * processingWidth * 4;
                processingGrayImageOffset = outputRgbaImageByteCount + inputImageByteCount + processingRgbaImageByteCount;
                processingGrayImageSize = processingHeight * processingWidth;

                totalRequiredMemory = outputRgbaImageByteCount + inputImageByteCount + processingGrayImageSize
                                      + processingRgbaImageByteCount;
            } else {
                totalRequiredMemory = outputRgbaImageByteCount + inputImageByteCount;
            }
        } else {
            // image is RGBA
            inputImageByteCount = outputRgbaImageByteCount = imageWidth * imageHeight * 4;
            inputImageOffset = outputRgbaImageOffset = RubikProcessorImpl::NO_OFFSET;

            if (needsResize) {
                processingRgbaImageOffset = outputRgbaImageByteCount;
                processingRgbaImageByteCount = processingHeight * processingWidth * 4;
                processingGrayImageOffset = outputRgbaImageByteCount + processingRgbaImageByteCount;
                processingGrayImageSize = processingHeight * processingWidth;

                totalRequiredMemory = outputRgbaImageByteCount + processingRgbaImageByteCount + processingGrayImageSize;
            } else {
                processingGrayImageSize = processingHeight * processingWidth;
                processingGrayImageOffset = outputRgbaImageByteCount;

                totalRequiredMemory = outputRgbaImageByteCount + processingGrayImageSize;
            }
        }
        faceletsDetector->onFrameSizeSelected(processingWidth, processingHeight);
    }

    std::vector<std::vector<RubikFacelet>> RubikProcessorImpl::findCubeInternal(const uint8_t *imageData) {
        double processingStart = 0;

        if (debuggable) {
            // only calculate frame rate if the processor is debuggable
            frameNumber++;
            processingStart = rbdt::getCurrentTimeMillis();
        }

        // Allocate the output mat
        cv::Mat outputFrameRgba(rotatedImageHeight, rotatedImageHeight, CV_8UC4, (uchar *) imageData + outputRgbaImageOffset);

        // IMPORTANT, processing color frames are in RGBA, if saved directly to disk through ImageSaver colors channels
        // will be mixed because it expects BGR, convert with CV_RGB2BGR before saving
        cv::Mat processingFrameRgba;
        cv::Mat processingFrameGrey;

        if (inputImageFormat != RubikProcessor::ImageFormat::RGBA8888) {
            // YUV image, need to convert the output & processing frames to RGBA8888
            if (debuggable) {
                LOG_DEBUG("NativeRubikProcessor", "Frame not RGB, needs conversion to RGBA.");
            }

            // Create a Mat from the YUV data and transform it into RGBA
            cv::Mat frameYuv(imageHeight + imageHeight / 2, imageWidth, CV_8UC1, (uchar *) imageData);
            cv::cvtColor(frameYuv, outputFrameRgba, cvColorConversionCode);

            // #Added
            rotateMat(outputFrameRgba, rotation);

            if (needsResize) {
                if (debuggable) {
                    LOG_DEBUG("NativeRubikProcessor", "Resizing frame to processing size.");
                    LOG_DEBUG("NativeRubikProcessor", "Image height: %d, width: %d. Processing height: %d, width: %d.",
                              rotatedImageHeight, rotatedImageWidth, processingHeight, processingWidth);
                }
                // Allocate the processing mats
                processingFrameRgba = cv::Mat(processingHeight, processingWidth, CV_8UC4, (uchar *) imageData + processingRgbaImageOffset);
                processingFrameGrey = cv::Mat(processingHeight, processingWidth, CV_8UC1, (uchar *) imageData + processingGrayImageOffset);
                // processing RGBA is obtained resizing output RGBA
                cv::resize(outputFrameRgba, processingFrameRgba, cv::Size(processingWidth, processingHeight));
                // Gray is obtained changing color space of the processing RGBA
                cv::cvtColor(processingFrameRgba, processingFrameGrey, CV_RGBA2GRAY);
            } else {
                if (debuggable) {
                    LOG_DEBUG("NativeRubikProcessor", "Frame already at processing size, no resize needed.");
                }
                // processing RGBA is just output RGBA
                processingFrameRgba = cv::Mat(outputFrameRgba);
                // Gray is obtained directly from the YUV
                // TODO check if this rotated stuff works here
                processingFrameGrey = frameYuv(cv::Rect(0, 0, rotatedImageWidth, rotatedImageHeight));
            }

        } else {
            // input already in RGBA8888 format
            if (debuggable) {
                LOG_DEBUG("NativeRubikProcessor", "###################ALREADY RGBA");
            }
            // Allocate the processing grey mat
            processingFrameGrey = cv::Mat(processingHeight, processingWidth, CV_8UC1, (uchar *) imageData + processingGrayImageOffset);

            // #Added
            rotateMat(outputFrameRgba, rotation);

            if (needsResize) {
                if (debuggable) {
                    LOG_DEBUG("NativeRubikProcessor", "Needs resize.");
                }
                // Allocate the processing RGBA mat
                processingFrameRgba = cv::Mat(processingHeight, processingWidth, CV_8UC4, (uchar *) imageData + processingRgbaImageOffset);
                // processing RGBA is obtained resizing output RGBA
                cv::resize(outputFrameRgba, processingFrameRgba, cv::Size(processingWidth, processingHeight));
            } else {
                if (debuggable) {
                    LOG_DEBUG("NativeRubikProcessor", "No resize.");
                }
                // processing RGBA is just output RGBA
                processingFrameRgba = cv::Mat(outputFrameRgba);
            }

            // Gray is obtained changing color space of the processing RGBA
            cv::cvtColor(processingFrameRgba, processingFrameGrey, CV_RGBA2GRAY);
        }

        // #Added
        auto verticalOffset = static_cast<float>((processingHeight-processingWidth) / 2.0);
        auto lensOffset = static_cast<float>(processingWidth* 0.112);
        std::vector<cv::Point2f> topFaceCorners;
        // top corner
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(processingWidth * 0.5),
                static_cast<float>(processingWidth * 0.0459 + verticalOffset + lensOffset)));
        // right corner
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(processingWidth * 0.9366),
                static_cast<float>(processingWidth * 0.2979 + verticalOffset)));
        // left corner
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(processingWidth * 0.0634),
                static_cast<float>(processingWidth * 0.2979 + verticalOffset)));
        // center
        topFaceCorners.emplace_back(cv::Point2f(
                static_cast<float>(processingWidth * 0.5),
                static_cast<float>(processingWidth * 0.55 + verticalOffset)));
        applyPerspectiveTransform(processingFrameRgba, processingFrameRgba, topFaceCorners, cv::Size(processingWidth, processingWidth));
        applyPerspectiveTransform(processingFrameGrey, processingFrameGrey, topFaceCorners, cv::Size(processingWidth, processingWidth));
//        cv::cvtColor(processingFrameRgba, processingFrameRgba, CV_RGBA2BGR);
        imageSaver->saveImage(processingFrameRgba, frameNumber, "_perspective");
        // End #Added


        if (isDebuggable()) {
            LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - Searching for facelets.");
        }
        std::vector<std::vector<RubikFacelet>> facelets = faceletsDetector->detect(
                processingFrameGrey,
                frameNumber);
        if (facelets.size() != 0) {
            if (isDebuggable()) {
                LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - Found facelets!");
            }
            std::vector<std::vector<RubikFacelet::Color>> colors = detectFacetColors(
                    processingFrameRgba,
                    facelets);
            if (isDebuggable()) {
                LOG_DEBUG("NativeRubikProcessor",
                          "RubikProcessor - Detected colors: {%c, %c, %c} {%c, %c, %c} {%c, %c, %c}\nApplying colors to facelets.",
                          rbdt::colorIntToChar(colors[0][0]),
                          rbdt::colorIntToChar(colors[0][1]),
                          rbdt::colorIntToChar(colors[0][2]),
                          rbdt::colorIntToChar(colors[1][0]),
                          rbdt::colorIntToChar(colors[1][1]),
                          rbdt::colorIntToChar(colors[1][2]),
                          rbdt::colorIntToChar(colors[2][0]),
                          rbdt::colorIntToChar(colors[2][1]),
                          rbdt::colorIntToChar(colors[2][2])
                );
            }
            applyColorsToResult(facelets, colors);

            if (needsResize) {
                if (isDebuggable()) {
                    LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - Rescaling result.");
                }
                upscaleResult(facelets);
            } else {
                if (isDebuggable()) {
                    LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - No rescaling needed.");
                }
            }
            if (isDebuggable()) {
                LOG_DEBUG("NativeRubikProcessor",
                          "RubikProcessor - Drawing the result to the output frame.");
            }
            faceletsDrawController->drawResultToMat(outputFrameRgba, facelets);
        } else {
            if (isDebuggable()) {
                LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - No cube found.");
            }
        }

        //BASICALLY DONE
        if (debuggable) {

            double processingEnd = rbdt::getCurrentTimeMillis();
            double delta = processingEnd - processingStart;
            double fps = 1000 / delta;
            frameRateSum += fps;
            float frameRateAverage = (float) frameRateSum / frameNumber;

            LOG_DEBUG("NativeRubikProcessor",
                      "Done processing in this frame.\nframeNumber: %d,\nframeRate current frame: %.2f,\nframeRateAverage: %.2f,\nframeRate: %.2f. \nRubikProcessor - Returning the found facelets",
                      frameNumber, fps, frameRateAverage, fps);
        }

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
                    colors[i][j] = colorDetector->detectColor(stickerRoiHSV,
                                                              whiteMinRatio,
                                                              i * 10 + j,
                                                              frameNumber);
                } else {
                    colors[i][j] = RubikFacelet::Color::WHITE;
                    if (debuggable) {
                        LOG_DEBUG("NativeRubikProcessor",
                                  "frameNumberOld: %d FOUND INVALID RECT WHEN DETECTING COLORS",
                                  frameNumber);
                    }
                }
            }
        }
        return colors;
    }

    // #Added
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
    // #EndAdded

    // #Added
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