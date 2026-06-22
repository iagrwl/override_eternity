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

void autoCascade() {
    // 1. Update the target position
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
        if (currentCascadePosition < 3) currentCascadePosition++;
        targetPosition = cascadePositions[currentCascadePosition];
        printf("\n MOVING UP TO PRESET %d \n", currentCascadePosition);
    }
    else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
        if (currentCascadePosition > 0) currentCascadePosition--;
        targetPosition = cascadePositions[currentCascadePosition];
        printf("\n MOVING DOWN TO PRESET %d <\n", currentCascadePosition);
    }

    // 2. The Tuned PD Controller (P + D + Gravity Hold)
    double currentPos = cascade.get_position();
    double error = targetPosition - currentPos; 
    
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
    if (targetPosition > 0) {
        power += 30; 
    }

    // Cap the power so it never exceeds VEX limits
    if (power > 127) power = 127;
    if (power < -127) power = -127;

    // 3. Telemetry Print
    static int printTimer = 0;
    if (std::abs(error) >= 20) {
        if (printTimer % 10 == 0) { 
            printf("Target: %d | Actual: %.1f | Power: %.1f\n", targetPosition, currentPos, power);
        }
        printTimer++;
    }

    // 4. Motor Execution
    if (std::abs(error) < 20) {
        cascade.brake();
    } else {
        cascade.move(power); 
    }
}