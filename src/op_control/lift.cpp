#include "main.h"


constexpr int liftVelocity = 100; 

// inital lift init stage. do not delete. this tares the liftat the start of the code. if init runs wrong way change motor port to neg sign.
void initLift() {
    lift.set_brake_mode_all(pros::MotorBrake::hold);

    // COMMENTED BECAUSE LIFT IS SKIPPING RIGHT NOW
    // lift.move(-100); 
    // pros::delay(200); 
    
    // double lastPos = -999;
    // uint32_t startTime = pros::millis();
    // const uint32_t homingTimeoutMs = 1000;
    // bool stalled = false;
    // while (pros::millis() - startTime < homingTimeoutMs) {
    //     double currentPos = lift.get_position();
    //     if (std::abs(currentPos - lastPos) < 2.0) {
    //         stalled = true;
    //         break;
    //     }
    //     lastPos = currentPos;
    //     pros::delay(50);
    // }

    lift.brake();
    lift.tare_position();

    // if (!stalled) {
    //     controller.print(0, 0, "LIFT NOT ZEROED");
    //     controller.rumble(".-.-");
    // }
}


void liftPos(double degree) {
    lift.move_absolute(degree, liftVelocity);
}

void liftControl() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        lift.move(127);
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        lift.move(-127);
    }
    else {
        lift.brake();
    }
}
