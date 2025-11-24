# Rationale – `include/sensor.h`

Declares the two sensor thread entry points:
- `sensor_thread_tcp`: connects to the simulator over TCP, reads JSON lines, updates `SharedState`.
- `sensor_thread_sim`: generates fake readings when the simulator is absent.

## Why this split
- Keeps the mode decision in `main.c` while isolating the implementations.
- Lets tests or future integrations choose a mode without touching internals.
