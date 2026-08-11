#include "main.h"

// no rotation sensor on this dr4b, so just drive down for a full second to guarantee it bottoms out
void resetLiftDown() {
    lift.set_brake_mode_all(pros::MotorBrake::hold);
    lift.move(-127);
    pros::delay(1000);
    lift.brake();
    lift.tare_position();
}

void liftControl() {
    static int currentPower = 0;
    const int rampStep = 10;
    const int minHoldPower = 40;
    const int slowDownPower = 40;

    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        currentPower = 127;
        lift.move(currentPower);
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        currentPower = -slowDownPower;
        lift.move(currentPower);
    }
    else if (currentPower > minHoldPower) {
        currentPower -= rampStep;
        lift.move(currentPower);
    }
    else {
        currentPower = 0;
        lift.brake();
    }
}
