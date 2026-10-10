// The one place the UI talks to the robot. Pages only see ui::Snapshot.
// Adding a motor/sensor? Add it to devices.hpp - ports are read from setup.hpp.

#include <atomic>
#include <cstdio>
#include <cstring>
#include "main.h"
#include "ui_internal.hpp"

namespace ui::hw {

// ---------------------------------------------------------------- device table
// the list itself is in devices.hpp; ports come from the objects in setup.hpp
#include "devices.hpp"

struct MotorEntry {
    const char* name;
    pros::AbstractMotor* motor; // Motor or MotorGroup
    int index;                  // which motor inside the group
};
#define UI_MOTOR_ENTRY(name, var, index) {name, &var, index},
static const MotorEntry kMotors[] = {UI_MOTORS(UI_MOTOR_ENTRY)};
static constexpr int kMotorCount = sizeof(kMotors) / sizeof(kMotors[0]);

struct SensorEntry {
    const char* name;
    pros::Device* dev;
    pros::DeviceType type;
};
#define UI_SENSOR_ENTRY(name, var, type) {name, &var, pros::DeviceType::type},
static const SensorEntry kSensors[] = {UI_SENSORS(UI_SENSOR_ENTRY)};

static bool plugged(int port, pros::DeviceType type) {
    if (port < 0) port = -port;
    return pros::Device::get_plugged_type(port) == type;
}

static const char* problemFor(int port, pros::DeviceType want) {
    pros::DeviceType got = pros::Device::get_plugged_type(port < 0 ? -port : port);
    if (got == want) return nullptr;
    if (got == pros::DeviceType::none) return "unplugged";
    return "wrong device";
}

void sample(Snapshot& s) {
    s.timeMs = pros::millis();

    s.compConnected = pros::competition::is_connected();
    s.fieldControl = pros::competition::is_field_control();
    s.disabled = pros::competition::is_disabled();
    s.autonomous = pros::competition::is_autonomous();

    lemlib::Pose p = chassis.getPose();
    s.x = p.x, s.y = p.y, s.theta = p.theta;

    s.batteryPct = pros::battery::get_capacity();
    s.batteryV = pros::battery::get_voltage() / 1000.0f;
    s.batteryA = pros::battery::get_current() / 1000.0f;
    s.controllerOk = controller.is_connected();
    s.controllerBattery = s.controllerOk ? controller.get_battery_capacity() : 0;
    s.sd = pros::usd::is_installed();

    // motors
    s.motorCount = 0;
    s.deviceCount = 0;
    s.deviceProblems = 0;
    float leftSum = 0, rightSum = 0;
    for (int i = 0; i < kMotorCount && i < kMaxMotors; i++) {
        const MotorEntry& e = kMotors[i];
        MotorStat& m = s.motors[s.motorCount++];
        m.name = e.name;
        m.port = e.motor->get_port(e.index);
        m.ok = plugged(m.port, pros::DeviceType::motor);
        m.rpm = e.motor->get_actual_velocity(e.index);
        m.watts = e.motor->get_power(e.index);
        m.amps = e.motor->get_current_draw(e.index) / 1000.0f;
        if (!m.ok) m.rpm = m.watts = m.amps = 0;
        if (e.motor == &left_dt) leftSum += m.rpm;
        if (e.motor == &right_dt) rightSum += m.rpm;

        DeviceStat& d = s.devices[s.deviceCount++];
        d.name = e.name;
        d.port = m.port < 0 ? -m.port : m.port;
        d.problem = problemFor(m.port, pros::DeviceType::motor);
        d.ok = !d.problem;
    }
    s.leftRpm = leftSum / 3;
    s.rightRpm = rightSum / 3;

    // sensors
    for (const SensorEntry& e : kSensors) {
        if (s.deviceCount >= kMaxDevices) break;
        DeviceStat& d = s.devices[s.deviceCount++];
        d.name = e.name;
        d.port = e.dev->get_port();
        d.problem = problemFor(d.port, e.type);
        d.ok = !d.problem;
    }

    // two devices configured on the same port is always a bug
    for (int i = 0; i < s.deviceCount; i++) {
        for (int j = 0; j < i; j++) {
            if (s.devices[i].port != s.devices[j].port) continue;
            s.devices[i].ok = s.devices[j].ok = false;
            s.devices[i].problem = s.devices[j].problem = "port shared";
        }
    }
    for (int i = 0; i < s.deviceCount; i++) s.deviceProblems += !s.devices[i].ok;

    s.imuOk = plugged(imu.get_port(), pros::DeviceType::imu);
    s.imuCalibrating = s.imuOk && imu.is_calibrating();
    if (s.imuOk) {
        s.imuHeading = imu.get_heading();
        s.imuPitch = imu.get_pitch();
        s.imuRoll = imu.get_roll();
    }
    s.vertOk = plugged(verticalEnc.get_port(), pros::DeviceType::rotation);
    s.horizOk = plugged(horizontalEnc.get_port(), pros::DeviceType::rotation);
    if (s.vertOk) s.vertDeg = verticalEnc.get_position() / 100.0f;
    if (s.horizOk) s.horizDeg = horizontalEnc.get_position() / 100.0f;
    s.leftDistOk = plugged(leftDistance.get_port(), pros::DeviceType::distance);
    s.rightDistOk = plugged(rightDistance.get_port(), pros::DeviceType::distance);
    if (s.leftDistOk) s.leftDistMm = leftDistance.get();
    if (s.rightDistOk) s.rightDistMm = rightDistance.get();

    s.liftDeg = lift.get_position();
    s.intake = intakeState == IntakeState::IN ? "IN" : intakeState == IntakeState::OUT ? "OUT" : "OFF";
    s.pivotOut = isPivotOut;
}

// ---------------------------------------------------------------- actions
void calibrateImu() {
    pros::Task([] {
        chassis.calibrate();
        ui::log("IMU calibrated");
    });
}

void zeroPose() {
    chassis.setPose(0, 0, 0);
    ui::log("pose zeroed");
}

void tareLift() {
    lift.tare_position_all();
    ui::log("lift tared");
}

void wallReset(bool rightSensor) {
    float v = wallDistance(false, rightSensor, false);
    ui::log("wall reset (%s sensor) -> %.1f", rightSensor ? "right" : "left", v);
}

void togglePivot() {
    isPivotOut = !isPivotOut;
    clawPivot.set_value(isPivotOut);
}

static std::atomic<bool> s_running{false};
static std::atomic<bool> s_abort{false};

void runAsync(void (*fn)(), const char* name) {
    if (s_running.exchange(true)) return;
    s_abort = false;
    ui::log("RUN: %s", name);
    pros::Task(
        [fn, name] {
            uint32_t t0 = pros::millis();
            fn();
            chassis.waitUntilDone(); // let the routine's last (async) move finish before driver control returns
            left_dt.move(0);
            right_dt.move(0);
            ui::log("%s: %s (%.1fs)", s_abort ? "ABORTED" : "DONE", name, (pros::millis() - t0) / 1000.0);
            s_running = false;
        },
        "ui routine");
}

bool routineRunning() { return s_running; }

void abortRoutine() {
    if (!s_running || s_abort.exchange(true)) return;
    // keep cancelling every motion the routine starts until it runs out of steps
    pros::Task([] {
        while (s_running) {
            chassis.cancelAllMotions();
            left_dt.move(0);
            right_dt.move(0);
            pros::delay(10);
        }
    });
}

void rumble(const char* pattern) { controller.rumble(pattern); }

void controllerLine(const char* text) { controller.print(2, 0, "%-15s", text); }

// ---------------------------------------------------------------- persistence
static const char* kSelFile = "/usd/eternity_auton.txt";

bool loadSelection(char* buf, int len) {
    if (!pros::usd::is_installed()) return false;
    FILE* f = fopen(kSelFile, "r");
    if (!f) return false;
    bool ok = fgets(buf, len, f) != nullptr;
    fclose(f);
    if (ok) buf[strcspn(buf, "\r\n")] = 0;
    return ok && buf[0];
}

void saveSelection(const char* name) {
    if (!pros::usd::is_installed()) return;
    FILE* f = fopen(kSelFile, "w");
    if (!f) return;
    fputs(name, f);
    fclose(f);
}

uint32_t millis() { return pros::millis(); }

static pros::Mutex* s_logMutex = nullptr;
void logLock() {
    if (!s_logMutex) s_logMutex = new pros::Mutex();
    s_logMutex->take();
}
void logUnlock() { s_logMutex->give(); }

// ---------------------------------------------------------------- tuning routines on the TOOLS page
static const Routine kRoutines[] = {
    {"LATERAL PID", lateralSweepTest},
    {"ANGULAR PID", angularSweepTest},
    {"DRIVE FWD", [] { driveTesting(true); }},
    {"DRIVE BACK", [] { driveTesting(false); }},
    {"TURN CW", [] { turnTesting(true); }},
    {"TURN CCW", [] { turnTesting(false); }},
    {"SKILLS", simpleSkills},
    {"DUMMY", dummy},
};

int testRoutines(const Routine** out) {
    *out = kRoutines;
    return sizeof(kRoutines) / sizeof(kRoutines[0]);
}

} // namespace ui::hw
