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
    d.temperature_c = 25.5f;
    d.pressure_kpa = 101.3f;
    d.flowing = 0;

    // Omit optional temperature/pressure/flowing; existing values should remain
    const char* partial = "{ \"flow_lpm\": 10.0, \"humidity_pct\": 55.0 }\n";
    int rc = parse_sensor_json(partial, &d);
    if (!expect(rc == 0, "partial parse failed")) return 1;
    if (!expect(d.flow_lpm == 10.0f && d.humidity_pct == 55.0f, "flow/humidity wrong")) return 1;
    if (!expect(d.temperature_c == 25.5f && d.pressure_kpa == 101.3f, "optional fields should be preserved")) return 1;
    if (!expect(d.flowing == 1, "flowing should default to true when missing")) return 1;

    // Explicit flowing=false should override default
    const char* stopped = "{ \"flow_lpm\": 0.0, \"humidity_pct\": 30.0, \"flowing\": false }\n";
    rc = parse_sensor_json(stopped, &d);
    if (!expect(rc == 0, "flowing=false parse failed")) return 1;
    if (!expect(d.flowing == 0, "flowing flag not updated")) return 1;

    printf("OK\n");
    return 0;
}
