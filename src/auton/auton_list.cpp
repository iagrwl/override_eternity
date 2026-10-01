// every route that shows up on the brain's AUTON page (the simulator reads this too).
// {name, description, alliance, function, hasStart, startX, startY, startTheta}
// alliance: ui::Alliance::Red / Blue / Skills / Any
// the start pose is only used for the little preview map.

#include "eternity_template/auton/autons.hpp"
#include "eternity_template/ui/ui.hpp"

extern const std::vector<ui::Auton> autonList;
const std::vector<ui::Auton> autonList = {
    {"Five Pin", "Lift flick, sweep to (50,-47), claw drop at (24,-24).", ui::Alliance::Any, five_pin, true, 0, -64,
     180},
    {"Solo AWP", "Solo autonomous win point.", ui::Alliance::Any, soloAWP},
    {"Simple Route", "Drive 6in, turn 90, drive 4in. Sanity check.", ui::Alliance::Any, simpleRoute, true, 0, 0, 0},
    {"Basic Parth", "Odom square test with printPose at each step.", ui::Alliance::Any, basicParth, true, 0, 0, 0},
    {"Skills", "Programming skills run.", ui::Alliance::Skills, simpleSkills},
};

// picked when the SD card has no saved choice
const char* defaultAuton = "Five Pin";
