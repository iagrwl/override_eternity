#include "main.h"


constexpr int liftVelocity = 100; 


void initLift() {
    lift.set_brake_mode_all(pros::MotorBrake::hold);
    lift.move(-100); 
    pros::delay(200); 
    
    double lastPos = -999;
    while (true) {
        double currentPos = lift.get_position(); 
        if (std::abs(currentPos - lastPos) < 2.0) {
            break; 
        }
        lastPos = currentPos;
        pros::delay(50);
    }
    
    lift.brake();
    lift.tare_position();
}


void liftPos(double degree) {
    lift.move_absolute(degree, liftVelocity);
}


void liftControl() {
    static int currentPower = 0;
    const int rampStep = 10;
    const int minHoldPower = 40;
    const int slowDownPower = 40;

    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        currentPower = 127;
        lift.move(currentPower);
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        currentPower = -slowDownPower;
        lift.move(currentPower);
    }
    else if (currentPower > minHoldPower) {
        currentPower -= rampStep;
        lift.move(currentPower);
    }
    else {
        currentPower = 0;
        lift.brake();
    }
}