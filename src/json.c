#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include "json.h"
#include "log.h"

// This file is a tiny, hand-rolled parser used by the sensor thread.
// Why custom parser? Input is a single trusted JSON line with fixed keys, so pulling a full JSON library
// would add complexity. This scanner keeps dependencies light and is easy to debug in class.
// The web server later reads SensorData to show the dashboard, so we keep this minimal and fast.

// Move a pointer to the start of the value for a given "key:"
// Using string search avoids heap allocations and keeps the footprint tiny.
static const char* find_value_start(const char* s, const char* key) {
    const char* spot = strstr(s, key);
    if (!spot) return NULL;
    spot = strchr(spot, ':');
    if (!spot) return NULL;
    spot++; // step past colon
    while (*spot && isspace((unsigned char)*spot)) spot++;
    if (*spot == '"') spot++; // ignore accidental quotes
    return spot;
}

// Pull a float out of the JSON line (like "12.3") for a known key; simple because keys are fixed.
static int extract_float(const char* s, const char* key, float* out) {
    const char* start = find_value_start(s, key);
    if (!start) return -1;
    char* endptr;
    float val = strtof(start, &endptr);
    if (endptr == start) return -1;
    *out = val;
    return 0;
}

// Pull a bool-like token ("true"/"false") out of the JSON line (no need for a full tokenizer).
static int extract_bool(const char* s, const char* key, int* out) {
    const char* start = find_value_start(s, key);
    if (!start) return -1;
    if (strncasecmp(start, "true", 4) == 0) {
        *out = 1;
        return 0;
    }
    if (strncasecmp(start, "false", 5) == 0) {
        *out = 0;
        return 0;
    }
    return -1;
}

int parse_sensor_json(const char* line, SensorData* out) {
    if (!line || !out) return -1;

    // Start with defaults; optional fields overwrite later.
    // If the simulator skipped a value, we keep the previous one already in SensorData.
    // This avoids marking readings as NaN and keeps the display stable.
    float flow = 0.0f;
    float hum = 0.0f;
    float temp = out->temperature_c;
    float pressure = out->pressure_kpa;
    int flowing = 1;
    int has_temp = 0, has_pressure = 0, has_flowing = 0;

    // Required fields must parse or we give up (flow and humidity are mandatory)
    if (extract_float(line, "flow_lpm", &flow) != 0) return -1;
    if (extract_float(line, "humidity_pct", &hum) != 0) return -1;
    // Optional fields
    if (extract_float(line, "temperature_c", &temp) == 0) has_temp = 1;
    if (extract_float(line, "pressure_kpa", &pressure) == 0) has_pressure = 1;
    if (extract_bool(line, "flowing", &flowing) == 0) has_flowing = 1;

    // Copy values into output so the rest of the program can read them
    out->flow_lpm = flow;
    out->humidity_pct = hum;
    if (has_temp) out->temperature_c = temp;
    if (has_pressure) out->pressure_kpa = pressure;
    if (has_flowing) out->flowing = flowing ? true : false;
    else out->flowing = true;

    // Build alert mask in one pass.
    // The HTTP thread uses these flags to display warning banners on the dashboard.
    AlertFlags mask = ALERTF_NONE;
    if (!isnan(out->flow_lpm) && out->flow_lpm > FLOW_HIGH_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_FLOW;
    if (!isnan(out->flow_lpm) && out->flow_lpm < FLOW_LOW_EMERGENCY_THRESHOLD)  mask |= ALERTF_LOW_FLOW;
    if (!isnan(out->humidity_pct) && out->humidity_pct > HUMIDITY_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_HUMIDITY;
    if (!isnan(out->temperature_c) && out->temperature_c > TEMP_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_TEMP;
    if (!isnan(out->pressure_kpa) && out->pressure_kpa > PRESSURE_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_PRESSURE;
    out->alerts_mask = mask;
    return 0;
}
