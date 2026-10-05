#include "main.h"

void tuneYOffsets() {
    chassis.setPose(24, 0, 180);
    wallDistance(true, false, true);

    std::cout << "x: " << chassis.getPose().x 
              << "y: " << chassis.getPose().y 
              << "theta: " << chassis.getPose().theta << std::endl;

    chassis.turnToHeading(150, 750, {}, false);
    pros::delay(1000);
    wallDistance(true, false, true);
    std::cout << "x: " << chassis.getPose().x 
              << "y: " << chassis.getPose().y 
              << "theta: " << chassis.getPose().theta << std::endl;

    chassis.turnToHeading(210, 750, {}, false);
    pros::delay(1000);
    wallDistance(true, false, true);
    std::cout << "x: " << chassis.getPose().x 
              << "y: " << chassis.getPose().y 
              << "theta: " << chassis.getPose().theta << std::endl;
    
}

void tuneXOffsets() {
    chassis.setPose(24, 0, 180);
    wallDistance(true, false, true);
}