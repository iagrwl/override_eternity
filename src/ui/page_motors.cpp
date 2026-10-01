// MOTORS page: one custom-drawn tile per motor (temp gauge, rpm, watts, port),
// drawn into a single object to keep LVGL memory low.

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_grid = nullptr;
static lv_obj_t* s_summary = nullptr;

static constexpr int kCols = 5;
static constexpr lv_coord_t kGap = 6;

static void onGridDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_grid, &a);
    const Snapshot& s = snapshot();

    int rows = (s.motorCount + kCols - 1) / kCols;
    if (rows < 1) rows = 1;
    lv_coord_t w = (lv_area_get_width(&a) - kGap * (kCols - 1)) / kCols;
    lv_coord_t h = (lv_area_get_height(&a) - kGap * (rows - 1)) / rows;

    for (int i = 0; i < s.motorCount; i++) {
        const MotorStat& m = s.motors[i];
        lv_coord_t x = a.x1 + (i % kCols) * (w + kGap);
        lv_coord_t y = a.y1 + (i / kCols) * (h + kGap);
        lv_area_t tile = area(x, y, w, h);

        float temp = displayTemp(m.tempC);
        lv_color_t heat = m.ok ? heatColor(temp, config::kMotorTempCool, config::kMotorTempHot) : color::bad();
        drawRect(ctx, tile, color::panel(), LV_OPA_COVER, 8);
        drawFrame(ctx, tile, m.ok ? color::line() : color::bad(), 1, 8);

        char buf[24];
        drawText(ctx, x + 8, y + 6, w - 16, m.name, &lv_font_montserrat_12, color::text());
        snprintf(buf, sizeof buf, "P%d", m.port < 0 ? -m.port : m.port);
        drawText(ctx, x + 8, y + 7, w - 16, buf, &lv_font_montserrat_10, color::dim(), LV_TEXT_ALIGN_RIGHT);

        // temperature arc gauge (VEX motors throttle at 55C)
        lv_coord_t cx = x + w / 2, cy = y + 44, r = 19;
        drawArc(ctx, cx, cy, r, 135, 45, color::panel2(), 5);
        if (m.ok) {
            float t = (m.tempC - 20) / 50.0f; // 20-70 C gauge span
            if (t < 0.02f) t = 0.02f;
            if (t > 1) t = 1;
            drawArc(ctx, cx, cy, r, 135, (uint16_t)(135 + 270 * t) % 360, heat, 5);
            snprintf(buf, sizeof buf, "%d\xC2\xB0", (int)temp); // unit is in the summary line
        } else {
            snprintf(buf, sizeof buf, "--");
        }
        drawText(ctx, cx - 30, cy - 8, 60, buf, &lv_font_montserrat_14, m.ok ? heat : color::bad(),
                 LV_TEXT_ALIGN_CENTER);

        if (m.ok) {
            snprintf(buf, sizeof buf, "%d rpm", (int)m.rpm);
            drawText(ctx, x + 4, y + h - 30, w - 8, buf, &lv_font_montserrat_10, color::muted(),
                     LV_TEXT_ALIGN_CENTER);
            snprintf(buf, sizeof buf, "%.1fW  %.1fA", m.watts, m.amps);
            drawText(ctx, x + 4, y + h - 17, w - 8, buf, &lv_font_montserrat_10, color::dim(),
                     LV_TEXT_ALIGN_CENTER);
        } else {
            drawText(ctx, x + 4, y + h - 24, w - 8, "UNPLUGGED", &lv_font_montserrat_10, color::bad(),
                     LV_TEXT_ALIGN_CENTER);
        }
    }
}

static void buildMotors(lv_obj_t* parent) {
    s_summary = makeLabel(parent, &lv_font_montserrat_12, color::muted(), "");
    lv_obj_set_pos(s_summary, 2, 0);
    s_grid = makeBox(parent, 0, 20, 426, 186);
    lv_obj_add_event_cb(s_grid, onGridDraw, LV_EVENT_DRAW_MAIN, nullptr);
}

static void updateMotors(const Snapshot& s) {
    float hottest = 0, total = 0;
    int hotIdx = -1, dead = 0;
    for (int i = 0; i < s.motorCount; i++) {
        const MotorStat& m = s.motors[i];
        if (!m.ok) {
            dead++;
            continue;
        }
        total += m.watts;
        if (m.tempC > hottest) hottest = m.tempC, hotIdx = i;
    }
    setTextf(s_summary, "%d motors   %d unplugged   %.0f W total   hottest %s %d%s", s.motorCount, dead, total,
             hotIdx >= 0 ? s.motors[hotIdx].name : "-", (int)displayTemp(hottest), tempUnit());
    lv_obj_set_style_text_color(s_summary, dead ? color::bad() : hottest >= 55 /* C, motors throttle here */ ? color::warn() : color::muted(), 0);
    lv_obj_invalidate(s_grid);
}

static void teardownMotors() { s_grid = s_summary = nullptr; }

const Page pageMotors = {"MOTORS", LV_SYMBOL_CHARGE, buildMotors, updateMotors, teardownMotors};

} // namespace ui
