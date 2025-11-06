#ifndef JSON_H
#define JSON_H
#include "shared.h"

// Parse a minimal JSON line with keys: flow_lpm, humidity_pct, flowing, leak, high_flow.
// Returns 0 on success, -1 on failure.
int parse_sensor_json(const char* line, SensorData* out);

#endif
