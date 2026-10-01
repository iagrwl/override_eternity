// AUTON page: card grid of routines + detail panel with a start-pose preview.

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_list = nullptr;
static lv_obj_t* s_name = nullptr;
static lv_obj_t* s_chip = nullptr;
static lv_obj_t* s_chipLabel = nullptr;
static lv_obj_t* s_desc = nullptr;
static lv_obj_t* s_preview = nullptr;
static lv_obj_t* s_startLabel = nullptr;
static lv_obj_t* s_lock = nullptr;
static int s_shown = -2;
static bool s_wasLocked = false;

static void styleCard(lv_obj_t* card, bool selected) {
    lv_obj_set_style_bg_color(card, selected ? color::accentDim() : color::panel(), 0);
    lv_obj_set_style_outline_width(card, selected ? 2 : 0, 0);
}

static void onCard(lv_event_t* e) {
    if (selectionLocked()) {
        toast(LV_SYMBOL_CLOSE "  locked during match", color::bad());
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
    snprintf(body, sizeof body, "\"%s\" will run on the brain. Robot will move. Stand clear.", a->name);
    confirm("Run auton?", body, LV_SYMBOL_PLAY " RUN", runSelected);
}

static void onPreviewDraw(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target(e);
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    drawField(ctx, a);
    const Auton* au = selectedAuton();
    if (au && au->hasStart) drawRobot(ctx, a, au->startX, au->startY, au->startTheta, 18, allianceColor(au->alliance));
}

static void buildAuton(lv_obj_t* parent) {
    s_shown = -2;
    const auto& list = autons();

    // ---- card grid
    s_list = makeBox(parent, 0, 0, 270, 206);
    lv_obj_add_flag(s_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(s_list, 6, 0);
    lv_obj_set_style_pad_column(s_list, 6, 0);
    lv_obj_set_style_pad_all(s_list, 2, 0);

    for (size_t i = 0; i < list.size(); i++) {
        const Auton& a = list[i];
        lv_obj_t* card = lv_obj_create(s_list);
        lv_obj_remove_style_all(card);
        lv_obj_add_style(card, &styles().panel, 0);
        lv_obj_add_style(card, &styles().btnPressed, LV_STATE_PRESSED);
        lv_obj_set_style_outline_color(card, color::accent(), 0);
        lv_obj_set_style_outline_pad(card, 0, 0);
        lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
        lv_obj_set_style_border_width(card, 4, 0);
        lv_obj_set_style_border_color(card, allianceColor(a.alliance), 0);
        lv_obj_set_size(card, 127, 52);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(card, onCard, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lv_obj_t* name = makeLabel(card, &lv_font_montserrat_14, color::text(), a.name);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, 110);
        lv_obj_set_pos(name, 10, 8);

        lv_obj_t* tag = makeLabel(card, &lv_font_montserrat_10, allianceColor(a.alliance), allianceName(a.alliance));
        lv_obj_set_style_text_letter_space(tag, 1, 0);
        lv_obj_set_pos(tag, 10, 30);
        styleCard(card, (int)i == selectedIndex());
    }

    // ---- detail panel
    lv_obj_t* panel = lv_obj_create(parent);
    lv_obj_remove_style_all(panel);
    lv_obj_add_style(panel, &styles().panel, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, 276, 2);
    lv_obj_set_size(panel, 150, 204);

    lv_obj_t* cap = lv_label_create(panel);
    lv_obj_add_style(cap, &styles().caption, 0);
    lv_label_set_text_static(cap, "SELECTED AUTON");
    lv_obj_set_pos(cap, 10, 8);

    s_lock = makeLabel(panel, &lv_font_montserrat_12, color::warn(), LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_pos(s_lock, 128, 6);

    s_name = makeLabel(panel, &lv_font_montserrat_18, color::text(), "");
    lv_label_set_long_mode(s_name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_name, 132);
    lv_obj_set_pos(s_name, 10, 22);

    s_chip = lv_obj_create(panel);
    lv_obj_remove_style_all(s_chip);
    lv_obj_add_style(s_chip, &styles().chip, 0);
    lv_obj_set_size(s_chip, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_pos(s_chip, 10, 46);
    s_chipLabel = makeLabel(s_chip, &lv_font_montserrat_10, lv_color_white(), "");

    s_desc = makeLabel(panel, &lv_font_montserrat_12, color::muted(), "");
    lv_label_set_long_mode(s_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_desc, 132);
    lv_obj_set_height(s_desc, 46);
    lv_obj_set_pos(s_desc, 10, 66);

    s_preview = makeBox(panel, 10, 114, 54, 54);
    lv_obj_add_event_cb(s_preview, onPreviewDraw, LV_EVENT_DRAW_MAIN, nullptr);

    s_startLabel = makeLabel(panel, &lv_font_montserrat_10, color::muted(), "");
    lv_obj_set_pos(s_startLabel, 72, 118);

    lv_obj_t* run = makeButton(panel, LV_SYMBOL_PLAY "  TEST RUN", 130, 26, onTestRun);
    lv_obj_set_style_text_font(run, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(run, 10, 172);
}

static void updateAuton(const Snapshot&) {
    bool locked = selectionLocked();
    int sel = selectedIndex();
    if (sel == s_shown && locked == s_wasLocked) return;

    // restyle the cards
    uint32_t n = lv_obj_get_child_cnt(s_list);
    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t* card = lv_obj_get_child(s_list, i);
        styleCard(card, (int)i == sel);
        lv_obj_set_style_opa(card, locked && (int)i != sel ? LV_OPA_40 : LV_OPA_COVER, 0);
    }
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
    lv_label_set_text_static(s_chipLabel, allianceName(a->alliance));
    lv_obj_set_style_bg_color(s_chip, allianceColor(a->alliance), 0);
    lv_label_set_text_static(s_desc, a->description ? a->description : "");
    if (a->hasStart)
        setTextf(s_startLabel, "START\nx  %.0f\ny  %.0f\n" "\xC2\xB0" "  %.0f", a->startX, a->startY,
                              a->startTheta);
    else lv_label_set_text_static(s_startLabel, "START\nnot set");
    lv_obj_invalidate(s_preview);
}

static void teardownAuton() { s_list = s_name = s_chip = s_chipLabel = s_desc = s_preview = s_startLabel = s_lock = nullptr; }

const Page pageAuton = {"AUTON", LV_SYMBOL_LIST, buildAuton, updateAuton, teardownAuton};

} // namespace ui
