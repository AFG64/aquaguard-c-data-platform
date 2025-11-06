#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <time.h>

static inline const char* _ts() {
    static char buf[32];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

#define LOG_INFO(fmt, ...)  fprintf(stderr, "[%s] [INFO] " fmt "\n", _ts(), ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fprintf(stderr, "[%s] [WARN] " fmt "\n", _ts(), ##__VA_ARGS__)
#define LOG_ERR(fmt, ...)   fprintf(stderr, "[%s] [ERR ] " fmt "\n", _ts(), ##__VA_ARGS__)

#endif
