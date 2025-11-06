# AquaGuard C Data Platform – Group 10 (IE University)

## Topic description
AquaGuard is a C-based gateway that ingests line-delimited JSON sensor data and serves a live local dashboard at `http://localhost:8080`. The system reports **flow (L/min)**, **humidity (%)**, and **flowing** state, plus **connection status** and an **emergency banner** for `LEAK` and `HIGH_FLOW`.
Data sources:
- **TCP Simulator (Python GUI)** on `127.0.0.1:5555`
- **SIM mode** for synthetic generation

Scope: single-machine demo showcasing systems programming fundamentals (threads, sockets, SSE, lightweight parsing).

## Design choices
- **Threads**: one thread handles the sensor (TCP client + parsing), another runs the HTTP server and SSE.
- **Networking**: POSIX sockets. The SSE endpoint (`/events`) pushes JSON updates to the browser.
- **Parsing**: Minimal, dependency-free reader tailored to our fixed schema (`flow_lpm`, `humidity_pct`, `flowing`, `leak`, `high_flow`). Unit-tested.
- **State**: Shared state with mutex; `SensorData` uses `struct` + `enum` (`AlertStatus`, `ConnectionStatus`). 
- **Error handling**: Reconnection with backoff, resilient to malformed input, consistent UI status.
- **Design iteration**: Single-threaded → split I/O + SSE → simplified JSON + hardened reconnection.
- **Division of responsibilities**: C gateway & SSE / Python simulator / Web UI / Tests & CI.

## Contributions
- **ADRIA FIJO GARRIGA (35%)**: Software architecture, gateway threading model, report.
- **HERNAN CHACON (35%)**: TCP client + reconnection, JSON parser, tests.
- **XX (15%)**: Web UI design & styling.
- **XX (15%)**: Presentation & project hygiene (Conda, CI, README).

## AI statement
LLMs were used for brainstorming, boilerplate (CMake, CI), and documentation drafts. The team reviewed and adapted all code for correctness and clarity.
