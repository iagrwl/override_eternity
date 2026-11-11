#include "main.h"

// tune these on the robot
static const double   lowerRateDegPerSec   = 60.0;  // how fast liftTarget ramps down
static const uint32_t sampleMs             = 50;
static const double   stallDeltaDeg        = 0.5;   // lift must move less than this per sample to count as stalled
static const double   stallErrorDeg        = 10.0;  // target must be this far below the lift before a stall counts
                                                    // (kG feedforward cancels the first ~5 deg of error, so the
                                                    //  PID is not really pushing down until the error is bigger)
static const uint32_t lowerTimeoutMs       = 2000;  // give up lowering after this long
static const uint32_t clawOpenMs           = 300;   // time for the claw to open before the lift drops away
static const double   liftHomeToleranceDeg = 3.0;
static const uint32_t liftHomeTimeoutMs    = 1500;

// Blocking. Ramps the lift PID target down until the lift stops following it
// (pin is resting on the goal), opens the claw, then sends the lift to 0 under
// PID and returns once it gets there. The lift PID task does all the driving.
void dynamicRelease() {
    const double stepDeg = lowerRateDegPerSec * sampleMs / 1000.0;

    // 1. start the PID from where the lift is now and walk the target down
    double target = liftAngle();
    liftTarget = target;
    liftPidOn = true;

    double lastAngle = liftAngle();
    bool stalled = false;
    uint32_t start = pros::millis();
    while (pros::millis() - start < lowerTimeoutMs) {
        target = std::max(0.0, target - stepDeg);
        liftTarget = target;
        pros::delay(sampleMs);

        double angle = liftAngle();
        bool pidPushing = (angle - target) > stallErrorDeg;
        if (pidPushing && std::abs(angle - lastAngle) < stallDeltaDeg) {
            stalled = true;
            break;
        }
        if (angle <= liftHomeToleranceDeg) break; // hit the bottom without meeting anything
        lastAngle = angle;
    }

    // stop pushing into the goal: hold right where it stalled
    liftTarget = liftAngle();
    printf("dynamicRelease: %s after %lu ms at lift %.1f deg (target was %.1f)\n",
           stalled ? "stalled" : "no stall", (unsigned long)(pros::millis() - start), liftAngle(), target);

    // 2. let go of the pin
    claw.move(127);
    pros::delay(clawOpenMs);

    // 3. send the lift home and wait for it
    liftTarget = 0;
    start = pros::millis();
    while (liftAngle() > liftHomeToleranceDeg && pros::millis() - start < liftHomeTimeoutMs) {
        pros::delay(10);
    }
}
