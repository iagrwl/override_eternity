#include "main.h"
#include "robodash/api.h"

rd::Selector selector({
  {"solo AWP", &soloAWP}
});

rd::Console console;

void initialize() {
    selector.focus();
    initLift();
    chassis.calibrate();
    pros::delay(500);

    left_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);
    right_dt.set_brake_mode_all(pros::motor_brake_mode_e::E_MOTOR_BRAKE_COAST);

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
        console.printf("le pose is X: %.1f  Y: %.1f  angle: %.1f\n", pose.x, pose.y, pose.theta);
            
        pros::delay(200);
    }
}

void autonomous() {
    pros::Task pos(&consoleWrite);
    // driveTesting(true);
    pros::Task logger([] {
        while (true) {
            auto p = chassis.getPose();
            std::cout << p.x << "," << p.y << "," << p.theta
                    << " | L=" << left_dt.get_actual_velocity_all()[0]
                    << " R=" << right_dt.get_actual_velocity_all()[0] << std::endl;
            pros::delay(50);
        }
    });
    chassis.setPose(0, 0, 0);
    chassis.moveToPoint(0, 24, 5000, {}, false);
    logger.remove();
        
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