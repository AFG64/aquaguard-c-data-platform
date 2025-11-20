#include <stdio.h>
#include <string.h>
#include "json.h"

int main() {
    SensorData d = {0};
    const char* line = "{ \"flow_lpm\": 12.5, \"humidity_pct\": 45.0, \"temperature_c\": 30.0, \"pressure_kpa\": 101.0, \"flowing\": true }\n";
    int rc = parse_sensor_json(line, &d);
    if (rc != 0) { printf("parse failed\n"); return 1; }
    if ((int)d.flow_lpm != 12 || (int)d.humidity_pct != 45 || d.flowing != 1) { printf("values wrong\n"); return 1; }
    if ((int)d.temperature_c != 30 || (int)d.pressure_kpa != 101) { printf("temp/pressure wrong\n"); return 1; }
    if (d.alerts_mask != ALERTF_NONE) { printf("unexpected alert triggered\n"); return 1; }

    // High humidity should raise humidity alert
    const char* line2 = "{ \"flow_lpm\": 5.0, \"humidity_pct\": 90.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line2, &d);
    if (rc != 0) { printf("second parse failed\n"); return 1; }
    if ((d.alerts_mask & ALERTF_HIGH_HUMIDITY) == 0) { printf("humidity alert missing\n"); return 1; }
    if ((d.alerts_mask & ALERTF_HIGH_TEMP) != 0 || (d.alerts_mask & ALERTF_HIGH_PRESSURE) != 0) { printf("unexpected extra alerts\n"); return 1; }

    // High flow should trigger high flow alert
    const char* line3 = "{ \"flow_lpm\": 50.0, \"humidity_pct\": 40.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line3, &d);
    if (rc != 0) { printf("third parse failed\n"); return 1; }
    if ((d.alerts_mask & ALERTF_HIGH_FLOW) == 0) { printf("high flow alert missing\n"); return 1; }

    // Low flow should trigger low flow alert
    const char* line4 = "{ \"flow_lpm\": 0.1, \"humidity_pct\": 40.0, \"flowing\": true }\n";
    rc = parse_sensor_json(line4, &d);
    if (rc != 0) { printf("fourth parse failed\n"); return 1; }
    if ((d.alerts_mask & ALERTF_LOW_FLOW) == 0) { printf("low flow alert missing\n"); return 1; }
    printf("OK\n");
    return 0;
}
