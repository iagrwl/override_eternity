#include "main.h"
#include "eternity_template/op_control/lift.hpp"

int ArmPositions[] = {0, 550, 5800, 9650};
int currentArmPosition = 0; 
int targetArmPosition = 0;

void manualArm() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        arm.move(127); 
    } 
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        arm.move(-127); 
    } 
    else {
        arm.brake();    
        printf("Arm Position: %.1f\n", arm.get_position());
    }
}

//usage: 
// moves to preset 2 autoArm(2); 
// moves to set degree autoArm(-1, 4000); 

void autoArm(int presetIndex = -1, int customDegree = -1) {
    if (customDegree != -1) {
        targetArmPosition = customDegree;
    } 
    else if (presetIndex >= 0 && presetIndex <= 3) {
        currentArmPosition = presetIndex;
        targetArmPosition = ArmPositions[currentArmPosition];
    } 
    else {

        if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
            if (currentArmPosition < 3) currentArmPosition++;
            targetArmPosition = ArmPositions[currentArmPosition];
            printf("\n movin to %d \n", currentArmPosition);
        }
        else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
            if (currentArmPosition > 0) currentArmPosition--;
            targetArmPosition = ArmPositions[currentArmPosition];
            printf("\n moving to %d <\n", currentArmPosition);
        }
    }


    double currentPos = arm.get_position();
    double error = targetArmPosition - currentPos; 
    
    static double prevError = 0;
    double derivative = error - prevError;
    prevError = error; 

    double kP = 0.20; 
    double kD = 0.80; 
    double power = (error * kP) + (derivative * kD);

    if (targetArmPosition > 0) {
        power += 30; 
    }
    if (power > 127) power = 127;
    if (power < -127) power = -127;

    static int printTimer = 0;
    if (std::abs(error) >= 20) {
        if (printTimer % 10 == 0) { 
            printf("target: %d  actual: %.1f  power: %.1f\n", targetArmPosition, currentPos, power);
        }
        printTimer++;
    }

    if (std::abs(error) < 20) {
        arm.brake();
    } else {
        arm.move(power); 
    }
}