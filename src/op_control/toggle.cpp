#include "main.h"

void handleToggle() {

    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)) {
        toggleMotor.move_relative(-720, 200);
    }
}
