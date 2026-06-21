#include "main.h"
#include "robodash/api.h"

bool tuneMode = false; // set true for green screen set false for competition

enum Route { // add more routes as we make them
  AWP,
  SKILLS
};
Route testRoute = AWP;

//tasks

// only runs when tuneMode true
void positionTrackerTask() {
    while (true) {
    pros::lcd::print(1, "X: %.2f, Y: %.2f, Theta: %.2f",
                         chassis.getPose().x, 
                         chassis.getPose().y, 
                         chassis.getPose().theta);
    pros::delay(10);
    }
}

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

void initialize() {
    selector.focus();

    // define + run tasks here
    if (tuneMode == true){
        chassis.setPose(0, 0, 0);
        pros::lcd::initialize();
        pros::Task pos(&positionTrackerTask);
    }

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
  if (tuneMode){
    switch (testRoute) {
    case AWP:
      soloAWP();
      break;
    case SKILLS:
      break;
    }
  } else{
    selector.run_auton();
  }
 }


void opcontrol() {
  while (true) {
    // driver control functions go here
    handleDriveCommands();

    // 20 ms delay to avoid strain on the brain
	pros::delay(20);
	}
}