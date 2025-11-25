# Slide 1 — AquaGuard (Group 10)
- Title: **AquaGuard – Real-Time Water Safety Dashboard**
- Names: Adrià Fijo Garriga, Hernan Chacon, Jad El Aawar, Zaid Ayman Shafik Jumean

_What to say_: “We’re Group 10. AquaGuard is our real-time water safety dashboard. I’m Adrià, with Hernan, Jad, and Zaid.”

# Slide 2 — What It Is (Plain Language)
- What it does: “AquaGuard watches water data in real time and shows if anything is wrong.”
- How it flows:
  - “A small simulator pretends to be sensors and sends readings over TCP to our C program on this laptop.”
  - “The C program checks the numbers, flags issues, and feeds them to a local web page.”
- What you see:
  - Live flow, humidity, temperature, pressure
  - A clear connected/disconnected status
  - An alert banner when values go out of safe range

_What to say_: “In plain terms, AquaGuard listens to a sensor feed, watches for trouble, and instantly updates a local web page. If flow drops or temperature spikes, you see an alert banner right away.” 

# Slide 3 — Architecture (High Level)
- Diagram: `Python Simulator → TCP Socket → C Gateway → HTTP Server → Browser Dashboard`
- Why TCP: Ordered, reliable delivery matches the simulator; no lost or jumbled readings.
- Why multithreading: One thread handles sensor I/O, another serves HTTP/SSE; UI stays live even if reads block.
- Why HTTP + static files: Zero-install front end; browser fetches HTML/CSS/JS locally while SSE streams data.

_What to say_: “Data flows from the Python simulator over TCP into our C gateway. We keep two threads—one for sensor input, one for the HTTP/SSE server—so the dashboard never stalls. The HTTP server just serves static files and an event stream, so anyone can open the local page without extra setup.” 

# Slide 4 — Main C Components (High Level)
- `sensor.c` (sockets): TCP client pulls sensor data; chosen to stay aligned with the simulator and guarantee ordered delivery.
- `json.c` (parser): Tiny custom parser tuned to our fixed keys; avoids heavy deps and keeps builds portable for classmates.
- `shared.h` + alert thresholds (state): One shared struct with mutex and the alert thresholds/bitmask used to decide when the “Call emergency” banner shows; keeps all parameters in one place.
- `http.c` (server): Serves static HTML/CSS/JS and an SSE endpoint; minimal surface, easy to trust/debug locally.
- `main.c` (threads): Spawns sensor + HTTP threads; separation keeps UI responsive even when network I/O stalls.

_What to say_: “Each piece is small on purpose: sockets stay in `sensor.c` to match the simulator, parsing lives in a tiny, dependency-free `json.c`, shared state plus alert thresholds sit in `shared.h`, `http.c` only serves files and SSE, and `main.c` ties it together with two threads so the dashboard never freezes.” 

# Slide 5 — Design Choices (Why)
- Iterated architecture: Started monolithic; split into modules so sockets/parsing/HTTP/UI can evolve independently.
- Custom JSON parser: Avoided heavy deps to keep builds painless for classmates and CI; tuned to our fixed keys.
- Thread separation: One thread for sensor I/O, one for HTTP/SSE so the dashboard stays responsive under slow networks.
- Mutexed shared state: Single struct with a mutex to guarantee consistent reads/writes across threads.
- Socket handling: TCP (ordered/reliable) plus backoff/reconnect to avoid tight failure loops; non-blocking reads to prevent freezes.
- Clean layout: `src/`, `include/`, `web/`, `tests/`, `presentation/` keep roles clear for onboarding and grading.

_What to say_: “We iterated from a monolith to clear modules. We kept dependencies tiny with a custom parser, split threads so the UI never blocks, guarded shared state with a mutex, and hardened sockets with TCP plus backoff. The folder layout stays clean so anyone can navigate and grade it quickly.” 

# Slide 6 — Project Hygiene
- CMake build: Single CMakeLists builds library, binary, tests; portable across macOS/Linux/Windows.
- Conda env: `environment.yml` pins Python/Tk/CMake/Make so the simulator and build toolchain match for everyone.
- README + docs: Clear Quick Start, build/test/run steps, troubleshooting, and flow overview.
- GitHub + commits: CI workflow on push/PR; history shows incremental, reviewed changes.
- Tests + logging: CTest for the parser/thresholds; logging macros surface errors and connection issues.
- Modular layout: `src/`, `include/`, `web/`, `tests/`, `presentation/`, `report/` keep roles obvious.

_What to say_: “Hygiene-wise: CMake builds everything, Conda locks dependencies, README/doc files guide setup, CI runs on GitHub, tests/logging catch regressions, and the folder structure keeps roles clear.” 
