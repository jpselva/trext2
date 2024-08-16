TEST_SOURCES := $(wildcard test/test*.c)
TEST_EXECUTABLES := $(TEST_SOURCES:.c=)

CC := gcc
TESTFLAGS := -Wall -g -O0
CFLAGS := -Wall -g -O0

all: $(TEST_EXECUTABLES)

ext2.o: ext2.c
	$(CC) $(CFLAGS) -c $@ $<

utils.o: test/utils.c
	$(CC) $(CFLAGS) -c $@ $<

test/%: test/%.c utils.o ext2.o
	$(CC) $(TESTFLAGS) -o $@ $< utils.o ext2.o

run-tests: $(TEST_EXECUTABLES)
	for test in $(TEST_EXECUTABLES) ; do \
		./$$test ; \
	done

# Clean up the build artifacts
clean:
	rm -f $(TEST_EXECUTABLES)
	rm ext2.o
	rm utils.o
