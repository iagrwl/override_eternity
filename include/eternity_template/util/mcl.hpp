#pragma once

#include <array>
#include <cstdint>

#include "eternity_template/util/mcl_config.hpp"
#include "lemlib/api.hpp"
#include "pros/distance.hpp"
#include "pros/rtos.hpp"

namespace mcl {

enum class Mode : uint8_t { OFF, LOG_ONLY, CORRECT };

struct Estimate {
    float x = 0.0f;
    float y = 0.0f;
    float stdX = 0.0f;
    float stdY = 0.0f;
    float confidence = 0.0f; // 1/(1 + max(stdX, stdY))
    uint32_t loopTimeUs = 0;
    bool initialized = false;
};

// LemLib's default heading is clockwise degrees from +y. MCL stores the same
// clockwise-from-+y convention in radians, making sensor/field transforms clear.
float lemlibDegreesToMclRadians(float headingDeg);
float mclRadiansToLemlibDegrees(float headingRad);

class Localizer {
  public:
    Localizer(lemlib::Chassis& chassis, const std::array<pros::Distance*, 4>& sensors,
              const Config& config = Config{});

    // Call after chassis.setPose() to begin a new particle cloud around that pose.
    void initialize(const lemlib::Pose& startingPose);
    void initializeFromChassis();
    void start();
    void setMode(Mode mode);
    Mode getMode();
    Estimate getEstimate();

  private:
    struct Particle { float x, y, weight; };
    struct SensorSample { int rawMm; float measuredIn; bool valid; float expectedIn; };
    struct SensorRay { float offsetX, offsetY, dirX, dirY; };

    void taskLoop();
    void updateOnce();
    void seedParticles(float x, float y);
    void estimateParticles();
    void resample();
    float randomUniform();
    float randomNormal();
    float expectedWallDistance(float x, float y, float dx, float dy) const;
    bool isGoodSensorReading(int rawMm) const;

    lemlib::Chassis& chassis_;
    std::array<pros::Distance*, 4> sensors_;
    Config config_;
    std::array<Particle, Config::kMaxParticles> particles_{};
    std::array<Particle, Config::kMaxParticles> resampled_{};
    std::array<SensorSample, 4> samples_{};
    Estimate estimate_{};
    lemlib::Pose previousPose_{0, 0, 0};
    bool havePreviousPose_ = false;
    bool taskStarted_ = false;
    Mode mode_ = Mode::LOG_ONLY;
    uint32_t rngState_ = 0xA341316Cu;
    pros::Mutex mutex_;
    pros::Task* task_ = nullptr;
};

} // namespace mcl

// Shared robot instance; set its mode at runtime with mclLocalizer.setMode(...).
extern mcl::Localizer mclLocalizer;
