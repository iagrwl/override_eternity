// every route that shows up on the brain's AUTON page (the simulator reads this too).
// {name, description, alliance, function, hasStart, startX, startY, startTheta}
// alliance: ui::Alliance::Red / Blue / Skills / Any
// the start pose is only used for the little preview map.

#include "eternity_template/auton/autons.hpp"
#include "eternity_template/ui/ui.hpp"

extern const std::vector<ui::Auton> autonList;
const std::vector<ui::Auton> autonList = {
    {"five pin", "ishy", ui::Alliance::Blue, five_pin, true, 0, -64, 180},
    {"dummy route", "6in forward route", ui::Alliance::Any, dummy, true, 0, 0, 0},
    {"simple skilly", "simple skill route", ui::Alliance::Skills, simpleSkills},
    {"advanced skilly", "advanced skill route", ui::Alliance::Skills, advancedSkills},
    {"three pin C - Blue", "close side; sort blue", ui::Alliance::Blue, [] { three_pin_C(false); }},
    {"three pin C - Red", "close side; sort red", ui::Alliance::Red, [] { three_pin_C(true); }},
    {"three pin F - Blue", "far side; sort blue", ui::Alliance::Blue, [] { three_pin_F(false); }},
    {"three pin F - Red", "far side; sort red", ui::Alliance::Red, [] { three_pin_F(true); }},
};

// picked when the SD card has no saved choice
const char* defaultAuton = "dummy route";
