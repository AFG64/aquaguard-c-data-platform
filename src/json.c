#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
#include <math.h>
#include "json.h"
#include "log.h"

static int extract_float(const char* s, const char* key, float* out) {
    const char* p = strstr(s, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p=='"') p++; // skip accidental quote
    char* endptr;
    float val = strtof(p, &endptr);
    if (endptr != p) { *out = val; return 0; }
    return -1;
}

static int extract_bool(const char* s, const char* key, int* out) {
    const char* p = strstr(s, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncasecmp(p, "true", 4) == 0) { *out = 1; return 0; }
    if (strncasecmp(p, "false", 5) == 0) { *out = 0; return 0; }
    return -1;
    return -1;
}

int parse_sensor_json(const char* line, SensorData* out) {
    if (!line || !out) return -1;
    float flow=0.0f, hum=0.0f;
    float temp = out->temperature_c;
    float pressure = out->pressure_kpa;
    int has_temp = 0, has_pressure = 0;
    int flowing=1;
    int has_flowing = 0;

    if (extract_float(line, "flow_lpm", &flow) != 0) return -1;
    if (extract_float(line, "humidity_pct", &hum) != 0) return -1;
    if (extract_float(line, "temperature_c", &temp) == 0) has_temp = 1;
    if (extract_float(line, "pressure_kpa", &pressure) == 0) has_pressure = 1;
    if (extract_bool(line, "flowing", &flowing) == 0) has_flowing = 1;

    out->flow_lpm = flow;
    out->humidity_pct = hum;
    if (has_temp) out->temperature_c = temp;
    if (has_pressure) out->pressure_kpa = pressure;
    if (has_flowing) out->flowing = flowing ? true : false;
    else out->flowing = true;
    AlertFlags mask = ALERTF_NONE;
    if (!isnan(out->flow_lpm) && out->flow_lpm > FLOW_HIGH_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_FLOW;
    if (!isnan(out->flow_lpm) && out->flow_lpm < FLOW_LOW_EMERGENCY_THRESHOLD) mask |= ALERTF_LOW_FLOW;
    if (!isnan(out->humidity_pct) && out->humidity_pct > HUMIDITY_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_HUMIDITY;
    if (!isnan(out->temperature_c) && out->temperature_c > TEMP_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_TEMP;
    if (!isnan(out->pressure_kpa) && out->pressure_kpa > PRESSURE_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_PRESSURE;
    out->alerts_mask = mask;
    return 0;
}
