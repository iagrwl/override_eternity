#!/usr/bin/env bash
# Brain UI simulator.
#   ./sim/run.sh           build + open the window
#   ./sim/run.sh --watch   same, and rebuild/reopen every time you save a UI file
# needs: brew install sdl2   (first run also downloads LVGL 8.3 and builds it, ~1 min)

set -e
cd "$(dirname "$0")"

if ! command -v sdl2-config >/dev/null; then
    echo "missing SDL2 - run: brew install sdl2"
    exit 1
fi
if [ ! -d .cache/lvgl ]; then
    echo "downloading LVGL 8.3 (one-time)..."
    git clone -q --depth 1 -b v8.3.8 https://github.com/lvgl/lvgl.git .cache/lvgl 2>/dev/null
fi
mkdir -p screenshots

build() { make -s -j8; }

if [ "$1" != "--watch" ]; then
    build && exec .cache/brain_sim
fi

# --watch: poll for saved changes, rebuild, relaunch
WATCH="../src/ui ../include/eternity_template/ui ../src/auton/auton_list.cpp ."
stamp() { find $WATCH -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -newer .cache/brain_sim 2>/dev/null | head -1; }

build || true
.cache/brain_sim & PID=$!
trap 'kill $PID 2>/dev/null; exit' INT TERM
echo "watching for changes (ctrl-c to stop)..."
while true; do
    sleep 0.7
    if [ -n "$(stamp)" ]; then
        echo "change detected, rebuilding..."
        if build; then
            kill $PID 2>/dev/null || true
            .cache/brain_sim & PID=$!
        else
            echo "build failed - fix the error and save again"
            touch .cache/brain_sim
        fi
    elif ! kill -0 $PID 2>/dev/null; then
        exit 0 # window was closed
    fi
done
