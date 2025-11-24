# Rationale – `include/json.h`

Declares `parse_sensor_json`, the minimal parser used by the sensor thread.

## Why this interface
- Single entry point that takes a JSON line and a `SensorData` pointer, returning `0/-1` so callers can drop bad packets without crashing.
- Kept small to avoid coupling sensor and HTTP code to a larger parsing library.
- Lives in `include/` so both sensor code and tests share the same contract.
