#include "main.h"

// inches from robot center to each side distance sensor, both measured.
static const float LEFT_SENSOR_OFFSET = 5.5;
static const float RIGHT_SENSOR_OFFSET = 5.5;

void three_pin_C() {
// robot starts facing east (90), so its right side faces the bottom wall
    float centerToWall = rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET;
    float startY = -71 + centerToWall;
    chassis.setPose(-60.5, startY, 90);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    liftPidOn = true; // lift already homed in initialize(); PID task drives it to liftTarget

    claw.move(-127);
    chassis.moveToPoint(-60,startY,300);
    chassis.turnToPoint(-46,-9.5, 750);
    liftTarget = 10;
    chassis.moveToPoint(-46,-9.5, 1000);
    chassis.turnToHeading(180,400,{},false);


    pros::delay(200);
    float xFromWall = -71 + (rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET);
    printf("wall reset: right dist %d mm -> x = %.2f (odom had %.2f)\n", (int)rightDistance.get(), xFromWall, chassis.getPose().x);
    chassis.setPose(xFromWall, chassis.getPose().y, chassis.getPose().theta);

    chassis.moveToPose(-47, -19.5, 180, 900, {.lead=0.3, .maxSpeed=50}, false);
    liftTarget = 0;
    pros::delay(200);
    claw.move(127);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    
    pros::delay(900);
    
    chassis.moveToPoint(-46,-12,750,{.forwards=false,.maxSpeed=80,.minSpeed=40},false);
    //pickup
    chassis.turnToPoint(-26,-19.5,500,{.maxSpeed=80},false);
    claw.move(-127);
    chassis.moveToPoint(-26,-19.5,1600,{.maxSpeed=60}, false);

    liftTarget = 20;
    // pros::delay(400);
    chassis.turnToHeading(250,500,{.maxSpeed=80},false);
    //drop off
    chassis.moveToPoint(-40,-24,1100,{},false);
    liftTarget = 0;
    // chassis.moveToPoint()
    pros::delay(450);
    claw.move(127);

    chassis.moveToPoint(-35,-22,700,{.forwards=false});
    chassis.moveToPoint(-30,-22,700,{.forwards=false});
    claw.move(-127);
    chassis.turnToHeading(220,500);
    chassis.moveToPose(-46,-48,210,2200,{.lead = 0.4,.maxSpeed=70,.minSpeed=50},false);
    liftTarget = 40;
    chassis.turnToPoint(-46,-24,800,{.maxSpeed=80},false);
    chassis.moveToPoint(-46,-24,1200,{.maxSpeed=60},false);
    liftTarget = 0;
    pros::delay(200);
    claw.move(127);
    chassis.moveToPoint(-46,-40,1200,{.forwards=false});
    chassis.turnToHeading(270,200);
}
