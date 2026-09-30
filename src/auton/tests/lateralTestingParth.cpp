#include "main.h"

void lateralSweepTest() {
    console.clear();
    console.focus();

    const double dists[] = {1, 2, 4, 8, 16};

    while (imu.is_calibrating()) pros::delay(20);
    chassis.setPose(0, 0, 0);
    pros::delay(300);

    std::cout << "dist_in,err_in,overshoot_in,settle_ms,dtheta_deg" << std::endl;

    for (double inches : dists) {
        lemlib::Pose start = chassis.getPose();
        double target = start.y - inches;
        double peak = start.y;

        auto sample = [&]() {
            double y = chassis.getPose().y;
            if (y < peak) peak = y;
        };

        uint32_t t0 = pros::millis();
        chassis.moveToPoint(start.x, target, 4000, {.forwards = false}, true);
        pros::delay(10);

        while (chassis.isInMotion()) {
            sample();
            pros::delay(10);
        }
        uint32_t settle = pros::millis() - t0;

        for (int i = 0; i < 25; i++) {
            sample();
            pros::delay(10);
        }

        lemlib::Pose end = chassis.getPose();

        std::cout << inches << "," << end.y - target << "," << target - peak << "," << settle << ","
                  << end.theta - start.theta << std::endl;
        console.printf("%.0f in: err %.2f\n", inches, end.y - target);

        chassis.moveToPoint(start.x, start.y, 4000, {}, false);
        pros::delay(150);
    }

    std::cout << "--- done ---" << std::endl;
    console.printf("DONE\n");
}
