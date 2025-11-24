#include <stdio.h>
#include <string.h>
#include "json.h"

// This file is a tiny self-check for the JSON parser.
// It feeds sample JSON strings (like the simulator would send) and verifies the parsed values.
// If these expectations fail, the dashboard would show wrong numbers or miss alerts.

// Tiny helper to keep assertions short
static int expect(int cond, const char* msg) {
    if (!cond) {
        printf("%s\n", msg);
        return 0;
    }
    return 1;
}

int main() {
    SensorData d = (SensorData){0};

    // Baseline parse: all fields present and no alerts expected
    const char* line = "{ \"flow_lpm\": 12.5, \"humidity_pct\": 45.0, \"temperature_c\": 30.0, \"pressure_kpa\": 101.0, \"flowing\": true }\n";
    int rc = parse_sensor_json(line, &d);
    if (!expect(rc == 0, "parse failed")) return 1;
    if (!expect((int)d.flow_lpm == 12 && (int)d.humidity_pct == 45 && d.flowing == 1, "values wrong")) return 1;
    if (!expect((int)d.temperature_c == 30 && (int)d.pressure_kpa == 101, "temp/pressure wrong")) return 1;
    if (!expect(d.alerts_mask == ALERTF_NONE, "unexpected alert triggered")) return 1;

    // High humidity should raise humidity alert (and nothing else)
    const char* line2 = "{ \"flow_lpm\": 5.0, \"humidity_pct\": 90.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line2, &d);
    if (!expect(rc == 0, "second parse failed")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_HUMIDITY) != 0, "humidity alert missing")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_TEMP) == 0 && (d.alerts_mask & ALERTF_HIGH_PRESSURE) == 0, "unexpected extra alerts")) return 1;

    // High flow should trigger high flow alert
    const char* line3 = "{ \"flow_lpm\": 50.0, \"humidity_pct\": 40.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line3, &d);
    if (!expect(rc == 0, "third parse failed")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_FLOW) != 0, "high flow alert missing")) return 1;

    // Low flow should trigger low flow alert
    const char* line4 = "{ \"flow_lpm\": 0.1, \"humidity_pct\": 40.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line4, &d);
    if (!expect(rc == 0, "fourth parse failed")) return 1;
    if (!expect((d.alerts_mask & ALERTF_LOW_FLOW) != 0, "low flow alert missing")) return 1;

    printf("OK\n");
    return 0;
}
