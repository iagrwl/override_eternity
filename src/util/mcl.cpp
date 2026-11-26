#include "main.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace mcl {
namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kMmPerIn = 25.4f;
constexpr float kLikelihoodFloor = 1.0e-12f;

float clamp(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }
}

float lemlibDegreesToMclRadians(float headingDeg) { return headingDeg * (kPi / 180.0f); }
float mclRadiansToLemlibDegrees(float headingRad) { return headingRad * (180.0f / kPi); }

Localizer::Localizer(lemlib::Chassis& chassis, const std::array<pros::Distance*, 4>& sensors,
                     const Config& config)
    : chassis_(chassis), sensors_(sensors), config_(config) {
    config_.particleCount = std::max(1, std::min(config_.particleCount, Config::kMaxParticles));
    rngState_ ^= pros::millis() + 0x9E3779B9u;
}

void Localizer::initialize(const lemlib::Pose& startingPose) {
    mutex_.take();
    seedParticles(startingPose.x, startingPose.y);
    previousPose_ = startingPose;
    havePreviousPose_ = true;
    estimate_.initialized = true;
    estimateParticles();
    mutex_.give();
}

void Localizer::initializeFromChassis() {
    mutex_.take();
    const lemlib::Pose pose = chassis_.getPose();
    seedParticles(pose.x, pose.y);
    previousPose_ = pose;
    havePreviousPose_ = true;
    estimate_.initialized = true;
    estimateParticles();
    mutex_.give();
}

void Localizer::start() {
    mutex_.take();
    if (!taskStarted_) {
        taskStarted_ = true;
        if (!estimate_.initialized) {
            previousPose_ = chassis_.getPose();
            havePreviousPose_ = true;
            seedParticles(previousPose_.x, previousPose_.y);
            estimate_.initialized = true;
            estimateParticles();
        }
        // Constructed once, outside the allocation-free update loop.
        task_ = new pros::Task([this] { taskLoop(); }, "MCL localization");
    }
    mutex_.give();
}

void Localizer::setMode(Mode mode) {
    mutex_.take();
    if (mode_ == Mode::OFF && mode != Mode::OFF) {
        // OFF pauses prediction; restart from the current LemLib state so the
        // accumulated disabled interval is not mistaken for a single motion.
        const lemlib::Pose pose = chassis_.getPose();
        seedParticles(pose.x, pose.y);
        previousPose_ = pose;
        havePreviousPose_ = true;
        estimate_.initialized = true;
        estimateParticles();
    }
    mode_ = mode;
    mutex_.give();
}

Mode Localizer::getMode() {
    mutex_.take();
    Mode mode = mode_;
    mutex_.give();
    return mode;
}

Estimate Localizer::getEstimate() {
    mutex_.take();
    Estimate result = estimate_;
    mutex_.give();
    return result;
}

void Localizer::taskLoop() {
    while (true) {
        uint32_t begin = pros::micros();
        if (getMode() != Mode::OFF) updateOnce();
        uint32_t elapsed = pros::micros() - begin;
        mutex_.take();
        estimate_.loopTimeUs = elapsed;
        mutex_.give();
        pros::delay(static_cast<uint32_t>(std::max(1.0f, config_.taskPeriodMs)));
    }
}

float Localizer::randomUniform() {
    // xorshift32: fast, deterministic, and adequate for this small filter.
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    return static_cast<float>(rngState_ & 0x00FFFFFFu) / 16777216.0f;
}

float Localizer::randomNormal() {
    // Sum of uniforms approximates N(0,1) without Box-Muller trig/sqrt per draw.
    float sum = 0.0f;
    for (int i = 0; i < 6; ++i) sum += randomUniform();
    return (sum - 3.0f) * 1.41421356237f;
}

void Localizer::seedParticles(float x, float y) {
    for (int i = 0; i < config_.particleCount; ++i) {
        particles_[i] = {x + randomNormal() * config_.initialPositionStdDevIn,
                         y + randomNormal() * config_.initialPositionStdDevIn,
                         1.0f / config_.particleCount};
    }
    for (int i = config_.particleCount; i < Config::kMaxParticles; ++i) particles_[i] = {x, y, 0.0f};
}

float Localizer::expectedWallDistance(float x, float y, float dx, float dy) const {
    float best = 1.0e6f;
    const float wall = config_.wallCoordinateIn;
    if (dx > 1.0e-5f) best = std::min(best, (wall - x) / dx);
    else if (dx < -1.0e-5f) best = std::min(best, (-wall - x) / dx);
    if (dy > 1.0e-5f) best = std::min(best, (wall - y) / dy);
    else if (dy < -1.0e-5f) best = std::min(best, (-wall - y) / dy);
    return best > 0.0f ? best : 0.0f;
}

bool Localizer::isGoodSensorReading(int rawMm) const {
    // PROS returns PROS_ERR for an API error and 9999 mm when no object is seen.
    return rawMm >= config_.validMinMm && rawMm <= config_.validMaxMm;
}

void Localizer::updateOnce() {
    // Our mutex guards MCL state. LemLib's packaged odometry implementation is
    // not present as source, so this mutex cannot lock LemLib's private task.
    mutex_.take();
    Mode mode = mode_;
    if (mode == Mode::OFF) { mutex_.give(); return; }

    const lemlib::Pose pose = chassis_.getPose(); // default API: inches, degrees
    const float h = lemlibDegreesToMclRadians(pose.theta);
    const float sinH = std::sin(h), cosH = std::cos(h);

    if (!havePreviousPose_) {
        previousPose_ = pose;
        havePreviousPose_ = true;
        seedParticles(pose.x, pose.y);
    }
    const float dx = pose.x - previousPose_.x;
    const float dy = pose.y - previousPose_.y;
    const float moved = std::sqrt(dx * dx + dy * dy);
    if (moved > config_.poseResetDistanceIn) {
        // Explicit LemLib setPose() calls (for example at autonomous start) are
        // not robot motion. Re-center instead of propagating a teleport.
        seedParticles(pose.x, pose.y);
    } else {
        const float noiseStd = config_.motionNoiseFloorIn + config_.motionNoisePerSqrtIn * std::sqrt(moved);
        for (int i = 0; i < config_.particleCount; ++i) {
            particles_[i].x += dx + randomNormal() * noiseStd;
            particles_[i].y += dy + randomNormal() * noiseStd;
            particles_[i].x = clamp(particles_[i].x, -config_.wallCoordinateIn, config_.wallCoordinateIn);
            particles_[i].y = clamp(particles_[i].y, -config_.wallCoordinateIn, config_.wallCoordinateIn);
        }
    }
    previousPose_ = pose;

    // Transform sensor offsets and directions once per cycle, not once per particle.
    std::array<SensorRay, 4> rays{};
    for (int s = 0; s < 4; ++s) {
        const SensorMount& mount = config_.sensors[s];
        const float local = lemlibDegreesToMclRadians(mount.headingOffsetDeg);
        const float sensorAngle = h + local;
        rays[s] = {mount.rightIn * cosH + mount.forwardIn * sinH,
                   mount.forwardIn * cosH - mount.rightIn * sinH,
                   std::sin(sensorAngle), std::cos(sensorAngle)};
        const int raw = sensors_[s] ? sensors_[s]->get() : PROS_ERR;
        samples_[s] = {raw, raw > 0 ? raw / kMmPerIn : 0.0f, isGoodSensorReading(raw), 0.0f};
    }

    // A short return that conflicts with nearly every particle is likely an
    // element/robot occlusion, not a field-wall range. Ignore that sensor cycle.
    for (int s = 0; s < 4; ++s) {
        SensorSample& sample = samples_[s];
        if (!sample.valid) continue;
        int shortConflicts = 0;
        for (int i = 0; i < config_.particleCount; ++i) {
            const float ex = particles_[i].x + rays[s].offsetX;
            const float ey = particles_[i].y + rays[s].offsetY;
            const float predicted = expectedWallDistance(ex, ey, rays[s].dirX, rays[s].dirY);
            if (predicted > sample.measuredIn + config_.shortOutlierMarginIn) ++shortConflicts;
        }
        if (shortConflicts >= static_cast<int>(config_.shortOutlierConsensus * config_.particleCount))
            sample.valid = false;
    }

    const float sigma = std::max(0.1f, config_.measurementSigmaIn);
    const float invTwoSigmaSq = 0.5f / (sigma * sigma);
    float weightSum = 0.0f;
    for (int i = 0; i < config_.particleCount; ++i) {
        Particle& p = particles_[i];
        float weight = p.weight;
        for (int s = 0; s < 4; ++s) {
            const SensorSample& sample = samples_[s];
            if (!sample.valid) continue;
            const SensorRay& ray = rays[s];
            const float expected = expectedWallDistance(p.x + ray.offsetX, p.y + ray.offsetY, ray.dirX, ray.dirY);
            const float incidence = std::max(std::abs(ray.dirX), std::abs(ray.dirY));
            const float angleTrust = clamp((incidence - config_.shallowAngleThreshold) /
                                               std::max(0.01f, 1.0f - config_.shallowAngleThreshold),
                                           0.0f, 1.0f);
            if (angleTrust < 0.05f) continue;
            const float residual = sample.measuredIn - expected;
            float likelihood = std::exp(-residual * residual * invTwoSigmaSq * angleTrust);
            likelihood = std::max(config_.sensorLikelihoodFloor, likelihood);
            weight *= likelihood;
        }
        p.weight = weight;
        weightSum += weight;
    }
    if (!(weightSum > kLikelihoodFloor)) {
        const float uniform = 1.0f / config_.particleCount;
        for (int i = 0; i < config_.particleCount; ++i) particles_[i].weight = uniform;
    } else {
        for (int i = 0; i < config_.particleCount; ++i) particles_[i].weight /= weightSum;
    }

    estimateParticles();
    float essInv = 0.0f;
    for (int i = 0; i < config_.particleCount; ++i)
        essInv += particles_[i].weight * particles_[i].weight;
    const float ess = essInv > 0.0f ? 1.0f / essInv : 0.0f;
    if (ess < config_.resampleEssFraction * config_.particleCount) resample();

    if (mode == Mode::CORRECT && std::max(estimate_.stdX, estimate_.stdY) <= config_.maxGoodSpreadIn) {
        // Use the public LemLib API, preserve its fresh heading, and apply only
        // a fractional x/y correction. Public headers do not expose a lock shared
        // with LemLib's odometry task; CORRECT stays opt-in until checked on-robot.
        const lemlib::Pose latest = chassis_.getPose();
        const float cx = config_.correctionGain * (estimate_.x - latest.x);
        const float cy = config_.correctionGain * (estimate_.y - latest.y);
        chassis_.setPose(latest.x + cx, latest.y + cy, latest.theta);
        previousPose_.x = latest.x + cx;
        previousPose_.y = latest.y + cy;
        // Keep the cloud aligned with the pose correction, so it is not mistaken
        // for physical movement on the next prediction step.
        for (int i = 0; i < config_.particleCount; ++i) {
            particles_[i].x += cx;
            particles_[i].y += cy;
        }
        estimate_.x += cx;
        estimate_.y += cy;
    }

    const uint32_t now = pros::millis();
    static uint32_t lastDebugMs = 0;
    if (config_.debugLogging && now - lastDebugMs >= static_cast<uint32_t>(config_.debugPeriodMs)) {
        lastDebugMs = now;
        const lemlib::Pose debugPose = chassis_.getPose();
        std::printf("MCL mode=%s Lem(%.1f,%.1f,%.1f) est(%.1f,%.1f) d(%.1f,%.1f) sd(%.1f,%.1f) us=%lu",
                    mode == Mode::CORRECT ? "CORRECT" : "LOG_ONLY", debugPose.x, debugPose.y, debugPose.theta,
                    estimate_.x, estimate_.y, estimate_.x - debugPose.x, estimate_.y - debugPose.y,
                    estimate_.stdX, estimate_.stdY, static_cast<unsigned long>(estimate_.loopTimeUs));
        static const char* sensorNames[4] = {"FRONT", "RIGHT", "BACK", "LEFT"};
        for (int s = 0; s < 4; ++s) {
            samples_[s].expectedIn = expectedWallDistance(estimate_.x + rays[s].offsetX,
                                                         estimate_.y + rays[s].offsetY, rays[s].dirX, rays[s].dirY);
            std::printf(" %s=%d/%.1f->%.1f%s", sensorNames[s], samples_[s].rawMm,
                        samples_[s].valid ? samples_[s].measuredIn : -1.0f,
                        samples_[s].expectedIn, samples_[s].valid ? "" : "x");
        }
        std::printf("\n");
    }
    mutex_.give();
}

void Localizer::estimateParticles() {
    float x = 0.0f, y = 0.0f;
    for (int i = 0; i < config_.particleCount; ++i) {
        x += particles_[i].x * particles_[i].weight;
        y += particles_[i].y * particles_[i].weight;
    }
    float vx = 0.0f, vy = 0.0f;
    for (int i = 0; i < config_.particleCount; ++i) {
        const float ex = particles_[i].x - x, ey = particles_[i].y - y;
        vx += particles_[i].weight * ex * ex;
        vy += particles_[i].weight * ey * ey;
    }
    estimate_.x = x;
    estimate_.y = y;
    estimate_.stdX = std::sqrt(std::max(0.0f, vx));
    estimate_.stdY = std::sqrt(std::max(0.0f, vy));
    estimate_.confidence = 1.0f / (1.0f + std::max(estimate_.stdX, estimate_.stdY));
}

void Localizer::resample() {
    float step = 1.0f / config_.particleCount;
    float target = randomUniform() * step;
    float cumulative = particles_[0].weight;
    int source = 0;
    for (int i = 0; i < config_.particleCount; ++i) {
        const float mark = target + i * step;
        while (mark > cumulative && source < config_.particleCount - 1)
            cumulative += particles_[++source].weight;
        resampled_[i] = {particles_[source].x, particles_[source].y, step};
    }
    for (int i = 0; i < config_.particleCount; ++i) particles_[i] = resampled_[i];
}

} // namespace mcl

mcl::Localizer mclLocalizer(chassis, {{&frontDistance, &rightDistance, &backDistance, &leftDistance}});
