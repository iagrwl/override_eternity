#pragma once

// Eternity brain UI (LVGL 8.3).
//
// Pages: AUTON selector, FIELD odom map, MOTORS health, SENSORS, GRAPH, LOG, TOOLS,
// plus a status bar (match mode/timer, battery, controller, SD, device alerts) and a
// boot splash. All LVGL work happens inside one lv_timer, so every function in this
// header is safe to call from any task.

#include <vector>

namespace ui {

enum class Alliance { Red, Blue, Skills, Any };

struct Auton {
    const char* name;
    const char* description;
    Alliance alliance;
    void (*run)();
    // optional start pose, drawn on the auton preview map
    bool hasStart = false;
    float startX = 0, startY = 0, startTheta = 0;
};

// call first thing in initialize(): shows the boot splash
void init(const std::vector<Auton>& autons, const char* defaultAuton = nullptr);
// text under the spinner on the boot splash
void setBootStatus(const char* text);
// leave the splash and show the dashboard
void bootComplete();

// run whatever is picked on the selector (call from autonomous())
void runSelectedAuton();
const Auton* selectedAuton();

// true while a routine started from the brain's TOOLS/AUTON page is running;
// opcontrol should skip driver input while this is set
bool routineRunning();

// show any number on a custom info tab: ui::publish("lift.target", 400), then put
// "lift.target" as a widget's source in src/ui/layout.cpp (or pick it in sim edit mode)
void publish(const char* key, float value);

// printf-style line on the LOG page (also mirrored to the terminal)
void log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// drop-in for the old robodash console: console.printf / clear / focus
class Console {
  public:
    void printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    void println(const char* text);
    void clear();
    void focus();
};

} // namespace ui
