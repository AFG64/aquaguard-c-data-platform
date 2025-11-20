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
            return fd;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return -1;
}

void* sensor_thread_tcp(void* arg) {
    SharedState* st = (SharedState*)arg;
    const int max_line = 1024;
    char buf[1024];
    int backoff_ms = 500;

    for (;;) {
        int fd = connect_tcp(st->tcp_host, st->tcp_port);
        if (fd < 0) {
            pthread_mutex_lock(&st->mu);
            st->data.conn = CONN_DISCONNECTED;
            snprintf(st->data.via, sizeof(st->data.via), "TCP");
            st->data.last_seq++;
            pthread_mutex_unlock(&st->mu);
            LOG_WARN("Simulator not reachable at %s:%d; retrying...", st->tcp_host, st->tcp_port);
            usleep(backoff_ms * 1000);
            if (backoff_ms < 5000) backoff_ms *= 2;
            continue;
        }
        LOG_INFO("Connected to simulator %s:%d", st->tcp_host, st->tcp_port);
        backoff_ms = 500;
        // Mark connected
        pthread_mutex_lock(&st->mu);
        st->data.conn = CONN_CONNECTED;
        snprintf(st->data.via, sizeof(st->data.via), "TCP");
        st->data.last_seq++;
        pthread_mutex_unlock(&st->mu);

        // Read loop
        char line[max_line];
        size_t idx = 0;
        for (;;) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n <= 0) {
                LOG_WARN("Simulator disconnected");
                close(fd);
                pthread_mutex_lock(&st->mu);
                st->data.conn = CONN_DISCONNECTED;
                st->data.last_seq++;
                pthread_mutex_unlock(&st->mu);
                break;
            }
            for (ssize_t i = 0; i < n; i++) {
                char c = buf[i];
                if (c == '\n' || idx >= max_line-1) {
                    line[idx] = 0;
                    SensorData tmp = st->data; // copy current
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
        // loop to reconnect
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

void* sensor_thread_sim(void* arg) {
    SharedState* st = (SharedState*)arg;
    srand((unsigned int)time(NULL));
    float flow = 2.0f;
    float hum = 40.0f;
    float temp = 22.0f;
    float pressure = 101.3f;
    int t = 0;
    for (;;) {
        flow = clamp(flow + ((rand()%200-100)/1000.0f), 0.f, 50.f);
        hum  = clamp(hum  + ((rand()%200-100)/1000.0f), 10.f, 90.f);
        temp = clamp(temp + ((rand()%200-100)/500.0f), -10.f, 60.f);
        pressure = clamp(pressure + ((rand()%200-100)/500.0f), 90.f, 130.f);
        int flowing = flow > 0.5f;
        AlertFlags alerts = eval_alerts(flow, hum, temp, pressure);

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

        t++;
        usleep(400 * 1000);
    }
    return NULL;
}
