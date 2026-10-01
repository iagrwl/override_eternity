// TOOLS page: pit utilities (calibrate, zero, tare, pneumatics) and tuning routines,
// plus a system panel showing UI memory and CPU use.

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_info = nullptr;
static lv_obj_t* s_memBar = nullptr;
static int s_pending = -1;

// first four buttons are instant utilities, the rest are routines that drive
static const char* kUtility[] = {LV_SYMBOL_REFRESH " IMU", LV_SYMBOL_HOME " ZERO", LV_SYMBOL_DOWN " TARE LIFT",
                                 LV_SYMBOL_SHUFFLE " CLAW"};
static constexpr int kUtilityCount = 4;

static void runPending() {
    const hw::Routine* list;
    int n = hw::testRoutines(&list);
    if (s_pending >= 0 && s_pending < n) hw::runAsync(list[s_pending].fn, list[s_pending].name);
}

static void onTool(lv_event_t* e) {
    uint16_t id = lv_btnmatrix_get_selected_btn(lv_event_get_target(e));
    if (id == LV_BTNMATRIX_BTN_NONE) return;
    switch (id) {
        case 0:
            hw::calibrateImu();
            toast(LV_SYMBOL_REFRESH "  calibrating IMU, hold still", color::accent());
            return;
        case 1:
            hw::zeroPose();
            clearTrail();
            toast(LV_SYMBOL_OK "  pose zeroed", color::good());
            return;
        case 2:
            hw::tareLift();
            toast(LV_SYMBOL_OK "  lift tared", color::good());
            return;
        case 3: hw::toggleClaw(); return;
    }
    if (snapshot().compConnected) {
        toast("unplug the comp cable to run tests", color::bad());
        return;
    }
    if (hw::routineRunning()) return;
    const hw::Routine* list;
    hw::testRoutines(&list);
    s_pending = id - kUtilityCount;
    static char body[96];
    snprintf(body, sizeof body, "\"%s\" will drive the robot. Give it room.", list[s_pending].name);
    confirm("Run test?", body, LV_SYMBOL_PLAY " RUN", runPending);
}

static void buildTools(lv_obj_t* parent) {
    const hw::Routine* list;
    int n = hw::testRoutines(&list);

    // 4 per row: utilities on the first row, routines after
    static const char* map[32];
    int m = 0, col = 0;
    auto add = [&](const char* txt) {
        if (col == 4) map[m++] = "\n", col = 0;
        map[m++] = txt;
        col++;
    };
    for (int i = 0; i < kUtilityCount; i++) add(kUtility[i]);
    for (int i = 0; i < n && m < 28; i++) add(list[i].name);
    map[m] = "";

    lv_obj_t* grid = lv_btnmatrix_create(parent);
    lv_obj_remove_style_all(grid);
    lv_btnmatrix_set_map(grid, map);
    lv_obj_set_pos(grid, 0, 0);
    lv_obj_set_size(grid, 426, 146);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_bg_opa(grid, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(grid, color::panel(), LV_PART_ITEMS);
    lv_obj_set_style_border_width(grid, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(grid, color::line(), LV_PART_ITEMS);
    lv_obj_set_style_radius(grid, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_font(grid, &lv_font_montserrat_12, LV_PART_ITEMS);
    lv_obj_set_style_text_color(grid, color::text(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(grid, color::accent(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(grid, onTool, LV_EVENT_VALUE_CHANGED, nullptr);
    // routines get a warm tint so it's obvious they move the robot
    for (int i = 0; i < n; i++) {
        lv_btnmatrix_set_btn_ctrl(grid, kUtilityCount + i, LV_BTNMATRIX_CTRL_CUSTOM_1);
    }
    lv_obj_add_event_cb(
        grid,
        [](lv_event_t* e) {
            lv_obj_draw_part_dsc_t* d = lv_event_get_draw_part_dsc(e);
            if (d->part != LV_PART_ITEMS || !d->rect_dsc) return;
            lv_obj_t* g = lv_event_get_target(e);
            if (!lv_btnmatrix_has_btn_ctrl(g, d->id, LV_BTNMATRIX_CTRL_CUSTOM_1)) return;
            if (lv_btnmatrix_get_selected_btn(g) == d->id && lv_obj_has_state(g, LV_STATE_PRESSED)) return;
            d->rect_dsc->bg_color = lv_color_mix(color::warn(), color::bg(), 25);
            d->rect_dsc->border_color = lv_color_mix(color::warn(), color::bg(), 90);
            if (d->label_dsc) d->label_dsc->color = color::warn();
        },
        LV_EVENT_DRAW_PART_BEGIN, nullptr);

    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &styles().panel, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, 0, 154);
    lv_obj_set_size(panel, 426, 52);

    lv_obj_t* cap = lv_label_create(panel);
    lv_obj_add_style(cap, &styles().caption, 0);
    lv_label_set_text_static(cap, "SYSTEM");
    lv_obj_set_pos(cap, 10, 7);

    s_memBar = lv_bar_create(panel);
    lv_obj_remove_style_all(s_memBar);
    lv_obj_set_size(s_memBar, 120, 6);
    lv_obj_set_pos(s_memBar, 10, 30);
    lv_obj_set_style_bg_opa(s_memBar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_memBar, color::panel2(), 0);
    lv_obj_set_style_radius(s_memBar, 3, 0);
    lv_obj_set_style_bg_opa(s_memBar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_memBar, color::accent2(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(s_memBar, 3, LV_PART_INDICATOR);

    s_info = makeLabel(panel, &lv_font_montserrat_12, color::muted(), "");
    lv_obj_set_pos(s_info, 142, 8);
}

static void updateTools(const Snapshot& s) {
    static uint32_t n = 0;
    if (n++ % 10) return;
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    uint32_t used = mon.total_size - mon.free_size;
    lv_bar_set_value(s_memBar, mon.used_pct, LV_ANIM_ON);
    uint32_t up = s.timeMs / 1000;
    setTextf(s_info, "UI mem %d.%d / %d KB  (%d%% frag)\nUI cpu %d%%   uptime %d:%02d",
                          (int)(used / 1024), (int)(used % 1024 * 10 / 1024), (int)(mon.total_size / 1024),
                          mon.frag_pct, 100 - lv_timer_get_idle(), (int)(up / 60), (int)(up % 60));
}

static void teardownTools() { s_info = s_memBar = nullptr; }

const Page pageTools = {"TOOLS", LV_SYMBOL_SETTINGS, buildTools, updateTools, teardownTools};

} // namespace ui
