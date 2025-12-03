#include <stdio.h>
#include <string.h>
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

    // Multiple simultaneous alerts should all be present in the bitmask
    const char* multi = "{ \"flow_lpm\": 50.0, \"humidity_pct\": 85.0, \"temperature_c\": 55.0, \"pressure_kpa\": 130.0, \"flowing\": true }\n";
    int rc = parse_sensor_json(multi, &d);
    if (!expect(rc == 0, "multi parse failed")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_FLOW) != 0, "missing high flow")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_HUMIDITY) != 0, "missing high humidity")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_TEMP) != 0, "missing high temp")) return 1;
    if (!expect((d.alerts_mask & ALERTF_HIGH_PRESSURE) != 0, "missing high pressure")) return 1;
    if (!expect((d.alerts_mask & ALERTF_LOW_FLOW) == 0, "low flow should not be set")) return 1;

    // Low-flow alert should set only the low-flow bit (and nothing else)
    const char* low = "{ \"flow_lpm\": 0.1, \"humidity_pct\": 40.0, \"temperature_c\": 20.0, \"pressure_kpa\": 101.0, \"flowing\": true }\n";
    rc = parse_sensor_json(low, &d);
    if (!expect(rc == 0, "low-flow parse failed")) return 1;
    if (!expect((d.alerts_mask & ALERTF_LOW_FLOW) != 0, "missing low flow")) return 1;
    if (!expect((d.alerts_mask & (ALERTF_HIGH_FLOW | ALERTF_HIGH_HUMIDITY | ALERTF_HIGH_TEMP | ALERTF_HIGH_PRESSURE)) == 0,
                "unexpected high alerts on low-flow case")) return 1;

    printf("OK\n");
    return 0;
}
