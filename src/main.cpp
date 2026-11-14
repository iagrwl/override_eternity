#include "main.h"

ui::Console console;

// routes live in src/auton/auton_list.cpp
extern const std::vector<ui::Auton> autonList;
extern const char* defaultAuton;

static void showPickOnController() {
    if (const ui::Auton* a = ui::selectedAuton()) controller.print(2, 0, "run: %-11.11s", a->name);
}

void initialize() {
    ui::init(autonList, defaultAuton);

    ui::setBootStatus("homing lift");
    initLift();

    ui::setBootStatus("calibrating");
    chassis.calibrate();
    pros::delay(500);

    left_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);
    right_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);

    claw.set_brake_mode(pros::MotorBrake::hold); // keeps the claw shut when you let go
    clawPivot.set_value(false);

    showPickOnController();
    ui::bootComplete();

    static pros::Task liftTask{liftPID};
    liftPidOn = false;

    pros::delay(300);
}

// called whenever the field / comp switch disables the robot (before auton, between
// auton and driver, and when the comp cable is first plugged in)
void disabled() {
    liftPidOn = false;
    
    if (ui::routineRunning()) ui::abortRoutine();
}


void competition_initialize() {
    showPickOnController();
}

void autonomous() {
   
    ui::runSelectedAuton();
}

void opcontrol() {
   
    liftPidOn = false;
    lift.brake();
    claw.move(0);

    while (true) {
        // a test started from the brain owns the robot until it finishes
        if (ui::routineRunning()) {
            pros::delay(20);
            continue;
        }

        handleArcade();
        //liftControl();
        manualLiftControl();
        handleClaw();
        handleToggle();

        pros::delay(20);
    }
}
