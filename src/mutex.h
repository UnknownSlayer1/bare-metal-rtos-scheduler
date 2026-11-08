#ifndef MUTEX_H
#define MUTEX_H

#include "scheduler.h"

/* A simple mutex with priority inheritance: if a higher-priority task
 * blocks on a mutex held by a lower-priority task, the holder is
 * temporarily boosted to the waiter's priority so it can finish and
 * release the lock instead of being starved by medium-priority work
 * (the classic priority-inversion problem). */
typedef struct {
    task_t *owner;
    task_t *waiter; /* single waiter for this small demo */
} mutex_t;

void mutex_init(mutex_t *m);
/* Returns 0 on lock acquired, -1 if busy (caller should block). */
int mutex_try_lock(mutex_t *m, task_t *t);
void mutex_unlock(mutex_t *m, task_t *t);

#endif
