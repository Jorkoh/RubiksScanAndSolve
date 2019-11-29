//
// Created by catalin on 12.07.2017.
//

#ifndef RUBIKDETECTOR_RUBIKPROCESSORIMPL_HPP
#define RUBIKDETECTOR_RUBIKPROCESSORIMPL_HPP

#include <opencv2/core/core.hpp>
#include "../../processing_templates/ImageProcessor.hpp"
#include "../../detectors/faceletsdetector/RubikFaceletsDetector.hpp"
#include "../../detectors/colordetector/RubikColorDetector.hpp"
#include "../../data/geometry/internal/Circle.hpp"
#include "../../data/processing/internal/HueColorEvidence.hpp"
#include "../../imagesaver/ImageSaver.hpp"
#include "../../data/config/DrawConfig.hpp"
#include "../../drawing/FaceletsDrawController.hpp"
#include "../../rubikprocessor/RubikProcessor.hpp"
#include "../../data/config/ImageProperties.hpp"
#include <iostream>
#include <memory>

namespace rbdt {
    class OnCubeDetectionResultListener;

    class RubikFacelet;

    class ImageProperties;

/**
 * Class to which a RubikProcessor delegates its work to.
 *
 * This is effectivelly the <i>private implementation</i> of a RubikProcessor's behavior.
 *
 * @warning Do not use RubikProcessorImpl directly, do not expose this in the API. Use the RubikProcessor instead.
 *
 * @note This class is effectivelly *is* the RubikProcessor (one does not make sense without the other), hence it should be regarded
 * as simply being the "RubikProcessor" instead of its implementation, or something else.
 *
 * This RubikProcessor relies on the concept of "processing frames". Typically, a processing frame is a regarded as being a scaled down
 * version of the input frame. Since there are enough details in a 320x240 frame for accurate detections to occur, we don't need the
 * full resolution of the input frame (if the input frame has a size of 1920x1080 pixels, for instance).
 *
 * Because of this this, before delegating work to its subcomponents, this processor first downscales the input frame to a resolution
 * close to 320x240 (actual aspect ratio may vary). This 320x240 frame is called the "processing frame". Generally, two processing frames
 * are required:
 *   - a grayscale one, which is used by the RubikFaceletsDetector to find where the facelets are located within the frame;
 *   - and a RGBA one, which will be used when detecting the facelets colors.
 *
 * These processing frames, when created, do not allocate additional memory. They instead are written back into the byte array received at
 * RubikProcessor::process(), at a location specific to the currently set ImageProperties. It is expected that the aforemention byte array is
 * large enough to also hold these processing frames, if its length is equal to the value returned by RubikProcessor::getRequiredMemory().
 *
 * The processing frames are written back in the array at the offset & length specified through fields such as RubikProcessorImpl::processingGrayImageOffset or
 * RubikProcessorImpl::processingRgbaImageByteCount. These are recomputed each time RubikProcessor::updateImageProperties() is called.
 *
 * Additionally, depending on the current ImageProperties, certain processing frames are not required. For instance, if the input frame size is already
 * 320x240, then the RGBA 320x240 processing frame is not needed, since the output frame will have the same format & size, and can be used instead
 * for color detection. In this case, only the grayscale processing frame will be created.
 *
 * These optimizations, although they substantially increased the implementation complexity, have been done primarily to prevent allocating & deallocating
 * memory each frame.
 *
 * @see RubikProcessor
 */
    class RubikProcessorImpl
            : public ImageProcessor<const uint8_t *, const ImageProperties &, std::vector<std::vector<RubikFacelet>>> {
    public:
        virtual ~RubikProcessorImpl();

        bool processScan(const uint8_t *scanData) override;

        bool processPhoto(const uint8_t *scanData, const uint8_t *photoData) override;

        std::string processColors(const uint8_t *imageData) override;

        void updateScanPhase(const bool &isSecondPhase) override;

        void updateImageProperties(const ImageProperties &imageProperties) override;

        int getRequiredMemory() override;

        int getFrameRGBABufferOffset() override;

        int getFaceletsByteCount() override;

        int getFrameYUVByteCount() override;

        int getFrameYUVBufferOffset() override;

        void updateDrawConfig(DrawConfig drawConfig);

    private:
        friend class RubikProcessor;

        RubikProcessorImpl(const ImageProperties scanProperties,
                           const ImageProperties photoProperties,
                           std::unique_ptr<RubikFaceletsDetector> faceletsDetector,
                           std::unique_ptr<RubikColorDetector> colorDetector,
                           std::unique_ptr<FaceletsDrawController> faceletsDrawController,
                           std::shared_ptr<ImageSaver> imageSaver);

        bool scanCubeInternal(const uint8_t *scanData);

        bool extractFaceletsInternal(const uint8_t *scanData, const uint8_t *photoData);

        std::string analyzeColorsInternal(const uint8_t *data);

        void rotateMat(cv::Mat &matImage, int rotFlag);

        void cropResizeAndRotate(cv::Mat &matImage, bool needsCrop, const cv::Rect& croppingRegion,
                                 int frameDimension, bool needsResize, int rotation);

        void extractFaces(cv::Mat &matImage, cv::Mat &topFace, cv::Mat &leftFace, cv::Mat &rightFace);

        void applyPerspectiveTransform(const cv::Mat &inputFrame, cv::Mat &outputFrame, const std::vector<cv::Point2f> &inputPoints,
                                       const cv::Size &outputSize);

        void saveFacelets(std::vector<std::vector<RubikFacelet>> &topFacelets, cv::Mat &topFaceHSV,
                          std::vector<std::vector<RubikFacelet>> &leftFacelets, cv::Mat &leftFaceHSV,
                          std::vector<std::vector<RubikFacelet>> &rightFacelets, cv::Mat &rightFaceHSV,
                          const uint8_t *data);

        void applyScanPhase(const bool &isSecondPhase);

        void applyScanProperties(const ImageProperties &properties);

        void applyPhotoProperties(const ImageProperties &properties);

        static constexpr int DEFAULT_DIMENSION = 480;

        static constexpr int DEFAULT_FACE_DIMENSION = 360;

        static constexpr int DEFAULT_FACELET_DIMENSION = 15;

        static constexpr int NO_OFFSET = 0;

        /**
         * Detector used to search for facelets within the frame
         */
        std::unique_ptr<RubikFaceletsDetector> faceletsDetector;

        /**
         * Detector used to extract facelet colors from the frame, once found
         */
        std::unique_ptr<RubikColorDetector> colorDetector;

        /**
         * Component used to draw the facelets on the output frame, if required
         */
        std::unique_ptr<FaceletsDrawController> faceletsDrawController;

        /**
         * Used to save debugging frames, if present and this RubikProcessor is is debuggable mode.
         */
        std::shared_ptr<ImageSaver> imageSaver;


        int frameNumber = 0;

        int frameRateSum = 0;

        bool isSecondPhase = false;

        int scanWidth;

        int photoWidth;

        int scanHeight;

        int photoHeight;

        int scanDimension;

        int photoDimension;

        cv::Rect scanCroppingRegion;

        cv::Rect photoCroppingRegion;

        int scanRotation;

        int photoRotation;

        /**
         * total required length in bytes of the input array passed to RubikProcessor::process()
         *
         * @see RubikProcessor::getRequiredMemory()
         */
        int totalRequiredMemory;

        /**
         * @see RubikProcessor::getOutputFrameBufferOffset()
         */
        int frameGrayOffset;

        /**
         * @see RubikProcessor::getOutputFrameByteCount()
         */
        int frameGrayByteCount;

        /**
         * @see RubikProcessor::getInputFrameBufferOffset()
         */
        int frameYUVOffset;

        /**
         * @see RubikProcessor::getInputFrameByteCount()
         */
        int frameYUVByteCount;

        float scanUpscalingRatio;

        float photoUpscalingRatio;

        float scanScalingRatio;

        float photoScalingRatio;

        bool scanNeedsCrop;

        bool photoNeedsCrop;

        bool scanNeedsResize;

        bool photoNeedsResize;

        int firstFaceGrayOffset;

        int firstFaceletOffset;

        int faceGrayByteCount;

        int faceletByteCount;
    };

} //namespace rbdt
#endif //RUBIKDETECTOR_RUBIKPROCESSORIMPL_HPP
