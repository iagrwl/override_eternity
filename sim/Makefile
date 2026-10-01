# desktop build of the brain UI. use ./sim/run.sh instead of calling this directly.
# (relative paths on purpose: make can't handle the space in "override_eternity copy")

CACHE    := .cache
LVGL_DIR := $(CACHE)/lvgl
BIN      := $(CACHE)/brain_sim
.DEFAULT_GOAL := $(BIN)

SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

COMMON  := -DLV_CONF_INCLUDE_SIMPLE -I. -I$(CACHE) -I../include -I../src/ui $(SDL_CFLAGS)
CFLAGS  := -O2 -w $(COMMON)
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wno-unused-function -MMD -MP $(COMMON)

LVGL_SRCS := $(shell find $(LVGL_DIR)/src -name '*.c' 2>/dev/null)
LVGL_OBJS := $(patsubst $(LVGL_DIR)/src/%.c,$(CACHE)/lvgl_obj/%.o,$(LVGL_SRCS))

UI_SRCS  := $(filter-out ../src/ui/robot_io.cpp,$(wildcard ../src/ui/*.cpp)) ../src/auton/auton_list.cpp
SIM_SRCS := sim_main.cpp fake_robot.cpp editor.cpp
UI_OBJS  := $(patsubst ../src/%.cpp,$(CACHE)/ui_obj/%.o,$(UI_SRCS)) $(patsubst %.cpp,$(CACHE)/ui_obj/sim/%.o,$(SIM_SRCS)) \
            $(CACHE)/ui_obj/auton_stubs.o

# empty versions of every auton function declared in include/eternity_template/auton/,
# so auton_list.cpp links without the real (pros-only) route code
AUTON_HEADERS := $(wildcard ../include/eternity_template/auton/*.hpp)
$(CACHE)/auton_stubs.cpp: $(AUTON_HEADERS)
	@mkdir -p $(dir $@)
	@sed -nE 's/^[[:space:]]*void[[:space:]]+([A-Za-z_0-9]+)[[:space:]]*\(([^)]*)\)[[:space:]]*;.*/void \1(\2) {}/p' $^ > $@

$(CACHE)/ui_obj/auton_stubs.o: $(CACHE)/auton_stubs.cpp
	@$(CXX) -std=c++17 -w -c $< -o $@

$(BIN): $(CACHE)/liblvgl.a $(UI_OBJS)
	@echo "  link  brain_sim"
	@$(CXX) $(UI_OBJS) $(CACHE)/liblvgl.a $(SDL_LIBS) -o $@

$(CACHE)/liblvgl.a: $(LVGL_OBJS)
	@echo "  ar    liblvgl.a (one-time)"
	@ar rcs $@ $^

$(CACHE)/lvgl_obj/%.o: $(LVGL_DIR)/src/%.c lv_conf.h
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(CACHE)/ui_obj/%.o: ../src/%.cpp
	@mkdir -p $(dir $@)
	@echo "  c++   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(CACHE)/ui_obj/sim/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "  c++   $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(UI_OBJS:.o=.d)
