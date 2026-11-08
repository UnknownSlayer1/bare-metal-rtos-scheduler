#include "sensor.h"

void sensor_init(sensor_t *s, int32_t threshold) {
    s->raw = 0;
    s->filtered = 0;
    for (int i = 0; i < FILTER_LEN; i++) s->window[i] = 0;
    s->window_idx = 0;
    s->window_count = 0;
    s->threshold = threshold;
    s->alarm = false;
    s->samples = 0;
}

/* Deterministic raw signal: a slow sine-like walk plus noise, so the
 * demo is repeatable. Range roughly 400-600, threshold crossing at 550. */
int32_t sensor_read_raw(sensor_t *s, uint32_t tick) {
    (void)s;
    int32_t base = 500 + (int32_t)(80 * ((tick % 200 < 100) ? (tick % 100) : (100 - (tick % 100)))) / 100;
    int32_t noise = (int32_t)((tick * 37u) % 41u) - 20;
    return base + noise;
}

void sensor_sample(sensor_t *s, uint32_t tick) {
    s->raw = sensor_read_raw(s, tick);
    s->window[s->window_idx] = s->raw;
    s->window_idx = (s->window_idx + 1) % FILTER_LEN;
    if (s->window_count < FILTER_LEN) s->window_count++;
    int32_t sum = 0;
    for (int i = 0; i < s->window_count; i++) sum += s->window[i];
    s->filtered = sum / s->window_count;
    s->alarm = s->filtered > s->threshold;
    s->samples++;
}
