#include "main.h"

// turns in place in 90 degree steps and prints the pose after each one. the robot never
// drives, so any x/y the pose picks up is odom error from turning. the usual cause is a
// tracking wheel offset that doesn't match where the wheel really sits (setup.hpp).
// drift after a 90 deg turn is about 1.4x the offset error in inches. if the pose walks
// off during CW and comes back during CCW it's an offset; if it drifts the same way in
// both directions it's wheel slip or a skipping encoder.
void odomTurnDriftTest() {
    console.clear();
    console.focus();

    while (imu.is_calibrating()) pros::delay(20);
    chassis.setPose(0, 0, 0);
    pros::delay(300);

    // raw wheel travel and imu rotation at the start. a wheel on a spinning robot rolls
    // offset * angle, so after the CW lap  offset = -travel / angle  (lemlib's sign).
    float h0 = horizontalTrackingWheel.getDistanceTraveled();
    float v0 = verticalTrackingWheel.getDistanceTraveled();
    float rot0 = imu.get_rotation();

    const int steps[] = {90, 180, 270, 360};

    console.printf("odom turn drift: CW");
    for (int target : steps) {
        chassis.turnToHeading(target, 2000, {.direction = lemlib::AngularDirection::CW_CLOCKWISE}, false);
        pros::delay(400);
        lemlib::Pose p = chassis.getPose();
        console.printf("%3d deg: x %+5.2f  y %+5.2f  th %6.1f", target, p.x, p.y, p.theta);
        printf("CW,%d,%.2f,%.2f,%.2f\n", target, p.x, p.y, p.theta);
    }

    {
        float dTheta = (imu.get_rotation() - rot0) * M_PI / 180.0;
        float dH = horizontalTrackingWheel.getDistanceTraveled() - h0;
        float dV = verticalTrackingWheel.getDistanceTraveled() - v0;
        float offH = -dH / dTheta;
        float offV = -dV / dTheta;
        console.printf("measured offsets: horiz %+.2f (cfg %+.2f)  vert %+.2f (cfg %+.2f)",
                       offH, horizontalTrackingWheel.getOffset(), offV, verticalTrackingWheel.getOffset());
        printf("OFFSETS,horiz,%.2f,%.2f,vert,%.2f,%.2f\n",
               offH, horizontalTrackingWheel.getOffset(), offV, verticalTrackingWheel.getOffset());
    }

    console.printf("odom turn drift: CCW");
    for (int i = 3; i >= 0; i--) {
        int target = i == 0 ? 0 : steps[i - 1];
        chassis.turnToHeading(target, 2000, {.direction = lemlib::AngularDirection::CCW_COUNTERCLOCKWISE}, false);
        pros::delay(400);
        lemlib::Pose p = chassis.getPose();
        console.printf("%3d deg: x %+5.2f  y %+5.2f  th %6.1f", target, p.x, p.y, p.theta);
        printf("CCW,%d,%.2f,%.2f,%.2f\n", target, p.x, p.y, p.theta);
    }

    lemlib::Pose p = chassis.getPose();
    console.printf("done: net drift %.2f in", sqrtf(p.x * p.x + p.y * p.y));
}
