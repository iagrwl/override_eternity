// UI core: boot splash, status bar, nav rail, page switching, the single LVGL timer
// that drives everything, the log buffer, and the modal/toast helpers.

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include "ui_internal.hpp"

namespace ui {

// ---------------------------------------------------------------- state
static std::vector<Auton> s_autons;
static std::atomic<int> s_selected{-1};
static std::atomic<bool> s_bootDone{false};
static std::atomic<int> s_requestedPage{-1};
static char s_bootStatus[48] = "starting up";
static std::atomic<bool> s_bootStatusDirty{true};

static Snapshot s_snap;
static lv_timer_t* s_timer = nullptr;
static uint32_t s_tick = 0;
static uint32_t s_initMs = 0;

// tabs come from src/ui/layout.cpp
static int s_page = 0;
static const Page* s_activePage = nullptr;

static const Page* pageFor(const Tab& t) {
    switch (t.builtin) {
        case Builtin::Auton: return &pageAuton;
        case Builtin::Field: return &pageField;
        case Builtin::Motors: return &pageMotors;
        case Builtin::Sensors: return &pageSensors;
        case Builtin::Graph: return &pageGraph;
        case Builtin::Log: return &pageLog;
        case Builtin::Tools: return &pageTools;
        default: return &pageCustom;
    }
}

// splash widgets
static lv_obj_t* s_splash = nullptr;
static lv_obj_t* s_splashStatus = nullptr;

// main screen widgets
static lv_obj_t* s_main = nullptr;
static lv_obj_t* s_content = nullptr;
static lv_obj_t* s_rail = nullptr;
static lv_obj_t* s_pageTitle = nullptr;
static lv_obj_t* s_modeChip = nullptr;
static lv_obj_t* s_modeLabel = nullptr;
static lv_obj_t* s_fieldLabel = nullptr;
static lv_obj_t* s_timerLabel = nullptr;
static lv_obj_t* s_alertChip = nullptr;
static lv_obj_t* s_alertLabel = nullptr;
static lv_obj_t* s_sdIcon = nullptr;
static lv_obj_t* s_ctrlLabel = nullptr;
static lv_obj_t* s_battLabel = nullptr;
static lv_obj_t* s_runBanner = nullptr;
static lv_obj_t* s_runLabel = nullptr;

// match timer
enum class Mode { Disabled, Auton, Driver };
static Mode s_mode = Mode::Disabled;
static uint32_t s_modeStart = 0;
static uint32_t s_runStart = 0;

// ---------------------------------------------------------------- log ring buffer
static constexpr int kLogLines = 48;
static constexpr int kLogWidth = 72;
static char s_log[kLogLines][kLogWidth];
static int s_logHead = 0, s_logCount = 0;
static std::atomic<uint32_t> s_logVersion{0};

static void logPush(const char* text) {
    hw::logLock();
    uint32_t t = hw::millis();
    // split on newlines so multi-line printf calls still read right
    const char* p = text;
    while (*p) {
        const char* nl = strchr(p, '\n');
        int n = nl ? (int)(nl - p) : (int)strlen(p);
        char* dst = s_log[(s_logHead + s_logCount) % kLogLines];
        snprintf(dst, kLogWidth, "%5.1f  %.*s", t / 1000.0, n, p);
        if (s_logCount < kLogLines) s_logCount++;
        else s_logHead = (s_logHead + 1) % kLogLines;
        if (!nl) break;
        p = nl + 1;
    }
    hw::logUnlock();
    s_logVersion++;
}

void logClear() {
    hw::logLock();
    s_logHead = s_logCount = 0;
    hw::logUnlock();
    s_logVersion++;
}

int logCopy(char* out, int len) {
    hw::logLock();
    int pos = 0;
    out[0] = 0;
    for (int i = 0; i < s_logCount && pos < len - 1; i++) {
        pos += snprintf(out + pos, len - pos, i ? "\n%s" : "%s", s_log[(s_logHead + i) % kLogLines]);
    }
    hw::logUnlock();
    return s_logCount;
}

uint32_t logVersion() { return s_logVersion; }

void log(const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    printf("%s%s", buf, buf[0] && buf[strlen(buf) - 1] == '\n' ? "" : "\n");
    logPush(buf);
}

void Console::printf(const char* fmt, ...) {
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = 0; // one entry per call
    ::printf("%s\n", buf);
    logPush(buf);
}
void Console::println(const char* text) { log("%s", text); }
void Console::clear() { logClear(); }
void Console::focus() {
    int i = pageIndex(&pageLog);
    if (i >= 0) s_requestedPage = i;
}

// ---------------------------------------------------------------- autons
const std::vector<Auton>& autons() { return s_autons; }
int selectedIndex() { return s_selected; }

const Auton* selectedAuton() {
    int i = s_selected;
    return (i >= 0 && i < (int)s_autons.size()) ? &s_autons[i] : nullptr;
}

bool selectionLocked() { return s_snap.compConnected && !s_snap.disabled; }

void selectAuton(int index) {
    if (index < 0 || index >= (int)s_autons.size() || index == s_selected) return;
    s_selected = index;
    const Auton& a = s_autons[index];
    hw::saveSelection(a.name);
    char line[32];
    snprintf(line, sizeof line, "%-6s %.12s", allianceName(a.alliance), a.name);
    hw::controllerLine(line);
    log("selected auton: %s", a.name);
}

void runSelectedAuton() {
    const Auton* a = selectedAuton();
    if (!a || !a->run) {
        log("no auton selected");
        return;
    }
    log("AUTON START: %s", a->name);
    uint32_t t0 = hw::millis();
    a->run();
    log("AUTON DONE: %s (%.1fs)", a->name, (hw::millis() - t0) / 1000.0);
}

bool routineRunning() { return hw::routineRunning(); }

const Snapshot& snapshot() { return s_snap; }

int pageIndex(const Page* p) {
    for (size_t i = 0; i < tabs().size(); i++)
        if (pageFor(tabs()[i]) == p) return i;
    return -1;
}

int currentPage() { return s_page; }

// ---------------------------------------------------------------- toast + confirm
void toast(const char* text, lv_color_t col) {
    lv_obj_t* t = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(t);
    lv_obj_add_style(t, &styles().chip, 0);
    lv_obj_set_style_bg_color(t, color::panel2(), 0);
    lv_obj_set_style_border_width(t, 1, 0);
    lv_obj_set_style_border_color(t, col, 0);
    lv_obj_set_style_pad_hor(t, 14, 0);
    lv_obj_set_style_pad_ver(t, 6, 0);
    lv_obj_set_size(t, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t* l = makeLabel(t, &lv_font_montserrat_12, color::text(), text);
    LV_UNUSED(l);
    lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 0, -40); // above the tab buttons
    lv_obj_fade_in(t, 150, 0);
    lv_obj_fade_out(t, 300, 1400);
    lv_obj_del_delayed(t, 1750);
}

static void (*s_confirmOk)() = nullptr;
static lv_obj_t* s_modal = nullptr;

static void closeModal() {
    if (s_modal) lv_obj_del(s_modal);
    s_modal = nullptr;
}

static void onModalBtn(lv_event_t* e) {
    bool ok = (bool)(intptr_t)lv_event_get_user_data(e);
    auto fn = s_confirmOk;
    closeModal();
    if (ok && fn) fn();
}

void confirm(const char* title, const char* body, const char* okText, void (*onOk)()) {
    closeModal();
    s_confirmOk = onOk;
    s_modal = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_modal);
    lv_obj_set_size(s_modal, 480, 240);
    lv_obj_set_style_bg_color(s_modal, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_modal, LV_OPA_70, 0);
    lv_obj_add_flag(s_modal, LV_OBJ_FLAG_CLICKABLE); // swallow taps behind the dialog

    lv_obj_t* box = lv_obj_create(s_modal);
    lv_obj_remove_style_all(box);
    lv_obj_add_style(box, &styles().panel, 0);
    lv_obj_set_style_bg_color(box, color::bg(), 0);
    lv_obj_set_size(box, 300, 150);
    lv_obj_center(box);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t = makeLabel(box, &lv_font_montserrat_18, color::text(), title);
    lv_obj_set_pos(t, 16, 14);

    lv_obj_t* b = makeLabel(box, &lv_font_montserrat_14, color::muted(), body);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(b, 268);
    lv_obj_set_pos(b, 16, 44);

    lv_obj_t* cancel = makeButton(box, "Cancel", 120, 36, onModalBtn, (void*)0);
    lv_obj_set_pos(cancel, 16, 100);
    lv_obj_t* ok = makeButton(box, okText, 140, 36, onModalBtn, (void*)1);
    lv_obj_set_style_bg_color(ok, color::text(), 0);
    lv_obj_set_style_border_width(ok, 0, 0);
    lv_obj_set_style_text_color(ok, lv_color_black(), 0);
    lv_obj_set_pos(ok, 144, 100);
}

// ---------------------------------------------------------------- splash
static void animWidth(void* obj, int32_t v) { lv_obj_set_width((lv_obj_t*)obj, v); }
static void animOpa(void* obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t*)obj, v, 0); }

static void buildSplash() {
    s_splash = lv_obj_create(nullptr);
    lv_obj_remove_style_all(s_splash);
    lv_obj_add_style(s_splash, &styles().screen, 0);
    lv_obj_clear_flag(s_splash, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name = makeLabel(s_splash, &lv_font_montserrat_30, color::text(), config::kTeamName);
    lv_obj_set_style_text_letter_space(name, 12, 0);
    lv_obj_align(name, LV_ALIGN_CENTER, 6, -18);

    lv_obj_t* team = makeLabel(s_splash, &lv_font_montserrat_10, color::muted(), config::kTeamNumber);
    lv_obj_set_style_text_letter_space(team, 6, 0);
    lv_obj_align(team, LV_ALIGN_CENTER, 3, 14);

    // thin line that sweeps back and forth while booting
    lv_obj_t* line = lv_obj_create(s_splash);
    lv_obj_remove_style_all(line);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(line, color::text(), 0);
    lv_obj_set_size(line, 0, 1);
    lv_obj_align(line, LV_ALIGN_CENTER, 0, 36);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, line);
    lv_anim_set_exec_cb(&a, animWidth);
    lv_anim_set_values(&a, 0, 140);
    lv_anim_set_time(&a, 900);
    lv_anim_set_playback_time(&a, 900);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, name);
    lv_anim_set_exec_cb(&a, animOpa);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 600);
    lv_anim_start(&a);

    s_splashStatus = makeLabel(s_splash, &lv_font_montserrat_10, color::muted(), s_bootStatus);
    lv_obj_set_style_text_letter_space(s_splashStatus, 1, 0);
    lv_obj_align(s_splashStatus, LV_ALIGN_BOTTOM_MID, 0, -16);

    if (config::kShowBuildStamp) {
        lv_obj_t* build = makeLabel(s_splash, &lv_font_montserrat_10, color::dim(), __DATE__);
        lv_obj_align(build, LV_ALIGN_BOTTOM_RIGHT, -10, -6);
    }
    lv_scr_load(s_splash);
}

// ---------------------------------------------------------------- main screen
// Nothing at the top: VEXos already draws its own header (program name, timer,
// battery) above the program, so ours would just stack under it. Tabs are real
// buttons along the bottom instead.
static constexpr lv_coord_t kBarY = kContentY + kContentH + 2, kBarH = 240 - kBarY - 2, kBarX = 4, kBarW = 472;
static bool s_warnShown = false;

int tabIndexAtX(int x) {
    int n = tabs().size();
    if (x < kBarX || x >= kBarX + kBarW || !n) return -1;
    return (x - kBarX) * n / kBarW;
}

static void onRail(lv_event_t* e) {
    lv_obj_t* m = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(m);
    if (id != LV_BTNMATRIX_BTN_NONE) showPage(id);
}

static void setRailMap() {
    static std::vector<std::string> labels;
    static std::vector<const char*> map;
    labels.clear();
    map.clear();
    for (size_t i = 0; i < tabs().size(); i++) {
        // a warning mark on SENSORS when a device / SD card / controller is missing
        bool warn = s_warnShown && tabs()[i].builtin == Builtin::Sensors;
        labels.push_back(warn ? tabs()[i].title + " " LV_SYMBOL_WARNING : tabs()[i].title);
    }
    for (auto& l : labels) map.push_back(l.c_str());
    map.push_back("");
    lv_btnmatrix_set_map(s_rail, map.data());
    lv_btnmatrix_set_btn_ctrl_all(s_rail, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(s_rail, true);
    lv_btnmatrix_set_btn_ctrl(s_rail, s_page, LV_BTNMATRIX_CTRL_CHECKED);
}

static void onAbort(lv_event_t*) {
    hw::abortRoutine();
    toast("aborting", color::bad());
}

static void buildMain() {
    s_main = lv_obj_create(nullptr);
    lv_obj_remove_style_all(s_main);
    lv_obj_add_style(s_main, &styles().screen, 0);
    lv_obj_clear_flag(s_main, LV_OBJ_FLAG_SCROLLABLE);

    s_content = makeBox(s_main, kContentX, kContentY, kContentW, kContentH);

    // ---- tab buttons along the bottom
    s_rail = lv_btnmatrix_create(s_main);
    lv_obj_remove_style_all(s_rail);
    setRailMap();
    lv_obj_set_pos(s_rail, kBarX, kBarY);
    lv_obj_set_size(s_rail, kBarW, kBarH);
    lv_obj_set_style_pad_column(s_rail, 4, 0);
    lv_obj_set_style_bg_opa(s_rail, LV_OPA_COVER, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_rail, color::bg(), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_rail, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_rail, color::line(), LV_PART_ITEMS);
    lv_obj_set_style_radius(s_rail, 6, LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_rail, &lv_font_montserrat_12, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_rail, color::muted(), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(s_rail, color::text(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(s_rail, color::text(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(s_rail, color::bg(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(s_rail, color::panel2(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(s_rail, onRail, LV_EVENT_VALUE_CHANGED, nullptr);

    s_alertChip = s_alertLabel = s_sdIcon = s_ctrlLabel = s_modeChip = s_modeLabel = nullptr;
    s_timerLabel = s_battLabel = s_fieldLabel = s_pageTitle = nullptr;

    // routine-running strip over the top of the content (hidden until something runs)
    s_runBanner = makeBox(s_main, 0, 0, 480, 26);
    lv_obj_set_style_bg_opa(s_runBanner, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_runBanner, color::bg(), 0);
    lv_obj_set_style_border_side(s_runBanner, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(s_runBanner, 1, 0);
    lv_obj_set_style_border_color(s_runBanner, color::warn(), 0);
    s_runLabel = makeLabel(s_runBanner, &lv_font_montserrat_12, color::warn(), "");
    lv_obj_align(s_runLabel, LV_ALIGN_LEFT_MID, kContentX, 0);
    lv_obj_t* abort = makeButton(s_runBanner, "STOP", 64, 20, onAbort);
    lv_obj_set_style_text_font(abort, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(abort, color::bad(), 0);
    lv_obj_set_style_border_color(abort, color::bad(), 0);
    lv_obj_align(abort, LV_ALIGN_RIGHT_MID, -kContentX, 0);
    lv_obj_add_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN);
}

void showPage(int index) {
    if (index < 0 || index >= (int)tabs().size() || !s_content) return;
    if (s_activePage && s_activePage->teardown) s_activePage->teardown();
    lv_obj_clean(s_content); // only the visible page lives in LVGL memory
    s_page = index;
    s_activePage = pageFor(tabs()[index]);
    lv_btnmatrix_set_btn_ctrl(s_rail, index, LV_BTNMATRIX_CTRL_CHECKED);
    s_activePage->build(s_content);
    s_activePage->update(s_snap);
}

void refreshPage() { showPage(s_page); }

void rebuildTabs() {
    if (!s_rail) return;
    int n = tabs().size();
    if (s_page >= n) s_page = n - 1;
    setRailMap();
    showPage(s_page);
}

// ---------------------------------------------------------------- status
static void updateStatus(const Snapshot& s) {
    Mode mode = s.disabled ? Mode::Disabled : s.autonomous ? Mode::Auton : Mode::Driver;
    if (mode != s_mode) {
        s_mode = mode;
        s_modeStart = s.timeMs;
        if (mode != Mode::Disabled) log("mode -> %s", mode == Mode::Auton ? "AUTONOMOUS" : "DRIVER");
    }

    bool problem = s.deviceProblems > 0 || !s.sd || !s.controllerOk;
    if (problem != s_warnShown) {
        s_warnShown = problem;
        setRailMap();
    }

    if (hw::routineRunning()) {
        if (lv_obj_has_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_runBanner);
            s_runStart = s.timeMs;
        }
        setTextf(s_runLabel, "running  %.1fs", (s.timeMs - s_runStart) / 1000.0);
    } else if (!lv_obj_has_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN);
    }
}

// ---------------------------------------------------------------- main loop
static void bootFinish() {
    buildMain();
    int start = config::kStartPage;
    showPage(start < (int)tabs().size() ? start : 0);
    updateStatus(s_snap);
    // plain load + delete keeps peak LVGL memory down (no two-screen fade)
    lv_scr_load(s_main);
    lv_obj_del(s_splash);
    s_splash = nullptr;
    s_splashStatus = nullptr;

    if (s_snap.deviceProblems) {
        if (config::kRumbleOnDeviceProblem) hw::rumble("-.-");
        toast(LV_SYMBOL_WARNING "  check devices", color::bad());
    }
}

static void tick(lv_timer_t*) {
    s_tick++;
    hw::sample(s_snap);

    if (!s_main) {
        if (!s_splash) {
            initStyles();
            buildSplash();
        }
        if (s_bootStatusDirty.exchange(false)) lv_label_set_text(s_splashStatus, s_bootStatus);
        // never get stuck on the splash if bootComplete() is forgotten
        if (s_bootDone || hw::millis() - s_initMs > config::kBootTimeoutMs) bootFinish();
        return;
    }

    if (s_tick % 2 == 0) {
        recordTrail(s_snap);
        recordGraph(s_snap);
    }

    int req = s_requestedPage.exchange(-1);
    if (req >= 0 && req != s_page) showPage(req);

    if (s_activePage) s_activePage->update(s_snap);
    if (s_tick % 4 == 0) updateStatus(s_snap);
}

void init(const std::vector<Auton>& list, const char* defaultAuton) {
    s_autons = list;
    s_initMs = hw::millis();

    char saved[48];
    const char* want = hw::loadSelection(saved, sizeof saved) ? saved : defaultAuton;
    s_selected = s_autons.empty() ? -1 : 0;
    for (size_t i = 0; want && i < s_autons.size(); i++) {
        if (strcmp(s_autons[i].name, want) == 0) s_selected = i;
    }
    if (const Auton* a = selectedAuton()) log("auton loaded: %s", a->name);

    if (!s_timer) {
        // everything LVGL happens inside this timer, on the display task
        s_timer = lv_timer_create(tick, 50, nullptr);
    }
}

void setBootStatus(const char* text) {
    snprintf(s_bootStatus, sizeof s_bootStatus, "%s", text);
    s_bootStatusDirty = true;
    log("boot: %s", text);
}

void bootComplete() { s_bootDone = true; }

} // namespace ui
