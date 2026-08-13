#include "main.h"

double flipperLoad = 0;
double flipperPara = 1060; 
double flipperUp = 1600;   

constexpr int flipperVelocity = 200; 
constexpr int scoreReverseMs = 500; 


void initFlipper() {
    intakePivot.set_brake_mode(pros::MotorBrake::hold);
    
    intakePivot.move(-100); 
    pros::delay(1000);      
    intakePivot.brake();
    intakePivot.tare_position(); 
    
    intakePivot.move(100); 
    pros::delay(300);      
    
    double lastPos = -999;
    while (true) {
        double currentPos = intakePivot.get_position();
        if (std::abs(currentPos - lastPos) < 2.0) {
            break; 
        }
        lastPos = currentPos;
        pros::delay(50);
    }
    
    intakePivot.brake();
    
    flipperUp = intakePivot.get_position(); 
    flipperPara = flipperUp * 0.6625; 
    
    intakePivot.move_absolute(flipperLoad, flipperVelocity); 
}

void flipperPos(double degree) {
    intakePivot.move_absolute(degree, flipperVelocity);
}

void flipperButtons() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L1)) {
        flipperPos(flipperPara);
    } else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
        flipperPos(flipperUp);
    } else if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)) {
        flipperPos(flipperLoad);
        liftPos(0); 
        setIntakeState(IntakeState::IN);
    }
}

void scoringMacro() {
    static bool isScoring = false;
    static uint32_t scoreStartTime = 0;


    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_L2)) {
        isScoring = true;
        scoreStartTime = pros::millis();
        setIntakeState(IntakeState::OUT);
    }

    
    if (isScoring) {
    
        if (pros::millis() - scoreStartTime >= scoreReverseMs) {
            setIntakeState(IntakeState::OFF);
            flipperPos(flipperUp);
            isScoring = false; // Macro finished
        }
    }
}