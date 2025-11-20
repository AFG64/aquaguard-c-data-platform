#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

typedef enum {
    ALERTF_NONE = 0,
    ALERTF_HIGH_FLOW = 1 << 0,
    ALERTF_LOW_FLOW = 1 << 1,
    ALERTF_HIGH_HUMIDITY = 1 << 2,
    ALERTF_HIGH_TEMP = 1 << 3,
    ALERTF_HIGH_PRESSURE = 1 << 4
} AlertFlags;

typedef enum {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTED = 1
} ConnectionStatus;

typedef struct {
    float flow_lpm;        // liters per minute
    float humidity_pct;    // percent
    float temperature_c;   // celsius
    float pressure_kpa;    // kilopascals
    bool flowing;          // true/false (still tracked for compatibility)
    AlertFlags alerts_mask; // bitmask of active alerts
    ConnectionStatus conn; // connected or not
    char via[16];          // "TCP" or "SIM"
    uint64_t last_seq;     // increment on update
} SensorData;

// Thresholds for emergency alerts
#define FLOW_HIGH_EMERGENCY_THRESHOLD 45.0f   // near max
#define FLOW_LOW_EMERGENCY_THRESHOLD 0.3f     // effectively stopped
#define HUMIDITY_EMERGENCY_THRESHOLD 80.0f
#define TEMP_EMERGENCY_THRESHOLD 50.0f
#define PRESSURE_EMERGENCY_THRESHOLD 120.0f

typedef struct {
    pthread_mutex_t mu;
    SensorData data;
    int mode_tcp;          // 1=tcp, 0=sim
    char tcp_host[64];
    int tcp_port;
    int web_port;
} SharedState;

#endif
