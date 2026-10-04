// every route that shows up on the brain's AUTON page (the simulator reads this too).
// {name, description, alliance, function, hasStart, startX, startY, startTheta}
// alliance: ui::Alliance::Red / Blue / Skills / Any
// the start pose is only used for the little preview map.

#include "eternity_template/auton/autons.hpp"
#include "eternity_template/ui/ui.hpp"

extern const std::vector<ui::Auton> autonList;
const std::vector<ui::Auton> autonList = {
    {"Five Pin", "ishaan aura", ui::Alliance::Blue, five_pin, true, 0, -64, 180},
    {"sawp", "sawpy", ui::Alliance::Any, soloAWP},
    {"dummy route", "6in forward route", ui::Alliance::Any, dummy, true, 0, 0, 0},
    {"skilly", "parth skills route", ui::Alliance::Skills, simpleSkills},
};

// picked when the SD card has no saved choice
const char* defaultAuton = "dummy route";
