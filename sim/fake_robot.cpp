// Fake robot for the desktop sim. Stands in for src/ui/robot_io.cpp: same ui::hw
// functions, but the "hardware" is a little physics toy you drive with the keyboard.
// Tweak anything here to see how the UI handles it (hot motors, unplugged ports...).

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "fake_robot.hpp"
#include "ui_internal.hpp"

FakeRobot robot;

// stubs for the auton functions referenced by src/auton/auton_list.cpp
void five_pin() {}
void soloAWP() {}
void simpleRoute() {}
void basicParth() {}
void simpleSkills() {}

namespace ui::hw {

// same names/ports as robot_io.cpp + setup.hpp
struct FakeMotor {
    const char* name;
    int port;
    bool drive;
    float temp;
};
static FakeMotor motors[] = {
    {"LEFT 1", 18, true, 34},  {"LEFT 2", -6, true, 35},  {"LEFT 3", -10, true, 36},
    {"RIGHT 1", 3, true, 34},  {"RIGHT 2", 5, true, 35},  {"RIGHT 3", -2, true, 37},
    {"LIFT A", 1, false, 33},  {"LIFT B", -9, false, 33}, {"INTAKE", -21, false, 32},
    {"CLAW IN", -16, false, 32},
};
static constexpr int kMotorN = sizeof(motors) / sizeof(motors[0]);

struct FakeSensor {
    const char* name;
    int port;
};
static const FakeSensor sensors[] = {
    {"IMU", 14}, {"Horiz wheel", 13}, {"Vert wheel", 8}, {"Left dist", 16}, {"Right dist", 1},
};

static uint32_t lastMs = 0;
static bool running = false;
static uint32_t runEnd = 0;

void sample(Snapshot& s) {
    uint32_t now = SDL_GetTicks();
    float dt = lastMs ? (now - lastMs) / 1000.0f : 0;
    lastMs = now;
    FakeRobot& r = robot;

    // a "routine" started from the brain just drives a lazy circle
    if (running) {
        r.throttle = 0.6f, r.turn = 0.35f;
        if (now > runEnd) running = false, r.throttle = r.turn = 0;
    }

    // simple arcade physics: 60 in/s top speed, 300 deg/s top turn
    bool canMove = !(r.compConnected && r.disabled);
    float v = canMove ? r.throttle * 60 : 0, w = canMove ? r.turn * 300 : 0;
    r.theta = fmodf(r.theta + w * dt + 360, 360);
    float t = r.theta * (float)M_PI / 180;
    r.x += sinf(t) * v * dt;
    r.y += cosf(t) * v * dt;
    if (r.x > 64) r.x = 64;
    if (r.x < -64) r.x = -64;
    if (r.y > 64) r.y = 64;
    if (r.y < -64) r.y = -64;
    r.vertDeg += v * dt / (2.75f * (float)M_PI) * 360;

    float leftRpm = (r.throttle + r.turn * 0.6f) * 600 * canMove;
    float rightRpm = (r.throttle - r.turn * 0.6f) * 600 * canMove;

    s.timeMs = now;
    s.compConnected = r.compConnected;
    s.fieldControl = r.compConnected && r.fieldControl;
    s.disabled = r.compConnected && r.disabled;
    s.autonomous = r.compConnected && r.autonomous;
    s.x = r.x, s.y = r.y, s.theta = r.theta;
    s.batteryPct = r.battery;
    s.batteryV = 12.0f + r.battery / 100 * 0.9f;
    s.batteryA = 1 + fabsf(r.throttle) * 9 + fabsf(r.turn) * 4;
    s.batteryTemp = 30;
    s.controllerOk = true;
    s.controllerBattery = 72;
    s.sd = true;

    s.motorCount = s.deviceCount = s.deviceProblems = 0;
    for (int i = 0; i < kMotorN; i++) {
        FakeMotor& fm = motors[i];
        bool ok = !(i == 7 && r.unplugLift);
        float load = fm.drive ? fabsf(i < 3 ? leftRpm : rightRpm) / 600 : (i == 8 || i == 9) && r.intakeOn ? 0.5f : 0;
        fm.temp += (load * 6 - (fm.temp - 30) * 0.02f) * dt; // heats with load, cools toward 30C
        MotorStat& m = s.motors[s.motorCount++];
        m.name = fm.name, m.port = fm.port, m.ok = ok;
        m.tempC = ok ? fm.temp : 0;
        m.rpm = !ok ? 0 : fm.drive ? (i < 3 ? leftRpm : rightRpm) : (i >= 8 && r.intakeOn ? 590 : 0);
        m.watts = ok ? 0.3f + load * 11 : 0;
        m.amps = m.watts / 12;
        DeviceStat& d = s.devices[s.deviceCount++];
        d.name = fm.name, d.port = abs(fm.port), d.ok = ok, d.problem = ok ? nullptr : "unplugged";
    }
    for (const FakeSensor& fs : sensors) {
        DeviceStat& d = s.devices[s.deviceCount++];
        d.name = fs.name, d.port = fs.port, d.ok = true, d.problem = nullptr;
    }
    // same shared-port check as the real robot_io.cpp
    for (int i = 0; i < s.deviceCount; i++)
        for (int j = 0; j < i; j++)
            if (s.devices[i].port == s.devices[j].port) {
                s.devices[i].ok = s.devices[j].ok = false;
                s.devices[i].problem = s.devices[j].problem = "port shared";
            }
    for (int i = 0; i < s.deviceCount; i++) s.deviceProblems += !s.devices[i].ok;

    s.imuOk = true;
    s.imuCalibrating = now < 1500;
    s.imuHeading = r.theta;
    s.imuPitch = r.throttle * 2;
    s.imuRoll = r.turn * -3;
    s.vertOk = s.horizOk = true;
    s.vertDeg = r.vertDeg;
    s.horizDeg = r.turn * 40;
    s.leftDistOk = s.rightDistOk = true;
    s.leftDistMm = (int)((72 + r.x) * 25.4f);
    s.rightDistMm = (int)((72 - r.x) * 25.4f);
    s.liftDeg = r.liftDeg;
    s.intake = r.intakeOn ? "IN" : "OFF";
    s.clawOpen = r.clawOpen;
    s.leftRpm = leftRpm;
    s.rightRpm = rightRpm;
}

void calibrateImu() { ui::log("[sim] calibrate IMU"); }
void zeroPose() { robot.x = robot.y = robot.theta = 0; }
void tareLift() { robot.liftDeg = 0; }
void wallReset(bool right) { ui::log("[sim] wall reset (%s)", right ? "right" : "left"); }
void toggleClaw() { robot.clawOpen = !robot.clawOpen; }

void runAsync(void (*)(), const char* name) {
    if (running) return;
    ui::log("RUN: %s  [sim drives a circle for 4s]", name);
    running = true;
    runEnd = SDL_GetTicks() + 4000;
}
bool routineRunning() { return running; }
void abortRoutine() {
    runEnd = 0;
    ui::log("ABORTED");
}

void rumble(const char* pattern) { printf("[controller rumble %s]\n", pattern); }
void controllerLine(const char* text) { printf("[controller] %s\n", text); }

// remembers the auton pick across sim restarts, like the SD card does
static const char* kSelFile = ".cache/auton_selection.txt";
bool loadSelection(char* buf, int len) {
    FILE* f = fopen(kSelFile, "r");
    if (!f) return false;
    bool ok = fgets(buf, len, f) != nullptr;
    fclose(f);
    return ok && buf[0];
}
void saveSelection(const char* name) {
    if (FILE* f = fopen(kSelFile, "w")) {
        fputs(name, f);
        fclose(f);
    }
}

uint32_t millis() { return SDL_GetTicks(); }
void logLock() {}   // the sim is single-threaded
void logUnlock() {}

static void noop() {}
static const Routine kRoutines[] = {
    {"LATERAL PID", noop}, {"ANGULAR PID", noop}, {"DRIVE FWD", noop}, {"DRIVE BACK", noop},
    {"TURN CW", noop},     {"TURN CCW", noop},    {"SKILLS", noop},    {"BASIC TEST", noop},
};
int testRoutines(const Routine** out) {
    *out = kRoutines;
    return sizeof(kRoutines) / sizeof(kRoutines[0]);
}

} // namespace ui::hw
