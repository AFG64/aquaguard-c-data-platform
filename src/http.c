#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "http.h"
#include "log.h"

// This file is a tiny web server for the dashboard.
// It serves two things:
//   1) Static files from the "web" folder (HTML/CSS/JS) so the page can load.
//   2) Live sensor updates over Server-Sent Events at /events so the graph stays fresh.
// We picked SSE over WebSockets to keep the protocol one-way and simple (browsers support it natively).
// A thread-per-connection model is used here because it is easy to read; performance is fine for a demo.

static const char* WEB_ROOT = "web";

// Tiny helpers for serving static files (HTML/JS/CSS/images) from the web folder
// Rolling our own here avoids pulling in a full HTTP library for a handful of routes.
static void send_404(int fd) {
    const char* msg = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: text/plain\r\n"
                      "Connection: close\r\n\r\nNot Found";
    write(fd, msg, strlen(msg));
}

static void send_file(int fd, const char* path, const char* ctype) {
    int f = open(path, O_RDONLY);
    if (f < 0) { send_404(fd); return; }

    struct stat st;
    fstat(f, &st);
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",
             (long)st.st_size, ctype);
    write(fd, hdr, strlen(hdr));

    char buf[4096];
    ssize_t n;
    while ((n = read(f, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }
    close(f);
}

// Guess content type by file extension (good enough for demo)
static const char* guess_ctype(const char* p) {
    const char* dot = strrchr(p, '.');
    if (!dot) return "text/plain";
    if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".png")  == 0) return "image/png";
    if (strcmp(dot, ".svg")  == 0) return "image/svg+xml";
    return "text/plain";
}

// Safer strcat for fixed buffers
static void cat_safe(char* dst, size_t dstsz, const char* src) {
    size_t len = strlen(dst);
    if (len >= dstsz - 1) return;
    strncat(dst, src, dstsz - len - 1);
}

// Fill both JSON alert list and human summary
static void build_alerts(AlertFlags mask, char* list_out, size_t list_sz, char* summary_out, size_t sum_sz) {
    int first = 1;
    list_out[0] = '['; list_out[1] = 0;
    summary_out[0] = 0;

#define ADD(alert_flag, code, label) \
    if (mask & alert_flag) { \
        if (!first) { cat_safe(list_out, list_sz, ","); cat_safe(summary_out, sum_sz, ", "); } \
        cat_safe(list_out, list_sz, "\"" code "\""); \
        cat_safe(summary_out, sum_sz, label); \
        first = 0; \
    }
    ADD(ALERTF_HIGH_FLOW, "HIGH_FLOW", "High flow");
    ADD(ALERTF_LOW_FLOW, "LOW_FLOW", "Low flow");
    ADD(ALERTF_HIGH_HUMIDITY, "HIGH_HUMIDITY", "High humidity");
    ADD(ALERTF_HIGH_TEMP, "HIGH_TEMP", "High temperature");
    ADD(ALERTF_HIGH_PRESSURE, "HIGH_PRESSURE", "High pressure");
#undef ADD

    if (first) {
        // No alerts at all
        strncpy(list_out, "[]", list_sz);
        list_out[list_sz - 1] = 0;
        strncpy(summary_out, "None", sum_sz);
        summary_out[sum_sz - 1] = 0;
        return;
    }
    cat_safe(list_out, list_sz, "]");
}

// Convert SensorData into a single JSON string for SSE.
// The browser receives this string and updates the dashboard in real time.
static void json_for_current(SensorData* d, char* out, size_t outsz) {
    const char* conn = d->conn == CONN_CONNECTED ? "CONNECTED" : "DISCONNECTED";
    char alert_list[128];
    char alert_summary[128];
    build_alerts(d->alerts_mask, alert_list, sizeof(alert_list), alert_summary, sizeof(alert_summary));
    snprintf(out, outsz,
        "{ \"flow_lpm\": %.2f, \"humidity_pct\": %.2f, \"temperature_c\": %.2f, \"pressure_kpa\": %.2f, \"alerts\": %s, \"connection\": \"%s\", \"via\": \"%s\", \"seq\": %llu, \"alert_summary\": \"%s\" }",
        d->flow_lpm, d->humidity_pct, d->temperature_c, d->pressure_kpa, alert_list, conn, d->via, (unsigned long long)d->last_seq, alert_summary);
}

typedef struct {
    int fd;
    SharedState* st;
} ClientCtx;

// Handle a single HTTP client (either SSE stream or static file request)
static void* handle_client(void* arg) {
    ClientCtx* ctx = (ClientCtx*)arg;
    int fd = ctx->fd;
    SharedState* st = ctx->st;
    free(ctx);

    char req[1024];
    int n = read(fd, req, sizeof(req) - 1);
    if (n <= 0) { close(fd); return NULL; }
    req[n] = 0;

    char method[8], path[512];
    if (sscanf(req, "%7s %511s", method, path) != 2) { close(fd); return NULL; }
    if (strcmp(path, "/") == 0) strcpy(path, "/index.html");

    // Live updates via Server-Sent Events:
    // we keep the connection open and push JSON whenever sensor data changes (last_seq changes).
    // SSE was chosen instead of polling to reduce reload latency and bandwidth.
    if (strcmp(path, "/events") == 0) {
        const char* hdr = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/event-stream\r\n"
                          "Cache-Control: no-cache\r\n"
                          "Connection: keep-alive\r\n\r\n";
        write(fd, hdr, strlen(hdr));

        uint64_t last_seq = 0;
        for (;;) {
            pthread_mutex_lock(&st->mu);
            uint64_t seq = st->data.last_seq;
            SensorData snap = st->data;
            pthread_mutex_unlock(&st->mu);

            if (seq != last_seq) {
                char json[384];
                json_for_current(&snap, json, sizeof(json));
                dprintf(fd, "data: %s\n\n", json);
                last_seq = seq;
            } else {
                dprintf(fd, ": keepalive\n\n");
            }
            // If client disconnects, writes will eventually fail
            usleep(2000 * 1000);
        }
    }

    // Otherwise serve a static file from web/ (HTML, CSS, JS, images) to render the dashboard
    char full[1024];
    snprintf(full, sizeof(full), "%s%s", WEB_ROOT, path);
    send_file(fd, full, guess_ctype(full));

    close(fd);
    return NULL;
}

// Basic HTTP server that spawns a thread per client.
// This avoids an event loop and keeps the concurrency model aligned with the sensor thread.
void* http_server_thread(void* arg) {
    SharedState* st = (SharedState*)arg;
    int port = st->web_port;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        LOG_ERR("bind failed");
        exit(1);
    }
    if (listen(fd, 64) != 0) {
        LOG_ERR("listen failed");
        exit(1);
    }
    LOG_INFO("HTTP server listening on http://localhost:%d", port);

    for (;;) {
        int cfd = accept(fd, NULL, NULL);
        if (cfd < 0) continue;

        pthread_t th;
        ClientCtx* ctx = malloc(sizeof(ClientCtx));
        ctx->fd = cfd;
        ctx->st = st;
        pthread_create(&th, NULL, handle_client, ctx);
        pthread_detach(th);
    }
    return NULL;
}
