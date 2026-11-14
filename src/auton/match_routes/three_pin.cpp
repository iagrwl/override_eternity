#include "main.h"

// inches from robot center to each side distance sensor, both measured.
static const float LEFT_SENSOR_OFFSET = 5.5;
static const float RIGHT_SENSOR_OFFSET = 5.5;

void three_pin_C() {
// robot starts facing east (90), so its right side faces the bottom wall
    float centerToWall = rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET;
    float startY = -71 + centerToWall;
    chassis.setPose(-60.5, startY, 90);
    toggleMotor.move(-127);
    pros::delay(500);
    toggleMotor.move(0);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    liftPidOn = true; // lift already homed in initialize(); PID task drives it to liftTarget
    
    claw.move(-127);
    // chassis.moveToPoint(-60,startY,300);
    // chassis.turnToPoint(-46,-9.5, 750);
    liftTarget = 10;
    // chassis.moveToPoint(-46,-9.5, 1000);
    // chassis.turnToHeading(180,400,{},false);

    chassis.moveToPose(-48, -22, 180, 1500, {.lead = 0.4, .minSpeed = 60}, false);
    chassis.turnToHeading(-180, 300, {}, false);

    pros::delay(200);
    float xFromWall = -71 + (rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET);
    printf("wall reset: right dist %d mm -> x = %.2f (odom had %.2f)\n", (int)rightDistance.get(), xFromWall, chassis.getPose().x);
    chassis.setPose(xFromWall, chassis.getPose().y, chassis.getPose().theta);

    // chassis.moveToPose(-47, -19.5, 180, 900, {.lead=0.3, .maxSpeed=50}, false);
    liftTarget = 0;
    pros::delay(300);
    claw.move(127);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    
    pros::delay(500);
    
    chassis.moveToPoint(-46,-8, 750,{.forwards=false,.maxSpeed=80,.minSpeed=40},false);
    // pickup
    chassis.turnToPoint(-26,-19.5,500,{.maxSpeed=80},false);
    claw.move(-127);
    chassis.moveToPoint(-26,-19.5,1600,{.maxSpeed=60}, false);

    liftTarget = 20;
    // pros::delay(400);
    chassis.turnToHeading(240,500,{.maxSpeed=80},false);
    //drop off
    chassis.moveToPoint(-40,-24,1100,{},false);
    liftTarget = 0;
    // chassis.moveToPoint()
    pros::delay(450);
    claw.move(127);

    chassis.moveToPoint(-25,-22,1000,{.forwards=false}, false);
    claw.move(-127);
    chassis.turnToHeading(215,500);
    chassis.moveToPose(-46,-50,215,2200,{.lead = 0.4,.maxSpeed=65,.minSpeed=50},false);
    liftTarget = 40;

    chassis.turnToPoint(-46,-24,800,{.maxSpeed=80},false);
    chassis.moveToPoint(-46,-24,1200,{.maxSpeed = 60},false);
    liftTarget = 30;
    pros::delay(200);
    claw.move(127);
    chassis.moveToPoint(-46,-50,1200,{.forwards=false, .minSpeed = 40, .earlyExitRange = 5});
    liftTarget = 0;
    chassis.turnToHeading(270,750);
}

void three_pin_F() {
// robot starts facing east (90), so its right side faces the bottom wall
    
    float centerToWall = leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET;
    float startX = -67.25 + centerToWall;
    chassis.setPose(startX, -60.5, 0);
    toggleMotor.move(-127);
    pros::delay(350);
    toggleMotor.move(0);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    liftPidOn = true; // lift already homed in initialize(); PID task drives it to liftTarget
    
    claw.move(-127);
    // chassis.moveToPoint(-60,startY,300);
    // chassis.turnToPoint(-46,-9.5, 750);
    liftTarget = 13;
    // chassis.moveToPoint(-46,-9.5, 1000);
    // chassis.turnToHeading(180,400,{},false);

    chassis.moveToPose(-22, -50, 270, 1500, {.lead = 0.4, .minSpeed = 60}, false);
    chassis.moveToPoint(-28, -50, 800);

    // pros::delay(200);
    float yFromWall = -71 + (leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET);
    // printf("wall reset: right dist %d mm -> x = %.2f (odom had %.2f)\n", (int)leftDistance.get(), yFromWall, chassis.getPose().y);
    double angleFromWall = 1.5 * M_PI - chassis.getPose(true).theta;
    chassis.setPose(chassis.getPose().x, yFromWall * cos(angleFromWall), chassis.getPose().theta);

    // chassis.moveToPose(-47, -19.5, 180, 900, {.lead=0.3, .maxSpeed=50}, false);
    liftTarget = 0;
    pros::delay(250);
    claw.move(127);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    
    pros::delay(500);
    
    chassis.moveToPoint(-8,-48, 750,{.forwards=false,.maxSpeed=80,.minSpeed=40},false);
    chassis.turnToPoint(-19, -60, 750);
    claw.move(-127);
    chassis.moveToPoint(-19, -60, 1200, {.maxSpeed = 50, .minSpeed = 30, .earlyExitRange = 3}, false);
    chassis.swingToHeading(-180, lemlib::DriveSide::LEFT, 750, {.minSpeed = 30});
    chassis.swingToHeading(-100, lemlib::DriveSide::LEFT, 750);
    pros::delay(300);
    liftTarget = 25;

    chassis.moveToPoint(-5, -48, 1500, {.forwards = false, .minSpeed = 30, .earlyExitRange = 3});
    chassis.turnToPoint(-24, -48, 750, {.minSpeed = 20, .earlyExitRange = 20});
    chassis.moveToPoint(-24, -48, 1000, {.maxSpeed = 60}, false);
    liftTarget = 5;
    pros::delay(150);
    claw.move(127);
    pros::delay(100);

    chassis.moveToPoint(4, -50, 1200, {.forwards = false});
    chassis.turnToPoint(19, -62, 750);
    liftTarget = 0;
    claw.move(-127);
    chassis.moveToPoint(20, -60, 1200, {.maxSpeed = 50, .minSpeed = 30, .earlyExitRange = 3}, false);
    chassis.swingToHeading(180, lemlib::DriveSide::RIGHT, 750, {.minSpeed = 30});
    chassis.swingToHeading(100, lemlib::DriveSide::RIGHT, 750, {.minSpeed = 30, .earlyExitRange = 20});
    pros::delay(100);
    liftTarget = 30;
    chassis.moveToPoint(5, -48, 1500, {.forwards = false, .minSpeed = 40, .earlyExitRange = 3});
    chassis.turnToPoint(24, -48, 750, {.minSpeed = 20, .earlyExitRange = 20});
    chassis.moveToPoint(26, -48, 1000, {.maxSpeed = 60}, false);
    liftTarget = 10;
    pros::delay(100);
    claw.move(127);
    pros::delay(100);
    
}