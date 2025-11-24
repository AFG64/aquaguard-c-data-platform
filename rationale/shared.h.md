# Rationale – `include/shared.h`

`SharedState` and `SensorData` define what every thread shares. The design favors clarity and compactness while keeping synchronization explicit.

## Structures and flags
- `SensorData`: One snapshot of readings (flow, humidity, temp, pressure), connection info, alert bitmask, and a `last_seq` counter.
- `AlertFlags`: Bitmask so multiple alerts can coexist in one integer; cheap to test and send over SSE.
- `ConnectionStatus`: Enum for connected/disconnected to drive UI state.
- Threshold macros: Centralized limits so parser/simulator/HTTP all agree on alert triggers.
- `SharedState`: Holds the mutex, current `SensorData`, and config (mode, host, ports).

## Why this layout
- Single shared struct keeps memory small and avoids copies; the `pthread_mutex_t` inside `SharedState` protects concurrent reads/writes so the sensor thread can overwrite fields while the HTTP thread reads a consistent snapshot. This avoids torn values (e.g., half-updated floats) and keeps synchronization local to the data it guards.
- Bitmask alerts are space-efficient and let the UI show multiple issues at once.
- Thresholds as macros keep them consistent across code and tests.
