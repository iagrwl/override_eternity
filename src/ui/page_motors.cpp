// MOTORS page: a plain two-column list - is each motor plugged in, its port, speed
// and power. (no temperatures: the motor temp readings aren't trustworthy)

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_list = nullptr;
static lv_obj_t* s_summary = nullptr;

static constexpr int kRows = 6; // per column
static constexpr lv_coord_t kRowH = 30, kColW = 206, kColGap = 14;

static void onListDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_list, &a);
    const Snapshot& s = snapshot();

    for (int i = 0; i < s.motorCount; i++) {
        const MotorStat& m = s.motors[i];
        lv_coord_t x = a.x1 + (i / kRows) * (kColW + kColGap);
        lv_coord_t y = a.y1 + (i % kRows) * kRowH;
        char buf[24];

        drawRect(ctx, area(x, y + 12, 6, 6), m.ok ? color::good() : color::bad(), LV_OPA_COVER, 3);
        drawText(ctx, x + 16, y + 7, 90, m.name, &lv_font_montserrat_14, m.ok ? color::text() : color::bad());
        snprintf(buf, sizeof buf, "P%d", m.port < 0 ? -m.port : m.port);
        drawText(ctx, x + 98, y + 9, 30, buf, &lv_font_montserrat_10, color::dim());
        if (m.ok) {
            snprintf(buf, sizeof buf, "%d", (int)m.rpm);
            drawText(ctx, x + 120, y + 7, 52, buf, &lv_font_montserrat_14, color::text(), LV_TEXT_ALIGN_RIGHT);
            snprintf(buf, sizeof buf, "%.0fW", m.watts);
            drawText(ctx, x + 172, y + 9, 34, buf, &lv_font_montserrat_10, color::muted(), LV_TEXT_ALIGN_RIGHT);
        } else {
            drawText(ctx, x + 120, y + 9, 86, "UNPLUGGED", &lv_font_montserrat_10, color::bad(), LV_TEXT_ALIGN_RIGHT);
        }
        drawLine(ctx, x, y + kRowH - 1, x + kColW, y + kRowH - 1, color::line(), 1);
    }
}

static void buildMotors(lv_obj_t* parent) {
    lv_obj_t* cap = lv_label_create(parent);
    lv_obj_add_style(cap, &styles().caption, 0);
    lv_label_set_text_static(cap, "MOTORS");
    lv_obj_set_pos(cap, 0, 2);
    lv_obj_t* rpm = lv_label_create(parent);
    lv_obj_add_style(rpm, &styles().caption, 0);
    lv_label_set_text_static(rpm, "RPM");
    lv_obj_set_pos(rpm, 152, 2);

    s_summary = makeLabel(parent, &lv_font_montserrat_10, color::muted(), "");
    lv_obj_align(s_summary, LV_ALIGN_TOP_RIGHT, 0, 2);

    s_list = makeBox(parent, 0, 20, kContentW, kRows * kRowH);
    lv_obj_add_event_cb(s_list, onListDraw, LV_EVENT_DRAW_MAIN, nullptr);
}

static void updateMotors(const Snapshot& s) {
    static uint32_t n = 0;
    if (n++ % 4) return; // 5 Hz is plenty for a list
    float total = 0;
    int dead = 0;
    for (int i = 0; i < s.motorCount; i++) {
        if (!s.motors[i].ok) dead++;
        else total += s.motors[i].watts;
    }
    if (dead) setTextf(s_summary, "%d UNPLUGGED", dead);
    else setTextf(s_summary, "ALL CONNECTED   %.0f W", total);
    lv_obj_set_style_text_color(s_summary, dead ? color::bad() : color::muted(), 0);
    lv_obj_invalidate(s_list);
}

static void teardownMotors() { s_list = s_summary = nullptr; }

const Page pageMotors = {"MOTORS", LV_SYMBOL_CHARGE, buildMotors, updateMotors, teardownMotors};

} // namespace ui
