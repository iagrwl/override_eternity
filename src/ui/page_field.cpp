// FIELD page: live odometry map with a fading path trail, pose readouts, and a
// tap-to-measure tool (tap the field to get distance + turn angle to that spot).

#include <cmath>
#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

// ---------------------------------------------------------------- trail recorder
static constexpr int kTrail = 240;
static float s_trailX[kTrail], s_trailY[kTrail];
static int s_trailHead = 0, s_trailCount = 0;
static float s_odometer = 0; // inches driven since last clear
static uint32_t s_trailVersion = 0;
static float s_speed = 0;    // in/s, smoothed
static float s_lastX = 0, s_lastY = 0;
static uint32_t s_lastT = 0;

void recordTrail(const Snapshot& s) {
    if (s_lastT) {
        float dt = (s.timeMs - s_lastT) / 1000.0f;
        if (dt > 0) {
            float v = hypotf(s.x - s_lastX, s.y - s_lastY) / dt;
            s_speed = s_speed * 0.6f + v * 0.4f;
        }
    }
    s_lastX = s.x, s_lastY = s.y, s_lastT = s.timeMs;

    if (s_trailCount) {
        int last = (s_trailHead + s_trailCount - 1) % kTrail;
        float d = hypotf(s.x - s_trailX[last], s.y - s_trailY[last]);
        if (d < 0.5f) return;
        if (d > 30) { // pose was reset, don't draw a line across the field
            clearTrail();
        } else {
            s_odometer += d;
        }
    }
    int idx = (s_trailHead + s_trailCount) % kTrail;
    s_trailX[idx] = s.x;
    s_trailY[idx] = s.y;
    if (s_trailCount < kTrail) s_trailCount++;
    else s_trailHead = (s_trailHead + 1) % kTrail;
    s_trailVersion++;
}

void clearTrail() {
    s_trailHead = s_trailCount = 0;
    s_odometer = 0;
    s_trailVersion++;
}

// ---------------------------------------------------------------- widgets
static lv_obj_t* s_field = nullptr;
static lv_obj_t* s_val[3] = {};
static lv_obj_t* s_info = nullptr;
static lv_obj_t* s_measure = nullptr;
static bool s_hasTarget = false;
static float s_tx = 0, s_ty = 0;
static float s_drawnX = 1e9, s_drawnY = 1e9, s_drawnT = 1e9;
static uint32_t s_drawnTrail = 0;

static void onFieldDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_field, &a);
    drawField(ctx, a);

    // trail, older segments fade out
    lv_coord_t px0 = 0, py0 = 0;
    for (int i = 0; i < s_trailCount; i++) {
        int idx = (s_trailHead + i) % kTrail;
        lv_coord_t px, py;
        fieldToPx(a, s_trailX[idx], s_trailY[idx], px, py);
        if (i) {
            lv_opa_t opa = (lv_opa_t)(40 + 215 * i / s_trailCount);
            drawLine(ctx, px0, py0, px, py, color::accent2(), 1, opa);
        }
        px0 = px, py0 = py;
    }

    const Snapshot& s = snapshot();
    if (s_hasTarget) {
        lv_coord_t rx, ry, tx, ty;
        fieldToPx(a, s.x, s.y, rx, ry);
        fieldToPx(a, s_tx, s_ty, tx, ty);
        drawLine(ctx, rx, ry, tx, ty, color::warn(), 1, LV_OPA_70);
        drawArc(ctx, tx, ty, 5, 0, 360, color::warn(), 2);
        drawLine(ctx, tx - 9, ty, tx - 6, ty, color::warn(), 1);
        drawLine(ctx, tx + 6, ty, tx + 9, ty, color::warn(), 1);
        drawLine(ctx, tx, ty - 9, tx, ty - 6, color::warn(), 1);
        drawLine(ctx, tx, ty + 6, tx, ty + 9, color::warn(), 1);
    }
    drawRobot(ctx, a, s.x, s.y, s.theta, config::kRobotSizeIn, color::text());
}

static void onFieldTap(lv_event_t*) {
    lv_point_t p;
    lv_indev_get_point(lv_indev_get_act(), &p);
    lv_area_t a;
    lv_obj_get_coords(s_field, &a);
    float scale = 144.0f / lv_area_get_width(&a);
    float x = (p.x - a.x1) * scale - 72, y = 72 - (p.y - a.y1) * scale;
    // second tap near the marker clears it
    if (s_hasTarget && hypotf(x - s_tx, y - s_ty) < 6) s_hasTarget = false;
    else s_hasTarget = true, s_tx = x, s_ty = y;
    lv_obj_invalidate(s_field);
}

static void onZero(lv_event_t*) {
    hw::zeroPose();
    clearTrail();
    toast("pose zeroed", color::good());
}
static void onWallL(lv_event_t*) { hw::wallReset(false); }
static void onWallR(lv_event_t*) { hw::wallReset(true); }
static void onClear(lv_event_t*) {
    clearTrail();
    s_hasTarget = false;
}

static void buildField(lv_obj_t* parent) {
    s_drawnX = 1e9;
    s_field = makeBox(parent, 0, 0, 206, 206);
    lv_obj_add_flag(s_field, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_field, onFieldDraw, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_add_event_cb(s_field, onFieldTap, LV_EVENT_CLICKED, nullptr);

    // three plain readouts, no boxes
    static const char* caps[3] = {"X", "Y", "HEADING"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t* cap = lv_label_create(parent);
        lv_obj_add_style(cap, &styles().caption, 0);
        lv_label_set_text_static(cap, caps[i]);
        lv_obj_set_pos(cap, 226 + i * 68, 2);
        s_val[i] = makeLabel(parent, &lv_font_montserrat_20, i == 2 ? color::accent2() : color::text(), "");
        lv_obj_set_pos(s_val[i], 226 + i * 68, 16);
    }

    s_info = makeLabel(parent, &lv_font_montserrat_10, color::muted(), "");
    lv_obj_set_style_text_line_space(s_info, 4, 0);
    lv_obj_set_pos(s_info, 226, 52);

    s_measure = makeLabel(parent, &lv_font_montserrat_10, color::warn(), "");
    lv_obj_set_style_text_line_space(s_measure, 4, 0);
    lv_obj_set_pos(s_measure, 226, 92);

    lv_obj_t* b;
    b = makeButton(parent, "ZERO", 96, 26, onZero);
    lv_obj_set_pos(b, 226, 146);
    b = makeButton(parent, "CLEAR PATH", 96, 26, onClear);
    lv_obj_set_pos(b, 330, 146);
    b = makeButton(parent, LV_SYMBOL_LEFT " WALL", 96, 26, onWallL);
    lv_obj_set_pos(b, 226, 180);
    b = makeButton(parent, "WALL " LV_SYMBOL_RIGHT, 96, 26, onWallR);
    lv_obj_set_pos(b, 330, 180);
}

static void updateField(const Snapshot& s) {
    setTextf(s_val[0], "%.1f", s.x);
    setTextf(s_val[1], "%.1f", s.y);
    setTextf(s_val[2], "%.1f\xC2\xB0", s.theta);
    setTextf(s_info, "%.1f in/s     %.0f in driven\nL %d   R %d rpm", s_speed, s_odometer, (int)s.leftRpm,
             (int)s.rightRpm);

    if (s_hasTarget) {
        float dx = s_tx - s.x, dy = s_ty - s.y;
        float turn = atan2f(dx, dy) * 180.0f / (float)M_PI - s.theta;
        turn = fmodf(turn + 540.0f, 360.0f) - 180.0f;
        setTextf(s_measure, "TARGET  %.0f, %.0f\n%.1f in   turn %+.0f\xC2\xB0", s_tx, s_ty,
                              hypotf(dx, dy), turn);
    } else {
        lv_label_set_text_static(s_measure, "tap field to measure");
        lv_obj_set_style_text_color(s_measure, color::dim(), 0);
    }
    if (s_hasTarget) lv_obj_set_style_text_color(s_measure, color::warn(), 0);

    // only redraw the map when something visibly moved
    if (fabsf(s.x - s_drawnX) > 0.2f || fabsf(s.y - s_drawnY) > 0.2f || fabsf(s.theta - s_drawnT) > 0.5f ||
        s_trailVersion != s_drawnTrail) {
        s_drawnX = s.x, s_drawnY = s.y, s_drawnT = s.theta, s_drawnTrail = s_trailVersion;
        lv_obj_invalidate(s_field);
    }
}

static void teardownField() {
    s_field = s_info = s_measure = nullptr;
    s_val[0] = s_val[1] = s_val[2] = nullptr;
}

const Page pageField = {"FIELD", LV_SYMBOL_GPS, buildField, updateField, teardownField};

} // namespace ui
