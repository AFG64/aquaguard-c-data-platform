# AquaGuard — High-Level Overview
- Purpose: Local, real-time water safety dashboard that shows flow, humidity, temperature, pressure, connection status, and an emergency banner when thresholds are breached.
- Data flow: Python simulator → TCP to C gateway → shared state → HTTP/SSE → browser dashboard at `http://localhost:8080`.
- Modes: `--mode tcp` (talk to simulator on `127.0.0.1:5555`) or `--mode sim` (synthetic data, no Python needed).
- Why TCP: Ordered, reliable delivery matches the simulator’s newline-delimited JSON; avoids dropped or jumbled readings.
- Why SSE + HTTP: One-way streaming to the browser with zero client dependencies; static HTML/CSS/JS served locally for easy trust/debugging.
- Why two threads: Sensor I/O and HTTP/SSE run in parallel so UI stays live even if socket reads block or reconnect.
- Safety model: Alert thresholds live in `shared.h`; a bitmask tracks active issues and drives the emergency banner and metric highlighting.
- Hygiene: CMake build, CTest for parser/thresholds, Conda env for tooling (Python/Tk/CMake/Make), GitHub CI on push/PR, clean `src/include/web/tests/presentation` layout.
