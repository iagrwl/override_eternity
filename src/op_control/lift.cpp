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

}

std::atomic<double> liftTarget{0};
std::atomic<bool> liftPidOn{true};

double liftAngle() {
    return liftRot.get_position() / 100.0;
}

void liftPID() {
    const double kP = 60.0;
    const double kI = 0.0;
    const double kD = 0.0;
    const double kG = 0.0;
    const double iMax = 3000;

    double integral = 0, prevError = 0;
    uint32_t now = pros::millis();

    while (true) {
        if (liftPidOn) {
            double error = liftTarget - liftAngle();

            integral += error * 0.01;
            if (integral > iMax / (kI + 1e-9)) integral = iMax / (kI + 1e-9);
            if (integral < -iMax / (kI + 1e-9)) integral = -iMax / (kI + 1e-9);
            if (fabs(error) < 1.0) integral = 0;       // reset near target

            double derivative = (error - prevError) / 0.01;
            prevError = error;

            double out = kP * error + kI * integral + kD * derivative + kG;
            out = std::clamp(out, -12000.0, 12000.0);
            lift.move_voltage(out);     // used move_voltage because it has more 
                                        // steps than just move, resulting in a
                                        // smoother pid.
                                        // (-12000 -> 12000 vs -127 -> 127)
        }
        pros::Task::delay_until(&now, 10);
    }

    
}
void liftPos(double degree, int speed) {
    lift.move_absolute(degree, speed);
}

static bool stopping = false;

void liftControl() {
    if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        lift.move(liftSpeed);
        stopping = true;
    }
    else if (controller.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        lift.move(-liftSpeed);
        stopping = true;
    }
    else if (stopping) {
        // let go: actively fight the leftover momentum, then hold once it's stopped
        lift.move_velocity(0);
        if (std::abs(lift.get_actual_velocity()) < 5) {
            lift.brake();
            stopping = false;
        }
    }
}