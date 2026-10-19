#include "main.h"

void three_pin_C() {
    // absolute field coords (origin = field center). robot starts facing the left wall;
    // x is 63 in from center; y comes from the left distance sensor (faces the bottom wall),
    // reading minus the 5.5 in sensor offset = center to wall (60.5 in at the usual start)
    float centerToWall = leftDistance.get() / 25.4 - 5.5;
    chassis.setPose(-63, -71 + centerToWall, -90);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    initLift();
    pros::delay(300);
    liftPos(650,20);
    pros::delay(300);
    chassis.moveToPoint(-60,-10.5,300);
    chassis.turnToPoint(-49,-20.5, 1000,{.forwards=false,.maxSpeed=50,.earlyExitRange=2});
    chassis.moveToPoint(-49,-20.5, 1000,{.forwards=false,.maxSpeed=50,.earlyExitRange=2});
    chassis.turnToHeading(0,300);
    chassis.moveToPoint(-49.5,-30.5,750,{.forwards=false,.maxSpeed=50},false);
    liftPos(0,70);
    pros::delay(750);
    claw.move(-127);
    chassis.moveToPoint(-50,-26.5,750,{.minSpeed=60},false);
    
    chassis.turnToPoint(-35,-27.5,500,{.maxSpeed=80});
    chassis.moveToPoint(-33,-31,1000,{.maxSpeed=70});
    pros::delay(400);
    claw.move(-127);
    pros::delay(200);

    liftPos(700, 80);
    chassis.turnToPoint(-45, -33, 750);
    chassis.moveToPoint(-45, -33, 1200, {.maxSpeed = 50}, false);
    chassis.turnToHeading(-90, 500);
    left_dt.move(-20);
    right_dt.move(-20);
    pros::delay(100);
    left_dt.move(0);
    right_dt.move(0);
    liftPos(100, 60);
    pros::delay(100);
    claw.move(127);

    // chassis.moveToPoint()
}
