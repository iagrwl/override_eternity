#include "main.h"

void turnTesting(bool isCW) {
    console.clear();
    console.focus();

    int turns[] = {30, 60, 90, 120, 180};

    for (int i = 0; i < 5; i++) {

        int currTurn = isCW ? turns[i] : -turns[i];

        chassis.turnToHeading(currTurn, 2000, {}, false);
        pros::delay(1000);
        console.printf("%d: %.2f \n", currTurn, chassis.getPose().theta);

        chassis.turnToHeading(0, 2000);
    }
}
