//
// Created by catalin on 10.09.2017.
//

#include "../include/RubikDetectorJniUtils.hpp"

namespace rbdt_jni {

rbdt::RubikFacelet::Color colorFromInt(const int val) {
    switch (val) {
        case JNI_COLOR_RED:
            return rbdt::RubikFacelet::Color::RED;
        case JNI_COLOR_ORANGE:
            return rbdt::RubikFacelet::Color::ORANGE;
        case JNI_COLOR_YELLOW:
            return rbdt::RubikFacelet::Color::YELLOW;
        case JNI_COLOR_GREEN:
            return rbdt::RubikFacelet::Color::GREEN;
        case JNI_COLOR_BLUE:
            return rbdt::RubikFacelet::Color::BLUE;
        case JNI_COLOR_WHITE:
            return rbdt::RubikFacelet::Color::WHITE;
        default:
            return rbdt::RubikFacelet::Color::WHITE;
    }
}

rbdt::DrawConfig::DrawMode drawModeFromInt(const int val) {
    switch (val) {
        case JNI_DRAW_MODE_DO_NOT_DRAW:
            return rbdt::DrawConfig::DrawMode::DO_NOT_DRAW;
        case JNI_DRAW_MODE_DRAW_RECTANGLES:
            return rbdt::DrawConfig::DrawMode::DRAW_RECTANGLES;
        case JNI_DRAW_MODE_DRAW_CIRCLES:
            return rbdt::DrawConfig::DrawMode::DRAW_CIRCLES;
        default:
            return rbdt::DrawConfig::DrawMode::DRAW_CIRCLES;
    }
}
} //namespace rbdt
