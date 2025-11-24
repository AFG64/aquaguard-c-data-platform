# Rationale – `tests/test_parser.c`

This test validates the JSON parser and alert thresholds to ensure the dashboard shows correct data and warnings.

## What it covers
- Baseline parse with all fields present (flow, humidity, temp, pressure, flowing) and no alerts.
- High humidity triggers the correct alert and no extras.
- High flow triggers the high-flow alert.
- Low flow triggers the low-flow alert.

## Why this matters
- Confirms `parse_sensor_json` sets values and alert bitmask correctly; prevents regressions that would mislead the UI.
- Uses simple `expect` helper for readability and clear failure messages, matching the student-friendly style of the project.
