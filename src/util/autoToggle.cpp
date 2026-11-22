#include "main.h"
#include <cstdlib>
#include <cstring>

static bool hueMatches(const char* color, double hue) {
    if (hue < 0.0 || hue > 360.0) return false;

    if (std::strcmp(color, "red") == 0) {
        return hue <= 20.0 || hue >= 345.0;
    }
    if (std::strcmp(color, "yellow") == 0) {
        return hue >= 25.0 && hue <= 75.0;
    }
    if (std::strcmp(color, "blue") == 0) {
        return hue >= 180.0 && hue <= 260.0;
    }
    return false;
}

static bool isValidColor(const char* color) {
    return color != nullptr &&
        (std::strcmp(color, "red") == 0 ||
         std::strcmp(color, "yellow") == 0 ||
         std::strcmp(color, "blue") == 0);
}

static bool detectsColor(const char* color) {
    return hueMatches(color, toggleColor.get_hue()) &&
        toggleColor.get_proximity() > 100;
}

void toggleUntil(const char* color, uint32_t timeoutMs) {
    if (!isValidColor(color) || timeoutMs == 0) {
        printf("toggleUntil: provide red, yellow, or blue and a nonzero timeout\n");
        toggleMotor.move(0);
        return;
    }

    toggleMotor.set_brake_mode(pros::MotorBrake::hold);
    while (true) {
        toggleMotor.move(-70);
        uint32_t start = pros::millis();
        while (pros::millis() - start < timeoutMs && !detectsColor(color)) {
            pros::delay(10);
        }

        toggleMotor.move(0);
        toggleMotor.brake();
        pros::delay(20);

        if (detectsColor(color)) return;
    }
}

void testingConsistency() {
    static const char* const colors[] = {"blue", "red", "yellow"};
    static const uint32_t sampleTimeoutMs = 1000;

    std::srand(pros::millis());
    printf("trial\ttarget\thit\telapsed_ms\n");
    for (int trial = 0; trial < 50; trial++) {
        const char* color = colors[std::rand() % 3];
        uint32_t start = pros::millis();
        bool hit = false;

        toggleMotor.set_brake_mode(pros::MotorBrake::hold);
        toggleMotor.move(-70);
        while (pros::millis() - start < sampleTimeoutMs) {
            if (detectsColor(color)) {
                hit = true;
                break;
            }
            pros::delay(10);
        }

        toggleMotor.move(0);
        toggleMotor.brake();
        uint32_t elapsedMs = pros::millis() - start;
        printf("%d\t%s\t%s\t%lu\n", trial + 1, color, hit ? "yes" : "no",
               static_cast<unsigned long>(elapsedMs));
        if (trial < 49) pros::delay(1000);
    }
}

