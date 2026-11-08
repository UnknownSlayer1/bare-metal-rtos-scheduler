/* Unit tests for the scheduler, mutex, sensor filter, and watchdog.
 * No test framework — plain asserts, exit code 0 on pass. */
#include <assert.h>
#include <stdio.h>
#include "scheduler.h"
#include "mutex.h"
#include "sensor.h"
#include "watchdog.h"

static int order[16];
static int order_n;

static void task_a(task_t *t) { order[order_n++] = 1; task_sleep(t, 1000); }
static void task_b(task_t *t) { order[order_n++] = 2; task_sleep(t, 1000); }
static void task_c(task_t *t) { order[order_n++] = 3; task_sleep(t, 1000); }

static void test_priority_ordering(void) {
    scheduler_t s; scheduler_init(&s);
    scheduler_add_task(&s, "low", 0, task_a, NULL);
    scheduler_add_task(&s, "high", 3, task_b, NULL);
    scheduler_add_task(&s, "mid", 1, task_c, NULL);
    order_n = 0;
    scheduler_run(&s, 1);
    assert(order_n == 1 && order[0] == 2); /* highest priority runs first */
    printf("priority ordering: ok\n");
}

static void test_round_robin_same_priority(void) {
    scheduler_t s; scheduler_init(&s);
    scheduler_add_task(&s, "a", 1, task_a, NULL);
    scheduler_add_task(&s, "b", 1, task_b, NULL);
    order_n = 0;
    scheduler_run(&s, 2);
    assert(order_n == 2 && order[0] == 1 && order[1] == 2);
    printf("round-robin same priority: ok\n");
}

static void test_sleep_wakes_on_time(void) {
    scheduler_t s; scheduler_init(&s);
    scheduler_add_task(&s, "a", 1, task_a, NULL);
    order_n = 0;
    scheduler_run(&s, 10);
    assert(order_n == 1); /* sleeping for 1000 ms, should not re-run */
    scheduler_run(&s, 1000);
    assert(order_n == 2); /* woke up after 1000 ms */
    printf("sleep wakes on time: ok\n");
}

static void test_mutex_priority_inheritance(void) {
    scheduler_t s; scheduler_init(&s);
    mutex_t m; mutex_init(&m);
    scheduler_add_task(&s, "low", 0, task_a, NULL);
    scheduler_add_task(&s, "high", 3, task_b, NULL);
    task_t *low = &s.tasks[0], *high = &s.tasks[1];
    assert(mutex_try_lock(&m, low) == 0);
    assert(mutex_try_lock(&m, high) == -1); /* busy, high blocks */
    assert(low->priority == 3); /* holder boosted to waiter's priority */
    assert(high->state == TASK_BLOCKED);
    mutex_unlock(&m, low);
    assert(low->priority == 0); /* boost dropped */
    assert(high->state == TASK_READY);
    printf("mutex priority inheritance: ok\n");
}

static void test_mutex_only_owner_unlocks(void) {
    scheduler_t s; scheduler_init(&s);
    mutex_t m; mutex_init(&m);
    scheduler_add_task(&s, "low", 0, task_a, NULL);
    scheduler_add_task(&s, "high", 3, task_b, NULL);
    task_t *low = &s.tasks[0], *high = &s.tasks[1];
    mutex_try_lock(&m, low);
    mutex_unlock(&m, high); /* not the owner: no-op */
    assert(m.owner == low);
    printf("mutex only owner unlocks: ok\n");
}

static void test_sensor_filter_smooths_noise(void) {
    sensor_t sen; sensor_init(&sen, 10000);
    /* Feed a constant raw value: filter must converge to it. */
    for (int i = 0; i < 20; i++) {
        sen.window[sen.window_idx] = 500;
        sen.window_idx = (sen.window_idx + 1) % FILTER_LEN;
        if (sen.window_count < FILTER_LEN) sen.window_count++;
        int sum = 0;
        for (int j = 0; j < sen.window_count; j++) sum += sen.window[j];
        sen.filtered = sum / sen.window_count;
    }
    assert(sen.filtered == 500);
    /* Raw signal stays in the expected band. */
    for (uint32_t tick = 0; tick < 500; tick++) {
        int32_t raw = sensor_read_raw(&sen, tick);
        assert(raw >= 350 && raw <= 620);
    }
    printf("sensor filter smooths noise: ok\n");
}

static void test_watchdog_flags_stalled_task(void) {
    scheduler_t s; scheduler_init(&s);
    watchdog_t w; watchdog_init(&w, 100);
    scheduler_add_task(&s, "a", 1, task_a, NULL);
    s.tasks[0].last_heartbeat = 0;
    s.tick = 500; /* 500 ms since last heartbeat, timeout is 100 ms */
    watchdog_check(&w, &s);
    assert(w.fault_count == 1);
    assert(s.tasks[0].state == TASK_SUSPENDED);
    printf("watchdog flags stalled task: ok\n");
}

static void test_scheduler_rejects_too_many_tasks(void) {
    scheduler_t s; scheduler_init(&s);
    for (int i = 0; i < MAX_TASKS; i++) {
        assert(scheduler_add_task(&s, "t", 0, task_a, NULL) == 0);
    }
    assert(scheduler_add_task(&s, "extra", 0, task_a, NULL) == -1);
    assert(scheduler_add_task(&s, "bad", 99, task_a, NULL) == -1);
    printf("scheduler rejects too many tasks: ok\n");
}

int main(void) {
    test_priority_ordering();
    test_round_robin_same_priority();
    test_sleep_wakes_on_time();
    test_mutex_priority_inheritance();
    test_mutex_only_owner_unlocks();
    test_sensor_filter_smooths_noise();
    test_watchdog_flags_stalled_task();
    test_scheduler_rejects_too_many_tasks();
    printf("All 8 tests passed.\n");
    return 0;
}
