//
// Created by Kohru on 29/11/2019.
//

#include <utility>
#include "../../../include/rubikdetector/data/processing/CubeState.h"


namespace rbdt {

    CubeState::CubeState() : CubeState(std::vector<Face>(), std::vector<cv::Scalar>()) {}

    CubeState::CubeState(std::vector<Face> facelets, std::vector<cv::Scalar> colors)
            : facelets(std::move(facelets)),
              colors(std::move(colors)) {}

    CubeState::~CubeState() {}

} //end namespace rbdt