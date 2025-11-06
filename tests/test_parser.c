#include <stdio.h>
#include <string.h>
#include "json.h"

int main() {
    SensorData d = {0};
    const char* line = "{ \"flow_lpm\": 12.5, \"humidity_pct\": 45.0, \"flowing\": true, \"leak\": false, \"high_flow\": true }\n";
    int rc = parse_sensor_json(line, &d);
    if (rc != 0) { printf("parse failed\n"); return 1; }
    if ((int)d.flow_lpm != 12 || (int)d.humidity_pct != 45 || d.flowing != 1) { printf("values wrong\n"); return 1; }
    if (d.alert != 2) { printf("alert wrong\n"); return 1; }
    printf("OK\n");
    return 0;
}
