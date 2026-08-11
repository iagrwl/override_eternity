#include "main.h"

void driveTesting(bool isForward) {
    int dist[] = {12, 24, 48};

    for (int i = 0; i < 3; i++) {
        int currDist = isForward ? dist[i] : -dist[i];

        chassis.moveToPoint(0, currDist, 3000, {}, false);
        pros::delay(500);
        pros::lcd::print(i + 4, "%d: %.2f", currDist, chassis.getPose().y);

        chassis.moveToPoint(0, 0, 3000, {.forwards = false});
    }
}
