#include "main.h"

ui::Console console;

// routes live in src/auton/auton_list.cpp
extern const std::vector<ui::Auton> autonList;
extern const char* defaultAuton;

void initialize() {
    ui::init(autonList, defaultAuton);

    ui::setBootStatus("homing lift");
    initLift();

    ui::setBootStatus("calibrating");
    chassis.calibrate();
    pros::delay(500);

    left_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);
    right_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);

    claw.set_value(false);

    if (const ui::Auton* a = ui::selectedAuton()) controller.print(2, 0, "run: %s", a->name);
    ui::bootComplete();
}

void disabled() {}

void competition_initialize() {}

void autonomous() {
    ui::runSelectedAuton();
}

void opcontrol() {
    while (true) {
        // a test started from the brain owns the robot until it finishes
        if (ui::routineRunning()) {
            pros::delay(20);
            continue;
        }

        handleArcade();
        liftControl();
        handleClaw();
        manualIntake();
        applyIntakeState();

        pros::delay(20);
    }
}
