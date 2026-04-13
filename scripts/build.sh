#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CONFIG="${1:-Release}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "Error: cmake is not installed or not in PATH."
  echo "Install with: brew install cmake"
  exit 1
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG"
cmake --build "$BUILD_DIR" --config "$CONFIG"

echo "Build completed for configuration: $CONFIG"
