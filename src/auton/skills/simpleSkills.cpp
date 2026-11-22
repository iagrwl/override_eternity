#include "main.h"

void simpleSkills() {
    // ---------------------------------------------------------------- setup
    // robot starts facing east (90), so its right side faces the bottom wall
    toggleColor.set_led_pwm(90);
    toggleUntil("red");
    chassis.setPose(-60.5, 0, 90);
    if (!resetPos("right")) return;
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    liftPidOn = true;
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n",
           chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    
    // ---------------------------------------------------------------- pin 1 entry
    claw.move(-127);
    
    liftTarget = 10;
    chassis.moveToPose(-48, -24, 180, 1500, {.lead = 0.4, .minSpeed = 60}, false);
    chassis.turnToHeading(-180, 300, {}, false);

    pros::delay(200);
    if (!resetPos("right")) return;

    // ---------------------------------------------------------------- pin 1 drop
    liftTarget = 0;
    pros::delay(800);
    claw.move(127);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    pros::delay(500);

    // ---------------------------------------------------------------- pin 2 pickup
    chassis.moveToPoint(-46, -8, 750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToPoint(-26, -19.5, 500, {.maxSpeed = 80}, false);
    claw.move(-127);
    chassis.moveToPoint(-26, -19.5, 1600, {.maxSpeed = 50}, false);
    liftTarget = 20;

    // ---------------------------------------------------------------- pin 2 drop
    chassis.turnToHeading(240, 500, {.maxSpeed = 80}, false);
    chassis.turnToHeading(255,500);
    chassis.moveToPoint(-48, -24, 1100, {.maxSpeed=60}, false);
    
    liftTarget = 0;
    pros::delay(450);
    claw.move(127);

    // ---------------------------------------------------------------- pin 3 pickup
    chassis.moveToPoint(-25,-22,1000,{.forwards=false}, false);
    claw.move(-127);
    chassis.turnToHeading(220,800);
    chassis.moveToPose(-50,-50,220,2200,{.lead = 0.4,.maxSpeed=55,.minSpeed=40},false);
    liftTarget = 40;
    pros::delay(500);


    
    // ---------------------------------------------------------------- pin 3 drop
    chassis.turnToPoint(-47, -22, 1000, {.maxSpeed = 80}, false);
    chassis.moveToPoint(-47, -22, 1200, {.maxSpeed = 50}, false);
    pros::delay(200);
    liftTarget = 30;
    pros::delay(500);
    claw.move(127);

    //LOADER1
    chassis.moveToPoint(-46, -48, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange =2});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    if (!resetPos("left")) return;
    if (!resetPos("front")) return;
    chassis.moveToPoint(-61, -60, 1000, {}, false);
    pros::delay(200);
    chassis.moveToPoint(-54,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToHeading(0, 750, {.maxSpeed = 80}, false);
//     // reset x using the left dist sensor: facing 0 the left side faces the left wall (valid until loader 3)
    chassis.moveToPoint(-48,-48,800);
    chassis.turnToHeading(0, 200, {.maxSpeed = 80}, false);
    pros::delay(200);
    if (!resetPos("left")) return;
    liftTarget = 65;
    pros::delay(500);
    chassis.moveToPoint(-48,-23,2000, {.maxSpeed = 50}, false);
    liftTarget=30;
    pros::delay(200);
    claw.move(127);
    pros::delay(500);
    

    //LOADER2
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 2});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    if (!resetPos("left")) return;
    if (!resetPos("front")) return;    
    chassis.moveToPoint(-61, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-54,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToHeading(0, 750, {.maxSpeed = 80}, false);
    // reset x using the left dist sensor: facing 0 the left side faces the left wall (valid until loader 3)
    chassis.moveToPoint(-48,-48,800,{},false);
    chassis.turnToHeading(0, 200, {.maxSpeed = 80}, false);
    pros::delay(200);
    if (!resetPos("left")) return;
    liftTarget = 80;
    pros::delay(300);
    chassis.moveToPoint(-48,-23,2200, {.maxSpeed = 45}, false);
    liftTarget = 50;
    pros::delay(200);
    claw.move(127);
    pros::delay(500);
    

    //LOADER4
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 3});
    pros::delay(200);
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    if (!resetPos("left")) return;
    if (!resetPos("front")) return;
    chassis.moveToPoint(-60, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-50,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    liftTarget = 30;
    chassis.turnToHeading(10,1000);
    chassis.moveToPoint(-24,-12,1500,{.maxSpeed=80,.earlyExitRange=7});
    chassis.moveToPoint(0,0,1700,{.maxSpeed=80},false);
    liftTarget = 0;
    pros::delay(1300);
    claw.move(127);
    liftTarget = 30;

    chassis.moveToPoint(-12, -12, 1000, {.forwards = false}, false);

    liftTarget = 0;
    // chassis.moveToPoint(-10,0,1000,{.forwards=false,.maxSpeed=80},false);
    // claw.move(-127);
    // chassis.moveToPoint(24,24,1300,{.maxSpeed=80},false);
}


