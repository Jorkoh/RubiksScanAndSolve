//
// Created by catalin on 06.09.2017.
//

#include "../../../include/rubikdetector/rubikprocessor/builder/RubikProcessorBuilder.hpp"
#include "../../../include/rubikdetector/detectors/faceletsdetector/SimpleFaceletsDetector.hpp"
#include "../../../include/rubikdetector/detectors/colordetector/HistogramColorDetector.hpp"
#include "../../../include/rubikdetector/data/config/ImageProperties.hpp"

namespace rbdt {

    RubikProcessorBuilder::RubikProcessorBuilder() :
            mScanRotation(DEFAULT_SCAN_ROTATION),
            mScanWidth(DEFAULT_SCAN_WIDTH),
            mScanHeight(DEFAULT_SCAN_HEIGHT),
            mPhotoRotation(DEFAULT_PHOTO_ROTATION),
            mPhotoWidth(DEFAULT_PHOTO_WIDTH),
            mPhotoHeight(DEFAULT_PHOTO_HEIGHT),
            mDrawConfig(DrawConfig::Default()),
            mFaceletsDetector(nullptr),
            mColorDetector(nullptr),
            mImageSaver(nullptr) {}

    RubikProcessorBuilder &RubikProcessorBuilder::scanRotation(int rotation) {
        mScanRotation = rotation;
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::photoRotation(int rotation) {
        mPhotoRotation = rotation;
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::scanSize(int width, int height) {
        mScanWidth = width;
        mScanHeight = height;
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::photoSize(int width, int height) {
        mPhotoWidth = width;
        mPhotoHeight = height;
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::drawConfig(DrawConfig drawConfig) {
        mDrawConfig = drawConfig;
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::colorDetector(
            std::unique_ptr<RubikColorDetector> colorDetector) {
        mColorDetector = std::move(colorDetector);
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::faceletsDetector(
            std::unique_ptr<RubikFaceletsDetector> faceletsDetector) {
        mFaceletsDetector = std::move(faceletsDetector);
        return *this;
    }

    RubikProcessorBuilder &RubikProcessorBuilder::imageSaver(std::shared_ptr<ImageSaver> imageSaver) {
        mImageSaver = imageSaver;
        return *this;
    }

    RubikProcessor *RubikProcessorBuilder::build() {

        if (mFaceletsDetector == nullptr) {
            //create default facelets detector, if a custom one wasn't set by caller
            mFaceletsDetector = std::unique_ptr<SimpleFaceletsDetector>(
                    new SimpleFaceletsDetector(mImageSaver));
        }

        if (mColorDetector == nullptr) {
            //create default color detector, if a custom one wasn't set by caller
            mColorDetector = std::unique_ptr<HistogramColorDetector>(
                    new HistogramColorDetector(mImageSaver));
        }

        std::unique_ptr<FaceletsDrawController> faceletsDrawController(
                new FaceletsDrawController(mDrawConfig));

        RubikProcessor *rubikDetector = new RubikProcessor(
                ImageProperties(mScanRotation, mScanWidth, mScanHeight),
                ImageProperties(mPhotoRotation, mPhotoWidth, mPhotoHeight),
                std::move(mFaceletsDetector),
                std::move(mColorDetector),
                std::move(faceletsDrawController),
                mImageSaver);

        return rubikDetector;
    }

} //end namespace rbdt