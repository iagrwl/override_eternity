#include "main.h"

void simpleRoute() {
    while (imu.is_calibrating()) pros::delay(20);
    chassis.setPose(0, 0, 0);
    pros::delay(300);

    chassis.moveToPoint(0, 6, 3000, {}, false);
    chassis.turnToHeading(90, 2000, {}, false);
    chassis.moveToPoint(4, 6, 3000, {}, false);

    lemlib::Pose end = chassis.getPose();
    std::cout << "end x," << end.x << ",y," << end.y << ",theta," << end.theta << std::endl;
    console.printf("x %.2f y %.2f h %.1f\n", end.x, end.y, end.theta);
}
