#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

typedef enum {
    ALERT_NONE = 0,
    ALERT_LEAK = 1,
    ALERT_HIGH_FLOW = 2
} AlertStatus;

typedef enum {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTED = 1
} ConnectionStatus;

typedef struct {
    float flow_lpm;        // liters per minute
    float humidity_pct;    // percent
    bool flowing;          // true/false
    AlertStatus alert;     // alarm state
    ConnectionStatus conn; // connected or not
    char via[16];          // "TCP" or "SIM"
    uint64_t last_seq;     // increment on update
} SensorData;

typedef struct {
    pthread_mutex_t mu;
    SensorData data;
    int mode_tcp;          // 1=tcp, 0=sim
    char tcp_host[64];
    int tcp_port;
    int web_port;
} SharedState;

#endif
