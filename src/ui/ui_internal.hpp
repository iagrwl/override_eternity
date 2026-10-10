#pragma once

// shared bits between the UI pages. Nothing in here touches pros/lemlib directly;
// robot_io.cpp is the only file that talks to hardware, so the UI can be run in a
// desktop simulator by swapping that one file.

#include <cstdint>
#include <string>
#include <vector>
#include "liblvgl/lvgl.h"
#include "eternity_template/ui/ui.hpp"
#include "eternity_template/ui/ui_config.hpp"

namespace ui {

// ---------------------------------------------------------------- palette
namespace color {
// edit colors in include/eternity_template/ui/ui_config.hpp
inline lv_color_t bg()      { return lv_color_hex(config::kBackground); }
inline lv_color_t panel()   { return lv_color_hex(config::kPanel); }
inline lv_color_t panel2()  { return lv_color_hex(config::kPanel2); }
inline lv_color_t line()    { return lv_color_hex(config::kBorder); }
inline lv_color_t text()    { return lv_color_hex(config::kText); }
inline lv_color_t muted()   { return lv_color_hex(config::kMuted); }
inline lv_color_t dim()     { return lv_color_mix(muted(), bg(), 150); }
inline lv_color_t accent()  { return lv_color_hex(config::kAccent); }
inline lv_color_t accent2() { return lv_color_hex(config::kAccent2); }
inline lv_color_t good()    { return lv_color_hex(config::kGood); }
inline lv_color_t warn()    { return lv_color_hex(config::kWarn); }
inline lv_color_t bad()     { return lv_color_hex(config::kBad); }
inline lv_color_t red()     { return lv_color_hex(config::kRedAlliance); }
inline lv_color_t blue()    { return lv_color_hex(config::kBlueAlliance); }
inline lv_color_t skills()  { return lv_color_hex(config::kSkills); }
// accent darkened toward the background (selected rail item / card)
inline lv_color_t accentDim() { return lv_color_mix(accent(), bg(), 70); }
} // namespace color

lv_color_t allianceColor(Alliance a);
const char* allianceName(Alliance a);
// green -> amber -> red across [lo, hi]
lv_color_t heatColor(float value, float lo, float hi);

// ---------------------------------------------------------------- styles
struct Styles {
    lv_style_t screen;   // full-screen background
    lv_style_t panel;    // rounded card
    lv_style_t chip;     // small pill
    lv_style_t btn;      // flat button
    lv_style_t btnPressed;
    lv_style_t title;    // big numbers
    lv_style_t caption;  // small muted captions
    lv_style_t noPad;    // bare container
};
Styles& styles();
void initStyles();

// label helper using shared styles only (no local style allocations)
lv_obj_t* makeLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t col, const char* text = "");
// printf into a label with the real libc formatter (LVGL's own one has no %f)
void setTextf(lv_obj_t* label, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
lv_obj_t* makeButton(lv_obj_t* parent, const char* text, lv_coord_t w, lv_coord_t h, lv_event_cb_t cb,
                     void* user = nullptr);
// bare container with no styling/scrolling
lv_obj_t* makeBox(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h);

// ---------------------------------------------------------------- draw helpers
// wrappers around lv_draw_* for custom-drawn widgets (they keep LVGL heap use tiny)
void drawRect(lv_draw_ctx_t* ctx, const lv_area_t& a, lv_color_t col, lv_opa_t opa = LV_OPA_COVER,
              lv_coord_t radius = 0);
void drawFrame(lv_draw_ctx_t* ctx, const lv_area_t& a, lv_color_t col, lv_coord_t width, lv_coord_t radius = 0,
               lv_opa_t opa = LV_OPA_COVER);
void drawLine(lv_draw_ctx_t* ctx, lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2, lv_color_t col,
              lv_coord_t width = 1, lv_opa_t opa = LV_OPA_COVER);
void drawText(lv_draw_ctx_t* ctx, lv_coord_t x, lv_coord_t y, lv_coord_t w, const char* txt, const lv_font_t* font,
              lv_color_t col, lv_text_align_t align = LV_TEXT_ALIGN_LEFT, lv_opa_t opa = LV_OPA_COVER);
void drawArc(lv_draw_ctx_t* ctx, lv_coord_t cx, lv_coord_t cy, uint16_t r, uint16_t start, uint16_t end,
             lv_color_t col, lv_coord_t width, lv_opa_t opa = LV_OPA_COVER);
inline lv_area_t area(lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    return {x, y, (lv_coord_t)(x + w - 1), (lv_coord_t)(y + h - 1)};
}

// draws the 12x12ft field (tiles, center line, alliance strips) into a square
void drawField(lv_draw_ctx_t* ctx, const lv_area_t& a);
// field inches (center origin, +y up) -> screen pixels inside a square area
void fieldToPx(const lv_area_t& a, float x, float y, lv_coord_t& px, lv_coord_t& py);
// filled robot marker with heading arrow
void drawRobot(lv_draw_ctx_t* ctx, const lv_area_t& a, float x, float y, float thetaDeg, float sizeIn,
               lv_color_t col);

struct Snapshot;

// ---------------------------------------------------------------- layout (src/ui/layout.cpp)
enum class Builtin { None, Auton, Field, Motors, Sensors, Graph, Log, Tools };
enum class WidgetType { Label, Value, Bar, Gauge, Light };

// compile-time tables written in layout.cpp
struct TabDef {
    const char* title;
    const char* icon;  // name from iconNames(), e.g. "HOME"
    Builtin builtin;   // None = custom info tab made of widgets
};
struct WidgetDef {
    const char* tab;   // title of the custom tab it lives on
    WidgetType type;
    const char* text;  // caption / label text
    const char* source; // data key, see sources.cpp ("" for plain labels)
    int x, y, w, h;    // inside the 426x206 content area
    uint32_t color;    // 0 = theme accent
};
extern const TabDef kLayoutTabs[];
extern const int kLayoutTabCount;
extern const WidgetDef kLayoutWidgets[];
extern const int kLayoutWidgetCount;

// editable runtime copies (the sim's edit mode changes these, then saves them back)
struct Tab {
    std::string title, icon;
    Builtin builtin;
};
struct Widget {
    std::string tab, text, source;
    WidgetType type;
    int x, y, w, h;
    uint32_t color;
};
std::vector<Tab>& tabs();
std::vector<Widget>& widgets();
void rebuildTabs();          // after adding/removing/reordering tabs
std::string layoutCode();    // the generated block for layout.cpp
const char* iconGlyph(const std::string& name);
const std::vector<const char*>& iconNames();
const char* widgetTypeName(WidgetType t);
constexpr lv_coord_t kContentX = 27, kContentY = 2, kContentW = 426, kContentH = 200;
int tabIndexAtX(int x); // which top-bar tab is under x (-1 = none)

// ---------------------------------------------------------------- data sources (sources.cpp)
struct SourceValue {
    float value = 0;
    const char* unit = "";
    float min = 0, max = 100;
    int decimals = 0;
    bool boolean = false;
};
bool readSource(const std::string& key, const Snapshot& s, SourceValue& out);
std::vector<std::string> sourceKeys(const Snapshot& s);

// ---------------------------------------------------------------- robot data
constexpr int kMaxMotors = 12;
constexpr int kMaxDevices = 24;

struct MotorStat {
    const char* name;
    int port;           // signed, negative = reversed
    bool ok;
    float rpm;
    float watts;
    float amps;
};

struct DeviceStat {
    const char* name;
    int port;
    bool ok;            // plugged in and the right type
    const char* problem; // null when ok
};

struct Snapshot {
    uint32_t timeMs = 0;

    // competition
    bool compConnected = false, fieldControl = false, disabled = true, autonomous = false;

    // odom
    float x = 0, y = 0, theta = 0;

    // power
    float batteryPct = 0, batteryV = 0, batteryA = 0;
    bool controllerOk = false;
    int controllerBattery = 0;
    bool sd = false;

    MotorStat motors[kMaxMotors];
    int motorCount = 0;

    DeviceStat devices[kMaxDevices];
    int deviceCount = 0;
    int deviceProblems = 0;

    // sensors
    bool imuOk = false, imuCalibrating = false;
    float imuHeading = 0, imuPitch = 0, imuRoll = 0;
    bool vertOk = false, horizOk = false;
    float vertDeg = 0, horizDeg = 0;
    bool leftDistOk = false, rightDistOk = false;
    int leftDistMm = 0, rightDistMm = 0;
    float liftDeg = 0;
    const char* intake = "OFF";
    bool pivotOut = false;
    float leftRpm = 0, rightRpm = 0;
};

namespace hw {
void sample(Snapshot& s);

// actions (called from the UI thread; anything that moves the robot goes through runAsync)
void calibrateImu();
void zeroPose();
void tareLift();
void wallReset(bool rightSensor);
void togglePivot();
void runAsync(void (*fn)(), const char* name);
bool routineRunning();
void abortRoutine();
void rumble(const char* pattern);
void controllerLine(const char* text);

// selection persisted to the SD card by name
bool loadSelection(char* buf, int len);
void saveSelection(const char* name);

uint32_t millis();
void logLock();
void logUnlock();

// tuning routines listed on the TOOLS page
struct Routine {
    const char* name;
    void (*fn)();
};
int testRoutines(const Routine** out);
} // namespace hw

// ---------------------------------------------------------------- pages
struct Page {
    const char* title;
    const char* icon;
    void (*build)(lv_obj_t* parent); // build widgets into a fresh container
    void (*update)(const Snapshot& s); // called ~20x/s while visible
    void (*teardown)();               // drop any lv_obj_t* the page cached
};

extern const Page pageAuton, pageField, pageMotors, pageSensors, pageGraph, pageLog, pageTools, pageCustom;

void showPage(int index);
int currentPage();
void refreshPage(); // rebuild the visible page (after a layout edit)
int pageIndex(const Page* p); // -1 if that tab was removed from the layout
const Snapshot& snapshot();

// shared state the pages need
const std::vector<Auton>& autons();
int selectedIndex();
void selectAuton(int index);
bool selectionLocked(); // true during a match

// background recorders (run even when their page is hidden)
void recordTrail(const Snapshot& s);
void clearTrail();
void recordGraph(const Snapshot& s);

// log ring buffer
void logClear();
int logCopy(char* out, int len); // newest at the bottom
uint32_t logVersion();

// modal confirm used for anything that moves the robot
void confirm(const char* title, const char* body, const char* okText, void (*onOk)());
void toast(const char* text, lv_color_t col);

} // namespace ui
