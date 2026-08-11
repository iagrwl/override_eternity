#include "main.h"

void turnTesting(bool isCW) {
    int turns[] = {30, 60, 90, 120, 180};

    for (int i = 0; i < 5; i++) {
        int currTurn = isCW ? turns[i] : -turns[i];

        chassis.turnToHeading(currTurn, 2000, {}, false);
        pros::delay(1000);
        pros::lcd::print(i + 2, "%d: %.2f", currTurn, chassis.getPose().theta);

        chassis.turnToHeading(0, 2000);
    }
}
