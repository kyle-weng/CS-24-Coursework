CC = gcc
CFLAGS = -g -fno-omit-frame-pointer

test: bt-test
	bash -c "diff <(./answer $<) <(./run $<)"

bt-test: test.c bt.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f bt-test
