#include "utils.h"
#define DISKIMG_FILE "test/disk1.img"
#define BLOCKSZ 2048
#define BLOCKS_PER_GROUP 256

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

    // disk with 2MiB
    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);

    // -b 2048 => block size = 2048 B (so disk has 512 blocks)
    // -g 256 => block groups have 256 blocks
    exec_cmd("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);

    int result = ext2_mount(&ext2, &cfg);

    test("mount succeeded", result == 0);
    test("block size", ext2.block_size == BLOCKSZ);
    test("block count", ext2.superblk.blocks_count = 512);
    test("blocks per group", ext2.superblk.blocks_per_group = BLOCKS_PER_GROUP);

    return 0;
}
