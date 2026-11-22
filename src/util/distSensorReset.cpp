#include "main.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>

bool resetPos(const char* sensorName) {
    constexpr double fieldHalfSizeIn = 71.0;
    constexpr double mmPerIn = 25.4;
    constexpr double frontOffsetIn = 5.25;
    constexpr double sideOffsetIn = 5.5;
    constexpr double pi = 3.14159265358979323846;

    pros::Distance* sensor = nullptr;
    double sensorOffsetIn = 0.0;
    double sensorAngleOffsetDeg = 0.0;

    if (sensorName == nullptr) {
        printf("resetPos: sensor name cannot be null\n");
        return false;
    }
    if (std::strcmp(sensorName, "front") == 0) {
        sensor = &frontDistance;
        sensorOffsetIn = frontOffsetIn;
    } else if (std::strcmp(sensorName, "left") == 0) {
        sensor = &leftDistance;
        sensorOffsetIn = sideOffsetIn;
        sensorAngleOffsetDeg = -90.0;
    } else if (std::strcmp(sensorName, "right") == 0) {
        sensor = &rightDistance;
        sensorOffsetIn = sideOffsetIn;
        sensorAngleOffsetDeg = 90.0;
    } else {
        printf("resetPos: unknown sensor '%s'; use front, left, or right\n", sensorName);
        return false;
    }

    int32_t readings[3];
    int readingCount = 0;
    for (int i = 0; i < 3; i++) {
        int32_t reading = sensor->get();
        if (reading > 0 && reading < 9999) {
            readings[readingCount++] = reading;
        }
        if (i < 2) pros::delay(20);
    }
    if (readingCount == 0) {
        printf("resetPos(%s): no valid distance reading\n", sensorName);
        return false;
    }

    std::sort(readings, readings + readingCount);
    double distanceIn = readings[readingCount / 2] / mmPerIn;
    if (readingCount == 2) {
        distanceIn = (readings[0] + readings[1]) / (2.0 * mmPerIn);
    }

    lemlib::Pose pose = chassis.getPose();
    double angleRad = (pose.theta + sensorAngleOffsetDeg) * pi / 180.0;
    double directionX = std::sin(angleRad);
    double directionY = std::cos(angleRad);
    double sensorX = pose.x + sensorOffsetIn * directionX;
    double sensorY = pose.y + sensorOffsetIn * directionY;

    double distanceToXWall = 1e9;
    double distanceToYWall = 1e9;
    double xWall = 0.0;
    double yWall = 0.0;
    if (directionX > 1e-6) {
        xWall = fieldHalfSizeIn;
        distanceToXWall = (xWall - sensorX) / directionX;
    } else if (directionX < -1e-6) {
        xWall = -fieldHalfSizeIn;
        distanceToXWall = (xWall - sensorX) / directionX;
    }
    if (directionY > 1e-6) {
        yWall = fieldHalfSizeIn;
        distanceToYWall = (yWall - sensorY) / directionY;
    } else if (directionY < -1e-6) {
        yWall = -fieldHalfSizeIn;
        distanceToYWall = (yWall - sensorY) / directionY;
    }

    if (distanceToXWall <= 0.0 && distanceToYWall <= 0.0) {
        printf("resetPos(%s): sensor is not aimed toward a field wall\n", sensorName);
        return false;
    }

    double measuredDistanceIn = distanceIn + sensorOffsetIn;
    if (distanceToXWall > 0.0 && distanceToXWall <= distanceToYWall) {
        double x = xWall - measuredDistanceIn * directionX;
        if (std::abs(x) > fieldHalfSizeIn) {
            printf("resetPos(%s): reading implies invalid x position %.2f\n", sensorName, x);
            return false;
        }
        chassis.setPose(x, pose.y, pose.theta);
        printf("resetPos(%s): x=%.2f in, y=%.2f in, heading=%.1f deg\n",
               sensorName, x, pose.y, pose.theta);
    } else if (distanceToYWall > 0.0) {
        double y = yWall - measuredDistanceIn * directionY;
        if (std::abs(y) > fieldHalfSizeIn) {
            printf("resetPos(%s): reading implies invalid y position %.2f\n", sensorName, y);
            return false;
        }
        chassis.setPose(pose.x, y, pose.theta);
        printf("resetPos(%s): x=%.2f in, y=%.2f in, heading=%.1f deg\n",
               sensorName, pose.x, y, pose.theta);
    } else {
        printf("resetPos(%s): unable to determine the wall\n", sensorName);
        return false;
    }

    return true;
}

// reads a side distance sensor, works out which wall it's facing from the robot's
// heading, and snaps that axis of the odom pose to the wall (walls at +/-71 in).
float wallDistance(bool shouldPrint = false, bool useRightSensor = true, bool testing = false) {
    float offsetX = useRightSensor ? 1.6 : 1.6;
    float offsetY = useRightSensor ? -0.5 : 2.0;

    float distanceIn = (useRightSensor ? rightDistance.get() : leftDistance.get()) / 25.4 + offsetX;

    float angDeg = chassis.getPose().theta;
    float angRad = angDeg * M_PI / 180.0;

    float angle = fmod(angDeg, 360.0);
    if (angle < 0) angle += 360.0;

    float rotatedX = offsetX * cos(angRad) - offsetY * sin(angRad);
    float rotatedY = offsetX * sin(angRad) + offsetY * cos(angRad);

    // which axis the sensor is looking down, and which wall (+71 or -71) for the right
    // sensor; the left sensor faces the opposite wall.
    bool isXAxis = (angle >= 315 || angle < 45) || (angle >= 135 && angle < 225);
    int wallSign = (angle >= 315 || angle < 45) || (angle >= 225 && angle < 315) ? 1 : -1;
    if (!useRightSensor) wallSign = -wallSign;

    float correctedDist = isXAxis ? distanceIn * fabs(cos(angRad)) + fabs(rotatedX)
                                  : distanceIn * fabs(sin(angRad)) + fabs(rotatedY);
    float finalPos = wallSign * (71 - correctedDist);

    lemlib::Pose pose = chassis.getPose();
    float odomPos = isXAxis ? pose.x : pose.y;
    float error = odomPos - finalPos;   // + means odom thinks it's further positive than the sensor does

    if (!testing) {
        if (isXAxis) chassis.setPose(finalPos, pose.y, pose.theta);
        else         chassis.setPose(pose.x, finalPos, pose.theta);
    }

    if (shouldPrint) {
        std::cout << std::fixed << std::setprecision(2)
              << "Distance: " << (distanceIn - offsetX) << "\n"
              << "Corrected: " << correctedDist << ", Axis: " << (isXAxis ? "X" : "Y") << "\n"
              << "Reset pos: " << finalPos << "\n"
              << "raw" << distanceIn - offsetX << std::endl;
    }

    return finalPos;
}
