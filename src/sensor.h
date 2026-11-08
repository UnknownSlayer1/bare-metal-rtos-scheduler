#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stdint.h>

#define FILTER_LEN 8

/* Simulated ADC sensor with a moving-average filter.
 * On real hardware this would read an ADC register; here the raw
 * value is produced by a deterministic pseudo-random walk so the
 * demo is repeatable. */
typedef struct {
    int32_t raw;
    int32_t filtered;
    int32_t window[FILTER_LEN];
    int window_idx;
    int window_count;
    int32_t threshold;
    bool alarm;
    uint32_t samples;
} sensor_t;

void sensor_init(sensor_t *s, int32_t threshold);
int32_t sensor_read_raw(sensor_t *s, uint32_t tick);
void sensor_sample(sensor_t *s, uint32_t tick);

#endif
