#include "main.h"

void backUp(float inches) {
    lemlib::Pose p = chassis.getPose(true);
    chassis.moveToPoint(p.x - inches * sin(p.theta), p.y - inches * cos(p.theta), 1000, {.forwards = false}, false);
}

void simpleSkills() {
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    // setClaw(true);
    initLift();
    pros::delay(300);
    liftPos(300);
    chassis.moveToPoint(0, -4, 200,{.forwards=false},false);
    chassis.swingToHeading(310, lemlib::DriveSide::RIGHT, 500);
    chassis.moveToPoint(19, -9, 1500, {.forwards=false,.maxSpeed=60}, false);
    chassis.turnToHeading(310,100);
    backUp(0.5);
    pros::delay(100);
    claw.set_value(true);
    pros::delay(200);
    chassis.moveToPoint(7.5,-3,800);
    chassis.turnToHeading(44,2000);
    chassis.moveToPoint(-5,-9,1000,{.maxSpeed=80},false);
    claw.set_value(false);
    pros::delay(50);
    liftPos(850);
    chassis.moveToPoint(0,-4,200,{.forwards=false},false);
    chassis.turnToHeading(310,300);
    chassis.moveToPoint(19, -9, 1500, {.forwards=false,.maxSpeed=60}, false);
    liftPos(200);
    pros::delay(500);
    claw.set_value(true);

}
