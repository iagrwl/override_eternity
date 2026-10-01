// GRAPH page: rolling 12 s plots of drive rpm, heading, temps and power. History is
// recorded in the background so switching here shows the last 12 s straight away.

#include <cmath>
#include <cstring>
#include "ui_internal.hpp"

namespace ui {

static constexpr int kPoints = 120; // 100 ms per sample
static constexpr int kSets = 4;

struct Dataset {
    const char* name;
    const char* series[2];
    const char* unit[2];
    lv_coord_t min[2], max[2];
};

static const Dataset kSetsInfo[kSets] = {
    {"DRIVE", {"LEFT", "RIGHT"}, {"rpm", "rpm"}, {-600, -600}, {600, 600}},
    {"HEADING", {"ODOM", "IMU"}, {"\xC2\xB0", "\xC2\xB0"}, {0, 0}, {360, 360}},
    {"TEMP", {"DRIVE MAX", "MECH MAX"}, {"", ""}, {68, 68}, {158, 158}}, // units/range fixed up for C below
    {"POWER", {"BATTERY", "MOTORS"}, {"%", "W"}, {0, 0}, {100, 140}},
};

static lv_coord_t s_hist[kSets][2][kPoints];
static bool s_paused = false;
static int s_set = 0;

void recordGraph(const Snapshot& s) {
    if (s_paused) return;
    float driveMax = 0, mechMax = 0, watts = 0;
    for (int i = 0; i < s.motorCount; i++) {
        const MotorStat& m = s.motors[i];
        if (!m.ok) continue;
        watts += m.watts;
        // first six motors are the drivetrain (see robot_io.cpp)
        float& peak = i < 6 ? driveMax : mechMax;
        if (m.tempC > peak) peak = m.tempC;
    }
    float odom = fmodf(fmodf(s.theta, 360) + 360, 360);
    float imu = fmodf(fmodf(s.imuHeading, 360) + 360, 360);
    const float v[kSets][2] = {
        {s.leftRpm, s.rightRpm}, {odom, imu}, {displayTemp(driveMax), displayTemp(mechMax)}, {s.batteryPct, watts}};

    for (int d = 0; d < kSets; d++) {
        for (int k = 0; k < 2; k++) {
            memmove(&s_hist[d][k][0], &s_hist[d][k][1], (kPoints - 1) * sizeof(lv_coord_t));
            s_hist[d][k][kPoints - 1] = (lv_coord_t)lroundf(v[d][k]);
        }
    }
}

static lv_obj_t* s_chart = nullptr;
static lv_chart_series_t* s_ser[2] = {};
static lv_obj_t* s_legend[2] = {};
static lv_obj_t* s_pauseBtn = nullptr;

static lv_color_t seriesColor(int k) { return k == 0 ? color::accent2() : color::accent(); }

static void applySet() {
    const Dataset& d = kSetsInfo[s_set];
    bool temp = s_set == 2;
    for (int k = 0; k < 2; k++) {
        lv_coord_t lo = temp ? (lv_coord_t)displayTemp(20) : d.min[k], hi = temp ? (lv_coord_t)displayTemp(70) : d.max[k];
        lv_chart_set_range(s_chart, k ? LV_CHART_AXIS_SECONDARY_Y : LV_CHART_AXIS_PRIMARY_Y, lo, hi);
    }
    for (int k = 0; k < 2; k++) lv_chart_set_ext_y_array(s_chart, s_ser[k], s_hist[s_set][k]);
    lv_chart_refresh(s_chart);
}

static void onSet(lv_event_t* e) {
    uint16_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id >= kSets) return;
    s_set = id;
    applySet();
}

static void onPause(lv_event_t*) {
    s_paused = !s_paused;
    lv_label_set_text_static(lv_obj_get_child(s_pauseBtn, 0), s_paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
    lv_obj_set_style_bg_color(s_pauseBtn, s_paused ? color::warn() : color::panel2(), 0);
}

static void buildGraph(lv_obj_t* parent) {
    static const char* map[] = {"DRIVE", "HEADING", "TEMP", "POWER", ""};
    lv_obj_t* seg = lv_btnmatrix_create(parent);
    lv_obj_remove_style_all(seg);
    lv_btnmatrix_set_map(seg, map);
    lv_obj_set_pos(seg, 0, 0);
    lv_obj_set_size(seg, 380, 28);
    lv_obj_set_style_pad_column(seg, 4, 0);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seg, color::panel(), LV_PART_ITEMS);
    lv_obj_set_style_border_width(seg, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(seg, color::line(), LV_PART_ITEMS);
    lv_obj_set_style_radius(seg, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_font(seg, &lv_font_montserrat_12, LV_PART_ITEMS);
    lv_obj_set_style_text_color(seg, color::muted(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seg, color::accent(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(seg, color::accent(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(seg, lv_color_white(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_btnmatrix_set_btn_ctrl_all(seg, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(seg, true);
    lv_btnmatrix_set_btn_ctrl(seg, s_set, LV_BTNMATRIX_CTRL_CHECKED);
    lv_obj_add_event_cb(seg, onSet, LV_EVENT_VALUE_CHANGED, nullptr);

    s_pauseBtn = makeButton(parent, s_paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE, 40, 28, onPause);
    lv_obj_set_pos(s_pauseBtn, 386, 0);
    if (s_paused) lv_obj_set_style_bg_color(s_pauseBtn, color::warn(), 0);

    s_chart = lv_chart_create(parent);
    lv_obj_remove_style_all(s_chart);
    lv_obj_add_style(s_chart, &styles().panel, 0);
    lv_obj_set_pos(s_chart, 0, 34);
    lv_obj_set_size(s_chart, 426, 150);
    lv_obj_set_style_pad_all(s_chart, 8, 0);
    lv_obj_set_style_line_color(s_chart, color::line(), 0);
    lv_obj_set_style_line_width(s_chart, 1, 0);
    lv_obj_set_style_line_opa(s_chart, LV_OPA_50, 0);
    lv_obj_set_style_line_width(s_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(s_chart, 0, LV_PART_INDICATOR);
    lv_chart_set_type(s_chart, LV_CHART_TYPE_LINE);
    lv_chart_set_div_line_count(s_chart, 5, 7);
    lv_chart_set_update_mode(s_chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_point_count(s_chart, kPoints);
    s_ser[0] = lv_chart_add_series(s_chart, seriesColor(0), LV_CHART_AXIS_PRIMARY_Y);
    s_ser[1] = lv_chart_add_series(s_chart, seriesColor(1), LV_CHART_AXIS_SECONDARY_Y);
    applySet();

    for (int k = 0; k < 2; k++) {
        s_legend[k] = makeLabel(parent, &lv_font_montserrat_12, seriesColor(k), "");
        lv_obj_set_pos(s_legend[k], 4 + k * 214, 190);
    }
}

static void updateGraph(const Snapshot&) {
    static uint32_t n = 0;
    if (n++ % 2) return; // 10 Hz, matching the sample rate
    const Dataset& d = kSetsInfo[s_set];
    for (int k = 0; k < 2; k++) {
        setTextf(s_legend[k], LV_SYMBOL_MINUS " %s  %d %s", d.series[k], s_hist[s_set][k][kPoints - 1],
                 s_set == 2 ? tempUnit() : d.unit[k]);
    }
    if (!s_paused) lv_chart_refresh(s_chart);
}

static void teardownGraph() {
    s_chart = s_pauseBtn = nullptr;
    s_ser[0] = s_ser[1] = nullptr;
    s_legend[0] = s_legend[1] = nullptr;
}

const Page pageGraph = {"GRAPH", LV_SYMBOL_SHUFFLE, buildGraph, updateGraph, teardownGraph};

} // namespace ui
