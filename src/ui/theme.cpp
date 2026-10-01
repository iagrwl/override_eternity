#include <cmath>
#include <cstdarg>
#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

lv_color_t allianceColor(Alliance a) {
    switch (a) {
        case Alliance::Red: return color::red();
        case Alliance::Blue: return color::blue();
        case Alliance::Skills: return color::skills();
        default: return color::accent();
    }
}

const char* allianceName(Alliance a) {
    switch (a) {
        case Alliance::Red: return "RED";
        case Alliance::Blue: return "BLUE";
        case Alliance::Skills: return "SKILLS";
        default: return "ANY";
    }
}

lv_color_t heatColor(float v, float lo, float hi) {
    float t = (v - lo) / (hi - lo);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    if (t < 0.5f) return lv_color_mix(color::warn(), color::good(), (lv_opa_t)(t * 2 * 255));
    return lv_color_mix(color::bad(), color::warn(), (lv_opa_t)((t - 0.5f) * 2 * 255));
}

// ---------------------------------------------------------------- styles
static Styles s_styles;
Styles& styles() { return s_styles; }

void initStyles() {
    Styles& s = s_styles;

    lv_style_init(&s.screen);
    lv_style_set_bg_color(&s.screen, color::bg());
    lv_style_set_bg_opa(&s.screen, LV_OPA_COVER);
    lv_style_set_text_color(&s.screen, color::text());
    lv_style_set_text_font(&s.screen, &lv_font_montserrat_14);

    lv_style_init(&s.panel);
    lv_style_set_bg_color(&s.panel, color::panel());
    lv_style_set_bg_opa(&s.panel, LV_OPA_COVER);
    lv_style_set_radius(&s.panel, 8);
    lv_style_set_border_width(&s.panel, 1);
    lv_style_set_border_color(&s.panel, color::line());
    lv_style_set_pad_all(&s.panel, 0);
    lv_style_set_shadow_width(&s.panel, 0);

    lv_style_init(&s.chip);
    lv_style_set_bg_color(&s.chip, color::panel2());
    lv_style_set_bg_opa(&s.chip, LV_OPA_COVER);
    lv_style_set_radius(&s.chip, LV_RADIUS_CIRCLE);
    lv_style_set_pad_hor(&s.chip, 7);
    lv_style_set_pad_ver(&s.chip, 2);
    lv_style_set_border_width(&s.chip, 0);
    lv_style_set_text_font(&s.chip, &lv_font_montserrat_12);

    lv_style_init(&s.btn);
    lv_style_set_bg_color(&s.btn, color::panel2());
    lv_style_set_bg_opa(&s.btn, LV_OPA_COVER);
    lv_style_set_radius(&s.btn, 8);
    lv_style_set_border_width(&s.btn, 1);
    lv_style_set_border_color(&s.btn, color::line());
    lv_style_set_shadow_width(&s.btn, 0);
    lv_style_set_text_color(&s.btn, color::text());
    lv_style_set_text_font(&s.btn, &lv_font_montserrat_14);
    lv_style_set_pad_all(&s.btn, 2);

    lv_style_init(&s.btnPressed);
    lv_style_set_bg_color(&s.btnPressed, color::accent());
    lv_style_set_border_color(&s.btnPressed, color::accent());
    lv_style_set_transform_width(&s.btnPressed, -2);
    lv_style_set_transform_height(&s.btnPressed, -2);

    lv_style_init(&s.title);
    lv_style_set_text_font(&s.title, &lv_font_montserrat_24);
    lv_style_set_text_color(&s.title, color::text());

    lv_style_init(&s.caption);
    lv_style_set_text_font(&s.caption, &lv_font_montserrat_10);
    lv_style_set_text_color(&s.caption, color::muted());
    lv_style_set_text_letter_space(&s.caption, 1);

    lv_style_init(&s.noPad);
    lv_style_set_bg_opa(&s.noPad, LV_OPA_TRANSP);
    lv_style_set_border_width(&s.noPad, 0);
    lv_style_set_pad_all(&s.noPad, 0);
    lv_style_set_radius(&s.noPad, 0);
    lv_style_set_shadow_width(&s.noPad, 0);
}

lv_obj_t* makeLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t col, const char* text) {
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, col, 0);
    lv_label_set_text(l, text);
    return l;
}

void setTextf(lv_obj_t* label, const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    lv_label_set_text(label, buf);
}

lv_obj_t* makeBox(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h) {
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_add_style(o, &s_styles.noPad, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    return o;
}

lv_obj_t* makeButton(lv_obj_t* parent, const char* text, lv_coord_t w, lv_coord_t h, lv_event_cb_t cb,
                     void* user) {
    lv_obj_t* b = lv_btn_create(parent);
    lv_obj_remove_style_all(b);
    lv_obj_add_style(b, &s_styles.btn, 0);
    lv_obj_add_style(b, &s_styles.btnPressed, LV_STATE_PRESSED);
    lv_obj_set_size(b, w, h);
    lv_obj_t* l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, user);
    return b;
}

// ---------------------------------------------------------------- draw helpers
void drawRect(lv_draw_ctx_t* ctx, const lv_area_t& a, lv_color_t col, lv_opa_t opa, lv_coord_t radius) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = col;
    d.bg_opa = opa;
    d.radius = radius;
    lv_draw_rect(ctx, &d, &a);
}

void drawFrame(lv_draw_ctx_t* ctx, const lv_area_t& a, lv_color_t col, lv_coord_t width, lv_coord_t radius,
               lv_opa_t opa) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_TRANSP;
    d.border_color = col;
    d.border_width = width;
    d.border_opa = opa;
    d.radius = radius;
    lv_draw_rect(ctx, &d, &a);
}

void drawLine(lv_draw_ctx_t* ctx, lv_coord_t x1, lv_coord_t y1, lv_coord_t x2, lv_coord_t y2, lv_color_t col,
              lv_coord_t width, lv_opa_t opa) {
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.color = col;
    d.width = width;
    d.opa = opa;
    d.round_start = width > 2;
    d.round_end = width > 2;
    lv_point_t p1 = {x1, y1}, p2 = {x2, y2};
    lv_draw_line(ctx, &d, &p1, &p2);
}

void drawText(lv_draw_ctx_t* ctx, lv_coord_t x, lv_coord_t y, lv_coord_t w, const char* txt, const lv_font_t* font,
              lv_color_t col, lv_text_align_t align, lv_opa_t opa) {
    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.font = font;
    d.color = col;
    d.align = align;
    d.opa = opa;
    lv_area_t a = area(x, y, w, lv_font_get_line_height(font));
    lv_draw_label(ctx, &d, &a, txt, nullptr);
}

void drawArc(lv_draw_ctx_t* ctx, lv_coord_t cx, lv_coord_t cy, uint16_t r, uint16_t start, uint16_t end,
             lv_color_t col, lv_coord_t width, lv_opa_t opa) {
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.color = col;
    d.width = width;
    d.opa = opa;
    d.rounded = 1;
    lv_point_t c = {cx, cy};
    lv_draw_arc(ctx, &d, &c, r, start, end);
}

// ---------------------------------------------------------------- field
void fieldToPx(const lv_area_t& a, float x, float y, lv_coord_t& px, lv_coord_t& py) {
    float size = lv_area_get_width(&a);
    float scale = size / 144.0f;
    px = (lv_coord_t)lroundf(a.x1 + size / 2 + x * scale);
    py = (lv_coord_t)lroundf(a.y1 + size / 2 - y * scale);
}

void drawField(lv_draw_ctx_t* ctx, const lv_area_t& a) {
    lv_coord_t size = lv_area_get_width(&a);
    drawRect(ctx, a, lv_color_mix(color::panel(), color::bg(), 100), LV_OPA_COVER, 4);

    // 6x6 foam tiles in a subtle checker
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 6; c++) {
            lv_coord_t x1 = a.x1 + c * size / 6, x2 = a.x1 + (c + 1) * size / 6 - 1;
            lv_coord_t y1 = a.y1 + r * size / 6, y2 = a.y1 + (r + 1) * size / 6 - 1;
            if ((r + c) % 2) drawRect(ctx, {x1, y1, x2, y2}, lv_color_mix(color::panel2(), color::bg(), 200));
        }
    }
    for (int i = 1; i < 6; i++) {
        lv_coord_t o = i * size / 6;
        drawLine(ctx, a.x1 + o, a.y1, a.x1 + o, a.y2, color::line(), 1, LV_OPA_60);
        drawLine(ctx, a.x1, a.y1 + o, a.x2, a.y1 + o, color::line(), 1, LV_OPA_60);
    }
    // midline
    lv_coord_t cx = a.x1 + size / 2;
    drawLine(ctx, cx, a.y1 + 2, cx, a.y2 - 2, color::text(), 1, LV_OPA_30);
    // alliance walls
    drawRect(ctx, {a.x1, a.y1, (lv_coord_t)(a.x1 + 2), a.y2}, color::red(), LV_OPA_80);
    drawRect(ctx, {(lv_coord_t)(a.x2 - 2), a.y1, a.x2, a.y2}, color::blue(), LV_OPA_80);
    drawFrame(ctx, a, color::line(), 1, 4);
}

void drawRobot(lv_draw_ctx_t* ctx, const lv_area_t& a, float x, float y, float thetaDeg, float sizeIn,
               lv_color_t col) {
    float scale = lv_area_get_width(&a) / 144.0f;
    lv_coord_t cx, cy;
    fieldToPx(a, x, y, cx, cy);
    float t = thetaDeg * (float)M_PI / 180.0f;
    // screen-space forward/right unit vectors (lemlib: 0 deg = +y, clockwise positive)
    float fx = sinf(t), fy = -cosf(t);
    float rx = cosf(t), ry = sinf(t);
    float h = sizeIn * scale / 2;

    lv_point_t body[4];
    const float corners[4][2] = {{1, 1}, {1, -1}, {-1, -1}, {-1, 1}};
    for (int i = 0; i < 4; i++) {
        float f = corners[i][0] * h, r = corners[i][1] * h;
        body[i].x = (lv_coord_t)lroundf(cx + fx * f + rx * r);
        body[i].y = (lv_coord_t)lroundf(cy + fy * f + ry * r);
    }
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = col;
    d.bg_opa = LV_OPA_50;
    lv_draw_polygon(ctx, &d, body, 4);
    for (int i = 0; i < 4; i++) {
        drawLine(ctx, body[i].x, body[i].y, body[(i + 1) % 4].x, body[(i + 1) % 4].y, col, 2);
    }

    // heading arrow
    float tip = h + 6;
    lv_point_t arrow[3] = {
        {(lv_coord_t)lroundf(cx + fx * tip), (lv_coord_t)lroundf(cy + fy * tip)},
        {(lv_coord_t)lroundf(cx + fx * (h - 3) + rx * 5), (lv_coord_t)lroundf(cy + fy * (h - 3) + ry * 5)},
        {(lv_coord_t)lroundf(cx + fx * (h - 3) - rx * 5), (lv_coord_t)lroundf(cy + fy * (h - 3) - ry * 5)},
    };
    d.bg_color = color::text();
    d.bg_opa = LV_OPA_COVER;
    lv_draw_polygon(ctx, &d, arrow, 3);
}

} // namespace ui
