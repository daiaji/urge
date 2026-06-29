#!/bin/bash
set -e

GAME_DIR="/home/daiaji/下载/レトリアの大冒険_URGE"
BUILD_DIR="/home/daiaji/repo/urge/build"

# Kill running game
if pgrep -x "Game" > /dev/null 2>&1; then
    echo "Killing running Game process..."
    pkill -x "Game" || true
    sleep 1
fi

# Build
cmake --build "$BUILD_DIR" -j"$(nproc)" 2>&1

# Copy binary
echo "Copying Game to $GAME_DIR"
cp "$BUILD_DIR/app/Game" "$GAME_DIR/Game"
echo "Done"
