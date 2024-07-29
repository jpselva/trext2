#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../ext2.h"

/*************************************
 * Test utilities                    *
 *************************************/
#define testsuite(descr) printf("TEST: " descr "\n");
#define test(descr, cond)                         \
    ((cond) ?                                     \
        printf("  PASS: %s\n", descr) :           \
        printf("  FAIL: %s (%s)\n", descr, #cond));

#define exec_cmd(cmd) system(cmd ">/dev/null 2>&1")

/*************************************
 * Disk Emulation using a file       *
 *************************************/
int readblock(uint32_t start, uint32_t size, void* buffer, void* context); 
