#include <stdio.h>
#include "json.h"

static int expect(int cond, const char* msg) {
    if (!cond) {
        printf("%s\n", msg);
        return 0;
    }
    return 1;
}

int main() {
    SensorData d = (SensorData){0};

    // Missing flow should fail
    const char* missing_flow = "{ \"humidity_pct\": 50.0, \"temperature_c\": 20.0 }\n";
    int rc = parse_sensor_json(missing_flow, &d);
    if (!expect(rc != 0, "parse should fail when flow_lpm is missing")) return 1;

    // Missing humidity should fail
    const char* missing_hum = "{ \"flow_lpm\": 5.0, \"temperature_c\": 20.0 }\n";
    rc = parse_sensor_json(missing_hum, &d);
    if (!expect(rc != 0, "parse should fail when humidity_pct is missing")) return 1;

    // Valid line should still succeed
    const char* ok = "{ \"flow_lpm\": 5.0, \"humidity_pct\": 40.0 }\n";
    rc = parse_sensor_json(ok, &d);
    if (!expect(rc == 0, "valid parse should succeed")) return 1;

    printf("OK\n");
    return 0;
}
