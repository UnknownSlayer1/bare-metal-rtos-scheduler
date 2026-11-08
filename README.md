# Bare-Metal RTOS Scheduler

A small real-time operating system scheduler in C, written the way
bare-metal firmware is written: no OS, no threads, no heap, no external
dependencies. A simulated 1 ms SysTick drives it on a host PC, so you
can build and test it with plain `gcc` — the same code structure ports
to an MCU by replacing the tick source and the sensor/UART stubs.

## What it does

- **Priority scheduler (`src/scheduler.c`)**: 4 priority levels, highest
  priority first, round-robin within a priority. Tasks are cooperative
  state machines: the scheduler re-evaluates the highest-priority ready
  task on every tick, so a high-priority task preempts lower work
  between steps.
- **Sleep / delay (`task_sleep`)**: tasks sleep for N ms; the tick
  handler wakes them on time.
- **Mutex with priority inheritance (`src/mutex.c`)**: if a high-priority
  task blocks on a mutex held by a low-priority task, the holder is
  temporarily boosted so it can finish and release the lock — the fix
  for classic priority inversion.
- **Watchdog (`src/watchdog.c`)**: flags and suspends any task that has
  not run within its timeout, the way a hardware watchdog would reset
  a stuck MCU.
- **Sensor driver (`src/sensor.c`)**: simulated ADC with a moving-average
  filter and a threshold alarm, protected by the mutex so the logger
  never reads a half-updated sample.

## Demo application (`src/main.c`)

A sensor node with 4 tasks:

| Task     | Priority | Period | Job |
|----------|----------|--------|-----|
| watchdog | 3 (high) | 1000 ms | check every task's heartbeat |
| sensor   | 2 | 100 ms | sample ADC, moving-average filter |
| logger   | 1 | 500 ms | print sensor state over "UART" |
| led      | 0 (low) | 250 ms | toggle an LED |

## Run it

```bash
make run    # build and run the 5-second demo
make test   # build and run the unit tests
```

Example output:

```
Bare-metal RTOS scheduler demo — running 5000 ms simulated
[     3 ms] sensor raw=514 filtered=514 alarm=0 samples=1
[     4 ms] LED ON
[   503 ms] sensor raw=559 filtered=536 alarm=0 samples=6
[  1003 ms] sensor raw=491 filtered=535 alarm=0 samples=11
Done. ticks=5000 sensor samples=50 alarms=13 watchdog faults=0
  sensor     runs=50
  logger     runs=10
  led        runs=20
  watchdog   runs=5
```

The run counts prove the timing: 50 sensor samples in 5000 ms
(every 100 ms), 10 logger runs (every 500 ms), 20 LED toggles
(every 250 ms), 5 watchdog checks (every 1000 ms).

## Tests

```bash
make test
# priority ordering: ok
# round-robin same priority: ok
# sleep wakes on time: ok
# mutex priority inheritance: ok
# mutex only owner unlocks: ok
# sensor filter smooths noise: ok
# watchdog flags stalled task: ok
# scheduler rejects too many tasks: ok
# All 8 tests passed.
```

Built with `-Wall -Wextra -Werror -std=c11`, no warnings.

## What I built / learned

- How a priority scheduler picks the next task, and why round-robin
  within a priority keeps same-priority tasks fair.
- What priority inversion is, and how priority inheritance fixes it.
- How a watchdog detects a stalled task from heartbeat timestamps.
- How a moving-average filter smooths noisy sensor data, and why a
  mutex is needed when two tasks share that data.
- How to structure firmware so the scheduler, drivers, and application
  are separate modules that can each be unit-tested on a host PC
  before ever touching hardware.
