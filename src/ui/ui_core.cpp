// UI core: boot splash, status bar, nav rail, page switching, the single LVGL timer
// that drives everything, the log buffer, and the modal/toast helpers.

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
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
    lv_obj_set_style_bg_color(t, col, 0);
    lv_obj_set_style_pad_hor(t, 14, 0);
    lv_obj_set_style_pad_ver(t, 6, 0);
    lv_obj_set_size(t, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_t* l = makeLabel(t, &lv_font_montserrat_14, lv_color_white(), text);
    LV_UNUSED(l);
    lv_obj_align(t, LV_ALIGN_BOTTOM_MID, 22, -10);
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
    lv_obj_set_style_border_color(box, color::warn(), 0);
    lv_obj_set_size(box, 300, 150);
    lv_obj_center(box);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* t = makeLabel(box, &lv_font_montserrat_18, color::warn(), "");
    setTextf(t, LV_SYMBOL_WARNING "  %s", title);
    lv_obj_set_pos(t, 16, 14);

    lv_obj_t* b = makeLabel(box, &lv_font_montserrat_14, color::muted(), body);
    lv_label_set_long_mode(b, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(b, 268);
    lv_obj_set_pos(b, 16, 44);

    lv_obj_t* cancel = makeButton(box, "Cancel", 120, 36, onModalBtn, (void*)0);
    lv_obj_set_pos(cancel, 16, 100);
    lv_obj_t* ok = makeButton(box, okText, 140, 36, onModalBtn, (void*)1);
    lv_obj_set_style_bg_color(ok, color::warn(), 0);
    lv_obj_set_style_border_width(ok, 0, 0);
    lv_obj_set_style_text_color(ok, lv_color_black(), 0);
    lv_obj_set_pos(ok, 144, 100);
}

// ---------------------------------------------------------------- splash
static void animWidth(void* obj, int32_t v) { lv_obj_set_width((lv_obj_t*)obj, v); }
static void animOpa(void* obj, int32_t v) { lv_obj_set_style_opa((lv_obj_t*)obj, v, 0); }
static void animY(void* obj, int32_t v) { lv_obj_set_y((lv_obj_t*)obj, v); }

static void buildSplash() {
    s_splash = lv_obj_create(nullptr);
    lv_obj_remove_style_all(s_splash);
    lv_obj_add_style(s_splash, &styles().screen, 0);
    lv_obj_clear_flag(s_splash, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name = makeLabel(s_splash, &lv_font_montserrat_48, color::text(), config::kTeamName);
    lv_obj_set_style_text_letter_space(name, 8, 0);
    lv_obj_align(name, LV_ALIGN_CENTER, 0, -34);

    lv_obj_t* bar = lv_obj_create(s_splash);
    lv_obj_remove_style_all(bar);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, color::accent(), 0);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_size(bar, 0, 4);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_align(bar, LV_ALIGN_CENTER, 0);

    lv_obj_t* team = makeLabel(s_splash, &lv_font_montserrat_14, color::accent2(), "");
    setTextf(team, "TEAM %s", config::kTeamNumber);
    lv_obj_set_style_text_letter_space(team, 6, 0);
    lv_obj_align(team, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t* spin = lv_spinner_create(s_splash, 900, 70);
    lv_obj_set_size(spin, 22, 22);
    lv_obj_set_style_arc_width(spin, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spin, color::line(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spin, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spin, color::accent(), LV_PART_INDICATOR);
    lv_obj_align(spin, LV_ALIGN_BOTTOM_MID, -70, -26);

    s_splashStatus = makeLabel(s_splash, &lv_font_montserrat_12, color::muted(), s_bootStatus);
    lv_obj_align_to(s_splashStatus, spin, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    if (config::kShowBuildStamp) {
        lv_obj_t* build = makeLabel(s_splash, &lv_font_montserrat_10, color::dim(), "build " __DATE__ " " __TIME__);
        lv_obj_align(build, LV_ALIGN_BOTTOM_RIGHT, -8, -6);
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_exec_cb(&a, animWidth);
    lv_anim_set_values(&a, 0, 300);
    lv_anim_set_time(&a, 900);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, name);
    lv_anim_set_exec_cb(&a, animOpa);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, 700);
    lv_anim_start(&a);

    lv_scr_load(s_splash);
}

// ---------------------------------------------------------------- main screen
static void onRail(lv_event_t* e) {
    lv_obj_t* m = lv_event_get_target(e);
    uint16_t id = lv_btnmatrix_get_selected_btn(m);
    if (id != LV_BTNMATRIX_BTN_NONE) showPage(id);
}

static void onAlertChip(lv_event_t*) { showPage(pageIndex(&pageSensors)); }

static void setRailMap() {
    static std::vector<const char*> railMap;
    railMap.clear();
    for (size_t i = 0; i < tabs().size(); i++) {
        if (i) railMap.push_back("\n");
        railMap.push_back(iconGlyph(tabs()[i].icon));
    }
    railMap.push_back("");
    lv_btnmatrix_set_map(s_rail, railMap.data());
    lv_btnmatrix_set_btn_ctrl_all(s_rail, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_one_checked(s_rail, true);
}

static void onAbort(lv_event_t*) {
    hw::abortRoutine();
    toast(LV_SYMBOL_STOP "  aborting routine", color::bad());
}

static lv_obj_t* statusChip(lv_obj_t* parent) {
    lv_obj_t* c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_add_style(c, &styles().chip, 0);
    lv_obj_set_size(c, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

static void buildMain() {
    s_main = lv_obj_create(nullptr);
    lv_obj_remove_style_all(s_main);
    lv_obj_add_style(s_main, &styles().screen, 0);
    lv_obj_clear_flag(s_main, LV_OBJ_FLAG_SCROLLABLE);

    // ---- status bar
    lv_obj_t* bar = makeBox(s_main, 0, 0, 480, 26);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, color::panel(), 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, color::line(), 0);

    lv_obj_t* mark = makeBox(bar, 8, 7, 4, 12);
    lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(mark, color::accent(), 0);
    lv_obj_set_style_radius(mark, 2, 0);

    lv_obj_t* logo = makeLabel(bar, &lv_font_montserrat_14, color::text(), config::kTeamName);
    lv_obj_set_style_text_letter_space(logo, 2, 0);
    lv_obj_set_pos(logo, 17, 5);

    s_pageTitle = makeLabel(bar, &lv_font_montserrat_12, color::muted(), "");
    lv_obj_set_pos(s_pageTitle, 112, 7);

    // right-hand cluster laid out right-to-left with flex
    lv_obj_t* right = makeBox(bar, 172, 0, 302, 26);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 5, 0);

    s_alertChip = statusChip(right);
    lv_obj_set_style_bg_color(s_alertChip, color::bad(), 0);
    lv_obj_add_flag(s_alertChip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(s_alertChip, 6);
    lv_obj_add_event_cb(s_alertChip, onAlertChip, LV_EVENT_CLICKED, nullptr);
    s_alertLabel = makeLabel(s_alertChip, &lv_font_montserrat_12, lv_color_white(), "");
    lv_obj_add_flag(s_alertChip, LV_OBJ_FLAG_HIDDEN);

    s_modeChip = statusChip(right);
    s_modeLabel = makeLabel(s_modeChip, &lv_font_montserrat_12, lv_color_white(), "DISABLED");
    lv_obj_set_style_text_letter_space(s_modeLabel, 1, 0);

    s_fieldLabel = makeLabel(right, &lv_font_montserrat_10, color::muted(), "");
    s_timerLabel = makeLabel(right, &lv_font_montserrat_16, color::text(), "--:--");
    lv_obj_set_width(s_timerLabel, 38);
    lv_obj_set_style_text_align(s_timerLabel, LV_TEXT_ALIGN_CENTER, 0);

    s_sdIcon = makeLabel(right, &lv_font_montserrat_14, color::dim(), LV_SYMBOL_SD_CARD);
    s_ctrlLabel = makeLabel(right, &lv_font_montserrat_12, color::dim(), LV_SYMBOL_WIFI);
    s_battLabel = makeLabel(right, &lv_font_montserrat_12, color::text(), LV_SYMBOL_BATTERY_FULL " --%");

    // ---- nav rail
    s_rail = lv_btnmatrix_create(s_main);
    lv_obj_remove_style_all(s_rail);
    setRailMap();
    lv_obj_set_pos(s_rail, 4, 30);
    lv_obj_set_size(s_rail, 40, 206);
    lv_obj_set_style_pad_row(s_rail, 3, 0);
    lv_obj_set_style_pad_all(s_rail, 0, 0);
    lv_obj_set_style_bg_opa(s_rail, LV_OPA_TRANSP, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_rail, 8, LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_rail, color::muted(), LV_PART_ITEMS);
    lv_obj_set_style_text_font(s_rail, &lv_font_montserrat_16, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(s_rail, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(s_rail, color::accentDim(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(s_rail, color::text(), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(s_rail, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(s_rail, color::panel2(), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_add_event_cb(s_rail, onRail, LV_EVENT_VALUE_CHANGED, nullptr);

    // rail divider
    lv_obj_t* div = makeBox(s_main, 46, 32, 1, 202);
    lv_obj_set_style_bg_opa(div, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(div, color::line(), 0);

    s_content = makeBox(s_main, 50, 30, 426, 206);

    // routine-running banner (hidden until something runs from the brain)
    s_runBanner = makeBox(s_main, 50, 30, 426, 30);
    lv_obj_set_style_bg_opa(s_runBanner, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(s_runBanner, lv_color_mix(color::warn(), color::bg(), 45), 0);
    lv_obj_set_style_border_color(s_runBanner, color::warn(), 0);
    lv_obj_set_style_border_width(s_runBanner, 1, 0);
    lv_obj_set_style_radius(s_runBanner, 8, 0);
    s_runLabel = makeLabel(s_runBanner, &lv_font_montserrat_14, color::warn(), "");
    lv_obj_align(s_runLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* abort = makeButton(s_runBanner, LV_SYMBOL_STOP " ABORT", 90, 22, onAbort);
    lv_obj_set_style_bg_color(abort, color::bad(), 0);
    lv_obj_set_style_border_width(abort, 0, 0);
    lv_obj_align(abort, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_add_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN);
}

void showPage(int index) {
    if (index < 0 || index >= (int)tabs().size() || !s_content) return;
    if (s_activePage && s_activePage->teardown) s_activePage->teardown();
    lv_obj_clean(s_content); // only the visible page lives in LVGL memory
    s_page = index;
    s_activePage = pageFor(tabs()[index]);
    lv_btnmatrix_set_btn_ctrl(s_rail, index, LV_BTNMATRIX_CTRL_CHECKED);
    setTextf(s_pageTitle, "/  %s", tabs()[index].title.c_str());
    s_activePage->build(s_content);
    s_activePage->update(s_snap);
}

void refreshPage() { showPage(s_page); }

void rebuildTabs() {
    if (!s_rail) return;
    setRailMap();
    int n = tabs().size();
    showPage(s_page < n ? s_page : n - 1);
}

// ---------------------------------------------------------------- status bar
static void updateStatus(const Snapshot& s) {
    // mode + match clock
    Mode mode = s.disabled ? Mode::Disabled : s.autonomous ? Mode::Auton : Mode::Driver;
    if (mode != s_mode) {
        s_mode = mode;
        s_modeStart = s.timeMs;
        if (mode != Mode::Disabled) log("mode -> %s", mode == Mode::Auton ? "AUTONOMOUS" : "DRIVER");
    }
    const char* modeText = "DISABLED";
    lv_color_t modeCol = color::dim();
    if (mode == Mode::Auton) modeText = "AUTON", modeCol = color::accent();
    if (mode == Mode::Driver) modeText = "DRIVER", modeCol = color::good();
    lv_label_set_text_static(s_modeLabel, modeText);
    lv_obj_set_style_bg_color(s_modeChip, modeCol, 0);

    lv_label_set_text_static(s_fieldLabel, s.fieldControl    ? "FIELD"
                                           : s.compConnected ? "SWITCH"
                                                             : "NO COMP");

    uint32_t elapsed = (s.timeMs - s_modeStart) / 1000;
    if (mode == Mode::Disabled) {
        lv_label_set_text_static(s_timerLabel, "--:--");
        lv_obj_set_style_text_color(s_timerLabel, color::dim(), 0);
    } else {
        int total = !s.compConnected ? -1 : mode == Mode::Auton ? config::kAutonSeconds : config::kDriverSeconds;
        int shown = total < 0 ? (int)elapsed : total - (int)elapsed;
        if (shown < 0) shown = 0;
        setTextf(s_timerLabel, "%d:%02d", shown / 60, shown % 60);
        bool endgame = total > 0 && shown <= config::kEndgameSeconds;
        lv_obj_set_style_text_color(s_timerLabel, endgame ? color::warn() : color::text(), 0);
    }

    // device alerts
    if (s.deviceProblems > 0) {
        setTextf(s_alertLabel, LV_SYMBOL_WARNING " %d", s.deviceProblems);
        lv_obj_clear_flag(s_alertChip, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_alertChip, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_set_style_text_color(s_sdIcon, s.sd ? color::accent2() : color::dim(), 0);

    if (s.controllerOk) {
        setTextf(s_ctrlLabel, LV_SYMBOL_WIFI " %d%%", s.controllerBattery);
        lv_obj_set_style_text_color(s_ctrlLabel, s.controllerBattery < 25 ? color::warn() : color::muted(), 0);
    } else {
        lv_label_set_text_static(s_ctrlLabel, LV_SYMBOL_WIFI " --");
        lv_obj_set_style_text_color(s_ctrlLabel, color::bad(), 0);
    }

    const char* icon = s.batteryPct > 80   ? LV_SYMBOL_BATTERY_FULL
                       : s.batteryPct > 55 ? LV_SYMBOL_BATTERY_3
                       : s.batteryPct > 30 ? LV_SYMBOL_BATTERY_2
                       : s.batteryPct > 10 ? LV_SYMBOL_BATTERY_1
                                           : LV_SYMBOL_BATTERY_EMPTY;
    setTextf(s_battLabel, "%s %d%%", icon, (int)s.batteryPct);
    lv_obj_set_style_text_color(s_battLabel, heatColor(100 - s.batteryPct, 20, 80), 0);

    // running routine banner
    if (hw::routineRunning()) {
        if (lv_obj_has_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(s_runBanner, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_runBanner);
            s_runStart = s.timeMs;
        }
        setTextf(s_runLabel, LV_SYMBOL_PLAY "  routine running  %.1fs",
                              (s.timeMs - s_runStart) / 1000.0);
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
