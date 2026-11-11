#pragma once

extern std::atomic<double> liftTarget;
extern std::atomic<bool> liftPidOn;
extern std::atomic<double> liftKP;
extern std::atomic<double> liftKD;


void liftControl();
void manualLiftControl();
void initLift();
void liftPos(double degree, int speed = 127); 
void liftToAngle(double targetDeg, int power = 80, uint32_t timeoutMs = 1500); // blocking, uses liftRot
void liftPID();
double liftAngle(); // lift rotation sensor, degrees