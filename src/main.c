// AquaGuard entry point.
// This program has two jobs:
//   1) get sensor readings (either over TCP from the simulator or generated locally)
//   2) share those readings with the web dashboard through a small HTTP server
// We keep two threads instead of an event loop so the code stays approachable for beginners.
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

// Stop the program when Ctrl+C is pressed (friendly shutdown)
static void on_sigint(int s) {
    (void)s;
    running = 0;
}

// Ignore SIGPIPE so a page refresh/disconnect does not crash the server
static void ignore_sigpipe(void) {
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
}

// Print a tiny help line so users know the knobs they can turn
static void print_usage(const char* prog) {
    printf("Usage: %s [--mode tcp|sim] [--tcp-host HOST] [--tcp-port P] [--web-port P]\n", prog);
}

// Put all the startup defaults in one spot for the shared state that EVERY thread uses:
// - TCP chosen because the Python simulator already exposes a simple TCP stream (newline JSON)
// - Web dashboard on port 8080 so it does not clash with privileged ports
// - Start as "disconnected" so the UI reflects reality until data arrives
static void init_defaults(SharedState* st) {
    memset(st, 0, sizeof(*st));
    pthread_mutex_init(&st->mu, NULL);
    st->mode_tcp = 1; // default to TCP streaming
    strcpy(st->tcp_host, "127.0.0.1");
    st->tcp_port = 5555;
    st->web_port = 8080;
    st->data.conn = CONN_DISCONNECTED;
    snprintf(st->data.via, sizeof(st->data.via), "TCP");
}

// Very direct argument parser (kept student-simple):
// picks between TCP vs simulator and overrides ports/host if provided.
// SharedState carries these choices so both threads see the same config.
static void parse_args(int argc, char** argv, SharedState* st) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            if (strcmp(argv[i + 1], "tcp") == 0) st->mode_tcp = 1;
            else if (strcmp(argv[i + 1], "sim") == 0) st->mode_tcp = 0;
            i++;
        } else if (strcmp(argv[i], "--tcp-host") == 0 && i + 1 < argc) {
            strncpy(st->tcp_host, argv[i + 1], sizeof(st->tcp_host) - 1);
            i++;
        } else if (strcmp(argv[i], "--tcp-port") == 0 && i + 1 < argc) {
            st->tcp_port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--web-port") == 0 && i + 1 < argc) {
            st->web_port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
    }
}

int main(int argc, char** argv) {
    SharedState st;
    init_defaults(&st);
    parse_args(argc, argv, &st);

    LOG_INFO("AquaGuard starting: mode=%s, tcp=%s:%d, web=:%d",
        st.mode_tcp ? "tcp" : "sim", st.tcp_host, st.tcp_port, st.web_port);

    // Set up signals before threads start so we can quit cleanly
    signal(SIGINT, on_sigint);
    ignore_sigpipe();

    // Start background threads:
    // - sensor thread pulls data (TCP or simulator) and writes into SharedState
    // - HTTP thread reads from SharedState to serve the dashboard + live updates
    // Threads + mutex were picked over message queues to stay minimal and portable.
    pthread_t th_sensor, th_http;
    if (st.mode_tcp) {
        pthread_create(&th_sensor, NULL, sensor_thread_tcp, &st);
    } else {
        pthread_create(&th_sensor, NULL, sensor_thread_sim, &st);
    }
    pthread_create(&th_http, NULL, http_server_thread, &st);

    // Main thread only waits for Ctrl+C, leaving work to the other threads
    while (running) {
        sleep(1);
    }

    LOG_INFO("Shutting down...");
    return 0;
}
