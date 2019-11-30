//
// Created by catalin on 05.09.2017.
//

#ifndef RUBIKDETECTOR_IMAGEPROCESSOR_HPP
#define RUBIKDETECTOR_IMAGEPROCESSOR_HPP

#include "../data/processing/CubeState.h"

namespace rbdt {

/**
 * Template which defines a generic Image Processor capable of performing general purpose image processing.
 *
 * Implementations of this processor are required to support changes to input & output image properties,
 * after an instance of the concrete processor has been instantiated, through ImageProcessor::updateImageProperties().
 *
 * Frames will be processed through the ImageProcessor::process() method. This processor is designed with performance in mind. Hence,
 * ImageProcessor::process() is expected to receive an array data container with enough memory allocated to store:
 *   - the input frame;
 *   - the output frame;
 *   - certain additional frames used internally during processing.
 *
 * Because of these memory requirements, each Processor implementation needs to compute its specific memory requirements and make them available
 * to clients through ImageProcessor::getRequiredMemory(). Since these requirements are dependent on the size & format of the input & output frames,
 * the value returned by ImageProcessor::getRequiredMemory() is expected to change  after a call to ImageProcessor::updateImageProperties().
 *
 * How the data will be organized by the Processor, within the array passed to ImageProcessor::process(), is implementation specific. However,
 * each subclass needs to be able to provide two things regarding this data:
 *  - the expected location of the <b>input</b> frame array, in the array passed to ImageProcessor::process();
 *    * this is achieved through ImageProcessor::getInputFrameBufferOffset().
 *  - the expected location of the <b>output</b> frame array, in the array passed to ImageProcessor::process().
 *    * this is achieved through ImageProcessor::getOutputFrameBufferOffset().
 *
 * @tparam INPUT_TYPE the type of the input frame data. This can be either a data array (e.g. <i>const uint8_t *</i>) or a data object which
 * contains a data array holding the image data.
 * @tparam IMAGE_PROPERTIES_TYPE type of the data class that holds the image properties of the input & output frame. This is expected to contain data such as:
 * the input/output frame width/height, image format, color space, etc.
 * @tparam RESULT_TYPE the type of the result
 */
    template<typename INPUT_TYPE, typename IMAGE_PROPERTIES_TYPE, typename RESULT_TYPE>
    class ImageProcessor {
    public:

        virtual ~ImageProcessor() {}

        virtual bool processScan(INPUT_TYPE scanFrame) = 0;

        virtual bool processPhoto(INPUT_TYPE scanFrame, INPUT_TYPE scanPhoto) = 0;

        virtual rbdt::CubeState processColors(INPUT_TYPE inputFrame) = 0;

        virtual void updateScanPhase(const bool &isSecondPhase) = 0;


        virtual void updateImageProperties(IMAGE_PROPERTIES_TYPE imageProperties) = 0;

        virtual int getRequiredMemory() = 0;

        virtual int getFrameYUVBufferOffset() = 0;

        virtual int getFrameYUVByteCount() = 0;

        virtual int getFrameRGBABufferOffset() = 0;

        virtual int getFaceletsByteCount() = 0;

    };
} //end namespace rbdt
#endif //RUBIKDETECTOR_IMAGEPROCESSOR_HPP
