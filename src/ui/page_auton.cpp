// AUTON page: plain list of routines on the left, the picked one big on the right.

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_list = nullptr;
static lv_obj_t* s_name = nullptr;
static lv_obj_t* s_side = nullptr;
static lv_obj_t* s_desc = nullptr;
static lv_obj_t* s_lock = nullptr;
static int s_shown = -2;
static bool s_wasLocked = false;

static constexpr lv_coord_t kRowH = 30;

static void styleRow(lv_obj_t* row, bool selected, bool dim) {
    lv_obj_set_style_border_color(row, selected ? color::text() : color::bg(), 0);
    lv_obj_t* label = lv_obj_get_child(row, 1);
    lv_obj_set_style_text_color(label, selected ? color::text() : color::muted(), 0);
    lv_obj_set_style_opa(row, dim ? LV_OPA_40 : LV_OPA_COVER, 0);
}

static void onRow(lv_event_t* e) {
    if (selectionLocked()) {
        toast("locked during the match", color::bad());
        return;
    }
    selectAuton((int)(intptr_t)lv_event_get_user_data(e));
}

static void runSelected() {
    const Auton* a = selectedAuton();
    if (a && a->run) hw::runAsync(a->run, a->name);
}

static void onTestRun(lv_event_t*) {
    const Auton* a = selectedAuton();
    if (!a) return;
    if (snapshot().compConnected) {
        toast("unplug the comp cable to test", color::bad());
        return;
    }
    if (hw::routineRunning()) return;
    static char body[96];
    snprintf(body, sizeof body, "%s will run now. The robot will move.", a->name);
    confirm("Run auton?", body, "RUN", runSelected);
}

static void buildAuton(lv_obj_t* parent) {
    s_shown = -2;
    const auto& list = autons();

    lv_obj_t* cap = lv_label_create(parent);
    lv_obj_add_style(cap, &styles().caption, 0);
    lv_label_set_text_static(cap, "AUTONOMOUS");
    lv_obj_set_pos(cap, 0, 2);

    s_lock = makeLabel(parent, &lv_font_montserrat_10, color::warn(), "LOCKED");
    lv_obj_set_pos(s_lock, 140, 2);

    s_list = makeBox(parent, 0, 20, 196, 180);
    lv_obj_add_flag(s_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);

    for (size_t i = 0; i < list.size(); i++) {
        const Auton& a = list[i];
        lv_obj_t* row = lv_obj_create(s_list);
        lv_obj_remove_style_all(row);
        lv_obj_set_size(row, 196, kRowH);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_width(row, 2, 0);
        lv_obj_set_style_bg_color(row, color::panel2(), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(row, onRow, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* dot = makeBox(row, 12, kRowH / 2 - 3, 6, 6);
        lv_obj_set_style_radius(dot, 3, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dot, allianceColor(a.alliance), 0);

        lv_obj_t* name = makeLabel(row, &lv_font_montserrat_14, color::muted(), a.name);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, 160);
        lv_obj_align(name, LV_ALIGN_LEFT_MID, 28, 0);
    }

    // hairline between the list and the detail side
    lv_obj_t* div = makeBox(parent, 210, 4, 1, 192);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(div, color::line(), 0);

    s_side = makeLabel(parent, &lv_font_montserrat_10, color::muted(), "");
    lv_obj_set_style_text_letter_space(s_side, 2, 0);
    lv_obj_set_pos(s_side, 226, 2);

    s_name = makeLabel(parent, &lv_font_montserrat_24, color::text(), "");
    lv_label_set_long_mode(s_name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_name, 200);
    lv_obj_set_pos(s_name, 226, 18);

    s_desc = makeLabel(parent, &lv_font_montserrat_12, color::muted(), "");
    lv_label_set_long_mode(s_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(s_desc, 200, 110);
    lv_obj_set_pos(s_desc, 226, 52);


    lv_obj_t* run = makeButton(parent, "TEST RUN", 200, 26, onTestRun);
    lv_obj_set_pos(run, 226, 172);
}

static void updateAuton(const Snapshot&) {
    bool locked = selectionLocked();
    int sel = selectedIndex();
    if (sel == s_shown && locked == s_wasLocked) return;

    uint32_t n = lv_obj_get_child_cnt(s_list);
    for (uint32_t i = 0; i < n; i++) styleRow(lv_obj_get_child(s_list, i), (int)i == sel, locked && (int)i != sel);
    if (sel != s_shown && sel >= 0) lv_obj_scroll_to_view(lv_obj_get_child(s_list, sel), LV_ANIM_ON);

    s_shown = sel;
    s_wasLocked = locked;
    if (locked) lv_obj_clear_flag(s_lock, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(s_lock, LV_OBJ_FLAG_HIDDEN);

    const Auton* a = selectedAuton();
    if (!a) {
        lv_label_set_text_static(s_name, "none");
        return;
    }
    lv_label_set_text_static(s_name, a->name);
    lv_label_set_text_static(s_side, allianceName(a->alliance));
    lv_obj_set_style_text_color(s_side, allianceColor(a->alliance), 0);
    lv_label_set_text_static(s_desc, a->description ? a->description : "");
}

static void teardownAuton() { s_list = s_name = s_side = s_desc = s_lock = nullptr; }

const Page pageAuton = {"AUTON", LV_SYMBOL_LIST, buildAuton, updateAuton, teardownAuton};

} // namespace ui
