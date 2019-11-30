//
// Created by catalin on 12.07.2017.
//

#ifndef RUBIKDETECTOR_RUBIKPROCESSOR_HPP
#define RUBIKDETECTOR_RUBIKPROCESSOR_HPP

#include <vector>
#include <cstdint>
#include <memory>
#include "../data/processing/RubikFacelet.hpp"
#include "../processing_templates/ImageProcessor.hpp"
#include "../detectors/colordetector/RubikColorDetector.hpp"
#include "../detectors/faceletsdetector/RubikFaceletsDetector.hpp"

namespace rbdt {

class OnCubeDetectionResultListener;

class RubikProcessorImpl;

class ImageSaver;

class RubikProcessorBuilder;

class ImageProperties;

/**
 * Concrete implementation of the ImageProcessor, capable of detecing a Rubik's Cube in an image, and of
 * returning the position & colors of the visible Rubiks' Cube facelets.
 *
 * In broad lines, the operations performed by this component are the following:
 *  -# Computes its memory requirements, s.t. it can inform its clients of these requirements through it various methods (e.g. RubikProcessor::getRequiredMemory())
 *    * this also happens every time RubikProcessor::updateImageProperties() is called)
 *  -# Performes minimal pre-processing to the input & output data (i.e. scales, convers colors, etc)
 *  -# Delegates searching for the facelets to a RubikFaceletsDetector
 *  -# Delegates to a RubikColorDetector the task of mapping each of the found RubikFacelet object to a RubikFacelet::Color
 *  -# Delegates the drawing of the found facelets to a FaceletsDrawController.
 *  -# Prints debug info & saves debug images through a ImageSaver, if debuggable mode is active.
 *
 * This class expects input data to be provided as a <i>const uint8_t *</i> array, and will return a 2D std::vector
 * of RubikFacelet objects, if the Rubik's Cube is detected within the processed frame.
 *
 * Additionally, it expects the image properties passed to RubikProcessor::updateImageProperties() to be of type ImageProperties.
 *
 * The currently supported image formats for the input frame are specified here: RubikProcessor::ImageFormat. For the output format
 * only RubikProcessor::ImageFormat::RGBA8888 is currently supported.
 *
 * Use the RubikProcessorBuilder to create instances of this class. Once created, only the following properties can be modified:
 *   - the current ImageProperties, through RubikProcessor::updateImageProperties();
 *   - the current DrawConfig, through RubikProcessor::updateDrawConfig();
 *   - the debuggable state, through RubikProcessor::setDebuggable().
 *
 * This class is <b>not</b> thread safe. If used from multiple threads, the only hard requirement is to make sure RubikProcessor::process() is
 * not called for obsolete ImageProperties. In other words, always make sure no thread calls RubikProcessor::process() before
 * RubikProcessor::updateImageProperties() is called, after a change in input image size or format.
 *
 * Enabling the debug mode on this processor makes it print information relevant for debugging. If a non-null ImageSaver is also provided through the
 * RubikProcessorBuilder, then internal processing images are saved for debugging purposes, when debugging is turned on. However, performance is drastically
 * reduced when the images are saved, since writing to disk is typically very slow.
 *
 * Please read the documentation of the ImageProcessor for more information about the inner workings & requirements of this component.
 *
 * @see ImageProcessor
 */
class RubikProcessor
        : public ImageProcessor<const uint8_t *, const ImageProperties &, std::vector<std::vector<RubikFacelet>>> {

public:

    /**
     * Virtual destructor. Only needed for debug logging.
     */
    virtual ~RubikProcessor();

    bool processScan(const uint8_t *scanData) override;

    bool processPhoto(const uint8_t *scanData, const uint8_t *photoData) override;

    CubeState processColors(const uint8_t *imageData) override;

    void updateScanPhase(const bool &isSecondPhase) override;

    void updateImageProperties(const ImageProperties &imageProperties) override;

    int getRequiredMemory() override;

    int getFrameRGBABufferOffset() override;

    int getFaceletsByteCount() override;

    int getFrameYUVByteCount() override;

    int getFrameYUVBufferOffset() override;

private:
    friend class RubikProcessorBuilder;

    RubikProcessor(const ImageProperties scanProperties,
                   const ImageProperties photoProperties,
                   std::unique_ptr<RubikFaceletsDetector> faceletsDetector,
                   std::unique_ptr<RubikColorDetector> colorDetector,
                   std::shared_ptr<ImageSaver> imageSaver);

    /**
     * Pointer to private implementation (PIMPL Pattern)
     */
    std::unique_ptr<RubikProcessorImpl> behavior;
};
} //end namespace rbdt
#endif //RUBIKDETECTOR_RUBIKPROCESSOR_HPP
