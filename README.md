# AquaGuard C Data Platform – Group 10 (Computer Programming 1 – IE University)

Live water-safety dashboard written in **C** with a **Python GUI simulator**.

- Reads sensor JSON from either a **TCP simulator** (Python GUI) or an internal **SIM mode**.
- Serves a local web dashboard at **http://localhost:8080** with:
  - Live metrics: **flow (L/min)**, **humidity (%)**, **temperature (°C)**, and **pressure (kPa)**
  - **Connection status** (Connected via TCP/SIM or Disconnected)
  - **Emergency banner** that lists all active issues and offers a “Call emergency” link
- Clean project hygiene: CMake, tests (CTest), GitHub CI, docs, and Conda env.

## Quick Start

1) **Set up Conda (recommended)**
   ```bash
   conda env create -f environment.yml
   conda activate aquaguard-c
   ```
   - macOS (Intel & M-series): use `python` (not `python3`); install Xcode CLT: `xcode-select --install`.
   - Windows: run from “Anaconda/Miniforge Prompt”; allow Developer Mode if CMake asks.
   - Linux: install build tools if needed: `sudo apt-get install build-essential cmake`.
   - `_tkinter` errors? Skip to the macOS fix section below.

2) **Build**
   ```bash
   rm -rf build
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   ```
   - macOS: if compilers are missing, run `xcode-select --install`.
   - Windows: use “x64 Native Tools Command Prompt” or ensure MSVC is present; CMake will pick Ninja/Make.
   - Linux: ensure you run from the repo root and `build` exists.
   - Run tests only later: `ctest --test-dir build --output-on-failure` from the repo root.

3) **Run the Python simulator (TCP on 127.0.0.1:5555)**
   ```bash
   python simulator_py/gui_simulator.py
   ```
   - Use **Start Server**, adjust Flow/Humidity/Temperature/Pressure.
   - macOS: avoid Homebrew `python3`; stay in the `aquaguard-c` env.
   - Windows/Linux: “No module named tkinter”? Activate the env and install `tk` (see troubleshooting).

4) **Run the C gateway + web dashboard**
   ```bash
   ./build/aquaguard --mode tcp --tcp-host 127.0.0.1 --tcp-port 5555 --web-port 8080
   ```
   Open http://localhost:8080
   - If the simulator runs, status shows **Connected via TCP** with live data.
   - If you stop/close the simulator, status flips to **Disconnected** within seconds.

Alternative (no Python): `./build/aquaguard --mode sim --web-port 8080`

## Alerts and safety thresholds
- **High flow:** > 45 L/min.
- **Low flow:** < 0.3 L/min (effectively stopped).
- **High humidity:** Humidity > 80%.
- **High temperature:** Temperature > 50 °C.
- **High pressure:** Pressure > 120 kPa.

When any threshold is exceeded, the dashboard shows the emergency banner with all active issues listed, highlights the metric(s) in red, and plays the alert tone. Normalize values to clear the banner.

## How it works (short)
- The **C gateway** spins up two threads: one TCP client to read JSON from the simulator and one HTTP/SSE server to push updates to the browser.
- **Server-Sent Events (SSE)** stream live metrics to `/events`; the web UI updates instantly without page reloads.
- A minimal JSON parser keeps dependencies low; a bitmask tracks active alerts (flow high/low, humidity/temp/pressure).
- `SIGPIPE` is ignored so reloading or closing the web page never stops the gateway process.
See `flowchart.txt` for a simple text flowchart of this pipeline.

## If the Python simulator fails with _tkinter errors (macOS Fix)
This error usually occurs on macOS when the Python you are using does not ship with the Tk frameworks, or when Homebrew’s Python is ahead of Conda’s Python in your `PATH`.

**Fix it step by step (M-series and Intel):**
1) Close other terminals. Open a fresh terminal and activate the env:  
`conda activate aquaguard-c`
2) Install Tk inside the env (already included, but safe to re-run):  
`conda install -c conda-forge tk`
3) Make sure you are using Conda’s Python (not Homebrew). These commands should point to your Conda install, not `/opt/homebrew` or `/usr/local`:
```
which python
python -c "import sys; print(sys.executable)"
```
If they point to Homebrew, add Conda to the front of `PATH` in `~/.zshrc` or `~/.bashrc`:
```
export PATH="$HOME/miniconda3/bin:$PATH"   # or $HOME/mambaforge/bin
```
Then reopen the terminal and re-run `conda activate aquaguard-c`.
4) Verify Tkinter really works before launching the simulator:
```
python - <<'PY'
import tkinter as tk
print("Tk version:", tk.TkVersion)
root = tk.Tk()
root.withdraw()
root.destroy()
print("Tkinter is OK")
PY
```
You should see a version number and “Tkinter is OK”.
5) Run the simulator with `python simulator_py/gui_simulator.py`.

**Why macOS PATH fixes matter:** Homebrew’s Python uses a different Tk location. If it is first in `PATH`, `_tkinter` cannot find the correct framework. Keeping Conda first ensures the bundled Tk is used.

### Quick troubleshooting table
| Symptom | What it means | How to fix |
| --- | --- | --- |
| `ModuleNotFoundError: No module named '_tkinter'` | You are using the wrong Python or Tk is missing. | `conda activate aquaguard-c` then `conda install -c conda-forge tk`; ensure `which python` is in Conda. |
| `ImportError: No module named tkinter` | Tk package not available in your current Python. | Same as above; verify PATH is not pointing to `/opt/homebrew/bin/python3`. |
| Tk GUI opens blank or crashes on launch | Mixed libraries from Homebrew Python. | Move Conda to the front of `PATH`, reopen terminal, reactivate the env. |
| `conda activate` works but Python still points to Homebrew | Shell startup files override PATH. | Remove `alias python=python3` or Homebrew PATH exports; keep the Conda PATH line last. |
| Still stuck | You may have multiple Pythons installed. | Uninstall or ignore Homebrew Python for this project; reinstall the Conda env and retry. |

## Project Structure
```
.
├── CMakeLists.txt
├── include/
│   ├── http.h
│   ├── json.h
│   ├── log.h
│   ├── sensor.h
│   └── shared.h
├── src/
│   ├── http.c
│   ├── json.c   <-- FIXED
│   ├── main.c
│   └── sensor.c
├── tests/
│   └── test_parser.c
├── web/
│   ├── assets/
│   │   └── logo.svg
│   ├── index.html
│   ├── style.css
│   └── app.js
├── simulator_py/
│   └── gui_simulator.py
├── presentation/
│   ├── presentation.html
│   ├── slides.js
│   └── styles.css
├── report/
│   └── report.md
├── .github/workflows/ci.yml
├── environment.yml
├── scripts/run_all.sh
└── README.md
```

## Project summary
AquaGuard simulates a smart water safety network: a Python GUI streams sensor JSON over TCP, the C gateway parses and broadcasts it via SSE, and the web UI shows live metrics, connection status, and an emergency banner listing every active issue (flow high/low, humidity/temp/pressure).

## Modules used
- **Module I – Error handling & testing:** Return-code checks in the parser, reconnection warnings, and CTest coverage for JSON parsing and alert thresholds.
- **Module II – Pointers, memory & structs:** Shared `SensorData` struct guarded by mutexes; sockets and threads use pointer-based state.
- **Module III – Preprocessor & macros:** Threshold constants and header organization; minimal dependencies.
- **Module IV – File I/O basics:** Static asset serving in the HTTP server; simple file reads for the web bundle.
- **Module V – Networking & concurrency:** POSIX sockets (TCP client, HTTP/SSE server) with pthreads for sensor and web threads; SSE keeps the dashboard live without reloads.

## Common mistakes and how to avoid them
- Using Homebrew Python first in `PATH`: keep the Conda `python` first so Tk loads correctly.
- Forgetting to activate the Conda env: always `conda activate aquaguard-c` before running commands.
- Running `python3` instead of `python`: the simulator expects the Conda `python` binary, not the system/Homebrew one.
- Missing Tk package: if `_tkinter` errors appear, run `conda install -c conda-forge tk` inside the env.

## Credits
**Group 10**  
- ADRIA FIJO GARRIGA — 25%  
- HERNAN CHACON — 25%  
- JAD EL AAWAR — 25%  
- ZAID AYMAN SHAFIK JUMEAN — 25%


**Note:** The simulator now sends valid JSON using `json.dumps`, ensuring booleans are lowercase `true/false`.
