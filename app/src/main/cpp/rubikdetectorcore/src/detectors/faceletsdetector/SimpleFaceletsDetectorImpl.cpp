//// Created by catalin on 12.07.2017.
//

#include <math.h>
#include <opencv2/imgproc/types_c.h>
#include "../../../include/rubikdetector/detectors/faceletsdetector/internal/SimpleFaceletsDetectorImpl.hpp"
#include "../../../include/rubikdetector/detectors/faceletsdetector/SimpleFaceletsDetector.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "../../../include/rubikdetector/utils/Utils.hpp"
#include "../../../include/rubikdetector/utils/CrossLog.hpp"

namespace rbdt {

#define CV_AA 16

/**##### PUBLIC API #####**/
    SimpleFaceletsDetectorImpl::SimpleFaceletsDetectorImpl() :
            SimpleFaceletsDetectorImpl(nullptr) {
        //empty default constructor
    }

    SimpleFaceletsDetectorImpl::SimpleFaceletsDetectorImpl(
            std::shared_ptr<ImageSaver> imageSaver) :
            imageSaver(imageSaver) {
    }

    SimpleFaceletsDetectorImpl::~SimpleFaceletsDetectorImpl() {
        LOG_DEBUG("RubikJniPart.cpp", "SimpleFaceletsDetectorBehavior - destructor.");
    }

    std::vector<std::vector<RubikFacelet>>
    SimpleFaceletsDetectorImpl::detect(cv::Mat &frameGray, const std::string &tag, const int frameNumber) {

        std::vector<std::vector<RubikFacelet>> facelets(0);
        std::vector<std::vector<cv::Point>> contours = detectContours(frameGray);
        std::vector<cv::RotatedRect> filteredRectangles;
        std::vector<Circle> filteredRectanglesInnerCircles;

        // Find rectangles, add inner circles to them
        filterContours(contours, filteredRectangles, filteredRectanglesInnerCircles);
//        LOG_DEBUG("RubikJniPart.cpp", "SimpleFaceletsDetectorBehavior - after filter. Found %d inner circles.",
//                  filteredRectanglesInnerCircles.size());
        /**/
//        cv::Mat drawing = cv::Mat::zeros(frameGray.size(), CV_8UC3);
//        drawFilteredRectangles(drawing, filteredRectangles);
//        imageSaver->saveImage(drawing, frameNumber * 10, tag + "filtered_rectangles");
        /**/

        // Test each one of those rectangles with inner circle (facelets)
        // potential facelets are those similar enough to the one being used as reference
        // estimated facelets are created based on the reference, a calculated margin and the estimated position of the reference
        bool faceFound = false;
        for (int i = 0; i < filteredRectanglesInnerCircles.size() && !faceFound; i++) {
            Circle referenceCircle = filteredRectanglesInnerCircles[i];
            // Filter inner circles (facelets) similar enough to the reference inner circle (facelet)
            std::vector<Circle> potentialFacelets = findPotentialFacelets(referenceCircle, filteredRectanglesInnerCircles, i);

            if (potentialFacelets.size() < MIN_POTENTIAL_FACELETS_REQUIRED) {
                LOG_DEBUG("RubikJniPart.cpp",
                          "SimpleFaceletsDetectorBehavior - found: %d out of a minimum of %d potential facelets. Ignore #%d.",
                          potentialFacelets.size(), MIN_POTENTIAL_FACELETS_REQUIRED, i);
                continue;
            }

            // Find the minimum distance between the circles
            float margin = computeMargin(referenceCircle, potentialFacelets);
            // Guess the facelet position in the face
            int position = estimatePositionOfFacelet(referenceCircle, potentialFacelets);
            if (position == -1) {
                LOG_DEBUG("RubikJniPart.cpp", "SimpleFaceletsDetectorBehavior - no valid position for the facelet. Ignore #%d.", i);
                continue;
            }
            // Create estimated facelet positions as circles assuming reference circle as top left position with margin and same area
            std::vector<Circle> estimatedFacelets = estimateRemainingFaceletsPositions(referenceCircle, margin, position);

            // Find those potential facelets that match the estimated ones
            std::vector<std::vector<Circle>> facetModel = matchEstimatedWithPotentialFacelets(potentialFacelets, estimatedFacelets);
//            saveDebugData(frameGray, filteredRectangles, referenceCircle, potentialFacelets, estimatedFacelets, frameNumber, tag);
            facetModel[0][0] = referenceCircle;
            // Decide if the cube has been found depending on the matched facelets placement
            faceFound = verifyIfFaceFound(facetModel);
            if (faceFound) {
                fillMissingFacelets(estimatedFacelets, facetModel);
                facelets = createResult(facetModel);

                // This check used to be done on the color detection part for some reason..
                for (int j = 0; j < 3; j++) {
                    for (int k = 0; k < 3; k++) {
                        RubikFacelet facelet = facelets[j][k];
                        float innerCircleRadius = facelet.innerCircleRadius();
                        if ((facelet.center.x - innerCircleRadius) < 0 ||
                            (facelet.center.x + innerCircleRadius) > frameGray.cols ||
                            (facelet.center.y - innerCircleRadius) < 0 ||
                            (facelet.center.y + innerCircleRadius) > frameGray.rows ||
                            innerCircleRadius < 0) {
                            LOG_DEBUG("NativeRubikProcessor", "frameNumber: %d FOUND INVALID RECT AFTER FINDING FACE", frameNumber);
                            facelets.clear();
                            faceFound = false;
                            break;
                        }
                    }
                }
            }
        }

        if (faceFound) {
            LOG_DEBUG("RubikJniPart.cpp", "%s detected", tag.c_str());
        } else {
            LOG_DEBUG("RubikJniPart.cpp", "%s not detected", tag.c_str());
        }

        return facelets;
    }

    void SimpleFaceletsDetectorImpl::onFrameSizeSelected(int dimension) {
        minValidShapeArea = (int) (dimension * 2 * MIN_VALID_SHAPE_TO_IMAGE_AREA_RATIO);
    }

    std::vector<std::vector<cv::Point>> SimpleFaceletsDetectorImpl::detectContours(const cv::Mat &frameGray) const {
        /// Reduce noise with a kernel
        cv::blur(frameGray, frameGray, cv::Size(BLUR_KERNEL_SIZE, BLUR_KERNEL_SIZE));
        // Canny detector
        cv::Canny(frameGray, frameGray, CANNY_LOW_THRESHOLD, CANNY_LOW_THRESHOLD * CANNY_THRESHOLD_RATIO, CANNY_APERTURE_SIZE, true);
        std::vector<cv::Vec4i> hierarchy;
        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(frameGray, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
        return contours;
    }

    //TODO: potential improvement, remove rectangles that are too similar
    void SimpleFaceletsDetectorImpl::filterContours(const std::vector<std::vector<cv::Point>> &contours,
                                                    std::vector<cv::RotatedRect> &possibleFacelets,
                                                    std::vector<Circle> &possibleFaceletsInnerCircles,
                                                    const int frameNumber) const {
        for (int i = 0; i < contours.size(); i++) {
            //approximate contour to a polygon, save it.
            cv::RotatedRect currentRect = cv::minAreaRect(contours[i]);
            //calculate aspect aspectRatio
            float maxDimensionValue = (float) std::max(currentRect.size.height, currentRect.size.width);
            float minDimensionValue = std::min(currentRect.size.height, currentRect.size.width);
            float aspectRatio = maxDimensionValue / minDimensionValue;
            //verify contour aspect ratio and area.
            if (aspectRatio < 1.4F && currentRect.size.area() > minValidShapeArea) {
                //if based on the above conditions it is considered as being a valid contour,
                //then save it for later processing
                possibleFacelets.push_back(currentRect);
                possibleFaceletsInnerCircles.emplace_back(currentRect);
            }
        }
    }

    float SimpleFaceletsDetectorImpl::computeMargin(Circle referenceCircle, std::vector<Circle> validCircles) {
        float margin = 320.0f;
        for (int i = 0; i < validCircles.size(); i++) {
            Circle testedCircle = validCircles[i];
            if (referenceCircle.contains(testedCircle.center) ||
                (testedCircle.center.x >
                 referenceCircle.center.x + 3 * referenceCircle.radius + CIRCLE_DISTANCE_BUFFER) ||
                (testedCircle.center.y >
                 referenceCircle.center.y + 3 * referenceCircle.radius + CIRCLE_DISTANCE_BUFFER)) {
                //if the center of the current rectangle/circle is  within the current reference, or is either too far from
                //the current rectangle (either to the left or to the right), then just skip it
                continue;
            }
            float currentMargin =
                    rbdt::pointsDistance(referenceCircle.center, testedCircle.center) -
                    referenceCircle.radius - testedCircle.radius;
            if (currentMargin < margin && currentMargin > 0) {
                margin = currentMargin;
            }
        }

        return (margin >= 320.0f || margin >= referenceCircle.radius) ? 10.f : margin;
    }

    int SimpleFaceletsDetectorImpl::estimatePositionOfFacelet(Circle referenceCircle, std::vector<Circle> validCircles) {
        // top row facelets are 0, 1 and 2, middle row 3, 4 and 5, bottom row 6, 7 and 8 from left to right
        bool hasFaceletsToTheRight = false;
        bool hasFaceletsToTheLeft = false;
        bool hasFaceletsDown = false;
        bool hasFaceletsUp = false;
        for (int i = 0; i < validCircles.size(); i++) {
            Circle testedCircle = validCircles[i];
            if (!hasFaceletsToTheRight && testedCircle.center.x > referenceCircle.center.x + referenceCircle.radius) {
                hasFaceletsToTheRight = true;
            }
            if (!hasFaceletsToTheLeft && testedCircle.center.x < referenceCircle.center.x - referenceCircle.radius) {
                hasFaceletsToTheLeft = true;
            }
            if (!hasFaceletsDown && testedCircle.center.y > referenceCircle.center.y + referenceCircle.radius) {
                hasFaceletsDown = true;
            }
            if (!hasFaceletsUp && testedCircle.center.y < referenceCircle.center.y - referenceCircle.radius) {
                hasFaceletsUp = true;
            }
            if (hasFaceletsToTheRight && hasFaceletsToTheLeft && hasFaceletsDown && hasFaceletsUp) {
                break;
            }
        }
        // Yes, this can be simplified. No, I won't change it because this way it's easily readable
        if (hasFaceletsToTheRight && !hasFaceletsToTheLeft && hasFaceletsDown && !hasFaceletsUp) {
            return 0;
        } else if (hasFaceletsToTheRight && hasFaceletsToTheLeft && hasFaceletsDown && !hasFaceletsUp) {
            return 1;
        } else if (!hasFaceletsToTheRight && hasFaceletsToTheLeft && hasFaceletsDown && !hasFaceletsUp) {
            return 2;
        } else if (hasFaceletsToTheRight && !hasFaceletsToTheLeft && hasFaceletsDown && hasFaceletsUp) {
            return 3;
        } else if (hasFaceletsToTheRight && hasFaceletsToTheLeft && hasFaceletsDown && hasFaceletsUp) {
            return 4;
        } else if (!hasFaceletsToTheRight && hasFaceletsToTheLeft && hasFaceletsDown && hasFaceletsUp) {
            return 5;
        } else if (hasFaceletsToTheRight && !hasFaceletsToTheLeft && !hasFaceletsDown && hasFaceletsUp) {
            return 6;
        } else if (hasFaceletsToTheRight && hasFaceletsToTheLeft && !hasFaceletsDown && hasFaceletsUp) {
            return 7;
        } else if (!hasFaceletsToTheRight && hasFaceletsToTheLeft && !hasFaceletsDown && hasFaceletsUp) {
            return 8;
        } else {
            return -1;
        }
    }

    std::vector<Circle> SimpleFaceletsDetectorImpl::findPotentialFacelets(
            const Circle &referenceCircle,
            const std::vector<Circle> &innerCircles,
            int referenceCircleIndex) const {

        //only have rectangles that have an area & orientation similar to the initial one
        std::vector<Circle> foundCircles;
        for (int j = 0; j < innerCircles.size(); j++) {
            if (referenceCircleIndex == j) {
                continue;
            }
            Circle testedCircle = innerCircles[j];
            float maxArea = (float) std::max(referenceCircle.area, testedCircle.area);
            int minArea = std::min(referenceCircle.area, testedCircle.area);
            float ratio = maxArea / minArea;
            float angleDifference = (float) std::abs((referenceCircle.angle - testedCircle.angle) * 180 / CV_PI);
            if (ratio < 1.5f && angleDifference < 35.0f) {
                foundCircles.push_back(testedCircle);
            }
        }
        return foundCircles;
    }

    std::vector<Circle> SimpleFaceletsDetectorImpl::estimateRemainingFaceletsPositions(
            const Circle &referenceCircle,
            float margin,
            int position) const {
        //draw the remaining rectangles
        std::vector<Circle> newCircles;
        float diameterWithMargin = referenceCircle.radius * 2 + margin;

        float angle = referenceCircle.angle;
        // column offsets
        float xOffsetColumn = diameterWithMargin * std::cos(angle);
        float yOffsetColumn = diameterWithMargin * std::sin(angle);
        // row offsets
        float xOffsetRow = diameterWithMargin * (float) std::cos(angle + M_PI_2);
        float yOffsetRow = diameterWithMargin * (float) std::sin(angle + M_PI_2);

        for (int i = 0; i < 9; i++) {
            if (i == position) {
                // skip the one being used as reference
                continue;
            }
            float xOffset = xOffsetColumn * (i % 3 - position % 3) + xOffsetRow * (i / 3 - position / 3);
            float yOffset = yOffsetColumn * (i % 3 - position % 3) + yOffsetRow * (i / 3 - position / 3);
            newCircles.emplace_back(referenceCircle, cv::Point2f(xOffset, yOffset));
        }

        return newCircles;
    }

    std::vector<std::vector<Circle>> SimpleFaceletsDetectorImpl::matchEstimatedWithPotentialFacelets(
            const std::vector<Circle> &potentialFacelets,
            const std::vector<Circle> &estimatedFacelets) {

        std::vector<std::vector<Circle>> facetModel(3, std::vector<Circle>(3));

        // for each one of the estimated facelets try to find a potential facelet whose overlaps enough
        for (int i = 0; i < 8; i++) {
            Circle testedCircle = estimatedFacelets[i];
            for (int j = 0; j < potentialFacelets.size(); j++) {
                if (potentialFacelets[j].contains(testedCircle.center)) {

                    float R = std::max(testedCircle.radius, potentialFacelets[j].radius);
                    float r = std::min(testedCircle.radius, potentialFacelets[j].radius);
                    float d = rbdt::pointsDistance(testedCircle.center, potentialFacelets[j].center);

                    float part1 = r * r * std::acos((d * d + r * r - R * R) / (2 * d * r));
                    float part2 = R * R * std::acos((d * d + R * R - r * r) / (2 * d * R));
                    float part3 = 0.5f * std::sqrt((-d + r + R) * (d + r - R) * (d - r + R) * (d + r + R));
                    float intersectionArea = part1 + part2 - part3;

                    float areasRatio = intersectionArea / potentialFacelets[j].area;
                    if (areasRatio > 0.55f) {
                        // found it
                        int auxI = (i + 1) / 3;
                        int auxJ = (i + 1) % 3;
                        facetModel[auxI][auxJ] = potentialFacelets[j];
                        //already found a matching rectangle, we are not interested in others. just continue
                        continue;
                    }
                }
            }
        }
        return facetModel;
    }

    bool SimpleFaceletsDetectorImpl::verifyIfFaceFound(const std::vector<std::vector<Circle>> &cubeFacet) const {
        int faceletsCount = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (!cubeFacet[i][j].isEmpty()) {
                    faceletsCount++;
                }
            }
        }
        return faceletsCount > 4;
    }

    void SimpleFaceletsDetectorImpl::fillMissingFacelets(
            const std::vector<Circle> &estimatedFacelets,
            std::vector<std::vector<Circle>> &facetModel) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (facetModel[i][j].isEmpty()) {
                    facetModel[i][j] = estimatedFacelets[i * 3 + j - 1];
                }
            }
        }
    }

    std::vector<std::vector<RubikFacelet>> SimpleFaceletsDetectorImpl::createResult(
            const std::vector<std::vector<Circle>> &faceModel) {

        std::vector<std::vector<RubikFacelet>> result(3, std::vector<RubikFacelet>(3));
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                result[i][j] = RubikFacelet(Point2d(faceModel[i][j].center.x,
                                                    faceModel[i][j].center.y),
                                            faceModel[i][j].originalRectWidth,
                                            faceModel[i][j].originalRectHeight,
                                            faceModel[i][j].angle);
            }
        }
        return result;
    }

    void SimpleFaceletsDetectorImpl::saveWholeFrame(const cv::Mat &currentFrame, int frameNr, const std::string &tag) const {
        cv::Mat bgr;
        cvtColor(currentFrame, bgr, CV_GRAY2BGR);
        imageSaver->saveImage(bgr, frameNr, tag + "full_frame");
    }

    cv::Mat SimpleFaceletsDetectorImpl::drawFilteredRectangles(const cv::Mat &drawing,
                                                               const std::vector<cv::RotatedRect> &filteredRectangles) const {
        for (int i = 0; i < filteredRectangles.size(); i++) {
            // rotated rectangle
            cv::Point2f rect_points[4];
            filteredRectangles[i].points(rect_points);
            for (int j = 0; j < 4; j++)
                line(drawing, rect_points[j], rect_points[(j + 1) % 4],
                     cv::Scalar(0, 255, 0), 1, CV_AA);
        }
        return drawing;
    }

    void SimpleFaceletsDetectorImpl::saveDebugData(const cv::Mat &frame,
                                                   const std::vector<cv::RotatedRect> &filteredRectangles,
                                                   const Circle &referenceCircle,
                                                   const std::vector<Circle> &potentialFacelets,
                                                   const std::vector<Circle> &estimatedFacelets,
                                                   const int frameNumber,
                                                   const std::string &tag) {
//        LOG_DEBUG("RUBIK_JNI_PART.cpp", "SimpleFaceletsDetectorBehavior - savingDebugData. imageSaver!=null: %d", imageSaver != nullptr);
        ///BEGIN PRINT
        if (imageSaver != nullptr) {
            ///save whole frame
            saveWholeFrame(frame, frameNumber * 10, tag);

            ///save filtered rectangles
            cv::Mat drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
            drawFilteredRectangles(drawing, filteredRectangles);
            imageSaver->saveImage(drawing, frameNumber * 10, tag + "filtered_rectangles");

            ///save potential facelets
            drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
            rbdt::drawCircles(drawing, potentialFacelets, cv::Scalar(255, 0, 0));
            rbdt::drawCircle(drawing, referenceCircle, cv::Scalar(0, 0, 255));
            imageSaver->saveImage(drawing, frameNumber * 10, tag + "potential_facelets");

            ///save estimated facelets
            drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
            rbdt::drawCircles(drawing, estimatedFacelets, cv::Scalar(0, 255, 80));
            rbdt::drawCircle(drawing, referenceCircle, cv::Scalar(0, 0, 255));
            imageSaver->saveImage(drawing, frameNumber * 10, tag + "estimated_facelets");

            ///save potential & estimated facelets which match
            ///recompute the incomplete facelet model, in order to print only the facelets which passed through all the filtering steps
            ///it's a waste indeed to compute them once more...but then again, this is for debugging so..
            std::vector<std::vector<Circle>> faceletIncompleteModel = matchEstimatedWithPotentialFacelets(potentialFacelets,
                                                                                                          estimatedFacelets);

            drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
            drawFilteredRectangles(drawing, filteredRectangles);
            rbdt::drawCircles(drawing, estimatedFacelets, cv::Scalar(0, 255, 80));
            rbdt::drawCircles(drawing, faceletIncompleteModel, cv::Scalar(255, 0, 0));
            rbdt::drawCircle(drawing, referenceCircle, cv::Scalar(255, 0, 0));
            rbdt::drawCircle(drawing, referenceCircle, cv::Scalar(0, 0, 255), 1, 2, true);
            imageSaver->saveImage(drawing, frameNumber * 10, tag + "matched_pot_est_facelets");

            ///save just the facelets which matched with the estimated ones
            drawing = cv::Mat::zeros(frame.size(), CV_8UC3);
            rbdt::drawCircles(drawing, faceletIncompleteModel, cv::Scalar(0, 255, 80));
            rbdt::drawCircle(drawing, referenceCircle, cv::Scalar(0, 0, 255), 1, 2, true);
            imageSaver->saveImage(drawing, frameNumber * 10, tag + "matched_potential_facelets");
        }
    }

    void SimpleFaceletsDetectorImpl::drawRectangleToMat(const cv::Mat &currentFrame,
                                                        const cv::RotatedRect &rotatedRect,
                                                        const cv::Scalar color) const {
        // rotated rectangle
        cv::Point2f rect_points[4];
        rotatedRect.points(rect_points);
        for (int j = 0; j < 4; j++)
            line(currentFrame, rect_points[j], rect_points[(j + 1) % 4],
                 color, 1, CV_AA);
    }

} //namespace rbdt