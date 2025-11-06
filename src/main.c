#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "shared.h"
#include "sensor.h"
#include "http.h"
#include "log.h"

static volatile int running = 1;
static void on_sigint(int s) { (void)s; running = 0; }

int main(int argc, char** argv) {
    SharedState st;
    memset(&st, 0, sizeof(st));
    pthread_mutex_init(&st.mu, NULL);

    // defaults
    st.mode_tcp = 1;
    strcpy(st.tcp_host, "127.0.0.1");
    st.tcp_port = 5555;
    st.web_port = 8080;
    st.data.conn = CONN_DISCONNECTED;
    snprintf(st.data.via, sizeof(st.data.via), "TCP");

    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i], "--mode")==0 && i+1<argc) {
            if (strcmp(argv[i+1], "tcp")==0) st.mode_tcp = 1;
            else if (strcmp(argv[i+1], "sim")==0) st.mode_tcp = 0;
            i++;
        } else if (strcmp(argv[i], "--tcp-host")==0 && i+1<argc) {
            strncpy(st.tcp_host, argv[i+1], sizeof(st.tcp_host)-1);
            i++;
        } else if (strcmp(argv[i], "--tcp-port")==0 && i+1<argc) {
            st.tcp_port = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "--web-port")==0 && i+1<argc) {
            st.web_port = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "--help")==0) {
            printf("Usage: %s [--mode tcp|sim] [--tcp-host HOST] [--tcp-port P] [--web-port P]\n", argv[0]);
            return 0;
        }
    }

    LOG_INFO("AquaGuard starting: mode=%s, tcp=%s:%d, web=:%d",
        st.mode_tcp ? "tcp":"sim", st.tcp_host, st.tcp_port, st.web_port);

    signal(SIGINT, on_sigint);

    pthread_t th_sensor, th_http;
    if (st.mode_tcp) pthread_create(&th_sensor, NULL, sensor_thread_tcp, &st);
    else             pthread_create(&th_sensor, NULL, sensor_thread_sim, &st);
    pthread_create(&th_http, NULL, http_server_thread, &st);

    while (running) { sleep(1); }
    LOG_INFO("Shutting down...");
    return 0;
}
