# Rationale – `src/main.c`

This explains why `main.c` is structured the way it is and how each C construct serves the product.

## Overall architecture
- **Two threads (sensor + HTTP)**: Splitting duties keeps code easy to follow. The sensor thread focuses on ingesting/generating data; the HTTP thread focuses on serving the dashboard. This avoids a complex event loop and matches the course goal of demonstrating basic pthreads.
- **SharedState with mutex**: Both threads share one struct. The sensor thread overwrites fields with fresh readings; the HTTP thread reads the same block to send to the browser. A `pthread_mutex_t` around this struct prevents races and torn reads—critical so the dashboard never shows half-written values. Mutexes are portable and simple for a two-thread design.
- **TCP defaults**: The Python simulator already exposes newline JSON over TCP on `127.0.0.1:5555`. TCP (vs. UDP) guarantees ordered, reliable delivery, so we don’t lose or reorder sensor lines.
- **SSE for live updates**: Server-Sent Events are one-way, lightweight, and built into browsers—no extra client library or WebSocket handshake. Perfect for pushing sensor readings to the UI.
- **Non-privileged ports**: 5555/8080 avoid root access and common conflicts, making the demo easy to run on any OS.

## Line-by-line intent
- Includes: POSIX headers (`pthread`, `signal`, `unistd`) for threads/sleep/signals, and project headers for shared state, threads, and logging.
- `static volatile int running`: Flag the main loop checks. `volatile` prevents the compiler from optimizing away changes made in the signal handler.
- `on_sigint`: Handles Ctrl+C to shut down cleanly instead of leaving threads/sockets hanging.
- `ignore_sigpipe`: Browsers may drop SSE/HTTP connections; ignoring `SIGPIPE` prevents crashes on reload.
- `print_usage`: Small helper so users know CLI knobs.
- `init_defaults`: Centralizes defaults (TCP to simulator, web on 8080, disconnected status). Initializes the mutex because the shared struct is accessed across threads.
- `parse_args`: Manual, readable parsing (tiny option set) to keep dependencies low; writes into `SharedState` so both threads see the same config.
- In `main`: Initialize state, parse CLI, log chosen mode/ports (user feedback), install signals, and start both threads.
- `pthread_create` calls: Launch sensor thread (TCP or SIM) and HTTP thread, both sharing the same `SharedState*`. Threads + mutex chosen over message queues to minimize allocations/copies; one shared struct is enough.
- Main loop: `while (running) sleep(1);` keeps the process alive until Ctrl+C without busy-waiting.
- Final log: Confirms intentional shutdown for user confidence.

## How this supports the product
- **Reliability**: Mutex + shared struct prevents inconsistent UI states. Ignoring `SIGPIPE` keeps the gateway alive during browser refreshes.
- **Real-time updates**: SSE pushes new readings immediately; no polling delays.
- **Alignment with simulator**: TCP defaults and newline framing match the provided Python GUI, reducing setup friction.
- **Portability/teachability**: Minimal dependencies (no getopt, no heavy frameworks) and standard pthreads make it easy to build and explain in class.
