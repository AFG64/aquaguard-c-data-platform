# Rationale – `src/json.c`

This file is a minimal, hand-rolled parser for one trusted JSON line from the simulator. Choices are tied to the product goal: keep dependencies tiny, parse fast, and provide alert flags for the dashboard.

## Why a custom parser
- Input is fixed-format JSON (known keys) from our simulator; pulling a full JSON library would add build complexity for little benefit in class.
- Single-line, trusted input means simple string scans are enough; fewer dependencies make the project portable and easy to grade.

## Walk-through
- `find_value_start`: string search to jump after `key:` and whitespace (skips optional quotes). No heap allocations; works for IPv4/IPv6 text and numbers alike.
- `extract_float` / `extract_bool`: reuse `find_value_start`, then `strtof` or case-insensitive compares to pull numbers/true/false. Returns `-1` on failure so callers can drop bad packets.
- `parse_sensor_json`:
  - Keeps prior values for optional fields (temperature/pressure/flowing) so the UI doesn’t flicker to NaN if a field is omitted.
  - Requires flow and humidity; otherwise returns `-1` to signal bad input.
  - Computes `alerts_mask` with bitwise flags (cheap to store/transmit) for the HTTP thread to display banners and highlight metrics.

## Project-specific rationale
- Alert thresholds are enforced here so every consumer (HTTP, tests) gets consistent status.
- Returns explicit status codes (0/-1) to let the sensor thread decide whether to ignore a malformed line without crashing.
- No dynamic allocations keep the parser deterministic and fast for classroom environments.
- Concurrency: Parsing is pure; the caller (sensor thread) handles locking via `SharedState`’s mutex so JSON parsing cannot race. Keeping it side-effect free makes it safe wherever the caller holds the lock.
