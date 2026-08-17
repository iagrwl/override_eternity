#pragma once

#include "api.h"
#include "lemlib/api.hpp"
#include "robodash/api.h"

extern rd::Console console;
extern rd::Selector selector;

// controller
inline pros::Controller controller(pros::E_CONTROLLER_MASTER);

// drivetrain (negative port = reversed)
inline pros::MotorGroup left_dt({-5, -9}, pros::MotorGearset::blue);
inline pros::MotorGroup right_dt({7, 20}, pros::MotorGearset::blue);

inline lemlib::Drivetrain drivetrain(&left_dt,
                                     &right_dt,
                                     11,    // track width
                                     3.25,  // wheel diameter
                                     360,   // rpm
                                     0);

// odom sensors
inline pros::Imu imu(8);
// inline pros::Rotation horizontalEnc(8);
inline pros::Rotation verticalEnc(10);

// inline lemlib::TrackingWheel horizontalTrackingWheel(&horizontalEnc, lemlib::Omniwheel::NEW_2, 0);
inline lemlib::TrackingWheel verticalTrackingWheel(&verticalEnc, lemlib::Omniwheel::NEW_275, 0.5);

inline lemlib::OdomSensors sensors(&verticalTrackingWheel,   // v1
                                   nullptr,                  // v2
                                   nullptr, // h1
                                   nullptr,                  // h2
                                   &imu);

// lateral pid
inline lemlib::ControllerSettings lateral_controller(7,    // kP
                                                     0,    // kI
                                                     30,   // kD
                                                     0,    // anti windup
                                                     1,  // small error range, in
                                                     100,  // small error timeout, ms
                                                     3,    // large error range, in
                                                     250,  // large error timeout, ms
                                                     5);   // max acceleration (slew)

// angular pid
inline lemlib::ControllerSettings angular_controller(5,    // kP
                                                     0.25,    // kI
                                                     43, // kD
                                                     4,  // anti windup
                                                     1,  // small error range, deg
                                                     100,   // small error timeout, ms
                                                     3,  // large error range, deg
                                                     350,  // large error timeout, ms
                                                     50);   // max acceleration (slew)

// drive curves: https://www.vexforum.com/t/expo-drive-lemlibs-implementation
inline lemlib::ExpoDriveCurve throttle_curve(3, 0, 1.01);
inline lemlib::ExpoDriveCurve steer_curve(3, 0, 1.01);

inline lemlib::Chassis chassis(drivetrain,
                               lateral_controller,
                               angular_controller,
                               sensors,
                               &throttle_curve,
                               &steer_curve);

// MOTORS (negative port = reversed) //

// lift (dr4b)
inline pros::MotorGroup lift({-19, 14}, pros::MotorGearset::green);

// intake
inline pros::Motor firstStage(-21, pros::MotorGearset::blue);
inline pros::Motor clawIntake(-16, pros::MotorGearset::green);

// flipper arm
inline pros::Motor intakePivot(-4, pros::MotorGearset::green);

// wall-reset distance sensors
inline pros::Distance leftDistance(20);
inline pros::Distance rightDistance(1);
