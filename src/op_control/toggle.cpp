#include "main.h"

static bool isToggleOn = false;

void handleToggle() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)) {
        toggleMotor.move(isToggleOn ? -127 : 0);
        isToggleOn = !isToggleOn;
    }
}