#include "main.h"

bool isClawOpen = false;
void handleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        isClawOpen = !isClawOpen;
        clawOpen.set_value(isClawOpen);
    }
    
}