// =====================================================================================
//  DATA SOURCES - the names you can put in a widget's "source" column (layout.cpp)
//
//  built in: see kSources below, plus per-motor keys like "motor.LEFT 1.rpm" /
//  "motor.LEFT 1.watts" (names from src/ui/devices.hpp).
//
//  your own values: call ui::publish("anything", value) from robot code, e.g.
//      ui::publish("lift.target", 400);
//  then use "lift.target" as a widget source. no other changes needed.
//  to add a new built-in instead, add a line to kSources.
// =====================================================================================

#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui_internal.hpp"

namespace ui {


struct Source {
    const char* key;
    const char* unit;
    float min, max;
    int decimals;
    bool boolean;
    float (*read)(const Snapshot& s);
};

// clang-format off
static const Source kSources[] = {
    // key                 unit       min    max  dec  bool
    {"pose.x",             "in",      -72,   72,  1, false, [](const Snapshot& s) { return s.x; }},
    {"pose.y",             "in",      -72,   72,  1, false, [](const Snapshot& s) { return s.y; }},
    {"pose.heading",       "\xC2\xB0", 0,   360,  1, false, [](const Snapshot& s) { return fmodf(fmodf(s.theta, 360) + 360, 360); }},
    {"imu.heading",        "\xC2\xB0", 0,   360,  1, false, [](const Snapshot& s) { return s.imuHeading; }},
    {"imu.pitch",          "\xC2\xB0", -45,  45,  1, false, [](const Snapshot& s) { return s.imuPitch; }},
    {"imu.roll",           "\xC2\xB0", -45,  45,  1, false, [](const Snapshot& s) { return s.imuRoll; }},
    {"battery.pct",        "%",        0,   100,  0, false, [](const Snapshot& s) { return s.batteryPct; }},
    {"battery.volts",      "V",       11,    13,  2, false, [](const Snapshot& s) { return s.batteryV; }},
    {"battery.amps",       "A",        0,    20,  1, false, [](const Snapshot& s) { return s.batteryA; }},
    {"controller.battery", "%",        0,   100,  0, false, [](const Snapshot& s) { return (float)s.controllerBattery; }},
    {"lift.deg",           "\xC2\xB0", 0,   720,  0, false, [](const Snapshot& s) { return s.liftDeg; }},
    {"drive.left_rpm",     "rpm",   -600,   600,  0, false, [](const Snapshot& s) { return s.leftRpm; }},
    {"drive.right_rpm",    "rpm",   -600,   600,  0, false, [](const Snapshot& s) { return s.rightRpm; }},
    {"motors.watts",       "W",        0,   140,  0, false, [](const Snapshot& s) { float w = 0; for (int i = 0; i < s.motorCount; i++) w += s.motors[i].watts; return w; }},
    {"intake.on",          "",         0,     1,  0, true,  [](const Snapshot& s) { return strcmp(s.intake, "OFF") ? 1.f : 0.f; }},
    {"pivot.out",          "",         0,     1,  0, true,  [](const Snapshot& s) { return s.pivotOut ? 1.f : 0.f; }},
    {"dist.left_in",       "in",       0,    80,  1, false, [](const Snapshot& s) { return s.leftDistMm / 25.4f; }},
    {"dist.right_in",      "in",       0,    80,  1, false, [](const Snapshot& s) { return s.rightDistMm / 25.4f; }},
    {"devices.problems",   "",         0,     5,  0, false, [](const Snapshot& s) { return (float)s.deviceProblems; }},
    {"comp.connected",     "",         0,     1,  0, true,  [](const Snapshot& s) { return s.compConnected ? 1.f : 0.f; }},
};
// clang-format on

// ---------------------------------------------------------------- ui::publish
static constexpr int kMaxPublished = 24;
struct Published {
    char key[28];
    float value;
};
static Published s_pub[kMaxPublished];
static int s_pubCount = 0;

void publish(const char* key, float value) {
    hw::logLock();
    int i = 0;
    while (i < s_pubCount && strcmp(s_pub[i].key, key)) i++;
    if (i == s_pubCount && s_pubCount < kMaxPublished) snprintf(s_pub[s_pubCount++].key, sizeof s_pub[0].key, "%s", key);
    if (i < s_pubCount) s_pub[i].value = value;
    hw::logUnlock();
}

// ---------------------------------------------------------------- lookup
static SourceValue fromSource(const Source& src, const Snapshot& s) {
    SourceValue v;
    v.value = src.read(s);
    v.unit = src.unit;
    v.min = src.min, v.max = src.max;
    v.decimals = src.decimals;
    v.boolean = src.boolean;
    return v;
}

bool readSource(const std::string& key, const Snapshot& s, SourceValue& out) {
    bool found = false;
    for (const Source& src : kSources) {
        if (key == src.key) {
            out = fromSource(src, s);
            found = true;
            break;
        }
    }

    // motor.<NAME>.rpm / .watts
    if (!found && key.rfind("motor.", 0) == 0) {
        size_t dot = key.rfind('.');
        std::string name = key.substr(6, dot - 6), field = key.substr(dot + 1);
        for (int i = 0; i < s.motorCount && !found; i++) {
            if (name != s.motors[i].name) continue;
            const MotorStat& m = s.motors[i];
            out = SourceValue();
            found = true;
            if (field == "rpm") out.value = m.rpm, out.unit = "rpm", out.min = -600, out.max = 600;
            else if (field == "watts") out.value = m.watts, out.unit = "W", out.max = 13, out.decimals = 1;
            else found = false;
        }
    }

    // anything sent with ui::publish()
    if (!found) {
        hw::logLock();
        for (int i = 0; i < s_pubCount && !found; i++) {
            if (key != s_pub[i].key) continue;
            out = SourceValue();
            out.value = s_pub[i].value;
            out.decimals = fabsf(out.value - roundf(out.value)) > 0.001f ? 2 : 0;
            found = true;
        }
        hw::logUnlock();
    }
    if (!found) return false;

    return true;
}

std::vector<std::string> sourceKeys(const Snapshot& s) {
    std::vector<std::string> keys;
    for (const Source& src : kSources) keys.push_back(src.key);
    for (int i = 0; i < s.motorCount; i++) {
        keys.push_back(std::string("motor.") + s.motors[i].name + ".rpm");
        keys.push_back(std::string("motor.") + s.motors[i].name + ".watts");
    }
    hw::logLock();
    for (int i = 0; i < s_pubCount; i++) keys.push_back(s_pub[i].key);
    hw::logUnlock();
    return keys;
}

} // namespace ui
