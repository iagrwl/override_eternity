#include "main.h"
#include "robodash/api.h"

rd::Selector selector({
  {"solo AWP", &soloAWP},
  {"simple route", &simpleRoute},
  {"basic-parth", &basicParth}
});

rd::Console console;

void initialize() {
    selector.focus();
    initLift();
    chassis.calibrate();
    pros::delay(500);

    selector.on_select([](std::optional<rd::Selector::routine_t> routine) {
        if (routine == std::nullopt) {
            controller.print(2, 0, "select route");
        } else {
            controller.print(2, 0, "run: %s", routine.value().name.c_str());
        }
    });
}

void disabled() {}

void competition_initialize() {
    selector.focus();
}

void consoleWrite() {
    console.focus();

    while (true) {
        console.clear();

        lemlib::Pose pose = chassis.getPose();
        console.printf("X: %.1f  Y: %.1f  angle: %.1f\n", pose.x, pose.y, pose.theta);
        console.printf("claw: %s\n", isClawOpen ? "OPEN" : "CLOSED");

        pros::delay(200);
    }
}

void autonomous() {
    // selector.run_auton();
    basicParth();
}

void opcontrol() {
    pros::Task consoleTask(consoleWrite);

    while (true) {
        handleArcade();
        liftControl();
        handleClaw();
        manualIntake();
        applyIntakeState();

        pros::delay(20);
    }
}
