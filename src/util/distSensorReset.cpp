#include "main.h"

// reads a side distance sensor, works out which wall it's facing from the robot's
// heading, and snaps that axis of the odom pose to the wall (walls at +/-71 in).
float wallDistance(bool shouldPrint = false, bool useRightSensor = true, bool testing = false) {
    float offsetX = useRightSensor ? 2.6 : 2.6;
    float offsetY = useRightSensor ? 0.5 : -2.0;

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
              << "raw" << distanceIn * 25.4 - offsetX << std::endl;
    }

    return finalPos;
}
