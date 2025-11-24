# Rationale – `src/sensor.c`

This file feeds `SharedState` with live readings. It supports two modes:
- **TCP mode**: connects to the Python simulator that streams newline-delimited JSON.
- **SIM mode**: generates plausible fake readings so the dashboard works without the simulator.

## Key choices
- **TCP (not UDP)**: Simulator already speaks TCP; we need ordered, reliable delivery for JSON lines. No packet loss/reordering headaches.
- **Newline framing**: One JSON per line keeps parsing trivial; no custom protocol required.
- **`getaddrinfo`**: Portable IPv4/IPv6 resolution for host/port across OSes.
- **Mutex-protected `SharedState`**: One shared struct; sensor thread writes, HTTP thread reads. The mutex ensures the writer and reader never collide, so the dashboard sees complete snapshots (no half-updated floats/flags). This keeps memory small (one struct) and synchronization explicit.
- **Exponential backoff**: On connection failure, wait longer (up to 5s) to avoid hammering the simulator and spamming logs.
- **SIM mode clamping**: Random walks bounded to realistic ranges so the UI shows believable numbers and alerts.

## Walk-through
- `connect_tcp`: resolves host/port and tries sockets until one connects. Returns `-1` on failure so the thread can back off and retry.
- `set_connection_status`: updates connection status/via and bumps `last_seq` under a mutex so the HTTP thread knows data changed. Mutex here matters because connection flags and `last_seq` must stay consistent.
- `sensor_thread_tcp`:
  - Attempts connection; on failure, marks disconnected and backs off.
  - On success, reads raw bytes, accumulates until newline, then parses with `parse_sensor_json`.
  - Copies current `SharedState` into a temp, overlays parsed fields (keeps optional values), writes back under mutex, increments `last_seq`.
  - If read fails, marks disconnected and reconnects. Mutexed writes plus `last_seq` guarantee the HTTP thread never sees mixed data from two lines.
- `eval_alerts`: Recomputes alert bitmask locally for SIM mode; mirrors thresholds used by the parser.
- `sensor_thread_sim`:
  - Seeds random once, then perturbs flow/humidity/temp/pressure each cycle.
  - Clamps values to realistic ranges; sets `flowing` if flow > 0.5 L/min.
  - Writes readings + alerts into `SharedState`, sets `via` to `SIM`, bumps `last_seq`, sleeps ~0.4s.

## How this serves the product
- Keeps the dashboard live whether or not the simulator is running (SIM fallback).
- Ensures the UI sees consistent snapshots (mutex + `last_seq`), avoiding half-updated data.
- Uses the simulator’s existing TCP format to reduce setup friction for demos and grading.
