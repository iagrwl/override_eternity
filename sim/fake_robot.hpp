#pragma once

// state of the pretend robot, poked by the keyboard in sim_main.cpp
struct FakeRobot {
    float x = 0, y = -48, theta = 0;
    float throttle = 0, turn = 0; // -1..1 from the arrow keys
    float vertDeg = 0, liftDeg = 0;
    float battery = 87;
    bool intakeOn = false, clawOpen = false, unplugLift = false;
    // competition state
    bool compConnected = false, fieldControl = true, disabled = true, autonomous = false;
};

extern FakeRobot robot;
