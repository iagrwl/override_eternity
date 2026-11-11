#include "main.h"

// inches from robot center to each side distance sensor, measured.
static const float LEFT_SENSOR_OFFSET = 5.5;
static const float RIGHT_SENSOR_OFFSET = 5.5;

void simpleSkills() {
    // ---------------------------------------------------------------- setup
    // robot starts facing east (90), so its right side faces the bottom wall
    float centerToWall = rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET;
    float startY = -71 + centerToWall;
    chassis.setPose(-60.5, startY, 90);
    printf("right dist: %d mm (%.2f in)\n", (int)rightDistance.get(), rightDistance.get() / 25.4);
    printf("starting pose: x = %.2f, y = %.2f, theta = %.2f\n", chassis.getPose().x, chassis.getPose().y, chassis.getPose().theta);
    chassis.setBrakeMode(pros::E_MOTOR_BRAKE_HOLD);
    liftPidOn = true; // lift already homed in initialize(); PID task drives it to liftTarget

    // ---------------------------------------------------------------- pin 1 entry
    claw.move(-127);
    
    liftTarget = 10;
    chassis.moveToPose(-49.5, -24, 180, 1500, {.lead = 0.5, .minSpeed = 55}, false);
    chassis.turnToHeading(-180, 300, {}, false);

    
    pros::delay(200);
    float xFromWall = -71 + (rightDistance.get() / 25.4 + RIGHT_SENSOR_OFFSET);
    printf("wall reset: right dist %d mm -> x = %.2f (odom had %.2f)\n", (int)rightDistance.get(), xFromWall, chassis.getPose().x);
    chassis.setPose(xFromWall, chassis.getPose().y, chassis.getPose().theta);

    // ---------------------------------------------------------------- pin 1 drop
    liftTarget = 0;
    pros::delay(300);
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
    chassis.turnToHeading(265,500);
    chassis.moveToPoint(-40, -24, 1100, {}, false);
    
    liftTarget = 0;
    pros::delay(450);
    claw.move(127);

    // ---------------------------------------------------------------- pin 3 pickup
    chassis.moveToPoint(-25,-22,1000,{.forwards=false}, false);
    claw.move(-127);
    chassis.turnToHeading(215,500);
    chassis.moveToPose(-46,-50,215,2200,{.lead = 0.4,.maxSpeed=65,.minSpeed=50},false);
    liftTarget = 40;

    // ---------------------------------------------------------------- pin 3 drop
    chassis.turnToPoint(-47, -22, 800, {.maxSpeed = 80}, false);
    chassis.moveToPoint(-47, -22, 1200, {.maxSpeed = 60}, false);
    pros::delay(200);
    liftTarget = 30;
    pros::delay(500);
    claw.move(127);

    //LOADER1
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 5});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    float yFromWall = -71 + (leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET);
    printf("wall reset: left dist %d mm -> y = %.2f (odom had %.2f)\n", (int)leftDistance.get(), yFromWall, chassis.getPose().y);
    chassis.setPose(chassis.getPose().x, yFromWall, chassis.getPose().theta);

    chassis.moveToPoint(-59, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-47,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToHeading(0, 750, {.maxSpeed = 80}, false);
    liftTarget = 70;
    pros::delay(500);
    chassis.moveToPoint(-48,-23,2000, {.maxSpeed = 50}, false);
    liftTarget=50;
    pros::delay(200);
    claw.move(127);
    pros::delay(500);
    

    //LOADER2
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 5});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    float yFromWall2 = -71 + (leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET);
    printf("wall reset: left dist %d mm -> y = %.2f (odom had %.2f)\n", (int)leftDistance.get(), yFromWall2, chassis.getPose().y);
    chassis.setPose(chassis.getPose().x, yFromWall2, chassis.getPose().theta);

    chassis.moveToPoint(-59, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-47,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToHeading(0, 750, {.maxSpeed = 80}, false);
    liftTarget = 80;
    pros::delay(500);
    chassis.moveToPoint(-48,-23,2000, {.maxSpeed = 50}, false);
    liftTarget = 60;
    pros::delay(200);
    claw.move(127);
    pros::delay(500);


    //LOADER3
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 5});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    float yFromWall3 = -71 + (leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET);
    printf("wall reset: left dist %d mm -> y = %.2f (odom had %.2f)\n", (int)leftDistance.get(), yFromWall3, chassis.getPose().y);
    chassis.setPose(chassis.getPose().x, yFromWall3, chassis.getPose().theta);

    chassis.moveToPoint(-59, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-47,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    chassis.turnToHeading(0, 750, {.maxSpeed = 80}, false);
    liftTarget = 120;
    pros::delay(500);
    chassis.moveToPoint(-48,-23,2000, {.maxSpeed = 50}, false);
    liftToAngle(80, 100);  
    pros::delay(500);
    claw.move(127);
    pros::delay(750);
    

    //LOADER4
    chassis.moveToPoint(-46, -50, 1200, {.forwards = false, .minSpeed = 40, .earlyExitRange = 5});
    liftTarget = 0;
    chassis.turnToHeading(270, 750,{},false);
    claw.move(-127);

    pros::delay(200);
    float yFromWall4 = -71 + (leftDistance.get() / 25.4 + LEFT_SENSOR_OFFSET);
    printf("wall reset: left dist %d mm -> y = %.2f (odom had %.2f)\n", (int)leftDistance.get(), yFromWall4, chassis.getPose().y);
    chassis.setPose(chassis.getPose().x, yFromWall4, chassis.getPose().theta);

    chassis.moveToPoint(-59, -60, 1000, {}, false);
    pros::delay(500);
    chassis.moveToPoint(-48,-60,750, {.forwards = false, .maxSpeed = 80, .minSpeed = 40}, false);
    liftTarget = 30;
    chassis.turnToPoint(0,10,500);
    chassis.moveToPoint(0,0,4000,{.maxSpeed=70},false);
    liftTarget = 0;
    pros::delay(500);
    claw.move(127);
    chassis.moveToPoint(-10,0,1000,{.forwards=false,.maxSpeed=80},false);

}


