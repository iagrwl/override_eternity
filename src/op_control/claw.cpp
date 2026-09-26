#include "main.h"

bool isClawOpen = false;
void handleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        isClawOpen = !isClawOpen;
        if (!isClawOpen) {
            lift.move_relative(50, 127);
        }
        claw.set_value(isClawOpen);
    }
    
}