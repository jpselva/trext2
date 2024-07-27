#include "../ext2_internal.h"
#include <stdio.h>
#include <stdint.h>

/*************************************
 * Test utilities                    *
 *************************************/
#define testsuite(descr) printf("TEST: " descr "\n");
#define test(descr, cond)                         \
    ((cond) ?                                     \
        printf("  PASS: %s\n", descr) :           \
        printf(">>FAIL: %s (%s)\n", descr, #cond));


/*************************************
 * Disk Emulation using a file       *
 *************************************/
#define DISKIMG_FILE "test/disk.img"
FILE* diskimg_fd;

int readblock(uint32_t start, uint32_t size, void* buffer) {
    // set read/write position to start of block
    fseek(diskimg_fd, start, SEEK_SET);

    // read 1 string of 'size' elements into buffer
    int items_read = fread(buffer, size, 1, diskimg_fd);

    return (items_read < 1) ? -1 : 0;
}

/*************************************
 * Filesystem instance and config    *
 *************************************/
ext2_t ext2;
ext2_config_t ext2_cfg = {
    .read = readblock
};

/*************************************
 * Tests                             *
 *************************************/
void test_read_superblock() {
    testsuite("read superblock");

    ext2_superblock_t super;

    int error = read_superblock(&ext2, &super);

    test("superblock read succeeds", error == 0);
}

int main() {
    diskimg_fd = fopen(DISKIMG_FILE, "r");
    ext2_mount(&ext2, &ext2_cfg);

    test_read_superblock();

    return 0;
}
