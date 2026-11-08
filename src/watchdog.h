#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "scheduler.h"

/* Watchdog: each task updates last_heartbeat after every step. If a
 * task has not run for longer than `timeout_ms`, the watchdog flags it.
 * On real hardware this would reset the MCU; here it records the fault
 * and suspends the offending task so the fault is visible. */
typedef struct {
    uint32_t timeout_ms;
    int fault_count;
    const char *last_fault_task;
} watchdog_t;

void watchdog_init(watchdog_t *w, uint32_t timeout_ms);
void watchdog_check(watchdog_t *w, scheduler_t *s);

#endif
