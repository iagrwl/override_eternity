#include "main.h"
#include "robodash/api.h"

//tasks

// only runs when tuneMode true

void wallTask(void* param) {
  while (true) {
    wallDistance(true, true);
    pros::delay(10);
  }
}

void telemetryFunc(void* param) {
  while (true) {
    telemetry();
    pros::delay(12);
  }
}

rd::Selector selector({
  {"solo AWP", &soloAWP},
});

rd::Console console;

void positionTracker() {
    while (true) {
      console.printf("X: %.2f\n", chassis.getPose().x);
      console.printf("Y: %.2f\n", chassis.getPose().y);
      console.printf("Theta: %.2f\n", chassis.getPose().theta);

      pros::delay(20);
    }
}

void initialize() {
    selector.focus();

    // define + run tasks here
    pros::Task pos(&positionTracker);

    //pros::Task telemetryTask(telemetry);
    // set default values here

    // drivetrain calibration
    chassis.calibrate();
    left_dt.set_brake_mode(pros::MotorBrake::coast);
    right_dt.set_brake_mode(pros::MotorBrake::coast);


    selector.on_select([](std::optional<rd::Selector::routine_t> routine) {
        if (routine == std::nullopt) {
            std::cout << "No routine selected" << std::endl;
        controller.print(2, 0, "select route");

        } else {
            std::cout << "Selected Routine: " << routine.value().name << std::endl;
        controller.print(2, 0, "run: %s", routine.value().name.c_str());

        }
    });
    
} 

void disabled() {
    
}

void competition_initialize() {
  selector.focus();
}

void autonomous() {
    selector.run_auton();
 }


void opcontrol() {
  while (true) {
    // driver control functions go here
    handleDriveCommands();

    // 20 ms delay to avoid strain on the brain
	pros::delay(20);
	}
}