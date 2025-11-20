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

static const char* WEB_ROOT = "web";

static void send_404(int fd) {
    const char* msg = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: text/plain\r\n"
                      "Connection: close\r\n\r\nNot Found";
    write(fd, msg, strlen(msg));
}

static void send_file(int fd, const char* path, const char* ctype) {
    int f = open(path, O_RDONLY);
    if (f < 0) { send_404(fd); return; }
    struct stat st; fstat(f, &st);
    char hdr[256];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Type: %s\r\nConnection: close\r\n\r\n",
             (long)st.st_size, ctype);
    write(fd, hdr, strlen(hdr));
    char buf[4096]; ssize_t n;
    while ((n = read(f, buf, sizeof(buf))) > 0) write(fd, buf, n);
    close(f);
}

static const char* guess_ctype(const char* p) {
    const char* dot = strrchr(p, '.');
    if (!dot) return "text/plain";
    if (strcmp(dot, ".html")==0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css")==0)  return "text/css";
    if (strcmp(dot, ".js")==0)   return "application/javascript";
    if (strcmp(dot, ".png")==0)  return "image/png";
    if (strcmp(dot, ".svg")==0)  return "image/svg+xml";
    return "text/plain";
}

static void cat_safe(char* dst, size_t dstsz, const char* src) {
    size_t len = strlen(dst);
    if (len >= dstsz-1) return;
    strncat(dst, src, dstsz - len - 1);
}

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
        strncpy(list_out, "[]", list_sz);
        list_out[list_sz-1] = 0;
        strncpy(summary_out, "None", sum_sz);
        summary_out[sum_sz-1] = 0;
        return;
    }
    cat_safe(list_out, list_sz, "]");
}

static void json_for_current(SensorData* d, char* out, size_t outsz) {
    const char* conn = d->conn == CONN_CONNECTED ? "CONNECTED" : "DISCONNECTED";
    char alert_list[128];
    char alert_summary[128];
    build_alerts(d->alerts_mask, alert_list, sizeof(alert_list), alert_summary, sizeof(alert_summary));
    snprintf(out, outsz,
        "{ \"flow_lpm\": %.2f, \"humidity_pct\": %.2f, \"temperature_c\": %.2f, \"pressure_kpa\": %.2f, \"alerts\": %s, \"connection\": \"%s\", \"via\": \"%s\", \"seq\": %llu, \"alert_summary\": \"%s\" }",
        d->flow_lpm, d->humidity_pct, d->temperature_c, d->pressure_kpa, alert_list, conn, d->via, (unsigned long long)d->last_seq, alert_summary);
}

typedef struct { int fd; SharedState* st; } client_ctx;

static void* handle_client(void* arg) {
    client_ctx* ctx = (client_ctx*)arg;
    int fd = ctx->fd; SharedState* st = ctx->st;
    free(ctx);

    char req[1024]; int n = read(fd, req, sizeof(req)-1);
    if (n <= 0) { close(fd); return NULL; }
    req[n] = 0;

    char method[8], path[512];
    if (sscanf(req, "%7s %511s", method, path) != 2) { close(fd); return NULL; }
    if (strcmp(path, "/") == 0) strcpy(path, "/index.html");

    if (strcmp(path, "/events") == 0) {
        const char* hdr = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/event-stream\r\n"
                          "Cache-Control: no-cache\r\n"
                          "Connection: keep-alive\r\n\r\n";
        write(fd, hdr, strlen(hdr));
        uint64_t last = 0;
        for (;;) {
            pthread_mutex_lock(&st->mu);
            uint64_t seq = st->data.last_seq;
            SensorData snap = st->data;
            pthread_mutex_unlock(&st->mu);
            if (seq != last) {
                char json[384];
                json_for_current(&snap, json, sizeof(json));
                dprintf(fd, "data: %s\n\n", json);
                last = seq;
            } else {
                dprintf(fd, ": keepalive\n\n");
            }
            // if client closed connection, writing will fail eventually
            usleep(2000 * 1000);
        }
    } else {
        char full[1024];
        snprintf(full, sizeof(full), "%s%s", WEB_ROOT, path);
        const char* ctype = guess_ctype(full);
        send_file(fd, full, ctype);
    }
    close(fd);
    return NULL;
}

void* http_server_thread(void* arg) {
    SharedState* st = (SharedState*)arg;
    int port = st->web_port;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET; addr.sin_addr.s_addr = htonl(INADDR_ANY); addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) { LOG_ERR("bind failed"); exit(1); }
    if (listen(fd, 64) != 0) { LOG_ERR("listen failed"); exit(1); }
    LOG_INFO("HTTP server listening on http://localhost:%d", port);

    for (;;) {
        int cfd = accept(fd, NULL, NULL);
        if (cfd < 0) continue;
        pthread_t th;
        client_ctx* ctx = malloc(sizeof(client_ctx));
        ctx->fd = cfd; ctx->st = st;
        pthread_create(&th, NULL, handle_client, ctx);
        pthread_detach(th);
    }
    return NULL;
}
