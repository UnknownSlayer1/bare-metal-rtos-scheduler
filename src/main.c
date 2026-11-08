/* Demo application: a sensor node running on the scheduler.
 *
 * Tasks:
 *   sensor   (prio 2) — samples the ADC every 100 ms, moving-average filter
 *   logger   (prio 1) — prints sensor state over "UART" every 500 ms
 *   led      (prio 0) — toggles an LED every 250 ms
 *   watchdog (prio 3) — checks every task's heartbeat every 1000 ms
 *
 * All tasks share a mutex protecting the sensor data, so the logger
 * never reads a half-updated sample. The simulated SysTick is 1 ms.
 */
#include <stdio.h>
#include "scheduler.h"
#include "mutex.h"
#include "sensor.h"
#include "watchdog.h"

static scheduler_t sched;
static mutex_t sensor_mutex;
static sensor_t sensor;
static watchdog_t watchdog;
static int led_state = 0;
static int alarm_count = 0;

static void sensor_task(task_t *self) {
    (void)self;
    if (mutex_try_lock(&sensor_mutex, self) == 0) {
        sensor_sample(&sensor, sched.tick);
        if (sensor.alarm) alarm_count++;
        mutex_unlock(&sensor_mutex, self);
    }
    task_sleep(self, 100);
}

static void logger_task(task_t *self) {
    (void)self;
    if (mutex_try_lock(&sensor_mutex, self) == 0) {
        printf("[%6u ms] sensor raw=%d filtered=%d alarm=%d samples=%u\n",
               sched.tick, sensor.raw, sensor.filtered,
               sensor.alarm ? 1 : 0, sensor.samples);
        mutex_unlock(&sensor_mutex, self);
    }
    task_sleep(self, 500);
}

static void led_task(task_t *self) {
    (void)self;
    led_state = !led_state;
    printf("[%6u ms] LED %s\n", sched.tick, led_state ? "ON" : "OFF");
    task_sleep(self, 250);
}

static void watchdog_task(task_t *self) {
    (void)self;
    watchdog_check(&watchdog, &sched);
    if (watchdog.fault_count > 0) {
        printf("[%6u ms] WATCHDOG FAULT: %s (count=%d)\n",
               sched.tick, watchdog.last_fault_task, watchdog.fault_count);
    }
    task_sleep(self, 1000);
}

int main(void) {
    scheduler_init(&sched);
    mutex_init(&sensor_mutex);
    sensor_init(&sensor, 540);
    watchdog_init(&watchdog, 2000);

    scheduler_add_task(&sched, "sensor", 2, sensor_task, NULL);
    scheduler_add_task(&sched, "logger", 1, logger_task, NULL);
    scheduler_add_task(&sched, "led", 0, led_task, NULL);
    scheduler_add_task(&sched, "watchdog", 3, watchdog_task, NULL);

    printf("Bare-metal RTOS scheduler demo — running 5000 ms simulated\n");
    scheduler_run(&sched, 5000);
    printf("Done. ticks=%u sensor samples=%u alarms=%u watchdog faults=%d\n",
           sched.tick, sensor.samples, alarm_count, watchdog.fault_count);
    for (int i = 0; i < sched.count; i++) {
        printf("  %-10s runs=%u\n", sched.tasks[i].name, sched.tasks[i].run_count);
    }
    return 0;
}
