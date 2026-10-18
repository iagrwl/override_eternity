#include "main.h"


constexpr int liftSpeed = 127;    // power for manual control with R1/R2 (0-127)

void initLift() {
    lift.set_brake_mode_all(pros::MotorBrake::hold);

    // COMMENTED BECAUSE LIFT IS SKIPPING RIGHT NOW
    lift.move(-100); 
    pros::delay(200); 
    
    double lastPos = -999;
    uint32_t startTime = pros::millis();
    const uint32_t homingTimeoutMs = 1000;
    bool stalled = false;
    while (pros::millis() - startTime < homingTimeoutMs) {
        double currentPos = lift.get_position();
        if (std::abs(currentPos - lastPos) < 2.0) {
            stalled = true;
            break;
        }
        lastPos = currentPos;
        pros::delay(50);
    }

    lift.brake();
    lift.tare_position_all(); // zero BOTH lift motors (tare_position() only does the first one)

}



void liftPos(double degree, int speed) {
    lift.move_absolute(degree, speed);
}

static bool stopping = false;

void liftControl() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        lift.move(liftSpeed);
        stopping = true;
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        lift.move(-liftSpeed);
        stopping = true;
    }
    else if (stopping) {
        // let go: actively fight the leftover momentum, then hold once it's stopped
        lift.move_velocity(0);
        if (std::abs(lift.get_actual_velocity()) < 5) {
            lift.brake();
            stopping = false;
        }
    }
}