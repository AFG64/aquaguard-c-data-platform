#!/usr/bin/env bash
set -euo pipefail
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
python3 simulator_py/gui_simulator.py &
SIM_PID=$!
./build/aquaguard --mode tcp --tcp-host 127.0.0.1 --tcp-port 5555 --web-port 8080
trap "kill ${SIM_PID} || true" EXIT
