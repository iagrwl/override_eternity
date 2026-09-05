#include "main.h"

// one shared state so the A-button toggle and any macros drive the intake through one place
IntakeState intakeState = IntakeState::OFF;

void manualIntake() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
        intakeState = (intakeState == IntakeState::IN) ? IntakeState::OFF : IntakeState::IN;
    }
}

void setIntakeState(IntakeState state) {
    intakeState = state;
}

void applyIntakeState() {
    int power = (intakeState == IntakeState::IN) ? 127
              : (intakeState == IntakeState::OUT) ? -127
              : 0;
    clawIntake.move(power);
    intake.move(power);
}
