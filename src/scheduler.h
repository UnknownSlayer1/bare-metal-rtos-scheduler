#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_TASKS 8
#define MAX_PRIORITY 4 /* 0 = lowest, 3 = highest */

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,   /* waiting on a mutex */
    TASK_SLEEPING,
    TASK_SUSPENDED,
} task_state_t;

typedef struct task task_t;

/* A task is a cooperative state machine: the scheduler calls step()
 * once per scheduling decision. The task must return promptly; a task
 * that never returns would starve lower-priority work, exactly like
 * a real bare-metal system with no preemption inside a task body.
 * Preemption happens *between* steps: the scheduler re-evaluates the
 * highest-priority ready task on every tick. */
struct task {
    const char *name;
    uint8_t priority;          /* 0..MAX_PRIORITY-1 */
    uint8_t base_priority;     /* for priority inheritance */
    task_state_t state;
    void (*step)(task_t *self);
    void *ctx;
    uint32_t wake_tick;        /* for TASK_SLEEPING */
    uint32_t run_count;
    uint32_t last_heartbeat;   /* for the watchdog */
};

typedef struct {
    task_t tasks[MAX_TASKS];
    int count;
    uint32_t tick;             /* simulated SysTick, 1 ms per tick */
    task_t *current;
    uint8_t last_index[MAX_PRIORITY]; /* round-robin cursor per priority */
} scheduler_t;

void scheduler_init(scheduler_t *s);
int  scheduler_add_task(scheduler_t *s, const char *name, uint8_t priority,
                        void (*step)(task_t *), void *ctx);
void scheduler_tick(scheduler_t *s);      /* advance 1 ms, wake sleepers */
void scheduler_run(scheduler_t *s, uint32_t ticks); /* run for N ticks */
task_t *scheduler_pick_next(scheduler_t *s);
void task_sleep(task_t *t, uint32_t ms);
void task_heartbeat(task_t *t);

#endif
