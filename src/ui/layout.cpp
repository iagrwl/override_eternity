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
    {"FIELD", "GPS", Builtin::Field},
    {"SENSORS", "EYE", Builtin::Sensors},
    {"LOG", "FILE", Builtin::Log},
    {"TOOLS", "SETTINGS", Builtin::Tools},
    {"MOTORS", "CHARGE", Builtin::Motors},
};
const int kLayoutTabCount = sizeof(kLayoutTabs) / sizeof(kLayoutTabs[0]);

const WidgetDef kLayoutWidgets[] = {
    // tab, type, text, source, x, y, w, h, color (0 = theme accent)
    {"HOME", WidgetType::Value, "BATTERY", "battery.pct", 104, 110, 90, 56, 0},
    {"HOME", WidgetType::Value, "HEADING", "pose.heading", 322, 0, 104, 62, 0x22D3EE},
    {"HOME", WidgetType::Value, "LIFT", "lift.deg", 238, 0, 84, 62, 0},
    {"HOME", WidgetType::Light, "CLAW OPEN", "claw.open", 294, 172, 132, 24, 0xF59E0B},
    {"HOME", WidgetType::Light, "INTAKE", "intake.on", 352, 152, 74, 20, 0x22C55E},
    {"HOME", WidgetType::Gauge, "DRIVE TEMP", "drive.max_temp", 0, 102, 104, 104, 0},
    {"HOME", WidgetType::Gauge, "MECH TEMP", "mech.max_temp", 0, 0, 104, 104, 0},
    {"HOME", WidgetType::Bar, "LEFT DRIVE", "drive.left_rpm", 282, 72, 144, 32, 0x22D3EE},
    {"HOME", WidgetType::Bar, "RIGHT DRIVE", "drive.right_rpm", 282, 110, 144, 34, 0x8B5CF6},
    {"HOME", WidgetType::Value, "X", "pose.x", 104, 0, 90, 56, 0},
    {"HOME", WidgetType::Value, "X", "pose.x", 104, 54, 90, 56, 0},
    {}, // end marker, keep this last
};
const int kLayoutWidgetCount = sizeof(kLayoutWidgets) / sizeof(kLayoutWidgets[0]) - 1;
// <<< LAYOUT

} // namespace ui
