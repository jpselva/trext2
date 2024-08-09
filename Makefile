TEST_SOURCES := $(wildcard test/test*.c)
TEST_EXECUTABLES := $(TEST_SOURCES:.c=)

CC := gcc
TESTFLAGS := -Wall -g -O0 -Wconversion
CFLAGS := -Wall -g -O2 -Wconversion

all: $(TEST_EXECUTABLES)

test/%: test/%.c test/utils.c test/utils.h ext2.c ext2.h
	$(CC) $(TESTFLAGS) -o $@ $< test/utils.c ext2.c

run-tests: $(TEST_EXECUTABLES)
	for test in $(TEST_EXECUTABLES) ; do \
		./$$test ; \
	done

# Clean up the build artifacts
clean:
	rm -f $(TEST_EXECUTABLES)

