#include "main.h"

void basicParth() {
    chassis.setPose(0, 0, 0);
    printPose("start");

    chassis.moveToPoint(0, 10, 3000, {}, false);
    printPose("moveToPoint(0,10)");

    chassis.turnToHeading(180, 2000, {}, false);
    printPose("turnToHeading(180)");

    chassis.moveToPoint(10, 0, 4000, {}, false);
    printPose("moveToPoint(10,0)");

    chassis.turnToHeading(-20, 2000, {}, false);
    printPose("turnToHeading(-20)");
}
