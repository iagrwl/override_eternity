// loads layout.cpp's tables into editable vectors and writes them back out as code

#include <cstdio>
#include "ui_internal.hpp"

namespace ui {

static std::vector<Tab> s_tabs;
static std::vector<Widget> s_widgets;
static bool s_loaded = false;

static void load() {
    if (s_loaded) return;
    s_loaded = true;
    for (int i = 0; i < kLayoutTabCount; i++) {
        const TabDef& t = kLayoutTabs[i];
        s_tabs.push_back({t.title, t.icon, t.builtin});
    }
    for (int i = 0; i < kLayoutWidgetCount; i++) {
        const WidgetDef& w = kLayoutWidgets[i];
        s_widgets.push_back({w.tab, w.text ? w.text : "", w.source ? w.source : "", w.type, w.x, w.y, w.w, w.h,
                             w.color});
    }
}

std::vector<Tab>& tabs() {
    load();
    return s_tabs;
}

std::vector<Widget>& widgets() {
    load();
    return s_widgets;
}

// ---------------------------------------------------------------- icons
struct Icon {
    const char* name;
    const char* glyph;
};
static const Icon kIcons[] = {
    {"LIST", LV_SYMBOL_LIST},   {"GPS", LV_SYMBOL_GPS},          {"CHARGE", LV_SYMBOL_CHARGE},
    {"EYE", LV_SYMBOL_EYE_OPEN}, {"SHUFFLE", LV_SYMBOL_SHUFFLE},  {"FILE", LV_SYMBOL_FILE},
    {"SETTINGS", LV_SYMBOL_SETTINGS}, {"HOME", LV_SYMBOL_HOME},   {"BELL", LV_SYMBOL_BELL},
    {"BATTERY", LV_SYMBOL_BATTERY_FULL}, {"WARNING", LV_SYMBOL_WARNING}, {"PLAY", LV_SYMBOL_PLAY},
    {"IMAGE", LV_SYMBOL_IMAGE}, {"DRIVE", LV_SYMBOL_DRIVE},      {"EDIT", LV_SYMBOL_EDIT},
    {"WIFI", LV_SYMBOL_WIFI},   {"POWER", LV_SYMBOL_POWER},      {"LOOP", LV_SYMBOL_LOOP},
    {"UP", LV_SYMBOL_UP},       {"DOWN", LV_SYMBOL_DOWN},        {"OK", LV_SYMBOL_OK},
    {"STOP", LV_SYMBOL_STOP},   {"CALL", LV_SYMBOL_CALL},        {"GAMEPAD", LV_SYMBOL_KEYBOARD},
};

const char* iconGlyph(const std::string& name) {
    for (const Icon& i : kIcons)
        if (name == i.name) return i.glyph;
    return LV_SYMBOL_WARNING;
}

const std::vector<const char*>& iconNames() {
    static std::vector<const char*> names;
    if (names.empty())
        for (const Icon& i : kIcons) names.push_back(i.name);
    return names;
}

const char* widgetTypeName(WidgetType t) {
    switch (t) {
        case WidgetType::Label: return "Label";
        case WidgetType::Value: return "Value";
        case WidgetType::Bar: return "Bar";
        case WidgetType::Gauge: return "Gauge";
        default: return "Light";
    }
}

static const char* builtinName(Builtin b) {
    switch (b) {
        case Builtin::Auton: return "Auton";
        case Builtin::Field: return "Field";
        case Builtin::Motors: return "Motors";
        case Builtin::Sensors: return "Sensors";
        case Builtin::Graph: return "Graph";
        case Builtin::Log: return "Log";
        case Builtin::Tools: return "Tools";
        default: return "None";
    }
}

static std::string quote(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out + "\"";
}

std::string layoutCode() {
    load();
    std::string o;
    char buf[64];
    o += "const TabDef kLayoutTabs[] = {\n";
    for (const Tab& t : s_tabs)
        o += "    {" + quote(t.title) + ", " + quote(t.icon) + ", Builtin::" + builtinName(t.builtin) + "},\n";
    o += "};\nconst int kLayoutTabCount = sizeof(kLayoutTabs) / sizeof(kLayoutTabs[0]);\n\n";
    o += "const WidgetDef kLayoutWidgets[] = {\n";
    o += "    // tab, type, text, source, x, y, w, h, color (0 = theme accent)\n";
    for (const Widget& w : s_widgets) {
        if (w.color) snprintf(buf, sizeof buf, ", %d, %d, %d, %d, 0x%06X", w.x, w.y, w.w, w.h, (unsigned)w.color);
        else snprintf(buf, sizeof buf, ", %d, %d, %d, %d, 0", w.x, w.y, w.w, w.h);
        o += "    {" + quote(w.tab) + ", WidgetType::" + widgetTypeName(w.type) + ", " + quote(w.text) + ", " +
             quote(w.source) + buf + "},\n";
    }
    o += "    {}, // end marker, keep this last\n";
    o += "};\nconst int kLayoutWidgetCount = sizeof(kLayoutWidgets) / sizeof(kLayoutWidgets[0]) - 1;\n";
    return o;
}

} // namespace ui
