#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../ext2.h"
#define MAX_CMD_SIZE 1024

/*************************************
 * Test utilities                    *
 *************************************/
#define testsuite(descr) printf("TEST: " descr "\n");
#define test(descr, cond)                         \
    ((cond) ?                                     \
        printf("  PASS: %s\n", descr) :           \
        printf("  FAIL: %s (%s)\n", descr, #cond));

int exec_cmd(const char* fmt, ...);
void exec_cmd_fail(const char* fmt, ...);

/*************************************
 * Disk Emulation using a file       *
 *************************************/
int readblock(uint32_t start, uint32_t size, void* buffer, void* context); 
int writeblock(uint32_t start, uint32_t size, const void* buffer, void* context);
