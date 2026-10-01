# Brain Screen Guide

This explains the robot's touchscreen: how to add autos, how to change what the screen shows, and how to preview it all on a laptop.

**Contents**
1. [What's on the screen](#1-whats-on-the-screen)
2. [Add a new auto](#2-add-a-new-auto) ← most common
3. [Preview the screen on your laptop](#3-preview-the-screen-on-your-laptop)
4. [Change the layout (drag and drop)](#4-change-the-layout-drag-and-drop)
5. [Show your own numbers on the screen](#5-show-your-own-numbers-on-the-screen)
6. [Where everything lives](#6-where-everything-lives)
7. [Competition day checklist](#7-competition-day-checklist)
8. [Problems and fixes](#8-problems-and-fixes)

---

## 1. What's on the screen

Tap the icons on the left side of the brain to switch tabs.

| Tab | What it's for |
|---|---|
| **AUTON** | Pick which auto runs. The pick is saved, so it remembers after a restart. |
| **HOME** | Quick info: battery, heading, temps, claw, intake. You can change what's here. |
| **FIELD** | Map of where the robot thinks it is, with the path it drove. |
| **MOTORS** | Temperature and health of every motor. |
| **SENSORS** | IMU, tracking wheels, distance sensors, and which ports are plugged in. |
| **GRAPH** | Live graphs (drive speed, heading, temps, power). |
| **LOG** | Messages from `console.printf(...)`. |
| **TOOLS** | Calibrate IMU, reset position, PID tests. |

The bar across the top shows the match mode, the time left, and battery.

A red **⚠** badge in the top bar means a device is unplugged or on the wrong port. Tap it to see which one.

---

## 2. Add a new auto

You only touch **3 files**. Example: an auto called `redRush`.

### Step 1: Write the auto

Make a new file: **`src/auton/match_routes/redRush.cpp`**

```cpp
#include "main.h"

void redRush() {
    // Where the robot starts. (x, y, heading)
    // x/y are inches from the center of the field.
    // heading: 0 = forward, 90 = right, 180 = back, 270 = left
    chassis.setPose(-36, -60, 0);

    // Drive to a spot (x, y, give up after 2000 ms)
    chassis.moveToPoint(-36, -24, 2000);

    // Turn to face a direction
    chassis.turnToHeading(90, 1000);

    // Drive backwards and slower, and WAIT until it's done (the "false" at the end)
    chassis.moveToPoint(-12, -24, 2000, {.forwards = false, .maxSpeed = 80}, false);

    // Use mechanisms like normal
    claw.set_value(true);
    lift.move(127);
    pros::delay(300);
    lift.move(0);

    console.printf("redRush done"); // shows up on the LOG tab
}
```

> 💡 **Moves don't wait by default.** The next line starts right away.
> To wait for a move to finish, put `false` as the last thing in the move (like above), or add `chassis.waitUntilDone();` after it.

### Step 2: Tell the code it exists

Open **`include/eternity_template/auton/match_routes.hpp`** and add:

```cpp
void redRush();
```

### Step 3: Put it on the brain's menu

Open **`src/auton/auton_list.cpp`** and add one line inside the list:

```cpp
{"Red Rush", "Rush middle, grab 2, back to bar.", ui::Alliance::Red, redRush, true, -36, -60, 0},
```

What each part means:

| Part | Example | Meaning |
|---|---|---|
| Name | `"Red Rush"` | What the button says on the brain |
| Description | `"Rush middle..."` | Short note shown when you pick it |
| Alliance | `ui::Alliance::Red` | Button color: `Red`, `Blue`, `Skills`, or `Any` |
| Function | `redRush` | The name from Step 1 (no `()`) |
| Start spot *(optional)* | `true, -36, -60, 0` | Same numbers as your `setPose`. Draws the robot on a mini map. Leave these 4 off if you don't care. |

**That's it.** The auto now shows up on the AUTON tab.

### Step 4: Test it

- **On the laptop:** with the simulator open (see section 3), you'll see the new button. The simulator doesn't run your actual moves; it's for checking the menu.
- **On the robot:**
  1. Upload with `pros mu`.
  2. Tap your auto on the **AUTON** tab.
  3. To test without a competition switch, tap **TEST RUN**, then **RUN**. Tap **ABORT** to stop.
  4. Check the **FIELD** tab to see the path it actually drove.

### Step 5: Save it to GitHub

```bash
git add -A && git commit -m "add redRush auto" && git push
```

---

## 3. Preview the screen on your laptop

The simulator shows the exact same screen as the brain, in a window on your Mac.

**First time only:**
```bash
brew install sdl2
```

**Every time:**
```bash
./sim/run.sh --watch
```

It reloads by itself every time you save a file. The first run takes about a minute to set up.

**Controls in the simulator:**

| Key | Does |
|---|---|
| Click | Tap the screen |
| Arrow keys | Drive the pretend robot |
| `1` / `2` / `3` | Disabled / Auton / Driver |
| `c` | Plug or unplug the competition cable |
| `E` | Edit mode (see section 4) |
| `s` | Screenshot |
| `m` | How much screen memory you're using |
| `q` | Quit |

---

## 4. Change the layout (drag and drop)

In the simulator, press **`E`** to start editing. An orange border means you're in edit mode.

**Add stuff:** drag items from the **panel on the right** onto a tab like HOME.

**Change tabs:** click **TABS** at the top of the right panel. Click a tab to show or hide it. Click **+ New info tab** to make a new one.

| To do this | Do this |
|---|---|
| Move something | Drag it |
| Resize | Drag its bottom-right corner |
| Change its text | Click it → press **Enter** → type → press **Enter** |
| Change what number it shows | Click it → press **`d`** (keep pressing to cycle) |
| Change its style | **`t`** (Label, Value, Bar, Gauge, Light) |
| Change its color | **`k`** |
| Copy it | **`c`** |
| Delete it | **Delete** |
| Reorder tabs | Drag the tab icons on the left up or down |
| Rename a tab | Click empty space → press **Enter** → type → **Enter** |
| Change a tab's icon | **`i`** |
| **Save** | **`w`** (pressing `E` to leave edit mode also saves) |

After saving, upload with `pros mu` and the brain will look exactly like the simulator.

> ⚠️ Press **Enter** before you type. If you press Backspace without pressing Enter first, it deletes the whole item instead of a letter.

---

## 5. Show your own numbers on the screen

Want to see a value from your code, like a lift target or which step the auto is on? Add this anywhere in robot code:

```cpp
ui::publish("lift.target", 400);
ui::publish("auton.step", 3);
```

Then, in the simulator's edit mode, add a **Value** widget, click it, and press **`d`** until it says `lift.target`. Or type the name into `src/ui/layout.cpp`.

---

## 6. Where everything lives

| I want to change… | File |
|---|---|
| The list of autos | `src/auton/auton_list.cpp` |
| The default auto (used if there's no SD card) | Bottom of `src/auton/auton_list.cpp` |
| Colors, team name, °F/°C, match timer | `include/eternity_template/ui/ui_config.hpp` |
| Which tabs show, and what's on HOME | `src/ui/layout.cpp` (or use edit mode) |
| What numbers widgets can show | `src/ui/sources.cpp` |
| Added a new motor or sensor | `src/ui/robot_io.cpp` (and `sim/fake_robot.cpp` for the sim) |

<details>
<summary><b>How it's hooked into main.cpp</b> (only needed for a brand new project)</summary>

```cpp
ui::Console console;                        // console.printf(...) goes to the LOG tab

void initialize() {
    ui::init(autonList, defaultAuton);      // FIRST line: shows the loading screen
    ui::setBootStatus("calibrating IMU");   // optional text on the loading screen
    chassis.calibrate();
    ui::bootComplete();                     // loading screen -> main screen
}

void autonomous() { ui::runSelectedAuton(); }

void opcontrol() {
    while (true) {
        if (ui::routineRunning()) { pros::delay(20); continue; } // a test started from the brain is driving
        // ... driver code ...
        pros::delay(20);
    }
}
```

</details>

---

## 7. Competition day checklist

- [ ] **SD card is in the brain.** Without it, the brain forgets your auto pick when it restarts.
- [ ] No red **⚠** badge in the top bar (all devices plugged in).
- [ ] Pick your auto on the **AUTON** tab **before** the match starts. The picker locks once the match is running.
- [ ] The controller screen shows the auto you picked.
- [ ] Battery is charged (top-right of the screen).

---

## 8. Problems and fixes

| Problem | Fix |
|---|---|
| My new auto isn't on the menu | Check you did all 3 steps in section 2, and that the name in `auton_list.cpp` matches your function exactly. |
| The brain forgot my auto pick | Put an SD card in the brain. Also, if you renamed the auto, you have to pick it again. |
| Can't change the auto pick | It locks during a match. Wait until the robot is disabled. |
| Red ⚠ in the top bar | Tap it. It shows which port has a problem. |
| Simulator says `No such file or directory` | Run `cd ~/parth-local/override_eternity && ./sim/run.sh --watch` (make sure you're in the right folder). |
| Simulator says `missing SDL2` | Run `brew install sdl2`. |
| Simulator build failed | Read the error, fix the file, and save again. It retries by itself. |
