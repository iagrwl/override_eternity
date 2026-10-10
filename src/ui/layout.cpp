// =====================================================================================
//  BRAIN LAYOUT - which tabs exist (in rail order) and what's on your custom info tabs
//
//  easiest: ./sim/run.sh --watch, press E in the window, drag stuff around, press W.
//  by hand:
//    - remove a tab: delete its line          - reorder: move lines
//    - new info tab: {"NAME", "ICON", Builtin::None}, then add widgets with tab "NAME"
//    - icons: LIST GPS CHARGE EYE SHUFFLE FILE SETTINGS HOME BELL BATTERY WARNING PLAY
//             IMAGE DRIVE EDIT WIFI POWER LOOP UP DOWN OK STOP CALL GAMEPAD
//    - widget types: Label (just text), Value (caption + big number), Bar, Gauge, Light
//    - data sources (the "source" column): see src/ui/sources.cpp, e.g. "battery.pct",
//      "motor.LIFT A.temp", or anything you send with ui::publish("name", value)
//    - x, y, w, h are pixels inside the 426 x 206 content area
// =====================================================================================

#include "ui_internal.hpp"

namespace ui {

// >>> LAYOUT  (the sim rewrites everything between these markers when you save in edit mode)
const TabDef kLayoutTabs[] = {
    {"AUTON", "LIST", Builtin::Auton},
    {"HOME", "HOME", Builtin::None},
    {"SENSORS", "EYE", Builtin::Sensors},
    {"LOG", "FILE", Builtin::Log},
    {"TOOLS", "SETTINGS", Builtin::Tools},
    {"MOTORS", "CHARGE", Builtin::Motors},
};
const int kLayoutTabCount = sizeof(kLayoutTabs) / sizeof(kLayoutTabs[0]);

const WidgetDef kLayoutWidgets[] = {
    // tab, type, text, source, x, y, w, h, color (0 = theme accent)
    {"HOME", WidgetType::Value, "BATTERY", "battery.pct", 0, 0, 130, 72, 0},
    {"HOME", WidgetType::Value, "HEADING", "pose.heading", 148, 0, 130, 72, 0x7DD3FC},
    {"HOME", WidgetType::Value, "LIFT", "lift.deg", 296, 0, 130, 72, 0},
    {"HOME", WidgetType::Value, "X", "pose.x", 0, 88, 130, 60, 0},
    {"HOME", WidgetType::Value, "Y", "pose.y", 148, 88, 130, 60, 0},
    {"HOME", WidgetType::Value, "CONTROLLER", "controller.battery", 296, 88, 130, 60, 0},
    {"HOME", WidgetType::Light, "PIVOT", "pivot.out", 0, 172, 130, 24, 0xFBBF24},
    {"HOME", WidgetType::Light, "INTAKE", "intake.on", 148, 172, 130, 24, 0x4ADE80},
    {}, // end marker, keep this last
};
const int kLayoutWidgetCount = sizeof(kLayoutWidgets) / sizeof(kLayoutWidgets[0]) - 1;
// <<< LAYOUT

} // namespace ui
