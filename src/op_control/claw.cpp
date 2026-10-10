#include "main.h"

bool isPivotOut = false;
static bool clawStarted = false;  // motor stays off until the first L1 tap
static bool clawClosing = true;   // first tap flips this to flexwheels
static uint32_t closeStart = 0;

void handleClaw() {
    // tap L1: spin the flexwheels (keeps going after you let go)
    // tap L1 again: spin the other way to close the claw. each tap flips between the two
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        clawClosing = !clawClosing;
        clawStarted = true;
        closeStart = pros::millis();
    }

    if (!clawStarted) {
        // nothing yet
    }
    else if (!clawClosing) {
        claw.move(-127);
    }
    else if (pros::millis() - closeStart > 300 && std::abs(claw.get_actual_velocity()) < 10) {
        claw.move(30); // claw is shut, just squeeze lightly so the 5.5W doesn't overheat
    }
    else {
        claw.move(127);
    }

    // B toggles the pneumatic pivot
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B)) {
        isPivotOut = !isPivotOut;
        clawPivot.set_value(isPivotOut);
    }
}
