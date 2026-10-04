#!/usr/bin/env bash
# Brain UI simulator (Mac, Linux, and Windows via MSYS2 - see UI_GUIDE.md).
#   ./sim/run.sh           build + open the window
#   ./sim/run.sh --watch   same, and rebuild/reopen every time you save a file
# first run downloads LVGL 8.3 and builds it (~1 min)

set -e
cd "$(dirname "$0")"

case "$(uname -s)" in
    MINGW* | MSYS* | CYGWIN*) PLATFORM=windows EXE=.exe ;;
    Darwin*) PLATFORM=mac EXE= ;;
    *) PLATFORM=linux EXE= ;;
esac
BIN=.cache/brain_sim$EXE

need() { # tool, install hint per platform
    command -v "$1" >/dev/null && return
    echo "missing '$1'. install it with:"
    case $PLATFORM in
        mac) echo "    $2" ;;
        windows) echo "    pacman -S --needed $3     (in the MSYS2 UCRT64 terminal)" ;;
        linux) echo "    $4" ;;
    esac
    exit 1
}
need sdl2-config "brew install sdl2" "mingw-w64-ucrt-x86_64-SDL2" "sudo apt install libsdl2-dev"
need make "xcode-select --install" "make" "sudo apt install build-essential"
need git "xcode-select --install" "git" "sudo apt install git"
command -v c++ >/dev/null || command -v g++ >/dev/null || command -v clang++ >/dev/null ||
    need g++ "xcode-select --install" "mingw-w64-ucrt-x86_64-gcc" "sudo apt install build-essential"

if [ ! -d .cache/lvgl ]; then
    echo "downloading LVGL 8.3 (one-time)..."
    git clone -q --depth 1 -b v8.3.8 https://github.com/lvgl/lvgl.git .cache/lvgl
fi
mkdir -p screenshots

build() { make -s -j8; }

if [ "$1" != "--watch" ]; then
    build && exec "$BIN"
fi

# --watch: poll for saved changes, rebuild, relaunch
WATCH="../src/ui ../include/eternity_template/ui ../include/eternity_template/auton
       ../include/eternity_template/setup.hpp ../src/auton/auton_list.cpp ."
STAMP=.cache/watch_stamp
changed() { find $WATCH -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -newer $STAMP 2>/dev/null | head -1; }

echo "building..."
until build; do
    echo "build failed - fix the error above and save, retrying..."
    touch $STAMP
    while [ -z "$(changed)" ]; do sleep 0.7; done
done
touch $STAMP
"$BIN" & PID=$!
trap 'kill $PID 2>/dev/null; exit' INT TERM
echo "watching for changes (ctrl-c to stop)..."
while true; do
    sleep 0.7
    if [ -n "$(changed)" ]; then
        touch $STAMP
        echo "change detected, rebuilding..."
        if build; then
            kill $PID 2>/dev/null || true
            "$BIN" & PID=$!
        else
            echo "build failed - fix the error and save again"
        fi
    elif ! kill -0 $PID 2>/dev/null; then
        exit 0 # window was closed
    fi
done
