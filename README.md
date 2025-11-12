# AquaGuard C Data Platform – Group 10 (Computer Programming 1 – IE University)

Live leak-detection dashboard written in **C** with a **Python GUI simulator**.

* Reads sensor JSON from either a **TCP simulator** (Python GUI) or an internal **SIM mode**.
* Serves a local web dashboard at **[http://localhost:8080](http://localhost:8080)** with:

  * Live metrics: **flow (L/min)**, **humidity (%)**, **flowing (boolean)**
  * **Connection status** (Connected via TCP/SIM or Disconnected)
  * **Emergency banner** for `LEAK` / `HIGH_FLOW` with a “Call emergency” link
* Clean project hygiene: CMake, tests (CTest), GitHub CI, docs, and Conda env.

---

## Quick Start

### 0) (Optional) Create the conda env

```bash
conda env create -f environment.yml
conda activate aquaguard-c
```

> macOS users: ensure you have Xcode Command Line Tools installed:
> `xcode-select --install`

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

Open: [http://localhost:8080](http://localhost:8080)

* If the simulator is **running**, the dashboard shows **Connected via TCP** and streams live data.
* If you **Stop Server** or close it, it switches to **Disconnected** within seconds.

### Alternative: SIM mode (no Python needed)

```bash
./build/aquaguard --mode sim --web-port 8080
```

---

## 📂 Project Structure

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

---

## Credits

**Group 10**

* ADRIA FIJO GARRIGA
* HERNAN CHACON
* JAD EL AAWAR
* ZAID AYMAN SHAFIK JUMEAN

> The simulator now sends valid JSON using `json.dumps`, ensuring booleans are lowercase `true/false`.

---

## Project Summary

**AquaGuard C Data Platform** is a real-time **water monitoring system** designed to simulate a smart leak-detection network.
The **Python GUI simulator** sends sensor readings (flow, humidity, leak alerts) through **TCP sockets**, while the **C gateway** receives, processes, and serves the data live on a **local web dashboard**.
The dashboard shows live metrics, connection status, and emergency alerts, switching automatically between **Connected** and **Disconnected** states depending on the simulator.

This project combines all core concepts from **Modules I–IV** and uses **Module VI (Sockets)** as its advanced topic.

---

## Modules Used

| **Module**                                               | **Concepts**                               | **Usage in AquaGuard**                                                                                                                             |
| -------------------------------------------------------- | ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Module I – Error Handling & Unit Testing**             | Error checks, assertions, test cases       | Used to handle invalid JSON, socket failures, and dashboard communication errors. Includes **CTest** for parser validation.                        |
| **Module II – Pointers, Memory Management & Structures** | Dynamic memory, structs, pointers          | Sensor data (`SensorData`) is stored in a struct and updated dynamically via pointers for efficient memory use.                                    |
| **Module III – Preprocessor Directives & Macros**        | `#define`, constants, compile-time control | Configuration for ports, thresholds, and modes (e.g. `#define SIM_MODE 1`). Keeps code modular and readable.                                       |
| **Module IV – File Input/Output**                        | File reading/writing, logs                 | Allows saving or loading sensor data for offline simulation or logging results from the gateway.                                                   |
| **Module VI – Network Sockets (Advanced Topic)**         | TCP communication between processes        | Handles real-time connection between the **Python simulator** and **C gateway**, enabling continuous data streaming and dynamic dashboard updates. |

---
**In short:**
AquaGuard demonstrates a complete C application integrating fundamentals (Modules I–IV) with advanced network communication (Module VI). It’s a creative, real-world IoT-style system showing clear understanding of C programming, data flow, and process interaction.

