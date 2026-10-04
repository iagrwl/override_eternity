#include "main.h"

// turns 30/60/90/120/180 degrees in one direction, measures how close each one lands,
// and turns back to 0 between them. results show on the brain's LOG tab.
void turnTesting(bool isCW) {
    console.clear();
    console.focus();

    while (imu.is_calibrating()) pros::delay(20);
    chassis.setPose(0, 0, 0); // every turn is measured from a known 0
    pros::delay(300);

    const int turns[] = {30, 60, 90, 120, 180};
    // force the direction, otherwise 180 can go either way (shortest path)
    lemlib::AngularDirection dir =
        isCW ? lemlib::AngularDirection::CW_CLOCKWISE : lemlib::AngularDirection::CCW_COUNTERCLOCKWISE;
    lemlib::AngularDirection back =
        isCW ? lemlib::AngularDirection::CCW_COUNTERCLOCKWISE : lemlib::AngularDirection::CW_CLOCKWISE;

    console.printf("%s turn test", isCW ? "CW" : "CCW");
    for (int turn : turns) {
        float target = isCW ? turn : -turn;

        uint32_t t0 = pros::millis();
        chassis.turnToHeading(target, 2000, {.direction = dir}, false); // wait until done
        uint32_t took = pros::millis() - t0;
        pros::delay(300); // let it settle before measuring

        float actual = chassis.getPose().theta;
        console.printf("%4.0f deg: got %6.1f  err %+5.1f  %lums", target, actual, actual - target, took);
        std::cout << target << "," << actual << "," << actual - target << "," << took << std::endl;

        chassis.turnToHeading(0, 2000, {.direction = back}, false); // back to 0, and wait for it
        pros::delay(300);
    }
    console.printf("done");
}
