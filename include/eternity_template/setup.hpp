#pragma once

#include "api.h"
#include "lemlib/api.hpp"
#include "robodash/api.h"

extern rd::Console console;
extern rd::Selector selector;

// controller
inline pros::Controller controller(pros::E_CONTROLLER_MASTER);

// drivetrain (negative port = reversed)
inline pros::MotorGroup left_dt({18, -6,10}, pros::MotorGearset::blue);
inline pros::MotorGroup right_dt({3, 5,-2}, pros::MotorGearset::blue);

inline lemlib::Drivetrain drivetrain(&left_dt,
                                     &right_dt,
                                     9.75,    // track width
                                     2.75,  // wheel diameter
                                     450,   // rpm
                                     0);

// odom sensors

inline pros::Imu imu(14); 

inline pros::Rotation horizontalEnc(13);
inline pros::Rotation verticalEnc(8);
// ver is 2.75
//horz is 2
inline lemlib::TrackingWheel horizontalTrackingWheel(&horizontalEnc, lemlib::Omniwheel::NEW_2 * 24/25.7, 3.5);
inline lemlib::TrackingWheel verticalTrackingWheel(&verticalEnc, lemlib::Omniwheel::NEW_275 * 24/23.5, -0.5);


inline lemlib::OdomSensors sensors(&verticalTrackingWheel,   // v1
                                   nullptr,                  // v2
                                   &horizontalTrackingWheel, // h1
                                   nullptr,                  // h2
                                   &imu);

// lateral pid
inline lemlib::ControllerSettings lateral_controller(8,    // kP
                                                     0.3,  // kI
                                                     36,   // kD
                                                     2,    // anti windup
                                                     0.4,  // small error range, in
                                                     100,  // small error timeout, ms
                                                     1,    // large error range, in
                                                     300,  // large error timeout, ms
                                                     5);   // max acceleration (slew)

// angular pid
inline lemlib::ControllerSettings angular_controller(2.2,      // kP
                                                     0.15,  // kI
                                                     12,    // kD
                                                     5,      // anti windup
                                                     1,      // small error range, deg
                                                     100,    // small error timeout, ms
                                                     3,      // large error range, deg
                                                     500,    // large error timeout, ms
                                                     0);     // max acceleration (slew)
// drive curves: https://www.vexforum.com/t/expo-drive-lemlibs-implementation
inline lemlib::ExpoDriveCurve throttle_curve(3, 0, 1.01);
inline lemlib::ExpoDriveCurve steer_curve(3, 0, 1.01);

inline lemlib::Chassis chassis(drivetrain,
                               lateral_controller,
                               angular_controller,
                               sensors,
                               &throttle_curve,
                               &steer_curve);

// PNEUMATICS //
inline pros::adi::DigitalOut clawPivot('A');
inline pros::adi::DigitalOut claw('B');




// MOTORS (negative port = reversed) //

// lift (dr4b)
inline pros::MotorGroup lift({1, -9}, pros::MotorGearset::green);

// intake
inline pros::Motor intake(-21, pros::MotorGearset::blue);
inline pros::Motor clawIntake(-16, pros::MotorGearset::green);

inline pros::Distance leftDistance(16);
inline pros::Distance rightDistance(1);