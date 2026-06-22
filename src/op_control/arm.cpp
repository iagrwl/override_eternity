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

void autoArm() {
    // 1. Update the target position
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
        if (currentArmPosition < 3) currentArmPosition++;
        targetArmPosition = ArmPositions[currentArmPosition];
        printf("\n MOVING UP TO PRESET %d \n", currentArmPosition);
    }
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
        if (currentArmPosition > 0) currentArmPosition--;
        targetArmPosition = ArmPositions[currentArmPosition];
        printf("\n MOVING DOWN TO PRESET %d <\n", currentArmPosition);
    }

    // 2. The Tuned PD Controller (P + D + Gravity Hold)
    double currentPos = arm.get_position();
    double error = targetArmPosition - currentPos; 
    
    // Track how fast the error is changing for the D term
    static double prevError = 0;
    double derivative = error - prevError;
    prevError = error; // Save current error for the next loop

    // Dropped kP so it stops overshooting
    double kP = 0.20; 
    // kD acts as a brake to fight momentum and kill oscillations
    double kD = 0.80; 

    // Calculate base power
    double power = (error * kP) + (derivative * kD);

    // --- SMOOTH GRAVITY HOLD ---
    // Instead of violently switching based on error, just apply a constant 
    // gentle upward force to hold the weight ONLY if we aren't at the bottom.
    if (targetArmPosition > 0) {
        power += 30; 
    }

    // Cap the power so it never exceeds VEX limits
    if (power > 127) power = 127;
    if (power < -127) power = -127;

    // 3. Telemetry Print
    static int printTimer = 0;
    if (std::abs(error) >= 20) {
        if (printTimer % 10 == 0) { 
            printf("Target: %d | Actual: %.1f | Power: %.1f\n", targetArmPosition, currentPos, power);
        }
        printTimer++;
    }

    // 4. Motor Execution
    if (std::abs(error) < 20) {
        arm.brake();
    } else {
        arm.move(power); 
    }
}