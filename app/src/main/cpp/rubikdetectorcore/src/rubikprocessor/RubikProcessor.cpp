//
// Created by catalin on 12.07.2017.
//

#include "../../include/rubikdetector/rubikprocessor/RubikProcessor.hpp"
#include "../../include/rubikdetector/rubikprocessor/internal/RubikProcessorImpl.hpp"
#include "../../include/rubikdetector/utils/CrossLog.hpp"
#include "../../include/rubikdetector/data/config/ImageProperties.hpp"

namespace rbdt {

    RubikProcessor::RubikProcessor(const ImageProperties imageProperties,
                                   std::unique_ptr<RubikFaceletsDetector> faceletsDetector,
                                   std::unique_ptr<RubikColorDetector> colorDetector,
                                   std::unique_ptr<FaceletsDrawController> faceletsDrawController,
                                   std::shared_ptr<ImageSaver> imageSaver)
            : behavior(std::unique_ptr<RubikProcessorImpl>(
            new RubikProcessorImpl(imageProperties,
                                   std::move(faceletsDetector),
                                   std::move(colorDetector),
                                   std::move(faceletsDrawController),
                                   imageSaver))) {}

    RubikProcessor::~RubikProcessor() {
        LOG_DEBUG("NativeRubikProcessor", "RubikProcessor - destructor.");
    }

    std::vector<std::vector<RubikFacelet>> RubikProcessor::process(const uint8_t *imageData) {
        return behavior->process(imageData);
    }

    void RubikProcessor::updateImageProperties(const ImageProperties &imageProperties) {
        behavior->updateImageProperties(imageProperties);
    }

    int RubikProcessor::getRequiredMemory() {
        return behavior->getRequiredMemory();
    }

    int RubikProcessor::getFrameRGBABufferOffset() {
        return behavior->getFrameRGBABufferOffset();
    }

    int RubikProcessor::getFrameRGBAByteCount() {
        return behavior->getFrameRGBAByteCount();
    }

    int RubikProcessor::getFrameYUVByteCount() {
        return behavior->getFrameYUVByteCount();
    }

    int RubikProcessor::getFrameYUVBufferOffset() {
        return behavior->getFrameYUVBufferOffset();
    }

    void RubikProcessor::updateDrawConfig(DrawConfig drawConfig) {
        behavior->updateDrawConfig(drawConfig);
    }
} //end namespace rbdt