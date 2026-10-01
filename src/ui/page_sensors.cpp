// SENSORS page: IMU compass, sensor/subsystem readouts, and a 21-port health map
// (tap a port to see what's supposed to be on it).

#include <cmath>
#include <cstdio>
#include <cstring>
#include "ui_internal.hpp"

namespace ui {

static lv_obj_t* s_compass = nullptr;
static lv_obj_t* s_rows = nullptr;
static lv_obj_t* s_ports = nullptr;
static float s_drawnHeading = 1e9;

static void onCompassDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_compass, &a);
    const Snapshot& s = snapshot();

    drawRect(ctx, a, color::panel(), LV_OPA_COVER, 8);
    drawFrame(ctx, a, color::line(), 1, 8);
    drawText(ctx, a.x1 + 10, a.y1 + 8, 120, "INERTIAL", &lv_font_montserrat_10, color::muted());
    drawText(ctx, a.x1 + 10, a.y1 + 8, lv_area_get_width(&a) - 20, s.imuCalibrating ? "CAL..." : s.imuOk ? "OK" : "MISSING",
             &lv_font_montserrat_10, s.imuCalibrating ? color::warn() : s.imuOk ? color::good() : color::bad(),
             LV_TEXT_ALIGN_RIGHT);

    lv_coord_t cx = (a.x1 + a.x2) / 2, cy = a.y1 + 92;
    const int r = 58;
    drawArc(ctx, cx, cy, r, 0, 360, color::panel2(), 10);

    // ticks every 30 degrees, rotate with the robot so "up" is where it's facing
    for (int d = 0; d < 360; d += 30) {
        float t = (d - s.imuHeading) * (float)M_PI / 180.0f;
        float sx = sinf(t), sy = -cosf(t);
        int len = d % 90 == 0 ? 8 : 4;
        drawLine(ctx, cx + sx * (r - 5), cy + sy * (r - 5), cx + sx * (r - 5 - len), cy + sy * (r - 5 - len),
                 d % 90 == 0 ? color::text() : color::dim(), 2);
        if (d % 90 == 0) {
            static const char* names[4] = {"N", "E", "S", "W"};
            drawText(ctx, cx + sx * (r - 22) - 10, cy + sy * (r - 22) - 7, 20, names[d / 90], &lv_font_montserrat_12,
                     d == 0 ? color::bad() : color::muted(), LV_TEXT_ALIGN_CENTER);
        }
    }
    // fixed forward pointer at the top
    lv_point_t tri[3] = {{cx, (lv_coord_t)(cy - r - 2)}, {(lv_coord_t)(cx - 6), (lv_coord_t)(cy - r + 9)},
                         {(lv_coord_t)(cx + 6), (lv_coord_t)(cy - r + 9)}};
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = color::accent2();
    lv_draw_polygon(ctx, &d, tri, 3);

    char buf[32];
    snprintf(buf, sizeof buf, "%.1f\xC2\xB0", s.imuHeading);
    drawText(ctx, cx - 50, cy - 12, 100, buf, &lv_font_montserrat_20, color::text(), LV_TEXT_ALIGN_CENTER);
    snprintf(buf, sizeof buf, "pitch %.1f   roll %.1f", s.imuPitch, s.imuRoll);
    drawText(ctx, a.x1 + 4, a.y2 - 20, lv_area_get_width(&a) - 8, buf, &lv_font_montserrat_10, color::muted(),
             LV_TEXT_ALIGN_CENTER);
}

static void row(lv_draw_ctx_t* ctx, const lv_area_t& a, int i, const char* name, bool ok, const char* value,
                lv_color_t valueCol) {
    lv_coord_t y = a.y1 + 6 + i * 18;
    drawRect(ctx, area(a.x1 + 10, y + 5, 6, 6), ok ? color::good() : color::bad(), LV_OPA_COVER, 3);
    drawText(ctx, a.x1 + 24, y, 110, name, &lv_font_montserrat_12, color::muted());
    drawText(ctx, a.x1 + 110, y, lv_area_get_width(&a) - 120, value, &lv_font_montserrat_12, valueCol,
             LV_TEXT_ALIGN_RIGHT);
}

static void onRowsDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_rows, &a);
    const Snapshot& s = snapshot();
    drawRect(ctx, a, color::panel(), LV_OPA_COVER, 8);
    drawFrame(ctx, a, color::line(), 1, 8);

    char buf[48];
    snprintf(buf, sizeof buf, "%.1f\xC2\xB0", s.vertDeg);
    row(ctx, a, 0, "Vertical wheel", s.vertOk, s.vertOk ? buf : "--", color::text());
    snprintf(buf, sizeof buf, "%.1f\xC2\xB0", s.horizDeg);
    row(ctx, a, 1, "Horizontal wheel", s.horizOk, s.horizOk ? buf : "--", color::text());
    snprintf(buf, sizeof buf, "%d mm  /  %.1f in", s.leftDistMm, s.leftDistMm / 25.4);
    row(ctx, a, 2, "Left distance", s.leftDistOk, s.leftDistOk ? buf : "--", color::text());
    snprintf(buf, sizeof buf, "%d mm  /  %.1f in", s.rightDistMm, s.rightDistMm / 25.4);
    row(ctx, a, 3, "Right distance", s.rightDistOk, s.rightDistOk ? buf : "--", color::text());
    snprintf(buf, sizeof buf, "%.0f\xC2\xB0", s.liftDeg);
    row(ctx, a, 4, "Lift", true, buf, color::text());
    row(ctx, a, 5, "Intake", true, s.intake, strcmp(s.intake, "OFF") ? color::accent2() : color::muted());
    row(ctx, a, 6, "Claw", true, s.clawOpen ? "OPEN" : "CLOSED", s.clawOpen ? color::warn() : color::muted());
    snprintf(buf, sizeof buf, "%.2fV  %.1fA  %.0f%s", s.batteryV, s.batteryA, displayTemp(s.batteryTemp), tempUnit());
    row(ctx, a, 7, "Battery", s.batteryPct > 20, buf, color::text());
}

// ---- port map
static constexpr int kPortCols = 11;

static void portInfo(const Snapshot& s, int port, int& expected, int& bad) {
    expected = bad = 0;
    for (int i = 0; i < s.deviceCount; i++) {
        const DeviceStat& d = s.devices[i];
        if (d.port != port) continue;
        expected++;
        if (!d.ok) bad++;
    }
}

static void onPortsDraw(lv_event_t* e) {
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(s_ports, &a);
    const Snapshot& s = snapshot();

    drawText(ctx, a.x1 + 2, a.y1, 200, "PORTS", &lv_font_montserrat_10, color::muted());
    char buf[48];
    snprintf(buf, sizeof buf, s.deviceProblems ? "%d problem%s - tap a port" : "all devices OK", s.deviceProblems,
             s.deviceProblems == 1 ? "" : "s");
    drawText(ctx, a.x1, a.y1, lv_area_get_width(&a) - 2, buf, &lv_font_montserrat_10,
             s.deviceProblems ? color::bad() : color::good(), LV_TEXT_ALIGN_RIGHT);

    lv_coord_t cell = (lv_area_get_width(&a) - 3 * (kPortCols - 1)) / kPortCols;
    for (int p = 1; p <= 21; p++) {
        int expected, bad;
        portInfo(s, p, expected, bad);
        lv_coord_t x = a.x1 + ((p - 1) % kPortCols) * (cell + 3);
        lv_coord_t y = a.y1 + 14 + ((p - 1) / kPortCols) * 17;
        lv_area_t c = area(x, y, cell, 15);
        lv_color_t col = !expected ? color::panel2() : bad ? color::bad() : expected > 1 ? color::warn() : color::good();
        drawRect(ctx, c, col, expected ? LV_OPA_COVER : LV_OPA_60, 4);
        snprintf(buf, sizeof buf, "%d", p);
        drawText(ctx, x, y + 1, cell, buf, &lv_font_montserrat_10, expected ? lv_color_black() : color::dim(),
                 LV_TEXT_ALIGN_CENTER);
    }
}

static void onPortTap(lv_event_t*) {
    lv_point_t pt;
    lv_indev_get_point(lv_indev_get_act(), &pt);
    lv_area_t a;
    lv_obj_get_coords(s_ports, &a);
    lv_coord_t cell = (lv_area_get_width(&a) - 3 * (kPortCols - 1)) / kPortCols;
    int col = (pt.x - a.x1) / (cell + 3), rowI = (pt.y - a.y1 - 14) / 17;
    if (pt.y < a.y1 + 14 || col < 0 || col >= kPortCols || rowI < 0 || rowI > 1) return;
    int port = rowI * kPortCols + col + 1;
    if (port > 21) return;

    const Snapshot& s = snapshot();
    char msg[96];
    int n = snprintf(msg, sizeof msg, "PORT %d: ", port);
    bool any = false, bad = false;
    for (int i = 0; i < s.deviceCount && n < (int)sizeof msg; i++) {
        const DeviceStat& d = s.devices[i];
        if (d.port != port) continue;
        n += snprintf(msg + n, sizeof msg - n, "%s%s%s%s", any ? " + " : "", d.name, d.ok ? "" : " - ",
                      d.ok ? "" : d.problem);
        any = true;
        bad |= !d.ok;
    }
    if (!any) snprintf(msg + n, sizeof msg - n, "unused");
    toast(msg, bad ? color::bad() : any ? color::accent() : color::panel2());
}

static void buildSensors(lv_obj_t* parent) {
    s_drawnHeading = 1e9;
    s_compass = makeBox(parent, 0, 0, 146, 206);
    lv_obj_add_event_cb(s_compass, onCompassDraw, LV_EVENT_DRAW_MAIN, nullptr);

    s_rows = makeBox(parent, 152, 0, 274, 154);
    lv_obj_add_event_cb(s_rows, onRowsDraw, LV_EVENT_DRAW_MAIN, nullptr);

    s_ports = makeBox(parent, 152, 158, 274, 48);
    lv_obj_add_flag(s_ports, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_ports, onPortsDraw, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_add_event_cb(s_ports, onPortTap, LV_EVENT_CLICKED, nullptr);
}

static void updateSensors(const Snapshot& s) {
    if (fabsf(s.imuHeading - s_drawnHeading) > 0.2f) {
        s_drawnHeading = s.imuHeading;
        lv_obj_invalidate(s_compass);
    }
    static uint32_t n = 0;
    if (n++ % 4 == 0) { // rows and ports at ~5Hz
        lv_obj_invalidate(s_rows);
        lv_obj_invalidate(s_ports);
    }
}

static void teardownSensors() { s_compass = s_rows = s_ports = nullptr; }

const Page pageSensors = {"SENSORS", LV_SYMBOL_EYE_OPEN, buildSensors, updateSensors, teardownSensors};

} // namespace ui
