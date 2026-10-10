#include "main.h"

void three_pin_C() {
    chassis.setPose(0, -64, 0);
    claw.move(127);
    lift.move(127);
    pros::delay(200);
    lift.move(-127);
    pros::delay(200);
    chassis.moveToPoint(0, -55, 1000, {.minSpeed = 100});
    chassis.turnToPoint(12, -47, 500);
    chassis.moveToPoint(12, -47, 1500, {.minSpeed = 30, .earlyExitRange = 3});
    lift.move_absolute(200, 127);
    chassis.turnToPoint(24, -47, 800);
    chassis.moveToPoint(24, -47, 1500, {.maxSpeed = 80});
    pros::delay(200);
    claw.move(-127);
    chassis.moveToPoint(12, -47, 1200, {.forwards = false});
    lift.move_absolute(0, 127);
    chassis.turnToPoint(24, -24, 750);
    claw.move(127);
    chassis.moveToPoint(24,-24, 1500, {.maxSpeed = 80});

    
    

}