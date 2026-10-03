#include "main.h"

void five_pin() {
    chassis.setPose(0, -64, 180);
    lift.move(127);
    pros::delay(200);
    lift.move(-127);
    pros::delay(200);
    chassis.moveToPoint(0, -47, 1000, {.forwards = false});
    lift.move(0);
    chassis.turnToPoint(24, -47, 500, {.forwards = false, .minSpeed = 30, .earlyExitRange = 30});
    pros::delay(200);
    lift.move_relative(400, 127);
    chassis.moveToPoint(24, -47, 1000, {.forwards = false}, false);
    chassis.moveToPoint(50, -47, 500, {.forwards = false});
    claw.set_value(true);
    chassis.moveToPoint(12, -48, 1000);
    chassis.turnToPoint(24, -24, 500, {.forwards = false});
    // chassis.moveToPoint(18, -30, 1500, {.forwards = false});
    chassis.moveToPoint(24, -24, 2000, {.forwards = false, .maxSpeed = 50});
    pros::delay(200);
    claw.set_value(false);
    lift.move(80);
    pros::delay(100);
    lift.move(0); 
   
   

   
}
