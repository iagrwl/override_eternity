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
// minimal theme: black, white, one soft highlight. accent = selection / robot marker,
// accent2 = highlights (path trail, auton mode, heading)
inline constexpr uint32_t kAccent = 0xFAFAFA;
inline constexpr uint32_t kAccent2 = 0x7DD3FC;
inline constexpr uint32_t kBackground = 0x000000;
inline constexpr uint32_t kPanel = 0x0A0A0B;   // cards / tiles (barely there)
inline constexpr uint32_t kPanel2 = 0x141416;  // buttons / chips
inline constexpr uint32_t kBorder = 0x1F1F23;  // hairlines
inline constexpr uint32_t kText = 0xFAFAFA;
inline constexpr uint32_t kMuted = 0x71717A;   // secondary text
inline constexpr uint32_t kGood = 0x4ADE80;
inline constexpr uint32_t kWarn = 0xFBBF24;
inline constexpr uint32_t kBad = 0xF87171;
inline constexpr uint32_t kRedAlliance = 0xF87171;
inline constexpr uint32_t kBlueAlliance = 0x60A5FA;
inline constexpr uint32_t kSkills = 0xFACC15;

// tabs and info-tab widgets live in src/ui/layout.cpp (or press E in the sim to edit them)

// ---- behavior ----------------------------------------------------------------------
inline constexpr int kStartPage = 0;          // index into the tab list in src/ui/layout.cpp
inline constexpr int kAutonSeconds = 15;      // match clock lengths
inline constexpr int kDriverSeconds = 105;
inline constexpr int kEndgameSeconds = 15;    // clock turns amber
inline constexpr float kRobotSizeIn = 15;     // robot square on the field map
inline constexpr bool kRumbleOnDeviceProblem = true;
inline constexpr uint32_t kBootTimeoutMs = 10000; // leave splash even if bootComplete() never gets called

} // namespace ui::config
