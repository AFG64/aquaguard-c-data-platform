# AquaGuard C Data Platform – Group 10 (IE University)

## Topic description
AquaGuard is a C-based gateway that ingests line-delimited JSON sensor data and serves a live local dashboard at `http://localhost:8080`. The system reports **flow (L/min)**, **humidity (%)**, **temperature (°C)**, and **pressure (kPa)**, plus **connection status** and an **emergency banner** for threshold breaches (flow high/low, humidity, temperature, pressure).
Data sources:
- **TCP Simulator (Python GUI)** on `127.0.0.1:5555`
- **SIM mode** for synthetic generation

Scope: single-machine demo showcasing systems programming fundamentals (threads, sockets per Module VI, SSE, lightweight parsing).

## Design choices (what we tried and why)
- **Two-thread split (sensor + HTTP/SSE)**: Started single-threaded but UI updates blocked on socket reads. Switched to two threads to keep code readable: one does sensor I/O, the other serves HTTP/SSE. Pthreads chosen for portability and course alignment.
- **Networking (TCP over UDP)**: Simulator already sends newline JSON over TCP; we needed ordered, reliable delivery. UDP was rejected (no ordering, no built-in reconnect semantics); TCP sockets via `getaddrinfo`/`connect`/`accept` match the simulator and simplify error handling.
- **Parsing (minimal, custom)**: First drafts tried to bring a JSON lib; build pain on student machines led us to a hand-rolled scanner for fixed keys. Unit tests back it to avoid silent misparses.
- **State model (shared struct + mutex)**: Early copies per thread caused divergence; we consolidated to one `SharedState` guarded by a mutex to prevent torn reads. Bitmask alerts keep multiple flags in one int for SSE payloads.
- **Error handling and resilience**: Added reconnect with exponential backoff after seeing tight retry loops spam logs. Return codes everywhere (parser, socket setup) so the app can drop bad lines without crashing. Ignore `SIGPIPE` after SSE disconnects killed the process during reloads.
- **Interface sanity**: CLI kept tiny (mode, host/ports) to reduce user error. Functions take explicit pointers/structs; no global state beyond the shared struct. SSE chosen over polling/WebSockets for a simple one-way interface that browsers support by default.
- **Iteration path**: Single-thread → two-thread split; heavy JSON idea → minimal parser; aggressive reconnect → backoff; ad-hoc alerts → centralized bitmask/threshold macros. Documented missteps kept us honest about trade-offs.
- **Team collaboration & lessons**: We paired on risky areas (parser, sockets) and split along interfaces (sensor/HTTP/UI/docs). Weekly syncs caught misalignments early; the main lesson was to prototype quickly (SIM mode) before integrating the Python simulator to avoid blocking on external tooling.

## Project hygiene (build, test, docs, CI)
- **CMake**: Single `CMakeLists.txt` builds a reusable `aquaguard_lib`, the `aquaguard` binary, links pthreads, adds `_DEFAULT_SOURCE`, and exposes an optional `install` target; the same file wires up CTest so a fresh `cmake -S . -B build && cmake --build build -j` always works.
- **Tests**: `tests/test_parser.c` is registered with CTest (`ctest --test-dir build --output-on-failure`) to lock the JSON parser and alert thresholds; CI runs the tests on every push/PR.
- **Docs**: `README.md` gives Quick Start, build/run, troubleshooting, and flow overview; `report/report.md` (this file), `flowchart.txt`, and `presentation/` back the design narrative; in-code headers describe the public surface.
- **Git/GitHub + workflows**: `.github/workflows/ci.yml` runs configure/build/ctest on Ubuntu and macOS for pushes/PRs to keep the main branch green; `scripts/run_all.sh` drives a full demo (build + simulator + gateway) locally.
- **Dependencies (Conda)**: `environment.yml` pins Python, Tk (for the simulator), CMake, and Make so macOS/Windows/Linux can reproduce the toolchain; C code avoids third-party libs to simplify student machines.
- **Onboarding ease (README)**: Quick Start shows Conda creation, build, simulator, and gateway commands; troubleshooting tables highlight Tk issues and PATH fixes so new users can run the project in minutes.

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
- **Module 0 – Tooling & Workflow**: `CMakeLists.txt`, `scripts/run_all.sh`, `environment.yml`, `README.md` (build/run steps), VSCode/Conda workflow. Chosen to keep the build/run story consistent across macOS/Windows/Linux and lower setup friction for teammates.
- **Module I – Error Handling & Debugging**: `src/json.c` (explicit return codes), `tests/test_parser.c` (unit tests), logging macros (`log.h` usage), graceful reconnects in `src/sensor.c`. Explicit codes + logs were favored over silent failures so the dashboard state always matches sensor status.
- **Module II – Pointers & Structures**: `include/shared.h` (`struct SensorData`, pointer-based shared state), mutex-protected shared memory in `src/sensor.c`, socket pointers in `src/http.c`. One heap-allocated `SharedState` is shared by pointer: the sensor thread overwrites fields with fresh readings while the HTTP thread reads the same block; the mutex ensures updates don’t race, and there are no duplicate buffers eating RAM.
- **Module III – File Handling & Preprocessor**: Preprocessor macros for thresholds (`include/shared.h`), header includes across modules; static file serving in `src/http.c` uses basic file I/O. Simple macros keep thresholds centralized; direct file reads avoid pulling an external HTTP stack.
- **Module IV – Bit Manipulation, Enums & typedef**: Alert bitmask flags (`AlertFlags`) and enums (`ConnectionStatus`) in `include/shared.h`, bitwise checks in `src/json.c` and `src/sensor.c`. Bit flags were picked so multiple alerts coexist in one integer and are cheap to test/send to the UI.
- **Module V – Concurrent Programming**: POSIX threads for sensor and HTTP servers (`src/main.c`, `src/sensor.c`, `src/http.c`), mutex synchronization (`include/shared.h`, `src/sensor.c`), SSE as a concurrent data stream. Two threads keep the design simple (I/O separated) while the mutex protects shared data without heavy frameworks.
- **Module VI – Networking & Sockets in C**: TCP client in `src/sensor.c` (Berkeley sockets via `getaddrinfo`/`connect` to the simulator) and TCP server in `src/http.c` (`socket`/`bind`/`listen`/`accept` to serve HTTP + SSE). TCP was chosen over UDP for delivery guarantees and because the Python simulator already provides newline-delimited JSON over TCP. `src/main.c` holds configurable host/port defaults in `SharedState` so both threads share the networking setup.


## Contributions
- **ADRIA FIJO GARRIGA (25%)**: Architecture, threading split, CLI wiring, report drafting, and integration of SIM mode for faster iteration.
- **HERNAN CHACON (25%)**: TCP client with reconnect/backoff, minimal JSON parser, alert bitmask logic, and parser unit tests.
- **JAD EL AAWAR (25%)**: Web dashboard layout, SSE client wiring, styling/branding, and alert banner behavior.
- **ZAID AYMAN SHAFIK JUMEAN (25%)**: Project hygiene (Conda env, CMake/CTest polish, CI workflow), README/troubleshooting, and presentation build.

## AI statement
LLMs were used for brainstorming, boilerplate (CMake, CI), and documentation drafts. The team reviewed and adapted all code for correctness and clarity.All the web data as well as the simulator was done with AI, and we used it towards our device, but we do not want to be evaluated by that. Our work for programming course was SRC,TESTS and INCLUDE, as well as the report, The Readme , Cmake lists, licence and environment. 

**WEB FOLDER AND SIMULATOR_PY FOlDER WERE DONE WITH AI AND WE DONT WANT THEM TO BE EVALUATED. THE REST OF THE REPOSITORY WAS OUR WORK**

