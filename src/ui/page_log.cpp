// LOG page: ui::log() / console.printf() output, newest at the bottom.

#include "ui_internal.hpp"

namespace ui {

static char s_text[48 * 74];
static lv_obj_t* s_box = nullptr;
static lv_obj_t* s_label = nullptr;
static lv_obj_t* s_count = nullptr;
static uint32_t s_shownVersion = 0;

static void onClear(lv_event_t*) { logClear(); }

static void buildLog(lv_obj_t* parent) {
    s_shownVersion = logVersion() - 1;

    s_box = lv_obj_create(parent);
    lv_obj_remove_style_all(s_box);
    lv_obj_set_style_pad_all(s_box, 0, 0);
    lv_obj_set_pos(s_box, 0, 0);
    lv_obj_set_size(s_box, 426, 166);
    lv_obj_set_scroll_dir(s_box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_box, LV_SCROLLBAR_MODE_ACTIVE);

    s_label = makeLabel(s_box, &lv_font_montserrat_12, color::text(), "");
    lv_obj_set_width(s_label, 420);
    lv_label_set_long_mode(s_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(s_label, 2, 0);

    s_count = makeLabel(parent, &lv_font_montserrat_12, color::muted(), "");
    lv_obj_set_pos(s_count, 4, 180);

    lv_obj_t* b = makeButton(parent, "CLEAR", 80, 24, onClear);
    lv_obj_set_pos(b, 346, 174);
}

static void updateLog(const Snapshot&) {
    uint32_t v = logVersion();
    if (v == s_shownVersion) return;
    s_shownVersion = v;
    int lines = logCopy(s_text, sizeof s_text);
    lv_label_set_text_static(s_label, lines ? s_text : "nothing logged yet");
    setTextf(s_count, "%d lines   (console.printf / ui::log)", lines);
    lv_obj_update_layout(s_box);
    lv_obj_scroll_to_y(s_box, LV_COORD_MAX, LV_ANIM_OFF);
}

static void teardownLog() { s_box = s_label = s_count = nullptr; }

const Page pageLog = {"LOG", LV_SYMBOL_FILE, buildLog, updateLog, teardownLog};

} // namespace ui
