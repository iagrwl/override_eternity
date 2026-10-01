#pragma once

// =====================================================================================
//  BRAIN UI SETTINGS - change stuff here, then run `./sim/run.sh --watch` to preview
// =====================================================================================

#include <cstdint>

namespace ui::config {

// ---- branding ----------------------------------------------------------------------
inline constexpr const char* kTeamName = "ETERNITY";  // splash + status bar
inline constexpr const char* kTeamNumber = "42824A";  // splash subtitle
inline constexpr bool kShowBuildStamp = true;         // "build <date>" on the splash

// ---- colors (0xRRGGBB) -------------------------------------------------------------
// accent = buttons/selection/robot marker, accent2 = highlights/trail/heading
// some combos that look good:
//   violet  0x8B5CF6 / 0x22D3EE      crimson 0xE11D48 / 0xFBBF24
//   ocean   0x2563EB / 0x2DD4BF      lime    0x65A30D / 0xA3E635
//   mono    0xA1A1AA / 0xF4F4F5
inline constexpr uint32_t kAccent = 0x8B5CF6;
inline constexpr uint32_t kAccent2 = 0x22D3EE;
inline constexpr uint32_t kBackground = 0x07090F;
inline constexpr uint32_t kPanel = 0x10141F;   // cards / tiles
inline constexpr uint32_t kPanel2 = 0x171C2B;  // buttons / chips
inline constexpr uint32_t kBorder = 0x262D40;
inline constexpr uint32_t kText = 0xE8ECF4;
inline constexpr uint32_t kMuted = 0x7D879C;   // secondary text
inline constexpr uint32_t kGood = 0x22C55E;
inline constexpr uint32_t kWarn = 0xF59E0B;
inline constexpr uint32_t kBad = 0xEF4444;
inline constexpr uint32_t kRedAlliance = 0xF43F5E;
inline constexpr uint32_t kBlueAlliance = 0x3B82F6;
inline constexpr uint32_t kSkills = 0xEAB308;

// tabs and info-tab widgets live in src/ui/layout.cpp (or press E in the sim to edit them)

// ---- behavior ----------------------------------------------------------------------
inline constexpr bool kFahrenheit = true;      // all temperatures on screen in F (false = C)
inline constexpr int kStartPage = 0;          // index into the tab list in src/ui/layout.cpp
inline constexpr int kAutonSeconds = 15;      // match clock lengths
inline constexpr int kDriverSeconds = 105;
inline constexpr int kEndgameSeconds = 15;    // clock turns amber
inline constexpr float kMotorTempCool = 95;   // gauge fully green at/below this (F if kFahrenheit)
inline constexpr float kMotorTempHot = 140;   // gauge fully red at/above this (VEX motors throttle ~131F)
inline constexpr float kRobotSizeIn = 15;     // robot square on the field map
inline constexpr bool kRumbleOnDeviceProblem = true;
inline constexpr uint32_t kBootTimeoutMs = 10000; // leave splash even if bootComplete() never gets called

} // namespace ui::config
