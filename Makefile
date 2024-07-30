TEST_SOURCES := $(wildcard test/test*.c)
TEST_EXECUTABLES := $(TEST_SOURCES:.c=)

CC := gcc
CFLAGS := -Wall -g -O2 -Wconversion

all: $(TEST_EXECUTABLES)

# Pattern rule to link .o files to executables
test/%: test/%.c
	$(CC) $(CFLAGS) -o $@ $< test/utils.c ext2.c

run-tests: $(TEST_EXECUTABLES)
	for test in $(TEST_EXECUTABLES) ; do \
		./$$test ; \
	done

# Clean up the build artifacts
clean:
	rm -f $(TEST_EXECUTABLES)

