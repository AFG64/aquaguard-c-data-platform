#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>
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
}

int parse_sensor_json(const char* line, SensorData* out) {
    if (!line || !out) return -1;
    float flow=0.0f, hum=0.0f; int flowing=0, leak=0, high=0;

    if (extract_float(line, "flow_lpm", &flow) != 0) return -1;
    if (extract_float(line, "humidity_pct", &hum) != 0) return -1;
    if (extract_bool(line, "flowing", &flowing) != 0) return -1;
    extract_bool(line, "leak", &leak);
    extract_bool(line, "high_flow", &high);

    out->flow_lpm = flow;
    out->humidity_pct = hum;
    out->flowing = flowing ? true : false;
    if (leak) out->alert = ALERT_LEAK;
    else if (high) out->alert = ALERT_HIGH_FLOW;
    else out->alert = ALERT_NONE;
    return 0;
}
