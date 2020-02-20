CC = gcc
CFLAGS = -O3 -Wall -Wextra -Iinclude

all: bin/q0 bin/q1 bin/q3 bin/q6 bin/q7

bin/q0: src/fail.c lib/util.c
	$(CC) $(CFLAGS) $^ -o $@

bin/q1: src/timing.c lib/util.c
	$(CC) $(CFLAGS) $^ -o $@

bin/q3: src/index-guesser.c lib/util.c lib/index-chooser.o
	$(CC) $(CFLAGS) $^ -o $@

bin/q6: src/user-space-secret.c lib/util.c
	$(CC) $(CFLAGS) $^ -o $@

bin/q7: src/meltdown.c lib/util.c
	$(CC) $(CFLAGS) $^ -o $@


clean:
	rm -f bin/q?
