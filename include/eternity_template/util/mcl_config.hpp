#pragma once

#include <array>

namespace mcl {

// Angles use the robot/field convention used by this project and LemLib's
// default pose: 0 degrees points along +y (north), positive angles turn right.
// Sensor mounting offsets are expressed (right, forward), in inches.
struct SensorMount {
    float rightIn;
    float forwardIn;
    float headingOffsetDeg; // 0 front, 90 right, 180 back, 270 left
};

struct Config {
    static constexpr int kMaxParticles = 160;
    int particleCount = 100;

    float initialPositionStdDevIn = 2.0f;
    float motionNoisePerSqrtIn = 0.12f;
    float motionNoiseFloorIn = 0.08f;
    float measurementSigmaIn = 2.5f;
    float sensorLikelihoodFloor = 0.02f;
    float shallowAngleThreshold = 0.35f;
    float shortOutlierMarginIn = 18.0f;
    float shortOutlierConsensus = 0.80f;
    float resampleEssFraction = 0.55f;
    float correctionGain = 0.10f;
    float maxGoodSpreadIn = 5.0f;
    float taskPeriodMs = 20.0f;
    float debugPeriodMs = 200.0f;
    bool debugLogging = false; // keep stdout clean for route telemetry by default
    int validMinMm = 20;
    int validMaxMm = 2000;
    float wallCoordinateIn = 71.0f; // 144 in field, 1 in walls
    float poseResetDistanceIn = 12.0f;

    // Fill these in after measuring the four sensor locations on the robot.
    // Order: front, right, back, left. Distances internally use inches.
    std::array<SensorMount, 4> sensors{{
        {0.0f, 0.0f, 0.0f},   // FRONT: (right, forward) = (__, __)
        {0.0f, 0.0f, 90.0f},  // RIGHT: (right, forward) = (__, __)
        {0.0f, 0.0f, 180.0f}, // BACK:  (right, forward) = (__, __)
        {0.0f, 0.0f, 270.0f}  // LEFT:  (right, forward) = (__, __)
    }};
};

} // namespace mcl
