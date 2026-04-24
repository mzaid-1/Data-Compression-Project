#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <time.h>
#include "../include/bzip2_impl.h"

double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
}