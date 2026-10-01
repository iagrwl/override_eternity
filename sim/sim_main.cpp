// Desktop window for the brain UI. Runs the exact same src/ui code as the robot,
// at 2x size; click = touch. Keys are printed on startup (see kHelp).

#include <cstdio>
#include <fstream>
#include <sstream>
#include <SDL.h>
#include "editor.hpp"
#include "fake_robot.hpp"
#include "ui_internal.hpp"

extern const std::vector<ui::Auton> autonList;
extern const char* defaultAuton;

static constexpr int W = 480, H = 240, SCALE = 2, PW = editor::kPaletteW;
static uint32_t fb[H][W];
static uint32_t pfb[H][PW]; // widget palette on the right
static bool dirty = true;
static int mouseX = 0, mouseY = 0;
static bool mouseDown = false;

static const char* kHelp =
    "\n  brain sim  -  click = touch\n"
    "  arrows  drive the robot          1/2/3  disabled / auton / driver\n"
    "  c       plug/unplug comp cable   f      field control vs switch\n"
    "  i       intake on/off            space  claw\n"
    "  u       unplug a lift motor      b      drain battery\n"
    "  r       reset robot pose         s      screenshot -> sim/screenshots/\n"
    "  m       print UI memory          q/esc  quit\n"
    "  e       EDIT MODE: drag/resize widgets, add/remove tabs, w saves to src/ui/layout.cpp\n\n";

static void flush(lv_disp_drv_t* d, const lv_area_t* a, lv_color_t* c) {
    for (int y = a->y1; y <= a->y2; y++)
        for (int x = a->x1; x <= a->x2; x++) fb[y][x] = (c++)->full | 0xFF000000;
    dirty = true;
    lv_disp_flush_ready(d);
}

static void flushPalette(lv_disp_drv_t* d, const lv_area_t* a, lv_color_t* c) {
    for (int y = a->y1; y <= a->y2; y++)
        for (int x = a->x1; x <= a->x2; x++) pfb[y][x] = (c++)->full | 0xFF000000;
    dirty = true;
    lv_disp_flush_ready(d);
}

static void readTouch(lv_indev_drv_t*, lv_indev_data_t* d) {
    d->point.x = mouseX;
    d->point.y = mouseY;
    d->state = mouseDown ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void printMem() {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    unsigned used = m.total_size - m.free_size;
    // 64-bit desktop pointers make LVGL objects ~1.4x bigger than on the V5
    printf("UI memory: %.1f KB here  (~%.1f KB of 32 KB on the brain)%s\n", used / 1024.0, used / 1024.0 / 1.4,
           used / 1.4 > 30 * 1024 ? "  <-- TOO CLOSE, trim widgets" : "");
}

static void screenshot() {
    static int n = 0;
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(fb, W, H, 32, W * 4, SDL_PIXELFORMAT_ARGB8888);
    char path[64];
    snprintf(path, sizeof path, "screenshots/brain_%03d.bmp", n++);
    if (SDL_SaveBMP(s, path) == 0) printf("saved sim/%s\n", path);
    SDL_FreeSurface(s);
}

static void setMode(bool disabled, bool auton) {
    robot.compConnected = true;
    robot.disabled = disabled;
    robot.autonomous = auton;
}

int main(int, char**) { // SDL needs this exact signature on Windows
    setvbuf(stdout, nullptr, _IONBF, 0);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* win = SDL_CreateWindow("V5 brain sim", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (W + PW) * SCALE,
                                       H * SCALE, SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(ren, W + PW, H);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_Texture* ptex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PW, H);

    lv_init();
    static lv_disp_draw_buf_t buf;
    static lv_color_t b1[W * 40];
    lv_disp_draw_buf_init(&buf, b1, nullptr, W * 40);
    static lv_disp_drv_t drv;
    lv_disp_drv_init(&drv);
    drv.hor_res = W, drv.ver_res = H, drv.flush_cb = flush, drv.draw_buf = &buf;
    lv_disp_t* brain = lv_disp_drv_register(&drv);
    static lv_indev_drv_t in;
    lv_indev_drv_init(&in);
    in.type = LV_INDEV_TYPE_POINTER, in.read_cb = readTouch;
    lv_indev_drv_register(&in);
    lv_disp_load_scr(lv_obj_create(nullptr)); // PROS starts with a blank screen too

    // second LVGL display for the drag-in widget palette (sim only)
    static lv_disp_draw_buf_t pbuf;
    static lv_color_t pb1[PW * H];
    lv_disp_draw_buf_init(&pbuf, pb1, nullptr, PW * H);
    static lv_disp_drv_t pdrv;
    lv_disp_drv_init(&pdrv);
    pdrv.hor_res = PW, pdrv.ver_res = H, pdrv.flush_cb = flushPalette, pdrv.draw_buf = &pbuf;
    lv_disp_t* palette = lv_disp_drv_register(&pdrv);
    ui::initStyles();
    lv_disp_set_default(palette);
    editor::buildPalette();
    lv_disp_set_default(brain);

    printf("%s", kHelp);
    ui::init(autonList, defaultAuton);
    ui::setBootStatus("calibrating IMU - don't touch");

    bool booted = false;
    uint32_t last = SDL_GetTicks();
    for (bool quit = false; !quit;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            if (editor::handle(e, mouseX, mouseY)) continue; // edit mode ate it
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) mouseDown = true;
            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) mouseDown = false;
            if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN) {
                int x = e.type == SDL_MOUSEMOTION ? e.motion.x : e.button.x;
                int y = e.type == SDL_MOUSEMOTION ? e.motion.y : e.button.y;
                mouseX = x < W ? x : W - 1, mouseY = y; // logical size already maps window -> logical px
            }
            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                switch (e.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_1: setMode(true, false); break;
                    case SDLK_2: setMode(false, true); break;
                    case SDLK_3: setMode(false, false); break;
                    case SDLK_c: robot.compConnected = !robot.compConnected; break;
                    case SDLK_f: robot.fieldControl = !robot.fieldControl; break;
                    case SDLK_i: robot.intakeOn = !robot.intakeOn; break;
                    case SDLK_SPACE: robot.clawOpen = !robot.clawOpen; break;
                    case SDLK_u: robot.unplugLift = !robot.unplugLift; break;
                    case SDLK_b: robot.battery = robot.battery > 10 ? robot.battery - 10 : 100; break;
                    case SDLK_r: robot.x = 0, robot.y = 0, robot.theta = 0; break;
                    case SDLK_s: screenshot(); break;
                    case SDLK_m: printMem(); break;
                    case SDLK_e: editor::toggle(); break;
                }
            }
        }
        const Uint8* k = SDL_GetKeyboardState(nullptr);
        if (editor::active()) {
            robot.throttle = robot.turn = 0;
        } else if (!ui::routineRunning()) {
            robot.throttle = (k[SDL_SCANCODE_UP] ? 1.f : 0.f) - (k[SDL_SCANCODE_DOWN] ? 1.f : 0.f);
            robot.turn = ((k[SDL_SCANCODE_RIGHT] ? 1.f : 0.f) - (k[SDL_SCANCODE_LEFT] ? 1.f : 0.f)) * 0.6f;
        }

        uint32_t now = SDL_GetTicks();
        lv_tick_inc(now - last);
        last = now;
        lv_timer_handler();
        if (!booted && now > 1500) { // pretend IMU calibration took 1.5 s
            booted = true;
            ui::bootComplete();
        }
        if (booted && now > 1700) editor::restoreState(); // reopen the tab you were on before a rebuild

        if (dirty || editor::active()) {
            SDL_UpdateTexture(tex, nullptr, fb, W * 4);
            SDL_UpdateTexture(ptex, nullptr, pfb, PW * 4);
            SDL_RenderClear(ren);
            SDL_Rect brainRect = {0, 0, W, H}, palRect = {W, 0, PW, H};
            SDL_RenderCopy(ren, tex, nullptr, &brainRect);
            SDL_RenderCopy(ren, ptex, nullptr, &palRect);
            editor::drawOverlay(ren);
            SDL_RenderPresent(ren);
            SDL_SetWindowTitle(win, editor::title());
            dirty = false;
        } else {
            SDL_Delay(5);
        }
    }
    SDL_Quit();
    return 0;
}
