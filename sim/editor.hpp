#pragma once

// sim-only layout editor (press E in the sim window)
#include <SDL.h>

namespace editor {
void toggle();
bool active();
bool handle(const SDL_Event& e, int mouseX, int mouseY); // true = event used by the editor
void drawOverlay(SDL_Renderer* ren);
const char* title();
void restoreState();

// widget palette shown to the right of the brain screen; drag items onto a custom tab
constexpr int kPaletteW = 130;
void buildPalette(); // call with the palette's LVGL display set as default
} // namespace editor
