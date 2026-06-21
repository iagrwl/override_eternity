#include "main.h"

// drive mode handler
void handleDriveCommands() {
  bool defaultDrive;
  int DHoldTime;
  if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_X)) {
            DHoldTime += 10; 
            if (DHoldTime >= 2000) { // how long the button has to be held to activate
                defaultDrive = !defaultDrive;
                controller.rumble(".."); 
                DHoldTime = 0; 
            }
        } else {
            DHoldTime = 0; 
        }

        handleDriveMode(defaultDrive);
}

void handleDriveMode(bool driveMode) {
  driveMode ? handleArcade() : handleTank();
}

void handleArcade() {
  int leftY = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
  int rightX = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
  chassis.arcade(leftY, rightX); // move the robot
}

void handleTank() {
  int leftY = controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
  int rightY = controller.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_Y);
  chassis.tank(leftY, rightY); // move the robot
}