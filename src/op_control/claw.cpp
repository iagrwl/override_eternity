#include "main.h"
#include "eternity_template/op_control/claw.hpp"

bool isClawIntaking = true;
int clawTimer = 0;

int clawTarget = 0;

lemlib::PID clawController(
                            0.08,
                            0,
                            0.05,
                            0,
                            true
                           );

void clawPID(void*) {
    while (true) {
        int current = clawPivot.get_position();
        int error = clawTarget - current;
        double output = clawController.update(error);

        console.printf("Pos: %d, Err: %d, Out: %d\n", current, error, output);


        lift.move(output);

        pros::delay(20);
    }
}

void pivotClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        isClawIntaking = !isClawIntaking;

        clawTarget = isClawIntaking ? 360 : 0;

        clawController.reset();
    }
}

