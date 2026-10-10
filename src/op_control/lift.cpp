#include "main.h"


constexpr int liftVelocity = 100; 

// inital lift init stage. do not delete. this tares the liftat the start of the code. if init runs wrong way change motor port to neg sign.
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

    // if (!stalled) {
    //     controller.print(0, 0, "LIFT NOT ZEROED");
    //     controller.rumble(".-.-");
    // }
}


// move the lift to a position (motor degrees from where it homed). wait = true blocks
// until it gets there (or 1.5 s passes), so the next line of an auto runs after it.
void liftPos(double degree, bool wait) {
    lift.move_absolute(degree, liftVelocity);
    if (!wait) return;
    uint32_t start = pros::millis();
    while (std::abs(lift.get_position() - degree) > 10 && pros::millis() - start < 1500) pros::delay(10);
}

static bool stopping = false;

void liftControl() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        lift.move(127);
        stopping = true;
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        lift.move(-127);
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
