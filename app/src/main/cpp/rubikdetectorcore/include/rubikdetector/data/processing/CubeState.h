//
// Created by Kohru on 29/11/2019.
//

#ifndef RUBIKSSCANANDSOLVE_CUBESTATE_H
#define RUBIKSSCANANDSOLVE_CUBESTATE_H


#include <vector>

namespace rbdt {

    class CubeState {
    public:

        enum class Face {
            UP, FRONT, RIGHT, DOWN, LEFT, BACK
        };

        CubeState();

        CubeState(std::vector<Face> facelets);

        ~CubeState();

        std::vector<Face> facelets;
    };

} //end namespace rbdt

#endif //RUBIKSSCANANDSOLVE_CUBESTATE_H
