//
// Created by Kohru on 29/11/2019.
//

#ifndef RUBIKSSCANANDSOLVE_CUBESTATE_H
#define RUBIKSSCANANDSOLVE_CUBESTATE_H


#include <vector>
#include <opencv2/core/types.hpp>

namespace rbdt {

    class CubeState {
    public:

        enum class Face {
            UP, FRONT, RIGHT, DOWN, LEFT, BACK
        };

        CubeState();

        CubeState(std::vector<Face> facelets, std::vector<cv::Scalar> colors);

        ~CubeState();

        std::vector<Face> facelets;

        std::vector<cv::Scalar> colors;
    };

} //end namespace rbdt

#endif //RUBIKSSCANANDSOLVE_CUBESTATE_H
