#pragma once

// =====================================================================================
//  DEVICES SHOWN ON THE BRAIN (MOTORS tab, SENSORS port map, warnings)
//
//  Ports are NOT set here - they come from include/eternity_template/setup.hpp.
//  Change a port in setup.hpp and the brain + the laptop sim both pick it up.
//
//  Only edit this file when you ADD or REMOVE a motor/sensor in setup.hpp:
//    UI_MOTOR("name on screen", variable_in_setup_hpp, which_motor_in_group)
//        - for a MotorGroup, use 0, 1, 2... for each motor in it
//        - for a single pros::Motor, use 0
//        - keep the drivetrain motors FIRST (the "drive temp" readouts use the first 6)
//    UI_SENSOR("name on screen", variable_in_setup_hpp, imu / rotation / distance / optical)
// =====================================================================================

#define UI_MOTORS(UI_MOTOR)                \
    UI_MOTOR("LEFT 1", left_dt, 0)         \
    UI_MOTOR("LEFT 2", left_dt, 1)         \
    UI_MOTOR("LEFT 3", left_dt, 2)         \
    UI_MOTOR("RIGHT 1", right_dt, 0)       \
    UI_MOTOR("RIGHT 2", right_dt, 1)       \
    UI_MOTOR("RIGHT 3", right_dt, 2)       \
    UI_MOTOR("LIFT A", lift, 0)            \
    UI_MOTOR("LIFT B", lift, 1)            \
    UI_MOTOR("INTAKE", intake, 0)          \
    UI_MOTOR("CLAW", claw, 0)

#define UI_SENSORS(UI_SENSOR)                       \
    UI_SENSOR("IMU", imu, imu)                      \
    UI_SENSOR("Horiz wheel", horizontalEnc, rotation) \
    UI_SENSOR("Vert wheel", verticalEnc, rotation)  \
    UI_SENSOR("Left dist", leftDistance, distance)  \
    UI_SENSOR("Right dist", rightDistance, distance)
