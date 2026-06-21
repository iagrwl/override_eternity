#include "lemlib/chassis/chassis.hpp"
#include "main.h"
#include "pros/distance.hpp"
#include "pros/llemu.hpp"

void turnTesting(bool isCW) {

    const int SIZE = 8;
    //int turns[SIZE] = {15, 30, 45, 60, 90, 120, 150, 180};
    int turns[SIZE] = { 30,  60, 90, 120, 180};
    int actualTurn[SIZE] = {};
    int currTurn;

    for (int i = 0; i < SIZE; i++) {
        isCW ? currTurn = turns[i] : currTurn = -turns[i];

        chassis.turnToHeading(currTurn,2000, {}, false);

        pros::delay(1000);

        pros::lcd::print(i+2, "%d: %.2f", currTurn, chassis.getPose().theta);

        chassis.turnToHeading(0, 2000);



    }
}