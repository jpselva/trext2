#include "utils.h"
#define DISKIMG_FILE "test/disk1.img"

/************************************* 
 * Filesystem instance and config    *
 *************************************/
ext2_t ext2;
ext2_config_t cfg = {
    .read = readblock,
    .context = DISKIMG_FILE
};

int main() {
    testsuite("test mount");

    if(exec_cmd("dd if=/dev/zero of=" DISKIMG_FILE " bs=1k count=500")) {
        printf("dd failed");
        return 1;
    }

    if (exec_cmd("mkfs.ext2 -r 0 -b 4096 " DISKIMG_FILE)) {
        printf("mkfs failed");
        return 1;
    }

    int result = ext2_mount(&ext2, &cfg);

    test("mount succeeded", result == 0);
    test("block size is correct", ext2.block_size == 4096);

    return 0;
}
