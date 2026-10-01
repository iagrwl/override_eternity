// Layout editor for the sim. Edits the same tab/widget lists the brain uses, then
// writes them back into src/ui/layout.cpp so the next upload matches what you see.

#include "editor.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "ui_internal.hpp"

using namespace ui;

namespace editor {

static bool s_active = false;
static int s_sel = -1;      // index into widgets()
static bool s_dirty = false;
static bool s_dragging = false, s_resizing = false;
static int s_grabDX = 0, s_grabDY = 0;
static bool s_typing = false; // editing text (widget caption or tab name)
static uint32_t s_removeArmed = 0;
static std::string s_status;
static char s_title[256];
static int s_palMode = 0; // palette view: 0 = widgets, 1 = tabs
static void rebuildPalette();

// ---- palette (things you can drag in from the side)
struct PaletteItem {
    const char* name; // nullptr = section header
    WidgetType type;
    const char* text;
    const char* source;
    int w, h;
};
static const PaletteItem kPalette[] = {
    {nullptr, WidgetType::Label, "WIDGETS", "", 0, 0},
    {"Label", WidgetType::Label, "LABEL", "", 140, 24},
    {"Value", WidgetType::Value, "VALUE", "battery.pct", 104, 62},
    {"Bar", WidgetType::Bar, "BAR", "drive.left_rpm", 206, 50},
    {"Gauge", WidgetType::Gauge, "GAUGE", "drive.max_temp", 104, 104},
    {"Light", WidgetType::Light, "LIGHT", "claw.open", 100, 28},
    {nullptr, WidgetType::Label, "PRESETS", "", 0, 0},
    {"Battery %", WidgetType::Value, "BATTERY", "battery.pct", 104, 62},
    {"Heading", WidgetType::Value, "HEADING", "pose.heading", 104, 62},
    {"Pose X / Y", WidgetType::Value, "X", "pose.x", 90, 56},
    {"Drive temp", WidgetType::Gauge, "DRIVE TEMP", "drive.max_temp", 104, 104},
    {"Lift angle", WidgetType::Value, "LIFT", "lift.deg", 100, 62},
    {"Claw", WidgetType::Light, "CLAW OPEN", "claw.open", 110, 28},
    {"Intake", WidgetType::Light, "INTAKE", "intake.on", 110, 28},
    {"Motor watts", WidgetType::Bar, "MOTOR POWER", "motors.watts", 206, 50},
};
static constexpr int kPaletteCount = sizeof(kPalette) / sizeof(kPalette[0]);
static constexpr int kPaletteX = 480;
static int s_paletteDrag = -1, s_hover = -1;
static int s_mx = 0, s_my = 0;

static constexpr int kHeaderH = 22, kRowH = 14;
static lv_disp_t* s_palDisp = nullptr;

static int paletteRowY(int i) { return kHeaderH + 2 + i * kRowH; }

// ---- TABS view: every built-in page + every custom tab, click to show/hide on the rail
struct TabEntry {
    std::string title, icon;
    Builtin builtin;
    int index; // position on the rail, -1 = hidden
};
static const TabDef kBuiltins[] = {
    {"AUTON", "LIST", Builtin::Auton},       {"FIELD", "GPS", Builtin::Field},  {"MOTORS", "CHARGE", Builtin::Motors},
    {"SENSORS", "EYE", Builtin::Sensors},    {"GRAPH", "SHUFFLE", Builtin::Graph}, {"LOG", "FILE", Builtin::Log},
    {"TOOLS", "SETTINGS", Builtin::Tools},
};
static std::vector<TabEntry> tabEntries() {
    std::vector<TabEntry> out;
    auto find = [](auto pred) {
        for (size_t i = 0; i < tabs().size(); i++)
            if (pred(tabs()[i])) return (int)i;
        return -1;
    };
    for (const TabDef& b : kBuiltins) {
        int i = find([&](const Tab& t) { return t.builtin == b.builtin; });
        out.push_back({i >= 0 ? tabs()[i].title : b.title, i >= 0 ? tabs()[i].icon : b.icon, b.builtin, i});
    }
    // custom tabs on the rail, then hidden ones that still have widgets
    for (size_t i = 0; i < tabs().size(); i++)
        if (tabs()[i].builtin == Builtin::None) out.push_back({tabs()[i].title, tabs()[i].icon, Builtin::None, (int)i});
    for (const Widget& w : widgets()) {
        bool known = false;
        for (const TabEntry& e : out) known |= e.builtin == Builtin::None && e.title == w.tab;
        if (!known) out.push_back({w.tab, "HOME", Builtin::None, -1});
    }
    out.push_back({"+ New info tab", "EDIT", Builtin::None, -2}); // action row
    return out;
}

static int paletteRows() { return s_palMode == 0 ? kPaletteCount : (int)tabEntries().size(); }

static int paletteHit(int x, int y) {
    if (x < kPaletteX || y < kHeaderH + 2) return -1;
    int i = (y - kHeaderH - 2) / kRowH;
    if (i >= paletteRows()) return -1;
    if (s_palMode == 0 && !kPalette[i].name) return -1; // section header
    return i;
}

static const char* kLayoutFile = "../src/ui/layout.cpp";
static const char* kStateFile = ".cache/sim_state.txt";

static const uint32_t kColors[] = {0, 0x22D3EE, 0x8B5CF6, 0x22C55E, 0xF59E0B, 0xEF4444, 0x3B82F6, 0xE8ECF4, 0x7D879C};
static constexpr int kColorCount = sizeof(kColors) / sizeof(kColors[0]);

static Tab* curTab() {
    int p = currentPage();
    return p >= 0 && p < (int)tabs().size() ? &tabs()[p] : nullptr;
}
static bool onCustomTab() { return curTab() && curTab()->builtin == Builtin::None; }

static void changed(const std::string& what) {
    s_dirty = true;
    s_status = what;
    refreshPage();
    if (s_palMode == 1) rebuildPalette(); // keep the TABS list in sync with key edits
}

static void saveState() {
    if (FILE* f = fopen(kStateFile, "w")) {
        fprintf(f, "%d %d\n", currentPage(), (int)s_active);
        fclose(f);
    }
}

void restoreState() {
    static bool done = false;
    if (done) return;
    done = true;
    int page = 0, edit = 0;
    if (FILE* f = fopen(kStateFile, "r")) {
        if (fscanf(f, "%d %d", &page, &edit) == 2) {
            if (page > 0 && page < (int)tabs().size()) showPage(page);
            s_active = edit;
        }
        fclose(f);
    }
}

static bool save() {
    std::ifstream in(kLayoutFile);
    if (!in) {
        s_status = "couldn't open src/ui/layout.cpp";
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    std::string src = ss.str();
    size_t a = src.find(">>> LAYOUT"), b = src.find("// <<< LAYOUT");
    if (a == std::string::npos || b == std::string::npos) {
        s_status = "layout.cpp is missing the >>> LAYOUT / <<< LAYOUT markers";
        return false;
    }
    a = src.find('\n', a) + 1;
    src = src.substr(0, a) + layoutCode() + src.substr(b);
    saveState();
    std::ofstream(kLayoutFile) << src;
    s_dirty = false;
    s_status = "saved to src/ui/layout.cpp - upload to put it on the brain";
    printf("%s\n", s_status.c_str());
    return true;
}

void toggle() {
    s_active = !s_active;
    s_sel = -1;
    s_typing = false;
    if (!s_active && s_dirty) save(); // never lose edits
    s_status = s_active ? "edit mode" : "";
    saveState();
    if (s_active) {
        printf("\nEDIT MODE\n"
               "  click/drag widget = move   drag its corner = resize   arrows = nudge (shift = 10px)\n"
               "  a add widget  c copy  del remove  t type  d/D data source  k color  enter rename\n"
               "  n new tab  x remove tab (twice)  , . move tab  i tab icon  enter (nothing selected) rename tab\n"
               "  w save to layout.cpp   e leave edit mode (auto-saves)\n\n");
    }
}

bool active() { return s_active; }

// widget under a screen point on the current tab (topmost first)
static int hit(int x, int y, bool* corner) {
    if (!onCustomTab()) return -1;
    auto& ws = widgets();
    for (int i = (int)ws.size() - 1; i >= 0; i--) {
        const Widget& w = ws[i];
        if (w.tab != curTab()->title) continue;
        int x1 = kContentX + w.x, y1 = kContentY + w.y;
        if (x >= x1 && x < x1 + w.w && y >= y1 && y < y1 + w.h) {
            if (corner) *corner = x >= x1 + w.w - 8 && y >= y1 + w.h - 8;
            return i;
        }
    }
    return -1;
}

static void clampWidget(Widget& w) {
    if (w.w < 16) w.w = 16;
    if (w.h < 12) w.h = 12;
    if (w.w > kContentW) w.w = kContentW;
    if (w.h > kContentH) w.h = kContentH;
    if (w.x < 0) w.x = 0;
    if (w.y < 0) w.y = 0;
    if (w.x + w.w > kContentW) w.x = kContentW - w.w;
    if (w.y + w.h > kContentH) w.y = kContentH - w.h;
}

static std::string& typingTarget() {
    if (s_sel >= 0) return widgets()[s_sel].text;
    return curTab()->title;
}

static std::string s_tabNameBefore;

static void startTyping() {
    if (s_sel < 0 && !curTab()) return;
    s_typing = true;
    s_tabNameBefore = curTab() ? curTab()->title : "";
    s_status = "typing - enter to finish";
}

static void endTyping() {
    s_typing = false;
    if (s_sel < 0 && curTab()) {
        for (Widget& w : widgets())
            if (w.tab == s_tabNameBefore) w.tab = curTab()->title;
        rebuildTabs();
    }
    changed("renamed");
}

static void cycleSource(int dir) {
    Widget& w = widgets()[s_sel];
    std::vector<std::string> keys = sourceKeys(snapshot());
    keys.insert(keys.begin(), ""); // no source (plain label)
    int i = 0;
    while (i < (int)keys.size() && keys[i] != w.source) i++;
    i = ((i + dir) % (int)keys.size() + (int)keys.size()) % (int)keys.size();
    w.source = keys[i];
    if (w.type == WidgetType::Label && !w.source.empty()) w.type = WidgetType::Value;
    changed("source: " + (w.source.empty() ? std::string("(none)") : w.source));
}

static bool key(const SDL_Keysym& k) {
    bool shift = k.mod & KMOD_SHIFT, ctrl = k.mod & (KMOD_CTRL | KMOD_GUI);
    auto& ws = widgets();
    Widget* w = s_sel >= 0 && s_sel < (int)ws.size() ? &ws[s_sel] : nullptr;

    if (s_typing) {
        std::string& t = typingTarget();
        if (k.sym == SDLK_RETURN || k.sym == SDLK_ESCAPE) endTyping();
        else if (k.sym == SDLK_BACKSPACE && !t.empty()) {
            while (!t.empty() && (t.back() & 0xC0) == 0x80) t.pop_back(); // whole utf-8 char
            if (!t.empty()) t.pop_back();
            if (s_sel < 0) rebuildTabs();
            else refreshPage();
        }
        return true;
    }

    switch (k.sym) {
        case SDLK_e: toggle(); return true;
        case SDLK_w: save(); return true;
        case SDLK_q: return false; // let the sim quit
        case SDLK_ESCAPE:
            s_sel = -1;
            return true;
        case SDLK_RETURN: startTyping(); return true;

        // ---- tabs
        case SDLK_n: {
            int n = 1;
            std::string name;
            auto taken = [&](const std::string& t) {
                for (const Tab& tb : tabs())
                    if (tb.title == t) return true;
                return false;
            };
            do name = "TAB " + std::to_string(n++);
            while (taken(name));
            tabs().insert(tabs().begin() + currentPage() + 1, Tab{name, "EDIT", Builtin::None});
            rebuildTabs();
            showPage(currentPage() + 1);
            s_sel = -1;
            changed("new tab " + name + " - press enter to rename, a to add widgets");
            return true;
        }
        case SDLK_x: {
            if (tabs().size() <= 1) return true;
            uint32_t now = SDL_GetTicks();
            if (now - s_removeArmed > 2000) {
                s_removeArmed = now;
                s_status = "press x again to remove tab " + curTab()->title;
                return true;
            }
            s_removeArmed = 0;
            std::string name = curTab()->title;
            if (curTab()->builtin == Builtin::None) {
                for (size_t i = ws.size(); i-- > 0;)
                    if (ws[i].tab == name) ws.erase(ws.begin() + i);
            }
            tabs().erase(tabs().begin() + currentPage());
            s_sel = -1;
            rebuildTabs();
            changed("removed tab " + name);
            return true;
        }
        case SDLK_COMMA:
        case SDLK_PERIOD: {
            int p = currentPage(), q = p + (k.sym == SDLK_COMMA ? -1 : 1);
            if (q < 0 || q >= (int)tabs().size()) return true;
            std::swap(tabs()[p], tabs()[q]);
            rebuildTabs();
            showPage(q);
            changed("moved tab");
            return true;
        }
        case SDLK_i: {
            const auto& names = iconNames();
            size_t i = 0;
            while (i < names.size() && curTab()->icon != names[i]) i++;
            curTab()->icon = names[(i + 1) % names.size()];
            rebuildTabs();
            changed("tab icon: " + curTab()->icon);
            return true;
        }

        // ---- widgets
        case SDLK_a: {
            if (!onCustomTab()) {
                s_status = "widgets go on custom tabs - press n to make one";
                return true;
            }
            ws.push_back({curTab()->title, "NEW", "battery.pct", WidgetType::Value, 10, 10, 100, 60, 0});
            s_sel = ws.size() - 1;
            changed("added widget - t type, d data, enter text");
            return true;
        }
        case SDLK_c:
            if (!w) return true;
            ws.push_back(*w);
            ws.back().x += 10, ws.back().y += 10;
            clampWidget(ws.back());
            s_sel = ws.size() - 1;
            changed("copied");
            return true;
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
            if (!w) return true;
            ws.erase(ws.begin() + s_sel);
            s_sel = -1;
            changed("deleted");
            return true;
        case SDLK_t:
            if (!w) return true;
            w->type = (WidgetType)(((int)w->type + 1) % 5);
            changed(std::string("type: ") + widgetTypeName(w->type));
            return true;
        case SDLK_d:
            if (w) cycleSource(shift ? -1 : 1);
            return true;
        case SDLK_k: {
            if (!w) return true;
            int i = 0;
            while (i < kColorCount && kColors[i] != w->color) i++;
            w->color = kColors[(i + 1) % kColorCount];
            changed("color");
            return true;
        }
        case SDLK_LEFT:
        case SDLK_RIGHT:
        case SDLK_UP:
        case SDLK_DOWN: {
            if (!w) return true;
            int step = shift ? 10 : 1;
            int dx = k.sym == SDLK_LEFT ? -step : k.sym == SDLK_RIGHT ? step : 0;
            int dy = k.sym == SDLK_UP ? -step : k.sym == SDLK_DOWN ? step : 0;
            if (ctrl) w->w += dx, w->h += dy;
            else w->x += dx, w->y += dy;
            clampWidget(*w);
            changed("");
            return true;
        }
    }
    return true; // swallow other keys so they don't drive the fake robot
}

static void rebuildPalette() {
    if (!s_palDisp) return;
    lv_disp_t* prev = lv_disp_get_default();
    lv_disp_set_default(s_palDisp);
    lv_obj_t* old = lv_scr_act();
    buildPalette();
    lv_obj_del(old);
    lv_disp_set_default(prev);
}

static void clickTabEntry(int row) {
    std::vector<TabEntry> list = tabEntries();
    if (row < 0 || row >= (int)list.size()) return;
    TabEntry& t = list[row];
    if (t.index == -2) { // new custom tab
        int n = 1;
        std::string name;
        bool taken;
        do {
            name = "TAB " + std::to_string(n++);
            taken = false;
            for (const TabEntry& e : list) taken |= e.title == name;
        } while (taken);
        tabs().push_back({name, "EDIT", Builtin::None});
        rebuildTabs();
        showPage(tabs().size() - 1);
        changed("added tab " + name + " - enter renames it, drag widgets onto it");
    } else if (t.index >= 0) { // hide it
        if (tabs().size() <= 1) {
            s_status = "need at least one tab";
            return;
        }
        tabs().erase(tabs().begin() + t.index);
        rebuildTabs();
        changed("hid " + t.title + (t.builtin == Builtin::None ? " (its widgets are kept, click to bring it back)" : ""));
    } else { // show it again at the end of the rail
        tabs().push_back({t.title, t.icon, t.builtin});
        rebuildTabs();
        showPage(tabs().size() - 1);
        changed("showing " + t.title);
    }
    s_sel = -1;
}

// rail reorder: drag a rail icon up/down in edit mode
static int s_railDrag = -1;
static int railIndexAt(int y) {
    int n = tabs().size();
    if (n == 0 || y < kContentY || y >= kContentY + kContentH) return -1;
    float pitch = (kContentH + 3) / (float)n;
    int i = (int)((y - kContentY) / pitch);
    return i < n ? i : n - 1;
}

static void dropPaletteItem(int x, int y) {
    const PaletteItem& p = kPalette[s_paletteDrag];
    s_paletteDrag = -1;
    if (x >= kPaletteX) return;
    if (!onCustomTab()) {
        s_status = "drop onto a custom tab (press n to make one)";
        return;
    }
    Widget w{curTab()->title, p.text, p.source, p.type, x - kContentX - p.w / 2, y - kContentY - p.h / 2, p.w, p.h, 0};
    clampWidget(w);
    widgets().push_back(w);
    s_sel = widgets().size() - 1;
    changed(std::string("added ") + p.name + " - enter to rename, d to change data");
}

bool handle(const SDL_Event& e, int mx, int my) {
    // the palette works even outside edit mode (dragging from it turns edit mode on)
    if (e.type == SDL_MOUSEMOTION) {
        s_mx = e.motion.x, s_my = e.motion.y;
        s_hover = paletteHit(s_mx, s_my);
        if (s_paletteDrag >= 0) return true;
    }
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.x >= kPaletteX) {
        if (e.button.y < kHeaderH) { // WIDGETS | TABS switch
            s_palMode = e.button.x < kPaletteX + kPaletteW / 2 ? 0 : 1;
            s_hover = -1;
            rebuildPalette();
            return true;
        }
        int row = paletteHit(e.button.x, e.button.y);
        if (row >= 0 && !s_active) toggle();
        if (s_palMode == 1) clickTabEntry(row);
        else s_paletteDrag = row;
        return true;
    }
    // edit mode: drag rail icons to reorder tabs
    if (s_active && e.type == SDL_MOUSEBUTTONDOWN && e.button.x < kContentX - 4) {
        s_railDrag = railIndexAt(e.button.y);
        return false; // still let LVGL switch to the tab
    }
    if (s_railDrag >= 0 && e.type == SDL_MOUSEBUTTONUP) {
        int to = railIndexAt(e.button.y), from = s_railDrag;
        s_railDrag = -1;
        if (to >= 0 && to != from) {
            Tab t = tabs()[from];
            tabs().erase(tabs().begin() + from);
            tabs().insert(tabs().begin() + to, t);
            rebuildTabs();
            showPage(to);
            changed("moved " + t.title);
        }
        return false; // let LVGL see the release too
    }
    if (e.type == SDL_MOUSEBUTTONUP && s_paletteDrag >= 0) {
        dropPaletteItem(e.button.x, e.button.y);
        return true;
    }
    if (!s_active) return false;

    if (e.type == SDL_TEXTINPUT) {
        if (s_typing) {
            typingTarget() += e.text.text;
            if (s_sel < 0) rebuildTabs();
            else refreshPage();
        }
        return true;
    }
    if (e.type == SDL_KEYDOWN) return key(e.key.keysym);
    if (e.type == SDL_KEYUP) return true;

    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP) {
        int x = e.type == SDL_MOUSEMOTION ? e.motion.x : e.button.x;
        int y = e.type == SDL_MOUSEMOTION ? e.motion.y : e.button.y;
        (void)mx, (void)my;
        // rail + status bar + built-in pages still work normally (switch tabs while editing)
        if (!onCustomTab() || x < kContentX - 2 || y < kContentY - 2) {
            if (e.type == SDL_MOUSEBUTTONDOWN) s_sel = -1;
            return false;
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (s_typing) endTyping();
            bool corner = false;
            s_sel = hit(x, y, &corner);
            if (s_sel >= 0) {
                Widget& w = widgets()[s_sel];
                s_resizing = corner;
                s_dragging = !corner;
                s_grabDX = x - (kContentX + w.x);
                s_grabDY = y - (kContentY + w.y);
                s_status = std::string(widgetTypeName(w.type)) + " \"" + w.text + "\"  src " +
                           (w.source.empty() ? "(none)" : w.source);
            }
        } else if (e.type == SDL_MOUSEMOTION && s_sel >= 0 && (s_dragging || s_resizing)) {
            Widget& w = widgets()[s_sel];
            if (s_dragging) {
                w.x = (x - kContentX - s_grabDX) / 2 * 2; // 2px snap
                w.y = (y - kContentY - s_grabDY) / 2 * 2;
            } else {
                w.w = (x - kContentX - w.x) / 2 * 2;
                w.h = (y - kContentY - w.y) / 2 * 2;
            }
            clampWidget(w);
            s_dirty = true;
            refreshPage();
        } else if (e.type == SDL_MOUSEBUTTONUP) {
            s_dragging = s_resizing = false;
        }
        return true;
    }
    return false;
}

static lv_obj_t* paletteLabel(lv_obj_t* scr, int x, int y, const lv_font_t* f, lv_color_t c, const char* txt) {
    lv_obj_t* l = lv_label_create(scr);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_text(l, txt);
    lv_obj_set_pos(l, x, y);
    return l;
}

void buildPalette() {
    s_palDisp = lv_disp_get_default();
    lv_obj_t* scr = lv_obj_create(nullptr);
    lv_obj_remove_style_all(scr);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0E16), 0);
    lv_obj_set_style_border_side(scr, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(scr, 1, 0);
    lv_obj_set_style_border_color(scr, color::line(), 0);

    // WIDGETS | TABS switch
    for (int m = 0; m < 2; m++) {
        lv_obj_t* l = paletteLabel(scr, m ? 76 : 10, 5, &lv_font_montserrat_12,
                                   m == s_palMode ? color::accent2() : color::muted(), m ? "TABS" : "WIDGETS");
        lv_obj_set_style_text_letter_space(l, 1, 0);
    }
    lv_obj_t* line = lv_obj_create(scr);
    lv_obj_remove_style_all(line);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(line, color::accent2(), 0);
    lv_obj_set_size(line, s_palMode ? 40 : 62, 2);
    lv_obj_set_pos(line, s_palMode ? 74 : 8, kHeaderH - 2);

    if (s_palMode == 0) {
        for (int i = 0; i < kPaletteCount; i++) {
            const PaletteItem& p = kPalette[i];
            if (p.name) {
                char txt[40];
                snprintf(txt, sizeof txt, LV_SYMBOL_PLUS "  %s", p.name);
                paletteLabel(scr, 12, paletteRowY(i), &lv_font_montserrat_12, color::text(), txt);
            } else {
                lv_obj_t* l = paletteLabel(scr, 10, paletteRowY(i) + 3, &lv_font_montserrat_10, color::dim(), p.text);
                lv_obj_set_style_text_letter_space(l, 1, 0);
            }
        }
    } else {
        std::vector<TabEntry> list = tabEntries();
        for (size_t i = 0; i < list.size(); i++) {
            const TabEntry& t = list[i];
            char txt[48];
            if (t.index == -2) snprintf(txt, sizeof txt, "%s", t.title.c_str());
            else snprintf(txt, sizeof txt, "%s  %s", t.index >= 0 ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE, t.title.c_str());
            lv_color_t c = t.index == -2 ? color::accent2() : t.index >= 0 ? color::text() : color::dim();
            lv_obj_t* l = paletteLabel(scr, 12, paletteRowY(i), &lv_font_montserrat_12, c, txt);
            lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
            lv_obj_set_width(l, kPaletteW - 20);
        }
        paletteLabel(scr, 10, 226, &lv_font_montserrat_10, color::dim(), "click = show/hide");
    }
    lv_scr_load(scr);
}

void drawOverlay(SDL_Renderer* ren) {
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    if (s_hover >= 0 || s_paletteDrag >= 0) {
        int i = s_paletteDrag >= 0 ? s_paletteDrag : s_hover;
        SDL_SetRenderDrawColor(ren, 139, 92, 246, 70);
        SDL_Rect row = {kPaletteX + 4, paletteRowY(i) - 1, kPaletteW - 8, kRowH};
        SDL_RenderFillRect(ren, &row);
    }
    if (s_railDrag >= 0) { // where the dragged tab will land
        int to = railIndexAt(s_my);
        float pitch = (kContentH + 3) / (float)tabs().size();
        SDL_SetRenderDrawColor(ren, 34, 211, 238, 255);
        SDL_Rect bar = {2, kContentY + (int)(to * pitch) - 2, 44, 2};
        if (to >= 0 && to != s_railDrag) SDL_RenderFillRect(ren, &bar);
    }
    if (s_paletteDrag >= 0) { // ghost of the widget being dragged
        const PaletteItem& p = kPalette[s_paletteDrag];
        SDL_Rect g = {s_mx - p.w / 2, s_my - p.h / 2, p.w, p.h};
        SDL_SetRenderDrawColor(ren, 34, 211, 238, 50);
        SDL_RenderFillRect(ren, &g);
        SDL_SetRenderDrawColor(ren, 34, 211, 238, 255);
        SDL_RenderDrawRect(ren, &g);
    }
    if (!s_active) return;
    // frame around the whole screen = you're in edit mode
    SDL_SetRenderDrawColor(ren, 245, 158, 11, 255);
    SDL_Rect frame = {0, 0, 480, 240};
    SDL_RenderDrawRect(ren, &frame);

    if (!onCustomTab()) return;
    SDL_SetRenderDrawColor(ren, 245, 158, 11, 60);
    SDL_Rect content = {kContentX, kContentY, kContentW, kContentH};
    SDL_RenderDrawRect(ren, &content);

    const auto& ws = widgets();
    for (int i = 0; i < (int)ws.size(); i++) {
        const Widget& w = ws[i];
        if (w.tab != curTab()->title) continue;
        SDL_Rect r = {kContentX + w.x, kContentY + w.y, w.w, w.h};
        if (i == s_sel) {
            SDL_SetRenderDrawColor(ren, 34, 211, 238, 255);
            SDL_RenderDrawRect(ren, &r);
            SDL_Rect handle = {r.x + r.w - 6, r.y + r.h - 6, 6, 6};
            SDL_RenderFillRect(ren, &handle);
        } else {
            SDL_SetRenderDrawColor(ren, 245, 158, 11, 110);
            SDL_RenderDrawRect(ren, &r);
        }
    }
}

const char* title() {
    static int lastPage = -1;
    if (currentPage() != lastPage) { // remember the tab so a rebuild reopens it
        lastPage = currentPage();
        saveState();
    }
    if (!s_active) return "V5 brain sim   (E = edit layout)";
    std::string sel;
    if (s_sel >= 0 && s_sel < (int)widgets().size()) {
        const Widget& w = widgets()[s_sel];
        sel = std::string(widgetTypeName(w.type)) + " \"" + w.text + "\"  src=" + (w.source.empty() ? "-" : w.source) +
              "  " + std::to_string(w.x) + "," + std::to_string(w.y) + " " + std::to_string(w.w) + "x" +
              std::to_string(w.h);
    }
    snprintf(s_title, sizeof s_title, "EDIT%s  |  %s  |  %s", s_dirty ? " *unsaved (w)" : "",
             sel.empty() ? (curTab() ? ("tab " + curTab()->title).c_str() : "") : sel.c_str(), s_status.c_str());
    return s_title;
}

} // namespace editor
