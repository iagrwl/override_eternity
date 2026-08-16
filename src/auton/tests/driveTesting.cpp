#include "main.h"

void driveTesting(bool isForward) {
    console.clear();
    console.focus();

    int dist[] = {12, 24, 48};

    for (int i = 0; i < 3; i++) {
        int currDist = isForward ? dist[i] : -dist[i];

        chassis.moveToPoint(0, currDist, 3000, {}, false);
        pros::delay(500);
        console.printf("%d: %.2f \n", currDist, chassis.getPose().y);

        chassis.moveToPoint(0, 0, 3000, {.forwards = false});
    }
}
