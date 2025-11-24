# AquaGuard C Data Platform – Group 10 (IE University)

## Topic description
AquaGuard is a C-based gateway that ingests line-delimited JSON sensor data and serves a live local dashboard at `http://localhost:8080`. The system reports **flow (L/min)**, **humidity (%)**, **temperature (°C)**, and **pressure (kPa)**, plus **connection status** and an **emergency banner** for threshold breaches (flow high/low, humidity, temperature, pressure).
Data sources:
- **TCP Simulator (Python GUI)** on `127.0.0.1:5555`
- **SIM mode** for synthetic generation

Scope: single-machine demo showcasing systems programming fundamentals (threads, sockets, SSE, lightweight parsing).

## Design choices
- **Threads**: one thread handles the sensor (TCP client + parsing), another runs the HTTP server and SSE.
- **Networking**: POSIX sockets. The SSE endpoint (`/events`) pushes JSON updates to the browser.
- **Parsing**: Minimal, dependency-free reader tailored to the schema (`flow_lpm`, `humidity_pct`, `temperature_c`, `pressure_kpa`, optional `flowing`). Unit-tested.
- **State**: Shared state with mutex; `SensorData` uses `struct` + bitmask (`alerts_mask`) and `ConnectionStatus`. 
- **Error handling**: Reconnection with backoff, resilient to malformed input, consistent UI status.
- **Resilience**: Ignore `SIGPIPE` so SSE disconnects (page reloads) do not terminate the app.
- **Design iteration**: Single-threaded → split I/O + SSE → simplified JSON + hardened reconnection + alert bitmask.
- **Division of responsibilities**: C gateway & SSE / Python simulator / Web UI / Tests & CI.

## File-by-file overview
- `src/main.c`: Bootstraps defaults, handles CLI flags, installs `SIGINT`/`SIGPIPE` handling, and launches the sensor and HTTP/SSE threads.
- `src/sensor.c`: TCP client (or SIM generator) that parses sensor JSON, applies alert thresholds, and updates shared state under a mutex.
- `src/json.c`: Minimal, dependency-free parser that extracts floats/bools and sets alert bitmasks; returns status codes for error handling.
- `src/http.c`: Lightweight HTTP server + SSE endpoint to stream JSON to the browser; serves static assets from `web/`.
- `include/shared.h`: Core data structures (`SensorData`), alert bitmask/thresholds, connection enums, and shared configuration.
- `include/json.h`, `include/sensor.h`, `include/http.h`, `include/log.h`: Interfaces for parser, sensor thread, HTTP server, and logging helpers.
- `tests/test_parser.c`: Unit test for the JSON parser and alert thresholds.
- `web/index.html`, `web/app.js`, `web/style.css`, `web/assets/logo.svg`: Front-end dashboard (SSE client, metrics, alert banner, branding).
- `simulator_py/gui_simulator.py`: Python Tk GUI that streams synthetic sensor JSON over TCP.
- `CMakeLists.txt`, `environment.yml`, `scripts/run_all.sh`: Tooling to configure, build, test, and run the stack.

## Module mapping (course alignment)
- **Module 0 – Tooling & Workflow**: `CMakeLists.txt`, `scripts/run_all.sh`, `environment.yml`, `README.md` (build/run steps), VSCode/Conda workflow.
- **Module I – Error Handling & Debugging**: `src/json.c` (explicit return codes), `tests/test_parser.c` (unit tests), logging macros (`log.h` usage), graceful reconnects in `src/sensor.c`.
- **Module II – Pointers & Structures**: `include/shared.h` (`struct SensorData`, pointer-based shared state), mutex-protected shared memory in `src/sensor.c`, socket pointers in `src/http.c`.
- **Module III – File Handling & Preprocessor**: Preprocessor macros for thresholds (`include/shared.h`), header includes across modules; static file serving in `src/http.c` uses basic file I/O.
- **Module IV – Bit Manipulation, Enums & typedef**: Alert bitmask flags (`AlertFlags`) and enums (`ConnectionStatus`) in `include/shared.h`, bitwise checks in `src/json.c` and `src/sensor.c`.
- **Module V – Concurrent Programming**: POSIX threads for sensor and HTTP servers (`src/main.c`, `src/sensor.c`, `src/http.c`), mutex synchronization (`include/shared.h`, `src/sensor.c`), SSE as a concurrent data stream.


## Contributions
- **ADRIA FIJO GARRIGA (25%)**: Software architecture, gateway threading model, report.
- **HERNAN CHACON (25%)**: TCP client + reconnection, JSON parser, tests.
- **JAD EL AAWAR (25%)**: Web UI design & styling.
- **ZAID AYMAN SHAFIK JUMEAN (25%)**: Presentation & project hygiene (Conda, CI, README).

## AI statement
LLMs were used for brainstorming, boilerplate (CMake, CI), and documentation drafts. The team reviewed and adapted all code for correctness and clarity.
