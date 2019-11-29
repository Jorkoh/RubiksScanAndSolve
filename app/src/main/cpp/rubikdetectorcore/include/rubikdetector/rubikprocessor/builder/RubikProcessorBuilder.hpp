//
// Created by catalin on 06.09.2017.
//

#ifndef RUBIKDETECTOR_RUBIKPROCESSORBUILDER_HPP
#define RUBIKDETECTOR_RUBIKPROCESSORBUILDER_HPP


//#include <bits/unique_ptr.h>
#include <memory>
#include "../../detectors/colordetector/RubikColorDetector.hpp"
#include "../../detectors/faceletsdetector/RubikFaceletsDetector.hpp"
#include "../../imagesaver/ImageSaver.hpp"
#include "../RubikProcessor.hpp"
#include "../../data/config/DrawConfig.hpp"

namespace rbdt {
/**
 * Builder class for a RubikProcessor. A new instance of a RubikProcessor can be created just with:
 *
 * <pre>
 * rbdt::RubikProcessor *rubikProcessor = rbdt::RubikProcessorBuilder().build();
 * </pre>
 *
 * This creates a RubikProcessor with the following defaults:
 *   - input frame size: 320 x 240;
 *   - input frame format: RubikProcessor::ImageFormat::YUV_NV21;
 *   - DrawConfig: DrawConfig::Default()
 *   - RubikFaceletsDetector: an instance of a SimpleFaceletsDetector
 *   - RubikColorDetector: an instance of a HistogramColorDetector
 *   - ImageSaver: nullptr
 *   - debuggable: false
 *
 * See various methods customizing the properties before building the desired RubikProcessor.
 *
 */
    class RubikProcessorBuilder {

    public:

        /**
         * Creates a builder with default settings. This implies:
         *   - input frame size: 320 x 240;
         *   - input frame format: RubikProcessor::ImageFormat::YUV_NV21;
         *   - DrawConfig: DrawConfig::Default()
         *   - RubikFaceletsDetector: an instance of a SimpleFaceletsDetector
         *   - RubikColorDetector: an instance of a HistogramColorDetector
         *   - ImageSaver: nullptr
         *   - debuggable: false
         *
         * @return a RubikProcessorBuilder
         */
        RubikProcessorBuilder();

        RubikProcessorBuilder &scanRotation(int rotation);

        RubikProcessorBuilder &photoRotation(int rotation);

        RubikProcessorBuilder &scanSize(int width, int height);

        RubikProcessorBuilder &photoSize(int width, int height);

        /**
         * Specifies the DrawConfig. Drawing only occurs when the facelets are found. If you wish to not draw even
         * if the facelets are found, use DrawConfig::DoNotDraw();
         *
         * This can later be changed through RubikProcessor::updateDrawConfig().
         *
         * @param [in] drawConfig the desired DrawConfig
         * @return the same RubikProcessorBuilder instance
         */
        RubikProcessorBuilder &drawConfig(DrawConfig drawConfig);

        /**
         * Specifies the RubikColorDetector used by the RubikProcessor when detecting color.
         *
         * A custom implementation can be provided here. If not set, then a HistogramColorDetector will be used by default.
         *
         * @param [in] colorDetector the RubikColorDetector to be used.
         * @return the same RubikProcessorBuilder instance
         */
        RubikProcessorBuilder &colorDetector(std::unique_ptr<RubikColorDetector> colorDetector);

        /**
         * Specifies the RubikFaceletsDetector used by the RubikProcessor to search for the Rubik's Cube in a frame.
         *
         * A custom implementation can be provided here. If not set, then a SimpleFaceletsDetector will be used by default.
         *
         * @param [in] faceletsDetector the RubikFaceletsDetector to be used.
         * @return the same RubikProcessorBuilder instance
         */
        RubikProcessorBuilder &faceletsDetector(
                std::unique_ptr<RubikFaceletsDetector> faceletsDetector);

        /**
         * Set an ImageSaver object which can be used by the RubikProcessor to save to disk internal processing frames, when debuggable.
         *
         * Passing nullptr is safe and tells the RubikProcessor that not frames need to be saved, even if debuggable. By default,
         * nullptr is used.
         *
         * @param [in] imageSaver instance of a ImageSaver
         * @return the same RubikProcessorBuilder instance
         */
        //TODO get string, build the ImageSaver internally. Do not expose the ImageSaver to the API.
        RubikProcessorBuilder &imageSaver(std::shared_ptr<ImageSaver> imageSaver);

        /**
         * Builds a RubikProcessor with the configuration provided through this builder.
         * @return RubikProcessor
         */
        RubikProcessor *build();

    private:

        static constexpr int DEFAULT_SCAN_ROTATION = 90;

        static constexpr int DEFAULT_SCAN_WIDTH = 640;

        static constexpr int DEFAULT_SCAN_HEIGHT = 480;

        static constexpr int DEFAULT_PHOTO_ROTATION = 90;

        static constexpr int DEFAULT_PHOTO_WIDTH = 4032;

        static constexpr int DEFAULT_PHOTO_HEIGHT = 3024;

        int mScanRotation;

        int mPhotoRotation;

        int mScanWidth;

        int mPhotoWidth;

        int mScanHeight;

        int mPhotoHeight;

        DrawConfig mDrawConfig;

        std::unique_ptr<RubikColorDetector> mColorDetector;

        std::unique_ptr<RubikFaceletsDetector> mFaceletsDetector;

        std::shared_ptr<ImageSaver> mImageSaver;
    };

} //end namespace rbdt
#endif //RUBIKDETECTOR_RUBIKPROCESSORBUILDER_HPP
