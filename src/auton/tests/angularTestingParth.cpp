#include "main.h"

static double wrap180(double deg) {
    while (deg > 180) deg -= 360;
    while (deg <= -180) deg += 360;
    return deg;
}

void angularSweepTest() {
    console.clear();
    console.focus();

    const double angles[] = {30, 45, 60, 90, 120, 180};

    while (imu.is_calibrating()) pros::delay(20);
    chassis.setPose(0, 0, 0);
    pros::delay(300);

    std::cout << "angle_deg,err_deg,overshoot_deg,settle_ms,dx_in,dy_in,offset_est_in" << std::endl;

    for (double angle : angles) {
        lemlib::Pose start = chassis.getPose();
        double prev = start.theta;
        double turned = 0;
        double peak = 0;

        auto sample = [&]() {
            double cur = chassis.getPose().theta;
            turned += wrap180(cur - prev);
            prev = cur;
            if (turned > peak) peak = turned;
        };

        uint32_t t0 = pros::millis();
        chassis.turnToHeading(start.theta + angle, 2000,
                              {.direction = lemlib::AngularDirection::CW_CLOCKWISE}, true);
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
        double dx = end.x - start.x;
        double dy = end.y - start.y;
        double offset = std::hypot(dx, dy) / (2 * std::sin(angle * M_PI / 360));

        std::cout << angle << "," << angle - turned << "," << peak - angle << "," << settle << ","
                  << dx << "," << dy << "," << offset << std::endl;
        console.printf("%.0f deg: err %.2f\n", angle, angle - turned);

        pros::delay(150);
    }

    std::cout << "--- done ---" << std::endl;
    console.printf("DONE\n");
}
