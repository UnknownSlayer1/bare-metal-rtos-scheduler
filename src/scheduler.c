#include "scheduler.h"
#include <string.h>

void scheduler_init(scheduler_t *s) {
    memset(s, 0, sizeof(*s));
}

int scheduler_add_task(scheduler_t *s, const char *name, uint8_t priority,
                       void (*step)(task_t *), void *ctx) {
    if (s->count >= MAX_TASKS || priority >= MAX_PRIORITY || !step) {
        return -1;
    }
    task_t *t = &s->tasks[s->count++];
    t->name = name;
    t->priority = priority;
    t->base_priority = priority;
    t->state = TASK_READY;
    t->step = step;
    t->ctx = ctx;
    t->wake_tick = 0;
    t->run_count = 0;
    t->last_heartbeat = 0;
    return 0;
}

void task_sleep(task_t *t, uint32_t ms) {
    /* wake_tick holds the remaining delay in ms; scheduler_tick
     * decrements it once per tick and wakes the task at zero. */
    t->state = TASK_SLEEPING;
    t->wake_tick = ms;
}

void task_heartbeat(task_t *t) {
    /* last_heartbeat is updated by the scheduler with the current tick
     * after each step; tasks just call this to mark themselves alive. */
    (void)t;
}

task_t *scheduler_pick_next(scheduler_t *s) {
    /* Highest priority first; round-robin within a priority by picking
     * the ready task after the one that ran last at that priority. */
    for (int p = MAX_PRIORITY - 1; p >= 0; p--) {
        int start = s->last_index[p];
        for (int i = 0; i < s->count; i++) {
            int idx = (start + i) % s->count;
            task_t *t = &s->tasks[idx];
            if (t->priority == (uint8_t)p && t->state == TASK_READY) {
                s->last_index[p] = (idx + 1) % s->count;
                return t;
            }
        }
    }
    return NULL;
}

void scheduler_tick(scheduler_t *s) {
    s->tick++;
    for (int i = 0; i < s->count; i++) {
        task_t *t = &s->tasks[i];
        if (t->state == TASK_SLEEPING && t->wake_tick > 0) {
            t->wake_tick--;
            if (t->wake_tick == 0) {
                t->state = TASK_READY;
            }
        }
    }
}

void scheduler_run(scheduler_t *s, uint32_t ticks) {
    for (uint32_t i = 0; i < ticks; i++) {
        scheduler_tick(s);
        task_t *next = scheduler_pick_next(s);
        if (!next) {
            continue; /* idle tick: nothing ready */
        }
        s->current = next;
        next->state = TASK_RUNNING;
        next->step(next);
        next->run_count++;
        next->last_heartbeat = s->tick;
        if (next->state == TASK_RUNNING) {
            next->state = TASK_READY; /* step returned without blocking */
        }
        s->current = NULL;
    }
}
