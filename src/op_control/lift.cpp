#include "main.h"
#include "eternity_template/op_control/lift.hpp"

int cascadePositions[] = {0, 550, 5800, 9650};
int currentCascadePosition = 0; 
int targetPosition = 0;

void manualCascade() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        cascade.move(127); 
    } 
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        cascade.move(-127); 
    } 
    else {
        cascade.brake();    
        printf("Cascade Position: %.1f\n", cascade.get_position());
    }
}

//usage: 
// moves to preset 2 autoCascade(2); 
// moves to set degree autoCascade(-1, 4000); 

void autoCascade(int presetIndex, int customDegree) {
    if (customDegree != -1) {
        targetPosition = customDegree;
    } 
    else if (presetIndex >= 0 && presetIndex <= 3) {
        currentCascadePosition = presetIndex;
        targetPosition = cascadePositions[currentCascadePosition];
    } 
    else {

        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
            if (currentCascadePosition < 3) currentCascadePosition++;
            targetPosition = cascadePositions[currentCascadePosition];
            printf("\n moving to %d \n", currentCascadePosition);
        }
        else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
            if (currentCascadePosition > 0) currentCascadePosition--;
            targetPosition = cascadePositions[currentCascadePosition];
            printf("\n moving to %d <\n", currentCascadePosition);
        }
    }

    double currentPos = cascade.get_position();
    double error = targetPosition - currentPos; 
    
    static double prevError = 0;
    double derivative = error - prevError;
    prevError = error; 

    double kP = 0.20; 
    double kD = 0.80; 
    double power = (error * kP) + (derivative * kD);

    if (targetPosition > 0) {
        power += 30; 
    }
    if (power > 127) power = 127;
    if (power < -127) power = -127;

    static int printTimer = 0;
    if (std::abs(error) >= 20) {
        if (printTimer % 10 == 0) { 
            printf("target: %d  actual: %.1f  power: %.1f\n", targetPosition, currentPos, power);
        }
        printTimer++;
    }

    if (std::abs(error) < 20) {
        cascade.brake();
    } else {
        cascade.move(power); 
    }
}