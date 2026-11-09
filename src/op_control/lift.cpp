#include "main.h"


constexpr int liftSpeed = 127;    // power for manual control with R1/R2 (0-127)

void initLift() {
    lift.set_brake_mode_all(pros::MotorBrake::hold);

    // COMMENTED BECAUSE LIFT IS SKIPPING RIGHT NOW
    lift.move(-100); 
    pros::delay(200); 
    
    double lastPos = -999;
    uint32_t startTime = pros::millis();
    const uint32_t homingTimeoutMs = 1000;
    bool stalled = false;
    while (pros::millis() - startTime < homingTimeoutMs) {
        double currentPos = lift.get_position();
        if (std::abs(currentPos - lastPos) < 2.0) {
            stalled = true;
            break;
        }
        lastPos = currentPos;
        pros::delay(50);
    }

    lift.brake();
    lift.tare_position_all(); // zero BOTH lift motors (tare_position() only does the first one)
    liftRot.reset_position();

}
double liftAngle() {
    return liftRot.get_position() / 100.0;
}

// lift.cpp
std::atomic<double> liftTarget{0};
std::atomic<bool> liftPidOn{true};
std::atomic<double> liftKP{500.0};
std::atomic<double> liftKD{40.0};

void liftPID() {
    const double kI = 0.0, kG = 2300.0, iMax = 1500.0;

    double integral = 0, prevError = 0;
    uint32_t now = pros::millis();


    while (true) {
        if (liftPidOn) {


            double error = liftTarget - liftAngle();
            double derivative = (error - prevError) / 0.01;
            prevError = error;

            if (fabs(error) < 10.0) {                    // only integrate when close
                integral += error * 0.01;
            } else {
                integral = 0;                            // reset during big moves
            }
            double iTerm = std::clamp(kI * integral, -iMax, iMax);

            double feedForward = (liftTarget < 5.0 && liftAngle() < 10.0) ? 0.0 : kG;

            double out = liftKP * error + iTerm + liftKD * derivative + feedForward;
            out = std::clamp(out, -12000.0, 12000.0);
            lift.move_voltage(out);
        }
        pros::Task::delay_until(&now, 10);
    }
}

void liftPos(double degree, int speed) {
    lift.move_absolute(degree, speed);
}

static bool stopping = false;
double liftHeights[] = {30, 47.5, 65, 82.5, 100};
int currLiftState = 0;

void liftControl() {
    if (controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_R1)) {
        currLiftState++;
        if (currLiftState = 5) currLiftState = 0;

        liftTarget = liftHeights[currLiftState];
        liftPidOn = true;
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        liftPidOn = false;
        lift.move(-liftSpeed);
        stopping = true;
    }
    else if (stopping) {
        // let go: actively fight the leftover momentum, then hold once it's stopped
        liftTarget  = liftRot.get_angle();
        liftPidOn = true;
    }
}
