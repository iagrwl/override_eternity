#include "main.h"

void simpleSkills() {
    // absolute field coords (origin = field center). robot starts facing the left wall;
    // x is 63 in from center; y comes from the left distance sensor (faces the bottom wall),
    // reading minus the 5.5 in sensor offset = center to wall (60.5 in at the usual start)
    float centerToWall = leftDistance.get() / 25.4 - 5.5;
    chassis.setPose(-63, -71 + centerToWall, -90);
    printf("left dist: %d mm (%.2f in)\n", (int)leftDistance.get(), leftDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    initLift();
    pros::delay(300);
    liftPos(650,20);
    pros::delay(300);
    chassis.moveToPoint(-60,-10.5,300,{.forwards=false,.maxSpeed=50});
    
    chassis.turnToPoint(-51,-20.5, 1000,{.forwards=false,.maxSpeed=50});
    chassis.moveToPoint(-51,-20.5, 1000,{.forwards=false,.maxSpeed=50});
    chassis.turnToHeading(0,300,{},false);
    printf("left dist: %d mm (%.2f in)\n", (int)leftDistance.get(), leftDistance.get() / 25.4);

        // reset x based on left distance: expected 470 mm
    // reading > 470 -> farther from the left wall -> x more positive
    // reading < 470 -> closer to the left wall   -> x more negative
    float xError = (leftDistance.get() - 470) / 25.4;
    printf("xError: %.2f\n", xError);
    chassis.setPose(chassis.getPose().x + xError, chassis.getPose().y, chassis.getPose().theta);

    chassis.moveToPose(-51, -30.5, 0, 1500, {.forwards=false, .lead=0.3, .maxSpeed=50}, false);
    
    liftPos(0,60);
    printf("left dist: %d mm (%.2f in)\n", (int)leftDistance.get(), leftDistance.get() / 25.4);
    pros::delay(1100);
    claw.move(-127);
    chassis.moveToPoint(-50,-24,1000,{.minSpeed=60},false);
    
    chassis.turnToPoint(-34,-32,500,{.maxSpeed=80},false);
    chassis.moveToPoint(-35,-32,1500,{.maxSpeed=60});
    pros::delay(400);
    claw.move(-127);
    pros::delay(800);
    liftPos(800,80);
    pros::delay(400);
    chassis.turnToHeading(250,500,{.maxSpeed=80},false);
    // chassis.moveToPoint()
}
