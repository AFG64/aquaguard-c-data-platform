#ifndef SENSOR_H
#define SENSOR_H
#include "shared.h"

void* sensor_thread_tcp(void* arg);
void* sensor_thread_sim(void* arg);

#endif
