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
        printf("  FAIL: %s (%s)\n", descr, #cond));


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
void test_mount() {
    testsuite("test mount");
    test("block size is correct", ext2.block_size == 1024);
}

void test_read_block_group_desc() {
    testsuite("read block group descriptor");

    ext2_block_group_descriptor_t bgd;
    read_block_group_descriptor(&ext2, 1, &bgd);
    test("ok", 1);
}

int main() {
    diskimg_fd = fopen(DISKIMG_FILE, "r");
    int mount_error = ext2_mount(&ext2, &ext2_cfg);

    if (mount_error) {
        printf("mount error");
        return 1;
    }

    test_mount();
    test_read_block_group_desc();

    fclose(diskimg_fd);
    return 0;
}
