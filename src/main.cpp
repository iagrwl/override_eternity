#include "main.h"
#include "robodash/api.h"

rd::Selector selector({
  {"solo AWP", &soloAWP}
});

rd::Console console;

void initialize() {
    selector.focus();

    initFlipper();
    initLift();

    chassis.calibrate();

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
void testIMU() {
    console.focus();
    chassis.setPose(0, 0, 0);

    while (true) {
        console.printf("Pose heading: %.2f", chassis.getPose().theta);
        pros::delay(100);
        console.clear();
    }

}

void autonomous() {
    // selector.run_auton();
    turnTesting(true);
    // driveTesting(true);
}

void opcontrol() {
    while (true) {
        handleArcade();

        flipperButtons();
        scoringMacro();
        liftControl();

        manualIntake();
        applyIntakeState();

        pros::delay(20);
    }
}
