# Brain UI guide

The brain UI and the laptop simulator run the same code (`src/ui/`). Anything you change and preview in the sim shows up on the brain the next time you upload.

## 1. Preview on your laptop

```
brew install sdl2          # once
./sim/run.sh --watch       # opens the sim, rebuilds every time you save
```

Arrow keys drive the sim robot. `1`/`2`/`3` switch between disabled, auton and driver. `c` plugs or unplugs the comp cable. `s` saves a screenshot, and `m` shows how much of the brain's 32 KB UI memory you're using.

## 2. Change the layout by dragging (edit mode)

Drag anything from the **palette on the right** onto a custom tab (like HOME), or press **`E`**.

| do this | how |
|---|---|
| move a widget | drag it (arrows nudge it, shift+arrows move 10 px) |
| resize | drag its bottom-right corner (or cmd+arrows) |
| change the text | select the widget, press enter, type, press enter |
| change what it shows | `d` / `D` cycles through the data sources |
| change the widget type | `t` (Label, Value, Bar, Gauge, Light) |
| change the color | `k` |
| copy / delete | `c` / `delete` |
| new tab | `n`, then enter to rename it |
| remove the current tab | `x` twice |
| reorder tabs | `,` and `.` |
| change the tab icon | `i` |
| **save** | `w` (leaving edit mode with `e` also saves) |

Saving writes to `src/ui/layout.cpp`. Upload the code and the brain will look the same.

## 3. Change things by hand

- **Colors, team name, °F/°C, match timers, temperature limits:** `include/eternity_template/ui/ui_config.hpp`
- **Tabs and widgets:** `src/ui/layout.cpp` has one line per tab and one line per widget. Delete a tab's line to remove it, move lines to reorder.
- **Data you can show:** listed at the top of `src/ui/sources.cpp`.

## 4. Show your own numbers on the brain

From anywhere in robot code:

```cpp
ui::publish("lift.target", 400);
ui::publish("auton.step", 3);
```

Then give a widget the source `lift.target`. You can type it in `layout.cpp`, or press `d` in edit mode while the sim is running something that publishes it.

## 5. Add an auto

1. Write it in `src/auton/match_routes/myAuto.cpp`:
   ```cpp
#include "main.h"
void myAuto() {
    chassis.setPose(0, -60, 0);
    chassis.moveToPoint(0, -36, 2000);
}
   ```
2. Declare it in `include/eternity_template/auton/match_routes.hpp`:
   ```cpp
void myAuto();
   ```
3. Add one line to `src/auton/auton_list.cpp`:
   ```cpp
{"My Auto", "what it does", ui::Alliance::Red, myAuto, true, 0, -60, 0},
   ```
   The fields are name, description, alliance (`Red` / `Blue` / `Skills` / `Any`), function, and then (optional) the start pose for the preview map.

It shows up as a card on the AUTON tab, in the sim and on the brain. Whatever card is picked is what runs in autonomous, and the pick is saved to the SD card.

## 6. How it hooks into the brain code

Everything is already wired into `src/main.cpp`. For a new project, these lines are all you need:

```cpp
ui::Console console;                        // console.printf(...) -> LOG tab

void initialize() {
    ui::init(autonList, defaultAuton);      // FIRST line: shows the splash
    ui::setBootStatus("calibrating IMU");   // optional status text on the splash
    chassis.calibrate();
    ui::bootComplete();                     // splash -> dashboard
}

void autonomous() { ui::runSelectedAuton(); }

void opcontrol() {
    while (true) {
        if (ui::routineRunning()) { pros::delay(20); continue; } // a brain-started test owns the robot
        // ... driver code ...
        pros::delay(20);
    }
}
```

If you add a motor or sensor to `setup.hpp`, also add it to `src/ui/robot_io.cpp` so it appears on the MOTORS and SENSORS tabs. Do the same in `sim/fake_robot.cpp` if you want it in the sim.
