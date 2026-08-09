#include "main.h"
#include "eternity_template/op_control/intake.hpp"

void manualIntake() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_UP)) {
        firstStage.move(127);
        secondStage.move(-127);
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT)) {
        firstStage.move(0);
        secondStage.move(0);
    }
}
