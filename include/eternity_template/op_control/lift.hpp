#pragma once

extern std::atomic<double> liftTarget;
extern std::atomic<bool> liftPidOn;
extern std::atomic<double> liftKP;
extern std::atomic<double> liftKD;


void liftControl();
void manualLiftControl();
void initLift();
void liftPos(double degree, int speed = 127); 
void liftPID();