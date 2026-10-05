#include "main.h"

void three_pin_C() {
    chassis.setPose(0, -64, 180);
    lift.move(127);
    pros::delay(200);
    lift.move(-127);
    pros::delay(200);
    chassis.moveToPoint(0, -47, 1000, {.forwards = false});
    

}