#include "main.h"

// flipper arm positions, in degrees (encoder zeroed at init)
constexpr double flipperLoad = 0;
constexpr double flipperPara = 1060;
constexpr double flipperUp = 1600;
constexpr int flipperVelocity = 200; // rpm

constexpr int scoreReverseMs = 500; // how long L2 reverses the intake before raising the flipper

static void flipperLogger(void*) {
    while (true) {
        printf("flipper pos: %.1f\n", intakePivot.get_position());
        pros::delay(100);
    }
}

// drive the arm down against its stop for a second, then zero there --
// so 0 is always the real loading position no matter where the arm was at boot
void initFlipper() {
    intakePivot.set_brake_mode(pros::MotorBrake::hold);
    intakePivot.move(-60);
    pros::delay(1000);
    intakePivot.brake();
    intakePivot.tare_position();
    pros::Task logger(flipperLogger);
}

// drives to the target with the motor's onboard position control and holds there
void flipperPos(double degree) {
    intakePivot.move_absolute(degree, flipperVelocity);
}

// L1 -> parallel, Up -> highest, X -> reset everything (flipper + dr4b down, intake on)
void flipperButtons() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        flipperPos(flipperPara);
    } else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
        flipperPos(flipperUp);
    } else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)) {
        flipperPos(flipperLoad);
        resetLiftDown();
        setIntakeState(IntakeState::IN);
    }
}

// L2: reverse the intake to release, then raise the flipper to clear
void scoringMacro() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)) {
        setIntakeState(IntakeState::OUT);
        applyIntakeState();
        pros::delay(scoreReverseMs);

        setIntakeState(IntakeState::OFF);
        applyIntakeState();
        flipperPos(flipperUp);
    }
}
