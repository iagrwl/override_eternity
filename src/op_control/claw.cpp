#include "main.h"

bool isClawOpen = false;
void handleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        isClawOpen = !isClawOpen;
        claw.set_value(isClawOpen);
        lift.move(127);
        pros::delay(150);
        lift.move(0);
    }
    
}