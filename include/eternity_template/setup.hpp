#pragma once

#include "api.h"
#include "lemlib/api.hpp"
#include "robodash/api.h"

extern rd::Console console;
extern rd::Selector selector;



// controller
inline pros::Controller controller(pros::E_CONTROLLER_MASTER); //controller

// drivetrain
inline pros::MotorGroup right_dt({12,11,-15}, pros::MotorGearset::blue);    // right side
inline pros::MotorGroup left_dt({20,14,-13}, pros::MotorGearset::blue);  // left side

// drivetrain config for movement functions
// (wheel size, width, rpm, etc.)
inline lemlib::Drivetrain drivetrain(&left_dt, // left mg
                                     &right_dt, // right mg
                                     11, // track width
                                     3.25, // wheel diameter
                                     360, // rpm
                                     0
);

inline pros::Imu imu(7);
// declare sensors needed for odom setup
inline pros::Rotation horizontalEnc(8);
inline pros::Rotation verticalEnc(9);

//configure tracking wheels
inline lemlib::TrackingWheel horizontalTrackingWheel(&horizontalEnc, lemlib::Omniwheel::NEW_2, 0);
inline lemlib::TrackingWheel verticalTrackingWheel(&verticalEnc, lemlib::Omniwheel::NEW_2, 0);

// configure entire odom setup
inline lemlib::OdomSensors sensors(&verticalTrackingWheel, // v1
							nullptr, // v2
							&horizontalTrackingWheel, // h1
							nullptr, // h2
							&imu // imu
);

// lateral pid
inline lemlib::ControllerSettings lateral_controller(7, //proportional gain (kP) //5.5
                                            0, // integral gain (kI) //0.5
                                        25, // derivative gain (kD) //18
                                         1,//windup
                                           0.5, // small error range, in inches
                                          100, // small error range timeout, in milliseconds
                                           1, // large error1range, in inches
                                         750, // large error range timeout, in milliseconds
                                              5 // maximum acceleration (slew)
);

// angular pid
inline lemlib::ControllerSettings angular_controller(5, // proportional gain (kP)
                                              0, // integral gain (kI)
                                              43.5, // derivative gain (kD)
                                              3.5, // anti windup
                                              0.5, // small error range, in degrees
                                                50, // small error range timeout, in milliseconds
                                              1.5, // large error range, in degrees
                                              200, // large error range timeout, in milliseconds
                                              0 // maximum acceleration (slew)
);


// look at  https://www.vexforum.com/t/expo-drive-lemlibs-implementation
// for drive curves explanation and interactive desmos graph

// input curve for throttle input during driver control
inline lemlib::ExpoDriveCurve throttle_curve(
    3,    // deadband
    0,    // min output
    1.01  
);
// input curve for steer input during driver control
inline lemlib::ExpoDriveCurve steer_curve(
    3,    // deadband 
    0,    // min output
    1.01   // LINEAR
);


inline lemlib::Chassis chassis(drivetrain, // drivetrain settings
                        lateral_controller, // lateral PID settings
                        angular_controller, // angular PID settings
                        sensors, // odometry sensors
                        &throttle_curve,
                        &steer_curve
);

// declare additional motors, sensors, and pnuematics here

//negative means regular direction is flipped
// MOTORS //

// lift
inline pros::MotorGroup lift({4, -5}, pros::MotorGearset::red);
inline pros::Motor arm({-6}, pros::MotorGearset::green);
inline pros::Rotation liftRotation(-9);

// PNEUMMATICS //
inline pros::adi::DigitalOut piston('A');



// vision sensors
inline pros::Vision vision(14);
inline pros::Optical topOptical(14);
inline pros::Distance bottomDistance(15);
inline pros::Distance leftDistance(20);
inline pros::Distance rightDistance(1);
