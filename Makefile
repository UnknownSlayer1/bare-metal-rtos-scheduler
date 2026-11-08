CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -O2 -Isrc
SRC = src/scheduler.c src/mutex.c src/sensor.c src/watchdog.c src/main.c
TEST_SRC = src/scheduler.c src/mutex.c src/sensor.c src/watchdog.c tests/test_scheduler.c

all: demo

demo: $(SRC)
	$(CC) $(CFLAGS) -o demo $(SRC)

test: $(TEST_SRC)
	$(CC) $(CFLAGS) -o test_scheduler $(TEST_SRC)
	./test_scheduler

run: demo
	./demo

clean:
	rm -f demo test_scheduler

.PHONY: all test run clean
