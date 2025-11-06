# AquaGuard C Data Platform – Group 10 (Computer Programming 1 – IE University)

Live leak-detection dashboard written in **C** with a **Python GUI simulator**.

- Reads sensor JSON from either a **TCP simulator** (Python GUI) or an internal **SIM mode**.
- Serves a local web dashboard at **http://localhost:8080** with:
  - Live metrics: **flow (L/min)**, **humidity (%)**, **flowing (boolean)**
  - **Connection status** (Connected via TCP/SIM or Disconnected)
  - **Emergency banner** for `LEAK` / `HIGH_FLOW` with a “Call emergency” link
- Clean project hygiene: CMake, tests (CTest), GitHub CI, docs, and Conda env.

## Quick Start

### 0) (Optional) Create the conda env
```bash
conda env create -f environment.yml
conda activate aquaguard-c
```

> macOS users: ensure you have Xcode Command Line Tools installed: `xcode-select --install`

### 1) Build
```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 2) Run the Python simulator (TCP on 127.0.0.1:5555)
```bash
python3 simulator_py/gui_simulator.py
```
Use the GUI to **Start Server** and tweak **Flow/Humidity**, toggles for **Flowing**, **Leak**, and **High flow**.

### 3) Run the C gateway + web dashboard
In a new terminal:
```bash
./build/aquaguard --mode tcp --tcp-host 127.0.0.1 --tcp-port 5555 --web-port 8080
```
Open: http://localhost:8080

- If the simulator is **running**, the dashboard shows **Connected via TCP** and streams live data.
- If you **Stop Server** or close it, it switches to **Disconnected** within seconds.

### Alternative: SIM mode (no Python needed)
```bash
./build/aquaguard --mode sim --web-port 8080
```

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

## Credits
**Group 10**  
- ADRIA FIJO GARRIGA  
- HERNAN CHACON  
- JAD EL AAWAR
- ZAID AYMAN SHAFIK JUMEAN


**Note:** The simulator now sends valid JSON using `json.dumps`, ensuring booleans are lowercase `true/false`.
