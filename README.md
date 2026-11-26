<h1 style="font-size: 100px;">eternity repo clone directions</h1>

### This repository contains code for team 42824A Eternity's Robot, below are instructions to get this code onto your computer:

## **Project Setup Guide**

This guide walks you through getting this code on your local machine.

### **Requirements:**

[PROS CLI + VS Code Extension](https://pros.cs.purdue.edu/v5/getting-started/index.html)

[Git](https://github.com/git-guides/install-git)

## 1. Clone the Repository

### Navigate to your desired folder (e.g., Desktop)
  1. Click on a folder
  2. Click on the 3 dots near the top of your screen
  3. Click on **"Copy Path"**

![Copy Path](https://static1.howtogeekimages.com/wordpress/wp-content/uploads/2024/01/3-copy-path-file-explorer-toolbar-option.jpg)

Then, go to your terminal (ex. Git CMD, VSC Terminal, etc.) and type:

cd ***-insert your copied path here-***

### Clone the repo
Enter the following commands into your terminal:

git clone https://github.com/iagrwl/pushbackGreatPlanes.git
cd pushbackGreatPlanes

## 2. Open in VS Code

  1. Open VS Code.
  2. Go to File > Open Folder.
  3. Select the folder you just cloned.

d compiled files and ensures a clean build.

# Done!

## Monte Carlo Localization (MCL)

The MCL module fuses LemLib's tracking-wheel/IMU odometry with the four V5 distance
sensors and a field-wall model. LemLib remains the odometry source; the filter only
estimates x/y. It never changes heading.

### Coordinates and setup

Positions use inches from the center of the 144 x 144 inch field, with +x to the
right and +y toward the far side. The modeled inside wall faces are at x/y = +/-71
inches. LemLib's default pose API returns x/y in inches and heading in degrees.
The copied 0.5.5 header documents heading units and a `standardPos` option, but does
not spell out the default positive direction. This project's autonomous code treats
0 degrees as +y and positive clockwise (for example, `three_pin_C` sets 90 degrees
for east); MCL follows that project convention internally in radians. Confirm the
direction on the robot before enabling corrections; conversion helpers are in
`mcl.hpp`.

Distance sensor ports are FRONT 16, RIGHT 19, BACK 20, LEFT 11, configured in
`include/eternity_template/setup.hpp`. Measure the sensor center offsets from the
robot center and fill the `(right, forward)` inch values in
`include/eternity_template/util/mcl_config.hpp`; zero placeholders will make the
predicted ranges inaccurate. The sensor angle offsets are already set to 0/90/180/270.
PROS `Distance::get()` returns millimeters, `PROS_ERR` on an API error, and 9999
when it sees no object. MCL validates the reading and converts it to inches once.

### Starting pose and modes

MCL starts in `LOG_ONLY` in `initialize()`. After setting a known start pose, call
`mclLocalizer.initializeFromChassis()` (or pass the exact pose to
`mclLocalizer.initialize(pose)`) to seed the particle cloud. Large pose jumps over
12 inches are also treated as a LemLib pose reset and reseed automatically. Select
a mode at runtime with:

```cpp
mclLocalizer.setMode(mcl::Mode::OFF);
mclLocalizer.setMode(mcl::Mode::LOG_ONLY); // default: estimates and logs only
mclLocalizer.setMode(mcl::Mode::CORRECT);  // fractional x/y corrections
```

`CORRECT` moves LemLib's x/y 10% toward the MCL estimate per update when the
weighted particle spread is below the configured limit. It preserves the latest
heading and shifts the particle cloud along with each correction. Its gain,
confidence threshold, particle count, noise, sensor sigma, wall model, and sensor
offsets are centralized in `mcl_config.hpp`. Terminal diagnostics (LemLib/MCL pose,
error, spread, loop time, and each sensor's raw/valid/expected range) are disabled
by default so they do not mix with route telemetry on stdout. Set
`Config::debugLogging = true` in `mcl_config.hpp` when you want the 5 Hz diagnostic
lines during tuning.

The local LemLib template is version 0.5.5. Its public headers confirm the
`getPose(bool radians = false, bool standardPos = false)` and `setPose(...)` APIs,
but the project contains only the compiled `LemLib.a`, not the odometry source.
The MCL mutex protects its own state and serializes its own calls; it cannot lock
LemLib's private odometry task. CORRECT therefore uses only LemLib's public pose
API and remains opt-in. Verify it carefully on the robot before relying on the
concurrent pose update.

### Tuning order

1. With MCL in `LOG_ONLY`, check each sensor's logged expected and actual range at
   measured field positions; fill in and tune the mounting offsets.
2. Start stationary with a slightly wrong pose and check that the estimate moves
   toward the true position without oscillating.
3. Drive straight lines and squares in `LOG_ONLY`; compare MCL and LemLib drift.
4. Tune motion noise and measurement sigma, then particle count and ESS threshold.
5. Enable `CORRECT` with a small gain and confirm it does not disturb motion control.

The desktop `sim/` application simulates the brain UI, not robot motion or distance
sensors, so it cannot validate MCL sensor fusion.




