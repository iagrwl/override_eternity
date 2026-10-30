#include "main.h"

void simpleSkills() {

    float centerToWall = leftDistance.get() / 25.4 + 5.5;
    float startY = -71 + centerToWall;
    chassis.setPose(-63, startY, -90);
    printf("left dist: %d mm (%.2f in)\n", (int)leftDistance.get(), leftDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    initLift();
    pros::delay(300);
    liftPos(650,20);
    pros::delay(300);

    chassis.moveToPoint(-60,startY,300,{.forwards=false,.maxSpeed=50});
    chassis.turnToPoint(-47,-9.5, 1000,{.forwards=false,.maxSpeed=50});
    chassis.moveToPoint(-47,-9.5, 1000,{.forwards=false,.maxSpeed=50});
    chassis.turnToHeading(0,300,{},false);

    pros::delay(200);
    float xFromWall = -71 + (leftDistance.get() / 25.4 + 5.5);
    printf("wall reset: left dist %d mm -> x = %.2f (odom had %.2f)\n", (int)leftDistance.get(), xFromWall, chassis.getPose().x);
    chassis.setPose(xFromWall, chassis.getPose().y, chassis.getPose().theta);

    chassis.moveToPose(-47, -19.5, 0, 1500, {.forwards=false, .lead=0.3, .maxSpeed=50}, false);
    liftPos(-200,60);
    printf("left dist: %d mm (%.2f in)\n", (int)leftDistance.get(), leftDistance.get() / 25.4);
    
    pros::delay(1100);
    claw.move(-127);
    chassis.moveToPoint(-46,-13,1000,{.minSpeed=60},false);
    
    // chassis.turnToPoint(-30,-21,500,{.maxSpeed=80},false);
    // chassis.moveToPoint(-31,-21,1500,{.maxSpeed=60});
    // pros::delay(400);
    // claw.move(-127);
    // pros::delay(800);
    // liftPos(800,80);
    // pros::delay(400);
    // chassis.turnToHeading(250,500,{.maxSpeed=80},false);
    // chassis.moveToPoint()
}
