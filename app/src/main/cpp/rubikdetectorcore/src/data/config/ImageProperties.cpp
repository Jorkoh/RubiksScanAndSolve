//
// Created by catalin on 06.09.2017.
//

#include "../../../include/rubikdetector/data/config/ImageProperties.hpp"

namespace rbdt {

    ImageProperties::ImageProperties(const int rotation,
                                     const int width,
                                     const int height) :
            rotation(rotation),
            width(width),
            height(height) {
        //empty
    }

} //end namespace rbdt
