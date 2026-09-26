#include "main.h"

bool isClawOpen = false;
void handleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        isClawOpen = !isClawOpen;
        claw.set_value(isClawOpen);
        if (isClawOpen) {
            lift.move(80);
            pros::delay(100);
            lift.move(0);
        }
    }
    
}