//
// Created by Kohru on 29/11/2019.
//

#include "../../../include/rubikdetector/data/processing/CubeState.h"


namespace rbdt {

    CubeState::CubeState() : CubeState(std::vector<Face>()) {}

    CubeState::CubeState(std::vector<Face> facelets)
            : facelets(facelets) {}

    CubeState::~CubeState() {}

} //end namespace rbdt