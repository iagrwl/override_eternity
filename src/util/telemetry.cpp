#include "main.h"

void telemetry() {
    std::ofstream file("/usd/log.csv", std::ios::app);
    if (!file.is_open()) return;

    if (file.tellp() == 0)
        file << "time,x,y,theta\n";

    file << pros::millis() << ","
         << chassis.getPose().x << ","
         << chassis.getPose().y << ","
         << chassis.getPose().theta << "\n";
}

void printPose(const char *label) {
    lemlib::Pose p = chassis.getPose();
    printf("%-22s x: %7.2f   y: %7.2f   theta: %7.2f\n", label, p.x, p.y, p.theta);
}
