#!/usr/bin/env bash
# Dirilis: Ertugrul — to'liq yig'ish (Git Bash / Linux / macOS)
set -euo pipefail
cd "$(dirname "$0")/.."

GPP="${GPP:-g++}"
[ -x "/d/gcc/mingw64/bin/g++.exe" ] && GPP="/d/gcc/mingw64/bin/g++.exe"
[ -d "/c/Program Files/Microsoft/jdk-21.0.9.10-hotspot/bin" ] && \
  export PATH="/c/Program Files/Microsoft/jdk-21.0.9.10-hotspot/bin:$PATH"

mkdir -p build backend/out

CORE="game/src/core/*.cpp game/src/world/*.cpp game/src/components/*.cpp \
      game/src/subsystems/*.cpp game/src/characters/*.cpp game/src/minigames/*.cpp \
      game/src/net/*.cpp game/src/app/*.cpp"

echo "[1/3] C++ o'yin..."
# shellcheck disable=SC2086
$GPP -std=c++20 -O2 -Wall -Wextra -Igame/include -o build/ertugrul $CORE game/src/Demo.cpp game/src/main.cpp \
  $([ "$OSTYPE" = "msys" ] && echo "-static -lws2_32 -lgdi32 -luser32 -lopengl32 -lglu32 -lgdiplus" || echo "")

echo "[2/3] Testlar..."
# shellcheck disable=SC2086
$GPP -std=c++20 -O2 -Wall -Wextra -Igame/include -o build/ertugrul_tests $CORE game/tests/test_main.cpp \
  $([ "$OSTYPE" = "msys" ] && echo "-static -lws2_32 -lgdi32 -luser32 -lopengl32 -lglu32 -lgdiplus" || echo "")

echo "[3/3] Java backend..."
find backend/src/main/java -name "*.java" > /tmp/ertugrul_sources.txt
javac -encoding UTF-8 -d backend/out @/tmp/ertugrul_sources.txt
cp -r backend/src/main/resources/web backend/out/

echo "Tayyor: build/ertugrul, build/ertugrul_tests, backend/out"
