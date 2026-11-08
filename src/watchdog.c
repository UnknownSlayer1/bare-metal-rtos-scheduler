#include "watchdog.h"

void watchdog_init(watchdog_t *w, uint32_t timeout_ms) {
    w->timeout_ms = timeout_ms;
    w->fault_count = 0;
    w->last_fault_task = NULL;
}

void watchdog_check(watchdog_t *w, scheduler_t *s) {
    for (int i = 0; i < s->count; i++) {
        task_t *t = &s->tasks[i];
        if (t->state == TASK_SUSPENDED) continue;
        if (s->tick > t->last_heartbeat &&
            s->tick - t->last_heartbeat > w->timeout_ms) {
            w->fault_count++;
            w->last_fault_task = t->name;
            t->state = TASK_SUSPENDED; /* fault: stop the task */
        }
    }
}
