#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
#include "sensor.h"
#include "json.h"
#include "log.h"

// This file feeds SensorData into the shared SharedState (used by both threads).
// It can either:
//   1) connect over TCP to an external simulator and parse its JSON lines, or
//   2) generate pretend readings locally (SIM mode) when no simulator is available.
// We use TCP (not UDP) because the Python simulator already exposes a reliable, newline-delimited TCP stream
// and we prefer delivery guarantees over minimal latency. The HTTP thread later reads SharedState to send
// live updates to the dashboard.

// Attempt to open a TCP socket to the simulator (host:port)
// Using getaddrinfo keeps it IPv4/IPv6 agnostic and portable across OSes.
static int connect_tcp(const char* host, int port) {
    char portstr[16];
    snprintf(portstr, sizeof(portstr), "%d", port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = NULL;
    int rc = getaddrinfo(host, portstr, &hints, &res);
    if (rc != 0) {
        LOG_ERR("getaddrinfo: %s", gai_strerror(rc));
        return -1;
    }

    int fd = -1;
    for (struct addrinfo* p = res; p; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {
            freeaddrinfo(res);
            return fd; // success
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return -1;
}

// Helper to update connection fields together (with locking).
// Also bumps last_seq so the HTTP thread knows data changed and should push an update.
static void set_connection_status(SharedState* st, ConnectionStatus status, const char* via) {
    pthread_mutex_lock(&st->mu);
    st->data.conn = status;
    if (via) snprintf(st->data.via, sizeof(st->data.via), "%s", via);
    st->data.last_seq++;
    pthread_mutex_unlock(&st->mu);
}

// Thread: read newline-separated JSON packets from the TCP simulator.
// Newline framing was chosen because the simulator already sends one JSON per line—no extra protocol needed.
// Each parsed packet overwrites SharedState so the dashboard shows fresh numbers.
void* sensor_thread_tcp(void* arg) {
    SharedState* st = (SharedState*)arg;
    const int max_line = 1024;
    char buf[1024];
    int backoff_ms = 500;

    for (;;) {
        int fd = connect_tcp(st->tcp_host, st->tcp_port);
        if (fd < 0) {
            set_connection_status(st, CONN_DISCONNECTED, "TCP");
            LOG_WARN("Simulator not reachable at %s:%d; retrying...", st->tcp_host, st->tcp_port);
            usleep(backoff_ms * 1000);
            if (backoff_ms < 5000) backoff_ms *= 2; // exponential backoff to avoid hammering the host
            continue;
        }

        LOG_INFO("Connected to simulator %s:%d", st->tcp_host, st->tcp_port);
        backoff_ms = 500;
        set_connection_status(st, CONN_CONNECTED, "TCP");

        char line[1024]; // fixed-size buffer for incoming line
        size_t idx = 0;
        for (;;) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) {
                LOG_WARN("Simulator disconnected");
                close(fd);
                set_connection_status(st, CONN_DISCONNECTED, "TCP");
                break; // reconnect loop
            }

            // Collect characters until newline, then parse that line as JSON.
            // When parsing succeeds, we store into st->data and bump last_seq to wake the HTTP thread.
            for (ssize_t i = 0; i < n; i++) {
                char c = buf[i];
                if (c == '\n' || idx >= max_line - 1) {
                    line[idx] = 0;
                    SensorData tmp = st->data; // start from previous values so optional fields stay (simulate partial updates)
                    if (parse_sensor_json(line, &tmp) == 0) {
                        tmp.conn = CONN_CONNECTED;
                        snprintf(tmp.via, sizeof(tmp.via), "TCP");
                        pthread_mutex_lock(&st->mu);
                        st->data = tmp;
                        st->data.last_seq++;
                        pthread_mutex_unlock(&st->mu);
                    }
                    idx = 0;
                } else {
                    line[idx++] = c;
                }
            }
        }
    }
    return NULL;
}

static float clamp(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static AlertFlags eval_alerts(float flow, float hum, float temp, float pressure) {
    AlertFlags mask = ALERTF_NONE;
    if (!isnan(flow) && flow > FLOW_HIGH_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_FLOW;
    if (!isnan(flow) && flow < FLOW_LOW_EMERGENCY_THRESHOLD)  mask |= ALERTF_LOW_FLOW;
    if (!isnan(hum) && hum > HUMIDITY_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_HUMIDITY;
    if (!isnan(temp) && temp > TEMP_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_TEMP;
    if (!isnan(pressure) && pressure > PRESSURE_EMERGENCY_THRESHOLD) mask |= ALERTF_HIGH_PRESSURE;
    return mask;
}

// Thread: generate pretend sensor readings locally (SIM mode).
// This lets the app run even if no TCP simulator is reachable.
void* sensor_thread_sim(void* arg) {
    SharedState* st = (SharedState*)arg;
    srand((unsigned int)time(NULL));

    float flow = 2.0f;
    float hum = 40.0f;
    float temp = 22.0f;
    float pressure = 101.3f;

    for (;;) {
        // Wander values a bit so the graph moves (adds randomness each cycle).
        // Clamp keeps the fake readings in a believable range.
        flow = clamp(flow + ((rand() % 200 - 100) / 1000.0f), 0.f, 50.f);
        hum  = clamp(hum  + ((rand() % 200 - 100) / 1000.0f), 10.f, 90.f);
        temp = clamp(temp + ((rand() % 200 - 100) / 500.0f), -10.f, 60.f);
        pressure = clamp(pressure + ((rand() % 200 - 100) / 500.0f), 90.f, 130.f);

        int flowing = flow > 0.5f;
        AlertFlags alerts = eval_alerts(flow, hum, temp, pressure);

        // Share the latest readings with the rest of the program
        pthread_mutex_lock(&st->mu);
        st->data.flow_lpm = flow;
        st->data.humidity_pct = hum;
        st->data.temperature_c = temp;
        st->data.pressure_kpa = pressure;
        st->data.flowing = flowing;
        st->data.alerts_mask = alerts;
        st->data.conn = CONN_CONNECTED;
        snprintf(st->data.via, sizeof(st->data.via), "SIM");
        st->data.last_seq++;
        pthread_mutex_unlock(&st->mu);

        usleep(400 * 1000); // pause ~0.4s between readings (about 2.5 updates/second)
    }
    return NULL;
}
