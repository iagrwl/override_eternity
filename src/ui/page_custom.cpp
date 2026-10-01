// custom info tabs: renders the widgets from layout.cpp for whichever custom tab is
// showing. each widget is one custom-drawn object, so they're cheap on memory.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <utility>
#include "ui_internal.hpp"

namespace ui {

static std::vector<lv_obj_t*> s_objs;

static const lv_font_t* const kFonts[] = {&lv_font_montserrat_48, &lv_font_montserrat_40, &lv_font_montserrat_36,
                                          &lv_font_montserrat_30, &lv_font_montserrat_24, &lv_font_montserrat_20,
                                          &lv_font_montserrat_18, &lv_font_montserrat_16, &lv_font_montserrat_14,
                                          &lv_font_montserrat_12, &lv_font_montserrat_10};

// biggest font where the text fits in w x h
static const lv_font_t* fitFont(const char* txt, lv_coord_t w, lv_coord_t h) {
    for (const lv_font_t* f : kFonts) {
        if (lv_font_get_line_height(f) > h) continue;
        if (lv_txt_get_width(txt, strlen(txt), f, 0, LV_TEXT_FLAG_NONE) <= w) return f;
    }
    return &lv_font_montserrat_10;
}

static lv_coord_t textW(const char* txt, const lv_font_t* f) {
    return lv_txt_get_width(txt, strlen(txt), f, 0, LV_TEXT_FLAG_NONE);
}

static void panel(lv_draw_ctx_t* ctx, const lv_area_t& a) {
    drawRect(ctx, a, color::panel(), LV_OPA_COVER, 8);
    drawFrame(ctx, a, color::line(), 1, 8);
}

static void formatValue(const SourceValue& v, bool ok, char* buf, int len) {
    if (!ok) snprintf(buf, len, "--");
    else if (v.boolean) snprintf(buf, len, v.value > 0.5f ? "ON" : "OFF");
    else snprintf(buf, len, "%.*f", v.decimals, v.value);
}

static void onWidgetDraw(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target(e);
    size_t idx = (size_t)(uintptr_t)lv_obj_get_user_data(obj);
    if (idx >= widgets().size()) return;
    const Widget& w = widgets()[idx];
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    lv_area_t a;
    lv_obj_get_coords(obj, &a);
    lv_coord_t W = lv_area_get_width(&a), H = lv_area_get_height(&a);

    SourceValue v;
    bool ok = !w.source.empty() && readSource(w.source, snapshot(), v);
    lv_color_t col = w.color ? lv_color_hex(w.color) : color::accent();
    lv_color_t valueCol = !ok ? color::dim()
                          : v.temperature ? heatColor(v.value, config::kMotorTempCool, config::kMotorTempHot)
                                          : color::text();
    char val[24];
    formatValue(v, ok, val, sizeof val);
    float frac = ok && v.max > v.min ? (v.value - v.min) / (v.max - v.min) : 0;
    frac = frac < 0 ? 0 : frac > 1 ? 1 : frac;
    const char* caption = w.text.c_str();

    switch (w.type) {
        case WidgetType::Label: {
            const lv_font_t* f = fitFont(caption, W, H);
            drawText(ctx, a.x1, a.y1 + (H - lv_font_get_line_height(f)) / 2, W, caption, f,
                     w.color ? col : color::text());
            break;
        }
        case WidgetType::Value: {
            panel(ctx, a);
            drawText(ctx, a.x1 + 8, a.y1 + 6, W - 16, caption, &lv_font_montserrat_10, color::muted());
            lv_coord_t unitW = ok && v.unit[0] ? textW(v.unit, &lv_font_montserrat_12) + 3 : 0;
            const lv_font_t* f = fitFont(val, W - 16 - unitW, H - 24);
            lv_coord_t y = a.y1 + 20 + (H - 24 - lv_font_get_line_height(f)) / 2;
            drawText(ctx, a.x1 + 8, y, W - 16, val, f, w.color && !v.temperature && ok ? col : valueCol);
            if (unitW) {
                lv_coord_t x = a.x1 + 8 + textW(val, f) + 3;
                drawText(ctx, x, y + lv_font_get_line_height(f) - 17, unitW, v.unit, &lv_font_montserrat_12,
                         color::muted());
            }
            break;
        }
        case WidgetType::Bar: {
            panel(ctx, a);
            drawText(ctx, a.x1 + 8, a.y1 + 6, W - 16, caption, &lv_font_montserrat_10, color::muted());
            char txt[40];
            snprintf(txt, sizeof txt, "%s %s", val, ok ? v.unit : "");
            drawText(ctx, a.x1 + 8, a.y1 + 4, W - 16, txt, &lv_font_montserrat_12, valueCol, LV_TEXT_ALIGN_RIGHT);
            lv_area_t track = area(a.x1 + 8, a.y2 - 13, W - 16, 6);
            drawRect(ctx, track, color::panel2(), LV_OPA_COVER, 3);
            lv_color_t fill = v.temperature ? valueCol : col;
            if (ok && v.min < 0 && v.max > 0) { // signed range: fill out from the middle
                float zero = -v.min / (v.max - v.min);
                lv_coord_t x0 = track.x1 + (lv_coord_t)(zero * (W - 16)), x1 = track.x1 + (lv_coord_t)(frac * (W - 16));
                if (x1 < x0) std::swap(x0, x1);
                drawRect(ctx, {x0, track.y1, (lv_coord_t)LV_MAX(x1, x0 + 2), track.y2}, fill, LV_OPA_COVER, 3);
                drawLine(ctx, track.x1 + (lv_coord_t)(zero * (W - 16)), track.y1 - 3,
                         track.x1 + (lv_coord_t)(zero * (W - 16)), track.y2 + 3, color::muted(), 1);
            } else if (ok) {
                drawRect(ctx, {track.x1, track.y1, (lv_coord_t)(track.x1 + LV_MAX(6, frac * (W - 16))), track.y2},
                         fill, LV_OPA_COVER, 3);
            }
            break;
        }
        case WidgetType::Gauge: {
            panel(ctx, a);
            lv_coord_t r = LV_MIN(W, H - 16) / 2 - 8;
            if (r < 8) r = 8;
            lv_coord_t cx = a.x1 + W / 2, cy = a.y1 + 6 + r + 4;
            lv_coord_t thick = LV_MAX(4, r / 5);
            drawArc(ctx, cx, cy, r, 135, 45, color::panel2(), thick);
            if (ok) drawArc(ctx, cx, cy, r, 135, (uint16_t)(135 + 270 * LV_MAX(frac, 0.02f)) % 360,
                            v.temperature ? valueCol : col, thick);
            char txt[32];
            snprintf(txt, sizeof txt, "%s%s", val, ok && v.temperature ? "\xC2\xB0" : "");
            const lv_font_t* f = fitFont(txt, r * 2 - thick * 2 - 4, r);
            drawText(ctx, cx - r, cy - lv_font_get_line_height(f) / 2, r * 2, txt, f, valueCol, LV_TEXT_ALIGN_CENTER);
            drawText(ctx, a.x1 + 4, a.y2 - 16, W - 8, caption, &lv_font_montserrat_10, color::muted(),
                     LV_TEXT_ALIGN_CENTER);
            break;
        }
        case WidgetType::Light: {
            panel(ctx, a);
            bool on = ok && v.value > 0.5f;
            lv_coord_t cy = a.y1 + H / 2, cx = a.x1 + 14;
            if (on) drawRect(ctx, area(cx - 9, cy - 9, 18, 18), col, LV_OPA_30, 9);
            drawRect(ctx, area(cx - 5, cy - 5, 10, 10), on ? col : color::panel2(), LV_OPA_COVER, 5);
            const lv_font_t* f = fitFont(caption, W - 36, LV_MIN(H - 4, 16));
            drawText(ctx, a.x1 + 28, cy - lv_font_get_line_height(f) / 2, W - 36, caption, f,
                     on ? color::text() : color::muted());
            break;
        }
    }
}

static void buildCustom(lv_obj_t* parent) {
    s_objs.clear();
    const std::string& tab = tabs()[currentPage()].title;
    const auto& list = widgets();
    for (size_t i = 0; i < list.size(); i++) {
        const Widget& w = list[i];
        if (w.tab != tab) continue;
        lv_obj_t* o = makeBox(parent, w.x, w.y, LV_MAX(w.w, 8), LV_MAX(w.h, 8));
        lv_obj_set_user_data(o, (void*)(uintptr_t)i);
        lv_obj_add_event_cb(o, onWidgetDraw, LV_EVENT_DRAW_MAIN, nullptr);
        s_objs.push_back(o);
    }
    if (s_objs.empty()) {
        lv_obj_t* hint = makeLabel(parent, &lv_font_montserrat_14, color::muted(),
                                   "empty tab\nadd widgets in src/ui/layout.cpp\nor press E in the sim");
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(hint);
    }
}

static void updateCustom(const Snapshot&) {
    static uint32_t n = 0;
    if (n++ % 2) return; // 10 Hz
    for (lv_obj_t* o : s_objs) lv_obj_invalidate(o);
}

static void teardownCustom() { s_objs.clear(); }

const Page pageCustom = {"CUSTOM", LV_SYMBOL_HOME, buildCustom, updateCustom, teardownCustom};

} // namespace ui
