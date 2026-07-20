#include "main.h"
#include "eternity_template/op_control/lift.hpp"

int cascadePositions[] = {0, 550, 5800, 9650};
int currentCascadePosition = 0; 
int targetPosition = 0;

int liftPos[] = {0, 8400};
int targetPosIndex = 0;
int liftTarget = 0;

bool isClawOpen = true;
int clawTimer = 0;

lemlib::PID liftController(0.08,
                    0,
                    0.05,
                    0,
                    true   
                    );

void liftPID(void*) {
    while (true) {
        int current = liftRotation.get_position();
        int error = liftTarget - current;
        double output = liftController.update(error);

        console.printf("Pos: %d, Err: %d, Out: %d\n", current, error, output);


        lift.move(output);

        pros::delay(20);
    }
}
void liftMacro() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        targetPosIndex = targetPosIndex == 0 ? 1 : 0;
        liftTarget = liftPos[targetPosIndex];

        liftController.reset();

    }

    
}

void manualCascade() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        lift.move(127); 
    } 
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        lift.move(-127); 
    } 
    else {
        lift.brake();    
        printf("Cascade Position: %.1f\n", lift.get_position());
    }
}

void toggleClaw() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        int i = isClawOpen ? 1 : -1;
        isClawOpen = !isClawOpen;

        clawMotor.move(127 * i);
        clawTimer = 0;
    }
    clawTimer += 20;

    if (clawTimer > 750 && clawMotor.get_actual_velocity() > 10) {
        clawMotor.move(0);
    }
}

void manualIntake() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_UP)) {
        firstStage.move(127);
        secondStage.move(-127);
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT)) {
        firstStage.move(0);
        secondStage.move(0);
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

    double currentPos = lift.get_position();
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
        lift.brake();
    } else {
        lift.move(power); 
    }
}