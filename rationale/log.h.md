# Rationale – `include/log.h`

Provides lightweight logging macros with timestamps.

## Why this approach
- Inline functions/macros avoid extra dependencies or init code.
- Timestamps help correlate events between threads (connect/disconnect, HTTP info).
- Kept minimal to stay portable across OSes and classroom setups.
