#include "main.h"
#include "eternity_template/op_control/claw.hpp"

bool isClawOpen = true;
int clawTimer = 0;
int clawDirection = 1;

void toggleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        clawDirection = isClawOpen ? 1 : -1;
        isClawOpen = !isClawOpen;

        clawMotor.move(127 * clawDirection);
        clawTimer = 0;
    }
    clawTimer += 20;

    if (clawTimer > 250) {
        if (clawDirection == -1) clawMotor.move(20 * clawDirection);
        if (clawDirection == 1) clawMotor.move(60 * clawDirection);
    }
}
