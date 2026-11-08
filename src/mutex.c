#include "mutex.h"

void mutex_init(mutex_t *m) {
    m->owner = NULL;
    m->waiter = NULL;
}

int mutex_try_lock(mutex_t *m, task_t *t) {
    if (m->owner == NULL) {
        m->owner = t;
        return 0;
    }
    if (m->owner == t) {
        return 0; /* already held by this task */
    }
    /* Priority inheritance: boost the holder to the waiter's priority. */
    if (t->priority > m->owner->priority) {
        m->owner->priority = t->priority;
    }
    m->waiter = t;
    t->state = TASK_BLOCKED;
    return -1;
}

void mutex_unlock(mutex_t *m, task_t *t) {
    if (m->owner != t) {
        return; /* only the owner may unlock */
    }
    t->priority = t->base_priority; /* drop any inherited boost */
    m->owner = NULL;
    if (m->waiter) {
        m->waiter->state = TASK_READY;
        m->waiter = NULL;
    }
}
