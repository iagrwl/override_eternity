#include "main.h"

void backUp(float inches) {
    lemlib::Pose p = chassis.getPose(true);
    chassis.moveToPoint(p.x - inches * sin(p.theta), p.y - inches * cos(p.theta), 1000, {.forwards = false}, false);
}

void simpleSkills() {
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    initLift();
    pros::delay(300);
    liftPos(650,100);
    pros::delay(300);
    chassis.moveToPoint(0,-14, 1000,{.forwards=false,.earlyExitRange=8});
    chassis.turnToHeading(90,1000);

    // chassis.moveToPose(-30,-10,90, 2300,{.forwards=false},false);
    liftPos(100,60);
    

}
